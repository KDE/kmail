/* simple hash table for kmail.  inspired by QDict */
/* Author: Ronen Tzur <rtzur@shani.net> */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmdict.h"
#include <kdebug.h>

#include <string.h>
#include <algorithm>

// List of prime numbers shamelessly stolen from GCC STL
enum { num_primes = 29 };

static const unsigned long prime_list[ num_primes ] =
{
  31ul,        53ul,         97ul,         193ul,       389ul,
  769ul,       1543ul,       3079ul,       6151ul,      12289ul,
  24593ul,     49157ul,      98317ul,      196613ul,    393241ul,
  786433ul,    1572869ul,    3145739ul,    6291469ul,   12582917ul,
  25165843ul,  50331653ul,   100663319ul,  201326611ul, 402653189ul,
  805306457ul, 1610612741ul, 3221225473ul, 4294967291ul
};

inline unsigned long nextPrime( unsigned long n )
{
  const unsigned long *first = prime_list;
  const unsigned long *last = prime_list + num_primes;
  const unsigned long *pos = std::lower_bound( first, last, n );
  return pos == last ? *( last - 1 ) : *pos;
}

//-----------------------------------------------------------------------------

KMDict::KMDict( int size )
{
  init( ( int ) nextPrime( size ) );
  kdDebug( 5006 ) << "KMMDict::KMDict Size: " << mSize << endl;
}

//-----------------------------------------------------------------------------

KMDict::~KMDict()
{
  clear();
}

//-----------------------------------------------------------------------------

void KMDict::init(int size)
{
  mSize = size;
  mVecs = new KMDictItem *[mSize];
  memset(mVecs, 0, mSize * sizeof(KMDictItem *));
}

//-----------------------------------------------------------------------------

void KMDict::clear()
{
  if (!mVecs)
    return;
  for (int i = 0; i < mSize; i++) {
    KMDictItem *item = mVecs[i];
    while (item) {
      KMDictItem *nextItem = item->next;
      delete item;
      item = nextItem;
    }
  }
  delete [] mVecs;
  mVecs = 0;
}

//-----------------------------------------------------------------------------

void KMDict::replace( long key, KMDictItem *item )
{
  insert( key, item );
  removeFollowing( item, key );           // remove other items with same key
}

//-----------------------------------------------------------------------------


void KMDict::insert( long key, KMDictItem *item )
{
  item->key = key;
  int idx = (unsigned long)key % mSize; // insert in
  item->next = mVecs[idx];              // appropriate
  mVecs[idx] = item;                    // column
}

//-----------------------------------------------------------------------------

void KMDict::remove(long key)
{
  int idx = (unsigned long)key % mSize;
  KMDictItem *item = mVecs[idx];

  if (item) {
    if (item->key == key) {             // if first in the column
      mVecs[idx] = item->next;
      delete item;
    } else
      removeFollowing(item, key);       // if deep in the column
  }
}

//-----------------------------------------------------------------------------

void KMDict::removeFollowing(KMDictItem *item, long key)
{
  while (item) {
    KMDictItem *itemNext = item->next;
    if (itemNext && itemNext->key == key) {
      KMDictItem *itemNextNext = itemNext->next;
      delete itemNext;
      item->next = itemNextNext;
    } else
      item = itemNext;
  }
}

//-----------------------------------------------------------------------------

KMDictItem *KMDict::find(long key)
{
  int idx = (unsigned long)key % mSize;
  KMDictItem *item = mVecs[idx];
  while (item) {
    if (item->key == key)
      break;
    item = item->next;
  }
  return item;
}
