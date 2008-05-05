/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmmsglist_h
#define kmmsglist_h

#include "kmmsgbase.h"

#include <QVector>

/**
 * @short An abstraction of an array of pointers to messages.
 *
 * This class represents an array of pointers to message objects. It
 * autoresizes and can load a KMMsgDict object from its contents. It's
 * a pure implementation detail of KMFolderIndex and should not be used by
 * the layers above that.
 *
 * @author Stefan Taferner <taferner@kde.org>
 */
class KMMsgList: public QVector<KMMsgBase*>
{
public:

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
  void clear(bool autoDelete=true, bool syncDict = false);

  /** Resize array and initialize new elements if any. Returns
    false if memory cannot be allocated. */
  bool resize(unsigned int size);

  /** Clear the array and resize it to given size. Returns false
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

protected:
  /** Set mHigh to proper value */
  void rethinkHigh();

  unsigned int mHigh, mCount;
};


#endif /*kmmsglist_h*/
