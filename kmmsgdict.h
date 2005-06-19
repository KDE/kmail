/*
 * This file is part of KMail, the KDE mail client
 * Copyright (c)  Ronen Tzur <rtzur@shani.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef __KMMSGDICT
#define __KMMSGDICT

class KMFolder;
class KMMsgBase;
class KMMessage;
class KMMsgDictEntry;
class KMMsgDictREntry;
class KMDict;
class QString;

/**
 * @short KMail message dictionary. Keeps location information for every
 * message. The message serial number is the key for the dictionary.
 *
 * The KMMsgDict singleton is managed by the KMail kernel and is used to look
 * up at which index in which folder a certain serial number can be found. Each
 * folder holds a "reverse entry", which is an array of message dict entries for
 * that folder and persists that to disk. In effect the whole message dict is therefor
 * persisted per folder and restored on startup when all folder dict entries
 * are read and re-enter their respective entries into the global dict.
 *
 * @author Ronen Tzur <rtzur@shani.net>
 */
class KMMsgDict
{
public:
  KMMsgDict();
  ~KMMsgDict();

  /** Insert a new message.  The message serial number is specified in
   * @p msgSerNum and may be zero, in which case a new serial number is
   * generated.  Returns the message serial number. */
  unsigned long insert(unsigned long msgSerNum, const KMMsgBase *msg, int index = -1);
  unsigned long insert(unsigned long msgSerNum, const KMMessage *msg, int index = -1);

  /** Insert a new message.  The message serial number is taken from
   * the message, and passed to the other insert().  Returns the message
   * serial number. */
  unsigned long insert(const KMMsgBase *msg, int index = -1);

  /** Set the serial number of @p msg to @p msgSerNum */
  void replace(unsigned long msgSerNum,
	       const KMMsgBase *msg, int index = -1);

  /** Removes a message. */
  void remove(unsigned long msgSerNum);

  /** Removes a message, and returns its message serial number. */
  unsigned long remove(const KMMsgBase *msg);

  /** Updates index for a message. */
  void update(const KMMsgBase *msg, int index, int newIndex);

  /** Returns (folder,index) pair for a message. */
  void getLocation(unsigned long key, KMFolder **retFolder, int *retIndex);
  void getLocation(const KMMsgBase *msg, KMFolder **retFolder, int *retIndex);
  void getLocation(const KMMessage *msg, KMFolder **retFolder, int *retIndex);

  /** Returns a message serial number for a (folder,index) pair.  Zero
    if no such message. */
  unsigned long getMsgSerNum(KMFolder *folder, int index);

  /** Returns the name of the .folder.index.ids file. */
  static QString getFolderIdsLocation(const KMFolder *folder);

  /** Returns TRUE if the .folder.index.ids file should not be read. */
  bool isFolderIdsOutdated(const KMFolder *folder);

  /** Reads the .folder.index.ids file.  Returns 0 on success. */
  int readFolderIds(KMFolder *folder);

  /** Writes the .folder.index.ids file.  Returns 0 on success. */
  int writeFolderIds(KMFolder *folder);

  /** Touches the .folder.index.ids file.  Returns 0 on success. */
  int touchFolderIds(KMFolder *folder);

  /** Appends the message to the .folder.index.ids file.
   * Returns 0 on success. */
  int appendtoFolderIds(KMFolder *folder, int index);

  /** Returns true if the folder has a .folder.index.ids file.  */
  bool hasFolderIds(const KMFolder *folder);

  /** Removes the .folder.index.ids file. */
  bool removeFolderIds(KMFolder *folder);

  // delete an entry that has been assigned to a folder
  static void deleteRentry(KMMsgDictREntry *entry);

protected:
  /** Returns the next message serial number for use. */
  unsigned long getNextMsgSerNum();

  /** Opens the .folder.index.ids file, and writes the header
   * information at the beginning of the file. */
  KMMsgDictREntry *openFolderIds(KMFolder *folder, bool truncate);

  /** The dictionary. */
  KMDict *dict;

  /** Highest message serial number we know of. */
  unsigned long nextMsgSerNum;
};

#endif /* __KMMSGDICT */
