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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// Virtual base class for mail folder with .*.index style index

#ifndef kmfolderindex_h
#define kmfolderindex_h

#include "folderstorage.h"
#include "kmmsglist.h"

class KMFolderIndex: public FolderStorage
{
  Q_OBJECT
  //TODO:Have to get rid of this friend declaration and add necessary pure
  //virtuals to kmfolder.h so that KMMsgBase::parent() can be a plain KMFolder
  //rather than a KMFolderIndex. Need this for database indices.
  friend class KMMsgBase;
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
  KMFolderIndex(KMFolder* folder, const char* name=0);
  virtual ~KMFolderIndex();
  virtual int count(bool cache = false) const;

  virtual KMMsgBase* takeIndexEntry( int idx ) { return mMsgList.take( idx ); }
  virtual KMMsgInfo* setIndexEntry( int idx, KMMessage *msg );
  virtual void clearIndex(bool autoDelete=true, bool syncDict = false);
  virtual void fillDictFromIndex(KMMsgDict *dict);
  virtual void truncateIndex();

  virtual const KMMsgBase* getMsgBase(int idx) const { return mMsgList[idx]; }
  virtual KMMsgBase* getMsgBase(int idx) { return mMsgList[idx]; }

  virtual int find(const KMMsgBase* msg) const { return mMsgList.find((KMMsgBase*)msg); }
  int find( const KMMessage * msg ) const { return FolderStorage::find( msg ); }

  /** Registered unique serial number for the index file */
  int serialIndexId() const { return mIndexId; }

  uchar *indexStreamBasePtr() { return mIndexStreamPtr; }

  bool indexSwapByteOrder() { return mIndexSwapByteOrder; }
  int  indexSizeOfLong() { return mIndexSizeOfLong; }

  virtual QString indexLocation() const;
  virtual int writeIndex( bool createEmptyIndex = false );

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

  bool updateIndexStreamPtr(bool just_close=FALSE);

  /** Tests whether the contents of this folder is newer than the index.
      Should return IndexTooOld if the index is older than the contents.
      Should return IndexMissing if there is contents but no index.
      Should return IndexOk if the folder doesn't exist anymore "physically"
      or if the index is not older than the contents.
  */
  virtual IndexStatus indexStatus() = 0;

  /** table of contents file */
  FILE* mIndexStream;
  /** list of index entries or messages */
  KMMsgList mMsgList;

  /** offset of header of index file */
  off_t mHeaderOffset;

  uchar *mIndexStreamPtr;
  int mIndexStreamPtrLength, mIndexId;
  bool mIndexSwapByteOrder; // Index file was written with swapped byte order
  int mIndexSizeOfLong; // Index file was written with longs of this size
};

#endif /*kmfolderindex_h*/
