/*********************                                                        */
/*! \file NetworkLevelReasoner.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __NetworkLevelReasoner_h__
#define __NetworkLevelReasoner_h__

#include "DeepPolyAnalysis.h"
#include "ITableau.h"
#include "Layer.h"
#include "LayerOwner.h"
#include "Map.h"
#include "MatrixMultiplication.h"
#include "NeuronIndex.h"
#include "PiecewiseLinearFunctionType.h"
#include "Tightening.h"
#include "Vector.h"

#include <memory>

namespace NLR {

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner : public LayerOwner
{
public:
    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    static bool functionTypeSupported( PiecewiseLinearFunctionType type );

    /*
      Populate the NLR by specifying the network's topology.
    */
    void addLayer( unsigned layerIndex, Layer::Type type, unsigned layerSize );
    void addLayerDependency( unsigned sourceLayer, unsigned targetLayer );
    void computeSuccessorLayers();
    void setWeight( unsigned sourceLayer,
                    unsigned sourceNeuron,
                    unsigned targetLayer,
                    unsigned targetNeuron,
                    double weight );
    void setBias( unsigned layer, unsigned neuron, double bias );
    void addActivationSource( unsigned sourceLayer,
                              unsigned sourceNeuron,
                              unsigned targetLeyer,
                              unsigned targetNeuron );

    unsigned getNumberOfLayers() const override;
    const Layer *getLayer( unsigned index ) const override;
    Layer *getLayer( unsigned index );

    /*
      Bind neurons in the NLR to the Tableau variables that represent them.
    */
    void setNeuronVariable( NeuronIndex index, unsigned variable );

    /*
      Perform an evaluation of the network for a specific input.
    */
    void evaluate( double *input, double *output );

    /*
      Perform an evaluation of the network for the current input variable
      assignment and store the resulting variable assignment in the assignment.
    */
    void concretizeInputAssignment( Map<unsigned, double> &assignment );

    /*
      Perform a simulation of the network for a specific input
    */
    void simulate( Vector<Vector<double>> *input );

    /*
      Bound propagation methods:

        - obtainCurrentBounds: make the NLR obtain the current bounds
          on all variables from the tableau.

        - Interval arithmetic: compute the bounds of a layer's neurons
          based on the concrete bounds of the previous layer.

        - Symbolic: for each neuron in the network, we compute lower
          and upper bounds on the lower and upper bounds of the
          neuron. This bounds are expressed as linear combinations of
          the input neurons. Sometimes these bounds let us simplify
          expressions and obtain tighter bounds (e.g., if the upper
          bound on the upper bound of a ReLU node is negative, that
          ReLU is inactive and its output can be set to 0.

        - LP Relaxation: invoking an LP solver on a series of LP
          relaxations of the problem we're trying to solve, and
          optimizing the lower and upper bounds of each of the
          varibales.

        - receiveTighterBound: this is a callback from the layer
          objects, through which they report tighter bounds.

        - getConstraintTightenings: this is the function that an
          external user calls in order to collect the tighter bounds
          discovered by the NLR.
    */

    void setTableau( const ITableau *tableau );
    const ITableau *getTableau() const override;

    void obtainCurrentBounds( const InputQuery &inputQuery );
    void obtainCurrentBounds();
    void obtainCurrentBoundsAfterSplit();
    void intervalArithmeticBoundPropagation();
    void symbolicBoundPropagation();
    void deepPolyPropagation();
    void lpRelaxationPropagation();
    void LPTighteningForOneLayer( unsigned targetIndex );
    void MILPPropagation();
    void MILPTighteningForOneLayer( unsigned targetIndex );
    void iterativePropagation();

    void receiveTighterBound( Tightening tightening ) override;
    void getConstraintTightenings( List<Tightening> &tightenings );
    void clearConstraintTightenings();

    /*
      For debugging purposes: dump the network topology
    */
    void dumpTopology( bool dumpLayerDetails = true ) const;

    /*
      Duplicate the reasoner
    */
    void storeIntoOther( NetworkLevelReasoner &other ) const;

    /*
      Methods that are typically invoked by the preprocessor, to
      inform us of changes in variable indices or if a variable has
      been eliminated
    */
    void eliminateVariable( unsigned variable, double value );
    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables );

    /*
      The various piecewise-linear constraints, sorted in topological
      order. The sorting is done externally.
    */
    List<PiecewiseLinearConstraint *> getConstraintsInTopologicalOrder();
    void addConstraintInTopologicalOrder( PiecewiseLinearConstraint *constraint );
    void removeConstraintFromTopologicalOrder( PiecewiseLinearConstraint *constraint );

    /*
      Add an ecoding of all the affine layers as equations in the given InputQuery
    */
    void encodeAffineLayers( InputQuery &inputQuery );

    /*
      Generate an input query from this NLR, according to the
      discovered network topology
    */
    InputQuery generateInputQuery();

    /*
      Finds logically consecutive WS layers and merges them, in order
      to reduce the total number of layers and variables in the
      network
    */
    unsigned mergeConsecutiveWSLayers( const Map<unsigned, double> &lowerBounds,
                                       const Map<unsigned, double> &upperBounds,
                                       const Set<unsigned> &varsInUnhandledConstraints,
                                       Map<unsigned, LinearExpression> &eliminatedNeurons );

    /*
      Print the bounds of variables layer by layer
    */
    void dumpBounds();

    /*
      Get the size of the widest layer
    */
    unsigned getMaxLayerSize() const override;

    const Map<unsigned, Layer *> &getLayerIndexToLayer() const override;

    bool isBoundsAfterSplitInitialized() const override;

private:
    Map<unsigned, Layer *> _layerIndexToLayer;
    const ITableau *_tableau;

    // Tightenings discovered by the various layers
    List<Tightening> _boundTightenings;


    std::unique_ptr<DeepPolyAnalysis> _deepPolyAnalysis;

    bool _boundsAfterSplitInitialized;

    void freeMemoryIfNeeded();

    List<PiecewiseLinearConstraint *> _constraintsInTopologicalOrder;

    // Map each neuron to a linear expression representing its weighted sum
    void generateLinearExpressionForWeightedSumLayer(
        Map<unsigned, LinearExpression> &variableToExpression,
        const Layer &layer );

    // Helper functions for generating an input query
    void generateInputQueryForLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForWeightedSumLayer( InputQuery &inputQuery, const Layer &layer );
    void generateEquationsForWeightedSumLayer( List<Equation> &equations, const Layer &layer );
    void generateInputQueryForReluLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForSigmoidLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForSignLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForAbsoluteValueLayer( InputQuery &inputQuery, const Layer &layer );
    void generateInputQueryForMaxLayer( InputQuery &inputQuery, const Layer &layer );

    bool suitableForMerging( unsigned secondLayerIndex,
                             const Map<unsigned, double> &lowerBounds,
                             const Map<unsigned, double> &upperBounds,
                             const Set<unsigned> &varsInConstraintsUnhandledByNLR );
    void mergeWSLayers( unsigned secondLayerIndex,
                        Map<unsigned, LinearExpression> &eliminatedNeurons );
    double *multiplyWeights( const double *firstMatrix,
                             const double *secondMatrix,
                             unsigned inputDimension,
                             unsigned middleDimension,
                             unsigned outputDimension );
    void reduceLayerIndex( unsigned layer, unsigned startIndex );

    /*
      If the NLR is manipulated manually in order to generate a new
      input query, this method can be used to assign variable indices
      to all neurons in the network
    */
    void reindexNeurons();
};

} // namespace NLR

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
