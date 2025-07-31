/*********************                                                        */
/*! \file NetworkReducer.h
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

#ifndef __NetworkReducer_h__
#define __NetworkReducer_h__

#include "NetworkLevelReasoner.h"
#include "InputQuery.h"

namespace NLR {

class NetworkReducer
{
public:
    NetworkReducer( NetworkLevelReasoner *nlr );
    ~NetworkReducer();

    InputQuery *reduce( const InputQuery &query, double reductionRate );

private:
    NetworkLevelReasoner *_nlr;
};

} // namespace NLR

#endif // __NetworkReducer_h__
