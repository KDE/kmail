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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef __KMMSGDICT
#define __KMMSGDICT

#include <QList>

class KMFolder;
class KMMsgBase;
class KMMessage;
class KMMsgDictREntry;
class KMDict;
class QString;
class FolderStorage;

class KMMsgDictSingletonProvider;

/**
 * @short KMail message dictionary. Keeps location information for every
 * message. The message serial number is the key for the dictionary.
 *
 * The KMMsgDict singleton is used to look up at which index in which folder a
 * certain serial number can be found. Each folder holds a "reverse entry",
 * which is an array of message dict entries for that folder and persists that
 * to disk as an array of serial numbers, the "$folder.index.ids" file.
 * In effect the whole message dict is therefor persisted per folder
 * and restored on startup when all folder dict entries are read and re-enter
 * their respective entries (serial numbers) into the global dict. The code for
 * creating, deleting and manipulating these files is in this class, rather than
 * the FolderStorage class, which only holds the pointer to the reverse entry
 * and otherwise knows nothing of the message dict.
 *
 * @author Ronen Tzur <rtzur@shani.net>
 */
class KMMsgDict
{
  friend class KMMsgDictSingletonProvider;
  public:
    /** Access the globally unique MessageDict */
    static const KMMsgDict* instance();

    /** Returns the folder the message represented by the serial number @p key is in
     * and the index in that folder at which it is stored. */
    void getLocation( unsigned long key, KMFolder **retFolder, int *retIndex ) const;
    /** Returns the folder the message represented by @p msg is in
      * and the index in that folder at which it is stored. */
    void getLocation( const KMMsgBase *msg, KMFolder **retFolder, int *retIndex ) const;
    /** Returns the folder the message represented by @p msg is in
     * and the index in that folder at which it is stored. */
    void getLocation( const KMMessage *msg, KMFolder **retFolder, int *retIndex ) const;

  /** Find the message serial number for the message located at index @p index in folder
   * @p folder.
   * @return the message serial number or zero is no such message can be found */
    unsigned long getMsgSerNum( KMFolder *folder, int index ) const;

  /** Convert a list of KMMsgBase pointers to a list of serial numbers */
    static QList<unsigned long> serNumList(QList<KMMsgBase *> msgList);

private:
 /* FIXME It would be better to do without these, they are the classes
  * involved in filling and maintaining the dict. The MsgList needs access
  * because of things it does that should be in FolderIndex, probably, which
  * the message list is an implementation detail of. */
  friend class FolderStorage;
  friend class KMMsgList;
  friend class KMFolderIndex;

  // Access for those altering the dict, our friend classes
  static KMMsgDict* mutableInstance();

  /** Insert a new message.  The message serial number is specified in
   * @p msgSerNum and may be zero, in which case a new serial number is
   * generated.  Returns the message serial number. */
  unsigned long insert(unsigned long msgSerNum, const KMMsgBase *msg, int index = -1);

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


  // ----- per folder serial number on-disk structure handling ("ids files")

  /** Returns the name of the .folder.index.ids file. */
  static QString getFolderIdsLocation( const FolderStorage &folder );

  /** Returns true if the .folder.index.ids file should not be read. */
  bool isFolderIdsOutdated( const FolderStorage &folder );

  /** Reads the .folder.index.ids file.  Returns 0 on success. */
  int readFolderIds( FolderStorage & );

  /** Writes the .folder.index.ids file.  Returns 0 on success. */
  int writeFolderIds( const FolderStorage & );

  /** Touches the .folder.index.ids file.  Returns 0 on success. */
  int touchFolderIds( const FolderStorage & );

  /** Appends the message to the .folder.index.ids file.
   * Returns 0 on success. */
  int appendToFolderIds( FolderStorage&, int index );

  /** Returns true if the folder has a .folder.index.ids file.  */
  bool hasFolderIds( const FolderStorage & );

  /** Removes the .folder.index.ids file. */
  bool removeFolderIds( FolderStorage & );

  /** Opens the .folder.index.ids file, and writes the header
   * information at the beginning of the file. */
  KMMsgDictREntry *openFolderIds( const FolderStorage &, bool truncate);


  // --------- helpers ------------

  /** delete an entry that has been assigned to a folder. Needs to be done from
   * inside this file, since operator delete is not available outside. */
  static void deleteRentry(KMMsgDictREntry *entry);

  /** Returns the next message serial number for use. */
  unsigned long getNextMsgSerNum();

  // prevent creation and deletion, we are a singleton
  KMMsgDict();
  ~KMMsgDict();

  /** Highest message serial number we know of. */
  unsigned long nextMsgSerNum;

  /** The dictionary. */
  KMDict *dict;
};

#endif /* __KMMSGDICT */
