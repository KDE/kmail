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
KMAccount* KMAcctFolder::account()
{
  if ( acctList() )
      return acctList()->first();
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::nextAccount()
{
  if ( acctList() )
      return acctList()->next();
  return 0;
}

//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount( KMAccount* aAcct )
{
  if ( !aAcct ) return;
  if ( acctList() )
      setAcctList( new KMAcctList );

  acctList()->append( aAcct );
  aAcct->setFolder( this );
}

//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList()
{
  if ( acctList() )
      acctList()->clear();
}

//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount( KMAccount* aAcct )
{
  if ( !aAcct || !acctList() ) return;

  acctList()->remove( aAcct );
  aAcct->setFolder( 0 );
  if ( acctList()->count() <= 0 ) {
    delete acctList();
    setAcctList( 0 );
  }
}
