/* List of basic messages. Used in the KMFolder class.
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsglist_h
#define kmmsglist_h

#include <qarray.h>
#include "kmmsgbase.h"


#define KMMsgListInherited QArray<KMMsgBasePtr>
class KMMsgList: public QArray<KMMsgBasePtr>
{
public:
  /** Valid parameters for sort() */
  typedef enum { sfNone=99, sfStatus=0, sfFrom=1, sfSubject=2, sfDate=3 } SortField;

  /** Constructor with optional initial size. */
  KMMsgList(int initialSize=32);

  /** Destructor also deletes all messages in the list. */
  virtual ~KMMsgList();

  /** Remove message at given index without deleting it. */
  virtual void remove(int idx);

  /** Returns message at given index and removes it from the list. */
  virtual KMMsgBasePtr take(int idx);

  /** Insert message at given index. Resizes the array if necessary. */
  virtual void insert(int idx, KMMsgBasePtr msg);

  /** Append given message after the last used message. Resizes the
    array if necessary. Returns index of new position. */
  virtual int append(KMMsgBasePtr msg);

  /** Clear messages. If autoDelete is set (default) the messages are 
      deleted. The array is not resized. */
  virtual void clear(bool autoDelete=TRUE);

  /** Resize array and initialize new elements if any. Returns
    FALSE if memory cannot be allocated. */
  virtual bool resize(int size);

  /** Clear the array and resize it to given size. Returns FALSE
    if memory cannot be allocated. */
  virtual bool reset(int size);

  /** Returns message at given index. */
  virtual KMMsgBasePtr at(int idx) { return KMMsgListInherited::at(idx); }
  const KMMsgBasePtr at(int idx) const { return KMMsgListInherited::at(idx); }
  KMMsgBasePtr operator[](int idx) { return KMMsgListInherited::at(idx); }
  const KMMsgBasePtr operator[](int idx) const { return KMMsgListInherited::at(idx); }

  /** Set message at given index. The array is resized if necessary. If
   there is already a message at the given index this message is *not*
   deleted. */
  virtual void set(int idx, KMMsgBasePtr msg);

  /** Sort messages by given field. */
  virtual void sort(SortField byField=sfDate, bool descending=FALSE);
  virtual void qsort(int from, int to, SortField byField=sfDate, bool descending=FALSE);

  /** Returns first unused index (index of last message plus one). */
  int high(void) const { return mHigh; }

  /** Number of messages in the array. */
  int count(void) const { return mCount; }

  /** Size of the array. */
  int size(void) const { return ((int)KMMsgListInherited::size()); }

protected:
  /** Set mHigh to proper value */
  void rethinkHigh(void);

  /** Function that does the compare in sort() method. */
  static int msgSortCompFunc(KMMsgBasePtr, KMMsgBasePtr, KMMsgList::SortField, bool);

  int mHigh, mCount;
};


#endif /*kmmsglist_h*/
