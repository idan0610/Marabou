/*********************                                                        */
/*! \file NetworkReducer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Your Name
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "NetworkReducer.h"
#include "NLRError.h"
#include "InputQuery.h"
#include "Layer.h"
#include "ReluConstraint.h"
#include "MockTableau.h"
#include "MatrixMultiplication.h"

#include <algorithm>
#include <cmath>

namespace NLR {

NetworkReducer::NetworkReducer( NetworkLevelReasoner *nlr )
    : _nlr( nlr )
{
}

NetworkReducer::~NetworkReducer()
{
}

double NetworkReducer::determineBucketTolerance( double reductionRate, const Vector<double> &scores )
{
    if ( reductionRate == 0.0 )
        return 0.0;

    Vector<double> sortedScores = scores;
    std::sort( sortedScores.begin(), sortedScores.end() );

    unsigned numToRemove = (unsigned)( reductionRate * sortedScores.size() );
    if ( numToRemove > sortedScores.size() )
        numToRemove = sortedScores.size();

    if ( numToRemove == 0 )
        return 0.0;

    return sortedScores[numToRemove - 1];
}

void NetworkReducer::reduce( InputQuery &query, double reductionRate, double tolerance )
{
    // 1. Compute bounds using DeepPoly
    _nlr->deepPolyPropagation();

    // 2. Implement the "merge buckets" strategy
    Vector<double> scores;
    Map<double, List<NeuronIndex>> stabilityBuckets;
    const Map<unsigned, Layer *> &layers = _nlr->getLayerIndexToLayer();
    unsigned totalReLUs = 0;

    for ( const auto &layerPair : layers )
    {
        Layer *layer = layerPair.second;
        if ( layer->getLayerType() == Layer::RELU )
        {
            for ( unsigned i = 0; i < layer->getSize(); ++i )
            {
                if ( layer->neuronEliminated( i ) )
                    continue;

                ++totalReLUs;

                // Get the source neuron
                NeuronIndex sourceIndex = *layer->getActivationSources( i ).begin();
                Layer *sourceLayer = _nlr->getLayer( sourceIndex._layer );
                double lb = sourceLayer->getLb( sourceIndex._neuron );
                double ub = sourceLayer->getUb( sourceIndex._neuron );

                // Calculate stability score
                double score = std::min( std::abs( lb ), std::abs( ub ) );
                scores.append( score );

                // Add to bucket
                if ( !stabilityBuckets.exists( score ) )
                    stabilityBuckets[score] = List<NeuronIndex>();
                stabilityBuckets[score].append( NeuronIndex( layer->getLayerIndex(), i ) );
            }
        }
    }

    double scoreThreshold = determineBucketTolerance( reductionRate, scores );

    // Identify neurons to remove
    List<NeuronIndex> neuronsToRemove;
    for ( const auto &bucket : stabilityBuckets )
    {
        if ( bucket.first <= scoreThreshold )
        {
            neuronsToRemove.append( bucket.second );
        }
    }

    // 4. Prune the network
    for ( const auto &reluToRemove : neuronsToRemove )
    {
        Layer *reluLayer = _nlr->getLayer( reluToRemove._layer );
        unsigned reluNeuronIndex = reluToRemove._neuron;

        NeuronIndex sourceIndex = *reluLayer->getActivationSources( reluNeuronIndex ).begin();
        Layer *sourceLayer = _nlr->getLayer( sourceIndex._layer );
        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        if ( ub <= 0 ) // Stable false
        {
            unsigned variable = reluLayer->neuronToVariable( reluNeuronIndex );
            query.setLowerBound( variable, 0.0 );
            query.setUpperBound( variable, 0.0 );
        }
        else if ( lb >= 0 ) // Stable true
        {
            // The ReLU is stable-true. This means y = x.
            // We can replace all occurrences of y with x.
            unsigned x = sourceLayer->neuronToVariable( sourceIndex._neuron );
            unsigned y = reluLayer->neuronToVariable( reluNeuronIndex );
            query.addEquation( Equation( x, y, 0.0, Equation::EQ ) );
        }
    }
}

} // namespace NLR
