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

InputQuery *NetworkReducer::reduce( const InputQuery &query, double reductionRate )
{
    // 1. Create a temporary NLR from the query
    NLR::NetworkLevelReasoner nlr;
    List<Equation> unhandledEquations;
    Set<unsigned> varsInUnhandledConstraints;
    query.constructNetworkLevelReasoner( nlr, unhandledEquations, varsInUnhandledConstraints );

    // 2. Compute bounds using DeepPoly
    MockTableau tableau;
    tableau.getBoundManager().initialize( query.getNumberOfVariables() );
    for ( unsigned i = 0; i < query.getNumberOfVariables(); ++i )
    {
        tableau.setLowerBound( i, query.getLowerBound( i ) );
        tableau.setUpperBound( i, query.getUpperBound( i ) );
    }
    nlr.setTableau( &tableau );
    nlr.deepPolyPropagation();

    // 3. Implement the "merge buckets" strategy
    Map<double, List<NeuronIndex>> stabilityBuckets;
    const Map<unsigned, Layer *> &layers = nlr.getLayerIndexToLayer();
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
                Layer *sourceLayer = nlr.getLayer( sourceIndex._layer );
                double lb = sourceLayer->getLb( sourceIndex._neuron );
                double ub = sourceLayer->getUb( sourceIndex._neuron );

                // Calculate stability score
                double score = std::min( std::abs( lb ), std::abs( ub ) );

                // Add to bucket
                if ( !stabilityBuckets.exists( score ) )
                    stabilityBuckets[score] = List<NeuronIndex>();
                stabilityBuckets[score].append( NeuronIndex( layer->getLayerIndex(), i ) );
            }
        }
    }

    // Determine the number of neurons to remove
    unsigned numToRemove = (unsigned)( reductionRate * totalReLUs );

    // Identify neurons to remove
    List<NeuronIndex> neuronsToRemove;
    unsigned removedCount = 0;

    // Iterate through the buckets in ascending order of score
    for ( const auto &bucket : stabilityBuckets )
    {
        if ( removedCount >= numToRemove )
            break;

        for ( const auto &neuron : bucket.second )
        {
            neuronsToRemove.append( neuron );
            ++removedCount;
            if ( removedCount >= numToRemove )
                break;
        }
    }

    // 4. Prune the network (on the temporary NLR)
    for ( const auto &reluToRemove : neuronsToRemove )
    {
        Layer *reluLayer = nlr.getLayer( reluToRemove._layer );
        unsigned reluNeuronIndex = reluToRemove._neuron;

        NeuronIndex sourceIndex = *reluLayer->getActivationSources( reluNeuronIndex ).begin();
        Layer *sourceLayer = nlr.getLayer( sourceIndex._layer );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        if ( ub <= 0 ) // Stable false
        {
            unsigned variable = reluLayer->neuronToVariable( reluNeuronIndex );
            nlr.eliminateVariable( variable, 0.0 );
        }
        else // Stable true
        {
            // TODO: Implement stable-true reduction
        }
    }

    // 5. Generate a new query from the reduced NLR
    InputQuery *reducedQuery = new InputQuery();
    nlr.generateQuery( *reducedQuery );

    return reducedQuery;
}

} // namespace NLR
