/* simple hash table for kmail.  inspired by QDict */
/* Author: Ronen Tzur <rtzur@shani.net> */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmdict.h"

#include <string.h>

//-----------------------------------------------------------------------------

KMDict::KMDict(int size)
{
  init(size);
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

void KMDict::replace(long key, KMDictItem *item)
{
  item->key = key;
  int idx = (unsigned long)key % mSize; // insert in
  item->next = mVecs[idx];              // appropriate
  mVecs[idx] = item;                    // column
  removeFollowing(item, key);           // remove other items with same key
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
