#include "CdclDivider.h"

#include <utility>

CdclDivider::CdclDivider( std::shared_ptr<CdclCore> cdclCore )
    : _cdclCore( std::move(cdclCore) )
{
}

//void CdclDivider::createSubQueries( unsigned int numNewSubQueries,
//                                    const String queryIdPrefix,
//                                    const unsigned int previousDepth,
//                                    const PiecewiseLinearCaseSplit &previousSplit,
//                                    const unsigned int timeoutInSeconds,
//                                    SubQueries &subQueries )
//{
//
//}
