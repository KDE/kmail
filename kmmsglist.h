/* List of basic messages. Used in the KMFolder class.
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsglist_h
#define kmmsglist_h

#include "kmmsgbase.h"

#include <qmemarray.h>

class KMMsgDict;

class KMMsgList: public QMemArray<KMMsgBase*>
{
public:
  /** Valid parameters for sort() */
  typedef enum { sfNone=99, sfStatus=0, sfFrom=1, sfSubject=2, sfDate=3 } SortField;

  /** Constructor with optional initial size. */
  KMMsgList(int initialSize=32);

  /** Destructor also deletes all messages in the list. */
  ~KMMsgList();

  /** Remove message at given index without deleting it.
    Also removes from message dictionary. */
  void remove(unsigned int idx);

  /** Returns message at given index and removes it from the list.
    Also removes from message dictionary. */
  KMMsgBase* take(unsigned int idx);

  /** Insert message at given index. Resizes the array if necessary.
    If @p syncDict, also updates message dictionary. */
  void insert(unsigned int idx, KMMsgBase* msg, bool syncDict = true);

  /** Append given message after the last used message. Resizes the
    array if necessary. Returns index of new position.
    If @p syncDict, also updates message dictionary. */
  unsigned int append(KMMsgBase* msg, bool syncDict = true);

  /** Clear messages. If autoDelete is set (default) the messages are
      deleted. The array is not resized.  If @p syncDict, also updates
      the message dictionary. */
  void clear(bool autoDelete=TRUE, bool syncDict = false);

  /** Resize array and initialize new elements if any. Returns
    FALSE if memory cannot be allocated. */
  bool resize(unsigned int size);

  /** Clear the array and resize it to given size. Returns FALSE
    if memory cannot be allocated. */
  bool reset(unsigned int size);

  /** Set message at given index. The array is resized if necessary. If
   there is already a message at the given index this message is *not*
   deleted.  Does not sync the message dictionary. */
  void set(unsigned int idx, KMMsgBase* msg);

  /** Returns first unused index (index of last message plus one). */
  unsigned int high() const { return mHigh; }

  /** Number of messages in the array. */
  unsigned int count() const { return mCount; }

  /** Inserts messages into the message dictionary.  Might be called
    during kernel initialization. */
  void fillMsgDict(KMMsgDict *dict);

protected:
  /** Set mHigh to proper value */
  void rethinkHigh();

  unsigned int mHigh, mCount;
};


#endif /*kmmsglist_h*/
