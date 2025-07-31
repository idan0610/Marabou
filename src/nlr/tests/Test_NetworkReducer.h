#include <cxxtest/TestSuite.h>

#include "NetworkReducer.h"
#include "NetworkLevelReasoner.h"
#include "InputQuery.h"
#include "MockTableau.h"

namespace NLR {

class NetworkReducerTestSuite : public CxxTest::TestSuite
{
public:
    void test_relu_reduction()
    {
        NLR::NetworkLevelReasoner nlr;

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases
        nlr.setWeight( 0, 0, 1, 0, 1.0 );
        nlr.setWeight( 0, 1, 1, 1, 1.0 );
        nlr.setWeight( 0, 2, 1, 2, 1.0 );
        nlr.setBias( 1, 0, 0.0 );
        nlr.setBias( 1, 1, 0.0 );
        nlr.setBias( 1, 2, 0.0 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.setWeight( 2, 0, 3, 0, 1.0 );
        nlr.setWeight( 2, 1, 3, 0, 1.0 );
        nlr.setWeight( 2, 2, 3, 0, 1.0 );
        nlr.setBias( 3, 0, 0.0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 8 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 9 );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 10 );
        nlr.setTableau( &tableau );

        // Initialize the bounds
        // ReLU 0: stable false
        tableau.setLowerBound( 0, -2.0 );
        tableau.setUpperBound( 0, -1.0 );

        // ReLU 1: stable true
        tableau.setLowerBound( 1, 1.0 );
        tableau.setUpperBound( 1, 2.0 );

        // ReLU 2: not stable
        tableau.setLowerBound( 2, -1.0 );
        tableau.setUpperBound( 2, 1.0 );


        // Obtain current bounds to set layer bounds
        nlr.obtainCurrentBounds();

        NetworkReducer reducer( &nlr );
        reducer.reduce( 0.5 );

        // Check that the correct neurons have been eliminated
        TS_ASSERT( nlr.getLayer( 2 )->neuronEliminated( 0 ) );
        TS_ASSERT( !nlr.getLayer( 2 )->neuronEliminated( 1 ) );
        TS_ASSERT( !nlr.getLayer( 2 )->neuronEliminated( 2 ) );
    }

    void test_merge_buckets()
    {
        NLR::NetworkLevelReasoner nlr;

        // Create a network with 5 ReLUs
        nlr.addLayer( 0, NLR::Layer::INPUT, 5 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 5 );
        nlr.addLayer( 2, NLR::Layer::RELU, 5 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        for ( unsigned i = 0; i < 5; ++i )
        {
            nlr.setWeight( 0, i, 1, i, 1.0 );
            nlr.setBias( 1, i, 0.0 );
            nlr.addActivationSource( 1, i, 2, i );
            nlr.setWeight( 2, i, 3, 0, 1.0 );
        }
        nlr.setBias( 3, 0, 0.0 );

        for ( unsigned i = 0; i < 11; ++i )
        {
            nlr.setNeuronVariable( NLR::NeuronIndex( i / 6, i % 6 ), i );
        }

        MockTableau tableau;
        tableau.getBoundManager().initialize( 11 );
        nlr.setTableau( &tableau );

        // Set bounds to create different stability scores
        tableau.setLowerBound( 0, -1.0 );
        tableau.setUpperBound( 0, -0.5 ); // score = 0.5

        tableau.setLowerBound( 1, 0.1 );
        tableau.setUpperBound( 1, 0.2 ); // score = 0.1

        tableau.setLowerBound( 2, -0.3 );
        tableau.setUpperBound( 2, 0.4 ); // score = 0.3

        tableau.setLowerBound( 3, 0.6 );
        tableau.setUpperBound( 3, 0.8 ); // score = 0.6

        tableau.setLowerBound( 4, -0.9 );
        tableau.setUpperBound( 4, -0.7 ); // score = 0.7

        nlr.obtainCurrentBounds();

        NetworkReducer reducer( &nlr );
        reducer.reduce( 0.6 ); // Remove 60% of ReLUs = 3 ReLUs

        // The ReLUs with scores 0.1, 0.3, 0.5 should be removed
        TS_ASSERT( !nlr.getLayer( 2 )->neuronEliminated( 0 ) );
        TS_ASSERT( nlr.getLayer( 2 )->neuronEliminated( 1 ) );
        TS_ASSERT( nlr.getLayer( 2 )->neuronEliminated( 2 ) );
        TS_ASSERT( !nlr.getLayer( 2 )->neuronEliminated( 3 ) );
        TS_ASSERT( nlr.getLayer( 2 )->neuronEliminated( 4 ) );
    }
};

}
