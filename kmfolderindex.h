/* -*- mode: C++; c-file-style: "gnu" -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// Virtual base class for mail folder with .*.index style index

#ifndef kmfolderindex_h
#define kmfolderindex_h

#include <QtCore/QFlags>

#include "folderstorage.h"
#include "kmmsglist.h"

#ifdef KMAIL_SQLITE_INDEX
struct sqlite3;
#endif

/**
 * @short A FolderStorage with an index for faster access to often used
 * message properties..
 *
 * This class represents a message store which has an index for providing fast
 * access to often used message properties, namely those displayed in the list
 * of messages (KMHeaders).
 * 
 * @author Don Sanders <sanders@kde.org>
 */

class KMFolderIndex: public FolderStorage
{
  Q_OBJECT
  //TODO:Have to get rid of this friend declaration and add necessary pure
  //virtuals to kmfolder.h so that KMMsgBase::parent() can be a plain KMFolder
  //rather than a KMFolderIndex. Need this for database indices.
  friend class ::KMMsgBase;
public:

  /** This enum indicates the status of the index file. It's returned by
      indexStatus().
   */
  enum IndexStatus { IndexOk,
                     IndexMissing,
                     IndexTooOld
  };

  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  explicit KMFolderIndex(KMFolder* folder, const char* name=0);
  virtual ~KMFolderIndex();
  virtual int count(bool cache = false) const;

  virtual KMMsgBase* takeIndexEntry( int idx ) { return mMsgList.take( idx ); }
  virtual KMMsgInfo* setIndexEntry( int idx, KMMessage *msg );
  virtual void clearIndex(bool autoDelete=true, bool syncDict = false);
  virtual void truncateIndex();

  virtual const KMMsgBase* getMsgBase(int idx) const { return mMsgList[idx]; }
  virtual KMMsgBase* getMsgBase(int idx) { return mMsgList[idx]; }

  virtual int find(const KMMsgBase* msg) const {
    return mMsgList.indexOf( const_cast<KMMsgBase*>( msg ) );
  }

  int find( const KMMessage * msg ) const { return FolderStorage::find( msg ); }

  /**
   * Adds all messages of this folder to the serial number cache
   * (by calling MessageProperty::setSerialCache for each message).
   *
   * This makes subsequent calls to KMMsgBase::getMsgSerNum() much faster since
   * the serial number is already in the cache.
   *
   * The folder needs to be open for this.
   */
  void addToSerialCache() const;

  /** Registered unique serial number for the index file */
  int serialIndexId() const { return mIndexId; }

#ifndef KMAIL_SQLITE_INDEX
  /** If we have mmap(2), then we have a pointer to the
   *   mmap()ed index file; return a pointer to the memory
   *   region containing the file.
   */
  uchar *indexStreamBasePtr() { return mIndexStreamPtr; }

  /** If we have mmap(2), then we know the size of the
    *   index file. Return it. 0 if we don't have mmap(2).
    */
  size_t indexStreamLength() const { return mIndexStreamPtrLength; }

  bool indexSwapByteOrder() { return mIndexSwapByteOrder; }
  int  indexSizeOfLong() { return mIndexSizeOfLong; }
#endif // !KMAIL_SQLITE_INDEX

  virtual int writeIndex( bool createEmptyIndex = false );

  bool recreateIndex();

  //! options for openInternal()
  enum OpenInternalOption {
    NoOptions = 0x0,
    CheckIfIndexTooOld = 0x1,
    CreateIndexFromContentsWhenReadIndexFailed = 0x2
  };
  Q_DECLARE_FLAGS( OpenInternalOptions, OpenInternalOption )

public slots:
  /** Incrementally update the index if possible else call writeIndex */
  virtual int updateIndex();

protected:
  bool readIndex();

  /** Read index header. Called from within readIndex(). */
  bool readIndexHeader(int *gv=0);

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents() = 0;

  /** Tests whether the contents of this folder is newer than the index.
      Should return IndexTooOld if the index is older than the contents.
      Should return IndexMissing if there is contents but no index.
      Should return IndexOk if the folder doesn't exist anymore "physically"
      or if the index is not older than the contents.
  */
  virtual IndexStatus indexStatus() = 0;

    /** Inserts messages into the message dictionary by iterating over the
     * message list. The messages will get new serial numbers. This is only
     * used on newly appeared folders, where there is no .ids file yet, or
     * when that has been invalidated. */
  virtual void fillMessageDict();

  /** Writes messages to the index. The stream is flushed if @a flush is true. 
   If @a msg is 0, all mesages from mMsgList are written, else only @a is written.
  */
  int writeMessages( KMMsgBase* msg, bool flush = true );

  /** @overload writeMessages( KMMsgBase* msg, bool flush ) 
   Allows to specify index stream to use. */
  int writeMessages( KMMsgBase* msg, bool flush, FILE* indexStream );

  /** Opens index stream (or database) without creating it.
   If @a checkIfIndexTooOld is true, message "The index of folder .. seems 
   to be out of date" is displayed.
   Called by KMFolderMaildir::open() and KMFolderMbox::open(). */
  int openInternal( OpenInternalOptions options );

  /** Creates index stream (or database).
   Called by KMFolderMaildir::create() and KMFolderMbox::create(). */
  int createInternal();

  /** list of index entries or messages */
  KMMsgList mMsgList;


#ifdef KMAIL_SQLITE_INDEX
  /** Opens database pointed by indexLocation() filename in mode @a mode,
   which can be SQLITE_OPEN_READWRITE or SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE. 
   Sets mIndexDb. */
  bool openDatabase( int mode );

  /** Used by writeMessages() */
  enum WriteMessagesMode {
    OverwriteMessages, //!< writeMessages() will first clear all messages and then write them
    UpdateExistingMessages //!< writeMessages() will update data for existing messages and add any new message
  };

  /* Writes messages to index database for the folder. Inserts or replaces existing messages,
   depending on @a mode. */
  int writeMessages( KMMsgBase* msg, WriteMessagesMode mode );

  /* Executes "DELETE FROM messages WHERE id=.." for every id included in @a rowsToDelete. 
   Used in readIndex() for messages with 0 serial number, especially for the outbox. */
  bool deleteIndexRows( const QList<sqlite3_int64>& rowsToDelete );

  /** main SQlite db handle */
  sqlite3 *mIndexDb;
#else
  bool updateIndexStreamPtr(bool just_close=false);

  /** table of contents file */
  FILE* mIndexStream;
  /** offset of header of index file */
  off_t mHeaderOffset;
  uchar *mIndexStreamPtr;
  size_t mIndexStreamPtrLength;

  bool mIndexSwapByteOrder; // Index file was written with swapped byte order
  int mIndexSizeOfLong; // Index file was written with longs of this size
#endif

  int mIndexId;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( KMFolderIndex::OpenInternalOptions )

#endif /*kmfolderindex_h*/
