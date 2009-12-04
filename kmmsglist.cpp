// kmmsglist.cpp


#include "kmmsglist.h"
#include "kmmsgdict.h" // FIXME Till - move those into kmfolderindex
#include "kmkernel.h"
#include <assert.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
KMMsgList::KMMsgList(int initSize)
  : QVector<KMMsgBase*>( initSize, static_cast<KMMsgBase*>( 0 ) ),
    mHigh( 0 ), mCount( 0 )
{
}


//-----------------------------------------------------------------------------
KMMsgList::~KMMsgList()
{
  clear( true );
}


//-----------------------------------------------------------------------------
void KMMsgList::clear( bool doDelete, bool syncDict )
{
  if ( mHigh > 0 )
    for ( int i = mHigh; i > 0; i-- )
    {
      KMMsgBase * msg = at(i-1);
      if ( msg ) {
        if ( syncDict )
          KMMsgDict::mutableInstance()->remove( msg );
        operator[]( i-1 ) = 0;
        if ( doDelete )
          delete msg;
      }
    }
  mHigh  = 0;
  mCount = 0;
}


//-----------------------------------------------------------------------------
bool KMMsgList::resize( int aSize )
{
  int oldSize = size();
  KMMsgBase* msg;

  // delete messages that will get lost, if any
  if ( aSize < mHigh ) {
    for ( int i = aSize; i < mHigh; i++ )
    {
      msg = operator[]( i );
      if ( msg ) {
        delete msg;
        mCount--;
      }
      mHigh = aSize;
    }
  }

  // do the resizing
  QVector<KMMsgBase*>::resize( aSize );

  // initialize new elements
  for ( int i = oldSize; i < aSize; i++ )
    operator[]( i ) = 0;

  return true;
}


//-----------------------------------------------------------------------------
bool KMMsgList::reset(int aSize)
{
  if ( !resize( aSize ) )
    return false;

  clear();
  return true;
}


//-----------------------------------------------------------------------------
void KMMsgList::set( int idx, KMMsgBase* aMsg )
{
  if ( idx >= size() )
    resize( idx > 2 * size() ? idx + 16 : 2 * size() );

  if ( !at(idx) && aMsg )
    mCount++;
  else if ( at(idx) && !aMsg )
    mCount--;

  delete at(idx);

  operator[]( idx ) = aMsg;

  if ( !aMsg || idx >= mHigh )
    rethinkHigh();
}


//-----------------------------------------------------------------------------
void KMMsgList::insert( int idx, KMMsgBase* aMsg, bool syncDict )
{
  if ( idx >= size() )
    resize( idx > 2 * size() ? idx + 16 : 2 * size() );

  if ( aMsg )
    mCount++;

  for ( int i = mHigh; i > idx; i-- ) {
    if ( syncDict )
      KMMsgDict::mutableInstance()->remove( at( i - 1 ) );
    operator[]( i ) = at( i-1 );
    if ( syncDict )
      KMMsgDict::mutableInstance()->insert( at(i), i );
  }

  operator[]( idx ) = aMsg;
  if ( syncDict )
    KMMsgDict::mutableInstance()->insert( at(idx), idx );

  mHigh++;
}


//-----------------------------------------------------------------------------
int KMMsgList::append( KMMsgBase* aMsg, bool syncDict )
{
  const int idx = mHigh;
  insert( idx, aMsg, syncDict ); // mHigh gets modified in here
  return idx;
}


//-----------------------------------------------------------------------------
void KMMsgList::remove( int idx )
{
  assert( idx < size() );
  if ( at(idx) ) {
    mCount--;
    KMMsgDict::mutableInstance()->remove( at(idx) );
  }

  mHigh--;
  for ( int i = idx; i < mHigh; i++ ) {
    KMMsgDict::mutableInstance()->update( at(i + 1), i + 1, i );
    operator[]( i ) = at( i+1 );
  }

  operator[]( mHigh ) = 0;

  rethinkHigh();
}


//-----------------------------------------------------------------------------
KMMsgBase* KMMsgList::take( int idx )
{
  KMMsgBase* msg = at( idx );
  remove( idx );
  return msg;
}


//-----------------------------------------------------------------------------
void KMMsgList::rethinkHigh()
{
  int sz = size();

  if ( mHigh < sz && at(mHigh) )
  {
    // forward search
    while ( mHigh < sz && at(mHigh) )
      mHigh++;
  }
  else
  {
    // backward search
    while ( mHigh > 0 && !at(mHigh-1) )
      mHigh--;
  }
}
