/* List of basic messages. Used in the KMFolder class.
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsglist_h
#define kmmsglist_h

#include "kmmsgbase.h"

#include <qmemarray.h>

class KMMsgDict;

#define KMMsgListInherited QMemArray<KMMsgBase*>
class KMMsgList: public QMemArray<KMMsgBase*>
{
public:
  /** Valid parameters for sort() */
  typedef enum { sfNone=99, sfStatus=0, sfFrom=1, sfSubject=2, sfDate=3 } SortField;

  /** Constructor with optional initial size. */
  KMMsgList(int initialSize=32);

  /** Destructor also deletes all messages in the list. */
  virtual ~KMMsgList();

  /** Remove message at given index without deleting it.
    Also removes from message dictionary. */
  virtual void remove(int idx);

  /** Returns message at given index and removes it from the list.
    Also removes from message dictionary. */
  virtual KMMsgBase* take(int idx);

  /** Insert message at given index. Resizes the array if necessary.
    If @p syncDict, also updates message dictionary. */
  virtual void insert(int idx, KMMsgBase* msg, bool syncDict = true);

  /** Append given message after the last used message. Resizes the
    array if necessary. Returns index of new position.
    If @p syncDict, also updates message dictionary. */
  virtual int append(KMMsgBase* msg, bool syncDict = true);

  /** Clear messages. If autoDelete is set (default) the messages are
      deleted. The array is not resized.  If @p syncDict, also updates
      the message dictionary. */
  virtual void clear(bool autoDelete=TRUE, bool syncDict = false);

  /** Resize array and initialize new elements if any. Returns
    FALSE if memory cannot be allocated. */
  virtual bool resize(int size);

  /** Clear the array and resize it to given size. Returns FALSE
    if memory cannot be allocated. */
  virtual bool reset(int size);

  /** Returns message at given index. */
  virtual KMMsgBase* at(int idx) { return KMMsgListInherited::at(idx); }
  const KMMsgBase* at(int idx) const { return KMMsgListInherited::at(idx); }
  KMMsgBase* operator[](int idx) { return KMMsgListInherited::at(idx); }
  const KMMsgBase* operator[](int idx) const { return KMMsgListInherited::at(idx); }

  /** Set message at given index. The array is resized if necessary. If
   there is already a message at the given index this message is *not*
   deleted.  Does not sync the message dictionary. */
  virtual void set(int idx, KMMsgBase* msg);

  /** Returns first unused index (index of last message plus one). */
  int high(void) const { return mHigh; }

  /** Number of messages in the array. */
  int count(void) const { return mCount; }

  /** Size of the array. */
  int size(void) const { return ((int)KMMsgListInherited::size()); }

  /** Inserts messages into the message dictionary.  Might be called
    during kernel initialization. */
  void fillMsgDict(KMMsgDict *dict);

protected:
  /** Set mHigh to proper value */
  void rethinkHigh(void);

  int mHigh, mCount;
};


#endif /*kmmsglist_h*/
