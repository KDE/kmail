// kmmsglist.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmsglist.h"
#include "kmmsgdict.h"
#include "kmkernel.h"
#include <assert.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
KMMsgList::KMMsgList(int initSize)
  : QMemArray<KMMsgBase*>(initSize),
    mHigh( 0 ), mCount( 0 )
{
  if ( size() > 0 )
    for (unsigned int i=size(); i>0; i--)
	QMemArray<KMMsgBase*>::at(i-1) = 0;
}


//-----------------------------------------------------------------------------
KMMsgList::~KMMsgList()
{
  clear(TRUE);
}


//-----------------------------------------------------------------------------
void KMMsgList::clear(bool doDelete, bool syncDict)
{
  KMMsgDict *dict = 0;
  if (syncDict)
    dict = kmkernel->msgDict();

  if ( mHigh > 0 )
    for (unsigned int i=mHigh; i>0; i--)
    {
      KMMsgBase * msg = at(i-1);
      if (msg) {
	if (dict)
	  dict->remove(msg);
	at(i-1) = 0;
	if (doDelete) delete msg;
      }
    }
  mHigh  = 0;
  mCount = 0;
}


//-----------------------------------------------------------------------------
bool KMMsgList::resize(unsigned int aSize)
{
  unsigned int i, oldSize = size();
  KMMsgBase* msg;

  // delete messages that will get lost, if any
  if (aSize < mHigh)
  {
    for (i=aSize; i<mHigh; i++)
    {
      msg = at(i);
      if (msg)
      {
	delete msg;
	mCount--;
      }
      mHigh = aSize;
    }
  }

  // do the resizing
  if (!QMemArray<KMMsgBase*>::resize(aSize)) return FALSE;

  // initialize new elements
  for (i=oldSize; i<aSize; i++)
    at(i) = 0;

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMMsgList::reset(unsigned int aSize)
{
  if (!resize(aSize)) return FALSE;
  clear();
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgList::set(unsigned int idx, KMMsgBase* aMsg)
{
  if (idx >= size())
    resize( idx > 2 * size() ? idx + 16 : 2 * size() );

  if (!at(idx) && aMsg) mCount++;
  else if (at(idx) && !aMsg) mCount--;

  delete at(idx);
  
  at(idx) = aMsg;

  if (!aMsg || idx >= mHigh) rethinkHigh();
}


//-----------------------------------------------------------------------------
void KMMsgList::insert(unsigned int idx, KMMsgBase* aMsg, bool syncDict)
{
  KMMsgDict *dict = 0;
  if (syncDict)
    dict = kmkernel->msgDict();

  if (idx >= size())
    resize( idx > 2 * size() ? idx + 16 : 2 * size() );

  if (aMsg) mCount++;

  for (unsigned int i=mHigh; i>idx; i--) {
    if (dict)
      dict->remove(at(i - 1));
    at(i) = at(i-1);
    if (dict)
      dict->insert(at(i), i);
  }

  at(idx) = aMsg;
  if (dict)
    dict->insert(at(idx), idx);

  mHigh++;
}


//-----------------------------------------------------------------------------
unsigned int KMMsgList::append(KMMsgBase* aMsg, bool syncDict)
{
  const unsigned int idx = mHigh;
  insert(idx, aMsg, syncDict); // mHigh gets modified in here
  return idx;
}


//-----------------------------------------------------------------------------
void KMMsgList::remove(unsigned int idx)
{
  assert(idx<size());
  KMMsgDict *dict = kmkernel->msgDict();
  
  if (at(idx)) {
    mCount--;
    if (dict)
      dict->remove(at(idx));
  }
  
  mHigh--;
  for (unsigned int i=idx; i<mHigh; i++) {
    if (dict)
      dict->update(at(i + 1), i + 1, i);
    at(i) = at(i+1);
  }
  
  at(mHigh) = 0;

  rethinkHigh();
}


//-----------------------------------------------------------------------------
KMMsgBase* KMMsgList::take(unsigned int idx)
{
  KMMsgBase* msg=at(idx);
  remove(idx);
  return msg;
}


//-----------------------------------------------------------------------------
void KMMsgList::rethinkHigh()
{
  unsigned int sz = size();

  if (mHigh < sz && at(mHigh))
  {
    // forward search
    while (mHigh < sz && at(mHigh))
      mHigh++;
  }
  else
  {
    // backward search
    while (mHigh>0 && !at(mHigh-1))
      mHigh--;
  }
}

//-----------------------------------------------------------------------------
void KMMsgList::fillMsgDict(KMMsgDict *dict)
{
  for (unsigned int idx = 0; idx < mHigh; idx++)
    if (at(idx))
      dict->insert(0, at(idx), idx);
}
