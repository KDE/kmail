// kmacctfolder.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctfolder.h"
#include "kmacctmgr.h"
#include <stdlib.h>

#define MAX_ACCOUNTS 16

//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::account(void)
{
  if (mAcctList) return mAcctList->first();
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::nextAccount(void)
{
  if (mAcctList) return mAcctList->next();
  return 0;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount(KMAccount* aAcct)
{
  if (!aAcct) return;
  if (!mAcctList) mAcctList = new KMAcctList;

  mAcctList->append(aAcct);
  aAcct->setFolder(this);
}


//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList(void)
{
  if (mAcctList) mAcctList->clear();
}


//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount(KMAccount* aAcct)
{
  if (!aAcct || !mAcctList) return;

  mAcctList->remove(aAcct);
  aAcct->setFolder(0);
  if (mAcctList->count() <= 0)
  {
    delete mAcctList;
    mAcctList = 0;
  }
}



