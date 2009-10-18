// kmmsglist.cpp


#include "kmmsglist.h"
#include "kmmsgdict.h" // FIXME Till - move those into kmfolderindex
#include "kmkernel.h"
#include <assert.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
KMMsgList::KMMsgList(int initSize)
  : QVector<KMime::Message*>( initSize, static_cast<KMime::Message*>( 0 ) ),
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
    for ( unsigned int i = mHigh; i > 0; i-- )
    {
      KMime::Content * msg = at(i-1);
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
bool KMMsgList::resize( unsigned int aSize )
{
  unsigned int oldSize = size();
  KMime::Content* msg;

  // delete messages that will get lost, if any
  if ( aSize < mHigh ) {
    for ( unsigned int i = aSize; i < mHigh; i++ )
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
  QVector<KMime::Message*>::resize( aSize );

  // initialize new elements
  for ( unsigned int i = oldSize; i < aSize; i++ )
    operator[]( i ) = 0;

  return true;
}


//-----------------------------------------------------------------------------
bool KMMsgList::reset(unsigned int aSize)
{
  if ( !resize( aSize ) )
    return false;

  clear();
  return true;
}


//-----------------------------------------------------------------------------
void KMMsgList::set( unsigned int idx, KMime::Message* aMsg )
{
  if ( idx >= static_cast<unsigned int>( size() ) )
    resize( idx > 2 * static_cast<unsigned int>( size() ) ? idx + 16 : 2 * size() );

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
void KMMsgList::insert( unsigned int idx, KMime::Message* aMsg, bool syncDict )
{
  if ( idx >= static_cast<unsigned int>( size() ) )
    resize( idx > 2 * static_cast<unsigned int>( size() ) ? idx + 16 : 2 * size() );

  if ( aMsg )
    mCount++;

  for ( unsigned int i = mHigh; i > idx; i-- ) {
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
unsigned int KMMsgList::append( KMime::Message* aMsg, bool syncDict )
{
  const unsigned int idx = mHigh;
  insert( idx, aMsg, syncDict ); // mHigh gets modified in here
  return idx;
}


//-----------------------------------------------------------------------------
void KMMsgList::remove( unsigned int idx )
{
  assert( idx < static_cast<unsigned int>( size() ) );
  if ( at(idx) ) {
    mCount--;
    KMMsgDict::mutableInstance()->remove( at(idx) );
  }

  mHigh--;
  for ( unsigned int i = idx; i < mHigh; i++ ) {
    KMMsgDict::mutableInstance()->update( at(i + 1), i + 1, i );
    operator[]( i ) = at( i+1 );
  }

  operator[]( mHigh ) = 0;

  rethinkHigh();
}


//-----------------------------------------------------------------------------
KMime::Content* KMMsgList::take( unsigned int idx )
{
  KMime::Content* msg = at( idx );
  remove( idx );
  return msg;
}


//-----------------------------------------------------------------------------
void KMMsgList::rethinkHigh()
{
  unsigned int sz = size();

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
