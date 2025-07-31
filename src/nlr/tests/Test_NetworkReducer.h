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
        InputQuery query;
        query.setNumberOfVariables( 10 );

        // Create the layers
        NLR::NetworkLevelReasoner nlr;
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

        nlr.generateQuery( query );

        // Initialize the bounds
        // ReLU 0: stable false
        query.setLowerBound( 0, -2.0 );
        query.setUpperBound( 0, -1.0 );

        // ReLU 1: stable true
        query.setLowerBound( 1, 1.0 );
        query.setUpperBound( 1, 2.0 );

        // ReLU 2: not stable
        query.setLowerBound( 2, -1.0 );
        query.setUpperBound( 2, 1.0 );

        NetworkReducer reducer( &nlr );
        InputQuery *reducedQuery = reducer.reduce( query, 1.0, 0.0 );

        // Check that the correct neurons have been eliminated
        TS_ASSERT_EQUALS( reducedQuery->getLowerBound( 6 ), 0.0 );
        TS_ASSERT_EQUALS( reducedQuery->getUpperBound( 6 ), 0.0 );

        bool equationFound = false;
        for ( const auto &eq : reducedQuery->getEquations() )
        {
            if ( eq._addends.size() == 2 &&
                 ( ( eq._addends[0]._variable == 4 && eq._addends[1]._variable == 7 ) ||
                   ( eq._addends[0]._variable == 7 && eq._addends[1]._variable == 4 ) ) )
            {
                equationFound = true;
                break;
            }
        }
        TS_ASSERT( equationFound );

        delete reducedQuery;
    }
};

}
