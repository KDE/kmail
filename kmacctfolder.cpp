// kmacctfolder.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctfolder.h"

//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::account()
{
  if ( acctList() )
      return acctList()->first();
  return 0;
}

//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount( KMAccount* aAcct )
{
  if ( !aAcct ) return;
  if ( !acctList() )
      setAcctList( new AccountList() );

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
