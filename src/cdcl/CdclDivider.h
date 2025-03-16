/*********************                                                        */
/*! \file CdclDivider.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __CdclDivider_h__
#define __CdclDivider_h__

#include "CdclCore.h"
#include "QueryDivider.h"

class CdclDivider : public QueryDivider
{
public:
    explicit CdclDivider( std::shared_ptr<CdclCore> cdclCore );

    void createSubQueries( unsigned numNewSubQueries,
                           const String queryIdPrefix,
                           const unsigned previousDepth,
                           const PiecewiseLinearCaseSplit &previousSplit,
                           const unsigned timeoutInSeconds,
                           SubQueries &subQueries );

private:
    std::shared_ptr<CdclCore> _cdclCore;
};


#endif // __CdclDivider_h__
