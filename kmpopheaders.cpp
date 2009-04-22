/***************************************************************************
                          kmpopheaders.cpp  -  description
                             -------------------
    begin                : Mon Oct 22 2001
    copyright            : (C) 2001 by Heiko Hund
                                       Thorsten Zachmann
    email                : heiko@ist.eigentlich.net
                           T.Zachmann@zagge.de
 ***************************************************************************/

#include <config.h>
#include "kmpopheaders.h"
#include <kdebug.h>

KMPopHeaders::KMPopHeaders()
  : mAction(NoAction),
    mId(),
    mUid(),
    mRuleMatched(false),
    mHeader(0)
{
}

KMPopHeaders::~KMPopHeaders(){
  delete mHeader;
}

/** No descriptions */
KMPopHeaders::KMPopHeaders(const QString& aId, const QString& aUid, KMPopFilterAction aAction)
  : mAction(aAction),
    mId(aId),
    mUid(aUid),
    mRuleMatched(false),
    mHeader(0)
{
}

/** No descriptions */
QString KMPopHeaders::id() const{
  return mId;
}

/** No descriptions */
QString KMPopHeaders::uid() const{
  return mUid;
}

/** No descriptions */
KMMessage * KMPopHeaders::header() const{
  return mHeader;
}

/** No descriptions */
void KMPopHeaders::setHeader(KMMessage *aHeader){
  mHeader = aHeader;
}

/** No descriptions */
KMPopFilterAction KMPopHeaders::action() const{
  return mAction;
}

/** No descriptions */
void KMPopHeaders::setAction(KMPopFilterAction aAction){
  mAction = aAction;
}
/** No descriptions */
void KMPopHeaders::setRuleMatched(bool b){
  mRuleMatched = b;
}
/** No descriptions */
bool KMPopHeaders::ruleMatched(){
  return mRuleMatched;
}
