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
  KMMsgBasePtr msg;
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

void KMMsgList::qsort(int left, int right, SortField aField, bool aDescending) {
  debug("KMmsgList::qsort()");

  if(right <= left)
    return;

  KMMsgBasePtr mb;
  int i = left;
  int j = right;
  
  // $markus: Yes, I am paranoid ;-)
  if((i + j / 2) < 0 || (i +j / 2) > KMMsgListInherited::size())
    return;
  KMMsgBasePtr pivot = KMMsgListInherited::at((int)((i + j) / 2));

  do {
    while(msgSortCompFunc(KMMsgListInherited::at(i), pivot, aField, aDescending) < 0)
      i++;
    while(msgSortCompFunc(pivot, KMMsgListInherited::at(j), aField, aDescending) < 0)
      j--;
    if(i <= j) {
      if(i != j) {
	mb = KMMsgListInherited::at(i);
	KMMsgListInherited::at(i) = KMMsgListInherited::at(j);
	KMMsgListInherited::at(j) = mb;
      }
      i++;
      j--;
    }
  } while(i <= j);

    if(left < j)
      this->qsort(left, j, aField, aDescending);
    if(i < right)
      this->qsort(i, right, aField, aDescending);

    debug("KMsgList::qsort() leaving");
}

//-----------------------------------------------------------------------------
void KMMsgList::sort(SortField aField, bool aDescending)
{
#ifndef SLOW_SORT
  qsort(0, mHigh-1, aField, aDescending);
#else
  debug("KMMsgList::sort()");
  int i, j;
  KMMsgBasePtr mb;

  if (mHigh < 2) return;

  for (j=0; j<mHigh; j++)
  {
    for (i=0; i<mHigh-1; i++)
    {
      if (msgSortCompFunc(KMMsgListInherited::at(i), KMMsgListInherited::at(i+1),
			  aField, aDescending) > 0)
      {
	mb = KMMsgListInherited::at(i);
	KMMsgListInherited::at(i) = KMMsgListInherited::at(i+1);
	KMMsgListInherited::at(i+1) = mb;
      }
    }
  }
#endif
}


//-----------------------------------------------------------------------------
int KMMsgList::msgSortCompFunc(KMMsgBasePtr mbA, KMMsgBasePtr mbB, 
			       KMMsgList::SortField sortCriteria, bool desc)
{
  int res = 0;

  if (sortCriteria==sfNone)
    res = mbA->compareByIndex(mbB);
  else
  {
    if (sortCriteria==sfStatus)
      res = mbA->compareByStatus(mbB);

    else if (sortCriteria==sfFrom)
      res = mbA->compareByFrom(mbB);

    else if (res==0 || sortCriteria==sfSubject)
      res = mbA->compareBySubject(mbB);

    if (res==0 || sortCriteria==sfDate)
      res = mbA->compareByDate(mbB);
  }

  if (desc) return -res;
  return res;
}
