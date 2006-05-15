// kmacctfolder.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctfolder.h"
#include "kmacctmgr.h"
#include "folderstorage.h"
#include <stdlib.h>

#define MAX_ACCOUNTS 16

//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::account(void)
{
  if (storage()->acctList()) return storage()->acctList()->first();
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::nextAccount(void)
{
  if (storage()->acctList()) return storage()->acctList()->next();
  return 0;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount(KMAccount* aAcct)
{
  if (!aAcct) return;
  if (!storage()->acctList()) storage()->setAcctList( new KMAcctList );

  storage()->acctList()->append(aAcct);
  aAcct->setFolder(this);
}


//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList(void)
{
  if (storage()->acctList()) storage()->acctList()->clear();
}


//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount(KMAccount* aAcct)
{
  if (!aAcct || !storage()->acctList()) return;

  storage()->acctList()->remove(aAcct);
  aAcct->setFolder(0);
  if (storage()->acctList()->count() <= 0)
  {
    delete storage()->acctList();
    storage()->setAcctList( 0 );
  }
}



