// kmmsglist.cpp

#include "kmmsglist.h"
#include <assert.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
KMMsgList::KMMsgList(int initSize): KMMsgListInherited(initSize)
{
  mHigh  = size();
  mCount = 0;
  clear(FALSE);
}


//-----------------------------------------------------------------------------
KMMsgList::~KMMsgList()
{
  clear(TRUE);
}


//-----------------------------------------------------------------------------
void KMMsgList::clear(bool doDelete)
{
  KMMsgBasePtr msg = 0;
  long i;

  for (i=mHigh-1; i>=0; i--)
  {
    msg = at(i);
    KMMsgListInherited::at(i) = NULL;

    if (msg && doDelete) delete msg;
  }
  mHigh  = 0;
  mCount = 0;
}


//-----------------------------------------------------------------------------
bool KMMsgList::resize(int aSize)
{
  int i, oldSize = size();
  KMMsgBasePtr msg;

  assert(aSize>=0);

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
  if (!KMMsgListInherited::resize(aSize)) return FALSE;

  // initialize new elements
  for (i=oldSize; i<aSize; i++)
    KMMsgListInherited::at(i) = NULL;

  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMMsgList::reset(int aSize)
{
  if (!resize(aSize)) return FALSE;
  clear();
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMMsgList::set(int idx, KMMsgBasePtr aMsg)
{
  int doubleSize;

  assert(idx>=0);

  if (idx >= size())
  {
    doubleSize = size() << 1;
    resize(idx>doubleSize ? idx+16 : doubleSize);
  }

  if (!at(idx) && aMsg) mCount++;
  else if (at(idx) && !aMsg) mCount--;

  delete at(idx);
  KMMsgListInherited::at(idx) = aMsg;
  if (!aMsg || idx >= mHigh) rethinkHigh();
}


//-----------------------------------------------------------------------------
void KMMsgList::insert(int idx, KMMsgBasePtr aMsg)
{
  int i, doubleSize;

  assert(idx>=0);

  if (idx >= size())
  {
    doubleSize = size() << 1;
    resize(idx>doubleSize ? idx+16 : doubleSize);
  }

  if (aMsg) mCount++;

  for (i=mHigh; i>idx; i--)
    KMMsgListInherited::at(i) = KMMsgListInherited::at(i-1);

  KMMsgListInherited::at(idx) = aMsg;
  mHigh++;
}


//-----------------------------------------------------------------------------
int KMMsgList::append(KMMsgBasePtr aMsg)
{
  int idx = mHigh;
  insert(idx, aMsg); // mHigh gets modified in here
  return idx;
}


//-----------------------------------------------------------------------------
void KMMsgList::remove(int idx)
{
  int i;

  assert(idx>=0 && idx<size());

  if (KMMsgListInherited::at(idx)) mCount--;

  mHigh--;
  for (i=idx; i<mHigh; i++)
    KMMsgListInherited::at(i) = KMMsgListInherited::at(i+1);
  KMMsgListInherited::at(mHigh) = NULL;

  rethinkHigh();
}


//-----------------------------------------------------------------------------
KMMsgBasePtr KMMsgList::take(int idx)
{
  KMMsgBasePtr msg=at(idx);
  remove(idx);
  return msg;
}


//-----------------------------------------------------------------------------
void KMMsgList::rethinkHigh(void)
{
  int sz = (int)size();

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
