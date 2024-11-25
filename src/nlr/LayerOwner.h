/*********************                                                        */
/*! \file LayerOwner.h
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

#ifndef __LayerOwner_h__
#define __LayerOwner_h__

#include "ITableau.h"
#include "Tightening.h"

namespace NLR {

class Layer;

class LayerOwner
{
public:
    virtual ~LayerOwner()
    {
    }
    virtual const Layer *getLayer( unsigned index ) const = 0;
    virtual const Map<unsigned, Layer *> &getLayerIndexToLayer() const = 0;
    virtual unsigned getMaxLayerSize() const = 0;
    virtual const ITableau *getTableau() const = 0;
    virtual unsigned getNumberOfLayers() const = 0;
    virtual void receiveTighterBound( Tightening tightening ) = 0;
    virtual bool isBoundsAfterSplitInitialized() const = 0;
    virtual const List<unsigned int> *getDeepPolyAuxVars( unsigned variable ) = 0;

    virtual bool shouldProduceProofs() const = 0;
    virtual const SparseUnsortedList &getLbExplanationForVariable( unsigned variable ) const = 0;
    virtual const SparseUnsortedList &getUbExplanationForVariable( unsigned variable ) const = 0;
    virtual void updateLbExplanationForVariable( unsigned variable, const SparseUnsortedList &explanation ) = 0;
    virtual void updateUbExplanationForVariable( unsigned variable, const SparseUnsortedList &explanation ) = 0;
    virtual void updateExplanationInExplainer( unsigned variable, bool isUpper) = 0;
};

} // namespace NLR

#endif // __LayerOwner_h__
