// kmacctfolder.cpp

#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmglobal.h"
#include <stdlib.h>

#define MAX_ACCOUNTS 16

//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::account(void)
{
  if (mAcctList) return mAcctList->first();
  return NULL;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::nextAccount(void)
{
  if (mAcctList) return mAcctList->next();
  return NULL;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount(KMAccount* aAcct)
{
  if (!aAcct) return;
  if (!mAcctList) 
  {
    mAcctList = new KMAcctList;
    mAcctList->setAutoDelete(TRUE);
  }
  mAcctList->append(aAcct);
  aAcct->setFolder(this);
}


//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList(void)
{
  mAcctList->clear();
}


//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount(KMAccount* aAcct)
{
  if (!aAcct || !mAcctList) return;

  mAcctList->remove(aAcct);
  aAcct->setFolder(NULL);
}



