/* -*- mode: C++; c-file-style: "gnu" -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>
    Copyright (C) 2008 Jaroslaw Staniek <js@iidea.pl>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//-----------------------------------------------------------------------------
// utils

static QString errorMessage( int code, const char* msg )
{
  return QString("error #%1 message=%2").arg(code).arg( QString::fromUtf8( msg ) );
}

static QString errorMessage( int code, sqlite3* db )
{
  return errorMessage( code, sqlite3_errmsg(db) );
}

static QString querySingleString( sqlite3* db, const QString& sql, int _column, bool& _result )
{
  _result = false;
  if ( !db ) return QString();
  sqlite3_stmt *pStmt;
  int result = sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &pStmt, 0 );
  if ( result != SQLITE_OK ) {
    kWarning() << "sqlite3_prepare_v2() error: sql=" << sql << errorMessage( result, db );
    return QString();
  }
  result = sqlite3_step( pStmt );
  if ( result == SQLITE_ROW ) {
    const int column = (_column >= 0) ? _column : 0;
    const int type = sqlite3_column_type( pStmt, column );
    _result = true;
    QString str;
    if ( type != SQLITE_NULL ) {
      const char* utf8Str = (const char*)sqlite3_column_text( pStmt, column );
      str = QString::fromUtf8( utf8Str );
    }
    sqlite3_finalize( pStmt );
    return str;
  }
  sqlite3_finalize( pStmt );
  if ( result == SQLITE_DONE )
    _result = true;
  return QString();
}

static bool executeQuery( sqlite3* db, const char* sql )
{
  if ( !db ) return false;
  char* errMsg = 0;
  int result = sqlite3_exec( db, sql, 0, 0, &errMsg );
  if( result != SQLITE_OK ){
    kWarning() << "SQL error: sql="
      << QString::fromUtf8( sql ) << errorMessage( result, errMsg );
    sqlite3_free( errMsg );
    return false;
  }
  return true;
}

static bool executeQuery( sqlite3* db, const QString& sql )
{
  return executeQuery( db, sql.toUtf8().constData() );
}

//-----------------------------------------------------------------------------

KMFolderIndex::KMFolderIndex(KMFolder* folder, const char* name)
  : FolderStorage(folder, name), mMsgList(INIT_MSGS)
{
  mIndexDb = 0;
//  mIndexStream = 0;
//    mIndexStreamPtr = 0;
//    mIndexStreamPtrLength = 0;
//    mIndexSwapByteOrder = false;
//    mIndexSizeOfLong = sizeof(long);
    mIndexId = 0;
//    mHeaderOffset   = 0;
}


KMFolderIndex::~KMFolderIndex()
{
}

int KMFolderIndex::updateIndex( bool aboutToClose )
{
  if ( !aboutToClose && !isOpened() ) {
    kWarning() << "updateIndex() called on a closed folder!";
    return 0;
  }

// TODO: run SQL "update" only for dirty messages
  if (!mAutoCreateIndex)
    return 0;
  mDirtyTimer->stop();
  if ( mIndexDb &&
	  (!mDirty || 0 == writeMessages( 0/* all */, UpdateExistingMessages ) ) ) {
      touchFolderIdsFile();
      return 0;
  }
  return writeIndex();
}

bool KMFolderIndex::openDatabase( int mode )
{
  QString indexName = QDir::toNativeSeparators( indexLocation() );
  mode_t old_umask = umask( 077 );
  int result = sqlite3_open_v2( indexName.toUtf8().constData(), &mIndexDb,
    mode, 0 );
  umask( old_umask );
  if( result != SQLITE_OK ){
    kWarning() << "Can't open database " << errorMessage( result, mIndexDb );
    sqlite3_close(mIndexDb);
    mIndexDb = 0;
    return false;
  }
  return true;
}

int KMFolderIndex::writeIndex( bool createEmptyIndex )
{
  if ( !createEmptyIndex && !isOpened() ) {
    kWarning() << "writeIndex() called on a closed folder!";
    return 0;
  }

  QString indexName = QDir::toNativeSeparators( indexLocation() );
  if ( mIndexDb ) {
    sqlite3_close( mIndexDb );
    mIndexDb = 0;
  }
  {
    QFile indexFile( indexName );
    if ( indexFile.exists() && !indexFile.remove() )
      return 1;
  }

  bool ok = openDatabase( SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE );
  if ( !ok )
    return 1;

  // create data structures if not available
  // - index version
  QString sql( QLatin1String("PRAGMA user_version = ") + QString::number( INDEX_VERSION ) );
  if ( !executeQuery( mIndexDb, sql ) )
    return 1;

  // - 'messages' table
  if ( !executeQuery( mIndexDb, "CREATE TABLE messages( id INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB )" ) )
    return 1;

  // - insert messages
  sqlite3_stmt *pStmt = 0;
  if ( createEmptyIndex )
    return 0;
  else {
    if ( writeMessages( 0/*all*/ ) != 0 )
      return 1;
  }

//  mIndexStream = KDE_fopen(QFile::encodeName(indexName), "r+"); // index file
//  KMailStorageInternalsDebug << "KDE_fopen(indexName=" << indexName << ", \"r+\") == mIndexStream == " << mIndexStream;
//  assert( mIndexStream );
//#ifndef Q_WS_WIN
//  fcntl(fileno(mIndexStream), F_SETFD, FD_CLOEXEC);
//#endif

//  if ( !updateIndexStreamPtr() )
//    return 1;
  if ( 0 != writeFolderIdsFile() )
    return 1;

  setDirty( false );
  return 0;
}


bool KMFolderIndex::readIndex()
{
//  assert(mIndexStream != 0);
//  rewind(mIndexStream);

  clearIndex();
  int version;

  setDirty( false );

  if (!readIndexHeader(&version))
    return false;

  mUnreadMsgs = 0;
  mTotalMsgs = 0;
//  mHeaderOffset = KDE_ftell(mIndexStream);

  clearIndex();

  // - load messages

  sqlite3_stmt *pStmt = 0;
  const char selectSql[] = "SELECT id, data FROM messages";
  int result = sqlite3_prepare_v2( mIndexDb, selectSql, -1, &pStmt, 0 );
  bool ok = true;
  QList<sqlite3_int64> rowsToDelete; // rows with 0 serial number to delete later
  if( result != SQLITE_OK ) {
    ok = false;
    kWarning() << "sqlite3_prepare_v2() error: sql=" << selectSql << errorMessage( result, mIndexDb );
  }
  kDebug( Test1Area ) << fileName();
  while ( ok ) {
    result = sqlite3_step( pStmt );
    if ( result == SQLITE_DONE )
      break;
    ok = result == SQLITE_ROW;
    if ( !ok ) {
      kWarning() << "sqlite3_step() error " << errorMessage( result, mIndexDb );
      break;
    }
    const sqlite3_int64 dbId = sqlite3_column_int64( pStmt, 0 );
    const char* buffer = (const char*)sqlite3_column_blob( pStmt, 1 );
    int len = sqlite3_column_bytes( pStmt, 1 );
    char* copy = (char*)malloc(len);
    memcpy( copy, buffer, len );
    KMMsgInfo* mi = new KMMsgInfo(folder(), copy, len, dbId);

    if (mi->status().isDeleted()) {
      delete mi;  // skip messages that are marked as deleted
      setDirty( true );
      continue;
    }
    if ((mi->status().isNew()) || (mi->status().isUnread()) ||
        (folder() == kmkernel->outboxFolder()))
    {
      ++mUnreadMsgs;
      if (mUnreadMsgs == 0)
        ++mUnreadMsgs;
    }
    mMsgList.append(mi, false);
    kDebug( Test1Area ) << "getMsgSerNum:" << mi->getMsgSerNum();
    if ( mi->getMsgSerNum() == 0 ) {
      kDebug() << "mi->getMsgSerNum() == 0: let's rebuild the index";
      rowsToDelete.append( dbId );
      break;
    }
  } // while

  if ( pStmt ) {
    int prevResult = result;
    result = sqlite3_reset( pStmt );
    if ( result != SQLITE_OK ) {
      kWarning() << "sqlite3_reset() error " << errorMessage( result, mIndexDb );
    }
    result = sqlite3_finalize( pStmt );
    if ( result == SQLITE_OK && prevResult != SQLITE_OK )
      result = prevResult; // rollback if sqlite3_finalize() or prev. command failed
  }

  mTotalMsgs = mMsgList.count();
  if ( ok )
    ok = deleteIndexRows( rowsToDelete );

  return ok;
}

bool KMFolderIndex::deleteIndexRows( const QList<sqlite3_int64>& rowsToDelete )
{
  if ( rowsToDelete.isEmpty() )
    return true;

  if ( !executeQuery( mIndexDb, "BEGIN" ) )
    return false;
  // We're preparing both versions of statements because regardless of the mode
  // both could be used.
  const char sqlDelete[] = "DELETE FROM messages WHERE id=?";
  sqlite3_stmt *pDeleteStmt = 0;
  int result = sqlite3_prepare_v2( mIndexDb, sqlDelete, -1, &pDeleteStmt, 0 );
  if( result != SQLITE_OK ) {
    kWarning() << "sqlite3_prepare_v2() error: sql=" << sqlDelete << errorMessage( result, mIndexDb );
  }
  if( result == SQLITE_OK ) {
    result = sqlite3_prepare_v2( mIndexDb, sqlDelete, -1, &pDeleteStmt, 0 );
    if( result != SQLITE_OK ) {
      kWarning() << "sqlite3_prepare_v2() error: sql=" << sqlDelete << errorMessage( result, mIndexDb );
    }
  }
  if( result == SQLITE_OK ) {
    QList<sqlite3_int64>::ConstIterator constEnd( rowsToDelete.constEnd() );
    for ( QList<sqlite3_int64>::ConstIterator it( rowsToDelete.constBegin() ); it != constEnd; ++it ) {
      result = sqlite3_bind_int64( pDeleteStmt, 1, *it ); // bind existing id value
      if ( result != SQLITE_OK ) {
        kWarning() << "sqlite3_bind_int64() error " << errorMessage( result, mIndexDb );
        break;
      }
      result = sqlite3_step(pDeleteStmt);
      if ( result != SQLITE_DONE ) {
        kWarning() << "sqlite3_step() error " << errorMessage( result, mIndexDb );
        break;
      }
      result = sqlite3_reset(pDeleteStmt);
      if ( result != SQLITE_OK ) {
        kWarning() << "sqlite3_reset() error " << errorMessage( result, mIndexDb );
        break;
      }
    }
  }
  // free the resources
  if ( pDeleteStmt ) {
    int prevResult = result;
    result = sqlite3_finalize( pDeleteStmt );
    if( result == SQLITE_OK && prevResult != SQLITE_OK )
      result = prevResult; // rollback if sqlite3_finalize() or prev. command failed
  }

  // commit or rollback
  if ( result != SQLITE_OK ) {
    executeQuery( mIndexDb, "ROLLBACK" );
    return false;
  }
  if ( !executeQuery( mIndexDb, "COMMIT" ) )
    return false;
  return true;
}


int KMFolderIndex::count(bool cache) const
{
  int res = FolderStorage::count(cache);
  if (res == -1)
    res = mMsgList.count();
  return res;
}


bool KMFolderIndex::readIndexHeader(int *gv)
{
  int indexVersion;
//  assert(mIndexStream != 0);
//  mIndexSwapByteOrder = false;
//  mIndexSizeOfLong = sizeof(long);

  bool ok;
  QString str = querySingleString( mIndexDb, "PRAGMA user_version", 0, ok );
  ok = ok && !str.isEmpty();
  if ( ok )
    indexVersion = str.toInt(&ok);
  if ( !ok ) {
    qWarning() << "index file has invalid header '" << str << "'";
    return false;
  }

  if(gv)
      *gv = indexVersion;
  if (indexVersion < INDEX_VERSION) {
      kDebug(5006) <<"Index file" << indexLocation() <<" is out of date. Re-creating it.";
      createIndexFromContents();
      return false;
  } else if(indexVersion > INDEX_VERSION) {
      QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
      int r = KMessageBox::questionYesNo(0,
        i18n(
          "The mail index for '%1' is from an unknown version of KMail (%2).\n"
          "This index can be regenerated from your mail folder, but some "
          "information, including status flags, may be lost. Do you wish "
          "to downgrade your index file?" , objectName() , indexVersion),
          QString(), KGuiItem(i18n("Downgrade")), KGuiItem(i18n("Do Not Downgrade")) );
      QApplication::restoreOverrideCursor();
      if (r == KMessageBox::Yes)
        createIndexFromContents();
      return false;
  }
  // indexVersion == INDEX_VERSION

  // Header
/*  quint32 byteOrder = 0;
  quint32 sizeOfLong = sizeof(long); // default

  quint32 header_length = 0;
  KDE_fseek(mIndexStream, sizeof(char), SEEK_CUR );
  fread(&header_length, sizeof(header_length), 1, mIndexStream);
  if (header_length > 0xFFFF)
     header_length = kmail_swap_32(header_length);

  off_t endOfHeader = KDE_ftell(mIndexStream) + header_length;

  bool needs_update = true;
  // Process available header parts
  if (header_length >= sizeof(byteOrder))
  {
     fread(&byteOrder, sizeof(byteOrder), 1, mIndexStream);
     mIndexSwapByteOrder = (byteOrder == 0x78563412);
     header_length -= sizeof(byteOrder);

     if (header_length >= sizeof(sizeOfLong))
     {
        fread(&sizeOfLong, sizeof(sizeOfLong), 1, mIndexStream);
        if (mIndexSwapByteOrder)
           sizeOfLong = kmail_swap_32(sizeOfLong);
        mIndexSizeOfLong = sizeOfLong;
        header_length -= sizeof(sizeOfLong);
        needs_update = false;
     }
  }
  if (needs_update || mIndexSwapByteOrder || (mIndexSizeOfLong != sizeof(long)))
    setDirty( true );
  // Seek to end of header
  KDE_fseek(mIndexStream, endOfHeader, SEEK_SET );

  if (mIndexSwapByteOrder)
     kDebug(5006) <<"Index File has byte order swapped!";
  if (mIndexSizeOfLong != sizeof(long))
     kDebug(5006) <<"Index File sizeOfLong is" << mIndexSizeOfLong <<" while sizeof(long) is" << sizeof(long) <<" !";
*/
  return true;
}


/*
#ifdef HAVE_MMAP
bool KMFolderIndex::updateIndexStreamPtr(bool just_close)
#else
bool KMFolderIndex::updateIndexStreamPtr(bool)
#endif
{
      // We touch the folder, otherwise the index is regenerated, if KMail is
    // running, while the clock switches from daylight savings time to normal time
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(location()) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(location()) << ", 0) failed (location())";
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(indexLocation()) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(indexLocation()) << ", 0) failed (indexLocation())";
    if (0 != utime(QFile::encodeName( QDir::toNativeSeparators(KMMsgDict::getFolderIdsLocation( *this )) ), 0))
      kWarning() << "utime(" << QDir::toNativeSeparators(KMMsgDict::getFolderIdsLocation( *this )) << ", 0) failed (KMMsgDict::getFolderIdsLocation( *this ))";

    mIndexSwapByteOrder = false;
#ifdef HAVE_MMAP
    if(just_close) {
      bool munmapResult = true;
      if(mIndexStreamPtr)
        munmapResult = 0 == munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return munmapResult;
    }

    assert(mIndexStream);
    KDE_struct_stat stat_buf;
    if(KDE_fstat(fileno(mIndexStream), &stat_buf) == -1) {
      if(mIndexStreamPtr)
        munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength);
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return false;
    }
    if(mIndexStreamPtr)
      if(0 != munmap((char *)mIndexStreamPtr, mIndexStreamPtrLength))
        return false;
    mIndexStreamPtrLength = stat_buf.st_size;
    mIndexStreamPtr = (uchar *)mmap(0, mIndexStreamPtrLength, PROT_READ, MAP_SHARED,
    fileno(mIndexStream), 0);
    if(mIndexStreamPtr == MAP_FAILED) {
      mIndexStreamPtr = 0;
      mIndexStreamPtrLength = 0;
      return false;
    }
#endif
    return true;
}*/


KMFolderIndex::IndexStatus KMFolderIndex::indexStatus()
{
    QFileInfo contInfo(location());
    QFileInfo indInfo(indexLocation());

    if (!contInfo.exists()) return KMFolderIndex::IndexOk;
    if (!indInfo.exists()) return KMFolderIndex::IndexMissing;

    return ( contInfo.lastModified() > indInfo.lastModified() )
        ? KMFolderIndex::IndexTooOld
        : KMFolderIndex::IndexOk;
}

void KMFolderIndex::clearIndex(bool autoDelete, bool syncDict)
{
    mMsgList.clear(autoDelete, syncDict);
}


void KMFolderIndex::truncateIndex()
{
  writeIndex( true );
}

void KMFolderIndex::fillMessageDict()
{
  open( "fillDict" );
  for ( unsigned int idx = 0; idx < mMsgList.high(); idx++ ) {
    KMMsgBase* msgBase = mMsgList.at( idx );
    if ( msgBase ) {
      KMMsgDict::mutableInstance()->insert(0, msgBase, idx);
    }
  }
  close( "fillDict" );
}


KMMsgInfo* KMFolderIndex::setIndexEntry( int idx, KMMessage *msg )
{
  KMMsgInfo *msgInfo = new KMMsgInfo( folder() );
  *msgInfo = *msg;
  mMsgList.set( idx, msgInfo );
  return msgInfo;
}

bool KMFolderIndex::recreateIndex()
{
  QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
  KMessageBox::error(0,
       i18n("The mail index for '%1' is corrupted and will be regenerated now, "
            "but some information, including status flags, will be lost.", label()));
  QApplication::restoreOverrideCursor();
  if ( createIndexFromContents() != 0 )
    return false;
  return readIndex();
}

int KMFolderIndex::writeMessages( KMMsgBase* msg, bool flush )
{
  Q_UNUSED( flush );
  return writeMessages( msg, msg ? UpdateExistingMessages : OverwriteMessages );
}

int KMFolderIndex::writeMessages( KMMsgBase* msg, WriteMessagesMode mode )
{
  if ( !executeQuery( mIndexDb, "BEGIN" ) )
    return 1;
  // We're preparing both versions of statements because regardless of the mode
  // both could be used.
  const char sqlInsert[] = "INSERT INTO messages(data) VALUES(?)";
  const char sqlUpdate[] = "INSERT OR REPLACE INTO messages(id, data) VALUES (?, ?)";
  const uint high = mMsgList.high();

  sqlite3_stmt *pInsertStmt = 0, *pUpdateStmt = 0;
  int result = SQLITE_OK;
  if ( high > 0 ) {
    result = sqlite3_prepare_v2( mIndexDb, sqlInsert, -1, &pInsertStmt, 0 );
    if( result != SQLITE_OK )
      kWarning() << "sqlite3_prepare_v2() error: sql=" << sqlInsert << errorMessage( result, mIndexDb );
  }

  if ( mode == OverwriteMessages ) {
    if ( !executeQuery( mIndexDb, "DELETE FROM messages" ) ) {
      executeQuery( mIndexDb, "ROLLBACK" );
      return 1;
    }
  }
  else if ( mode == UpdateExistingMessages ) {
    if ( high > 0 ) {
      result = sqlite3_prepare_v2( mIndexDb, sqlUpdate, -1, &pUpdateStmt, 0 );
      if( result != SQLITE_OK )
        kWarning() << "sqlite3_prepare_v2() error: sql=" << sqlUpdate << errorMessage( result, mIndexDb );
    }
  }

  sqlite3_stmt *pStmt = pInsertStmt; // current statement to use
  for (unsigned int i=0; result == SQLITE_OK && i<high; i++) // when result != SQLITE_OK, this loop ends
  {
    KMMsgBase* msgBase = msg ? msg : mMsgList.at(i);
    if ( !msgBase )
      continue;
    if ( mode == UpdateExistingMessages && !msgBase->dirty() )
      continue;

    // pick proper statement
    bool updating = false;
    if ( mode == UpdateExistingMessages ) {
      if ( msgBase->dbId() != 0 )
        updating = true;
      pStmt = updating ? pUpdateStmt : pInsertStmt;
    }

    if ( updating ) { // also bind existing id value
      result = sqlite3_bind_int64(pStmt, 1, msgBase->dbId());
      if ( result != SQLITE_OK ) {
        kWarning() << "sqlite3_bind_int64() error " << errorMessage( result, mIndexDb );
        break;
      }
    }

    const int dataColumnNumber = updating ? 2 : 1;
    int len;
    const uchar *buffer = msgBase->asIndexString(len);
    result = sqlite3_bind_blob(pStmt, dataColumnNumber, buffer, len, SQLITE_STATIC);
    if ( result != SQLITE_OK ) {
      kWarning() << "sqlite3_bind_blob() error " << errorMessage( result, mIndexDb );
      break;
    }

    result = sqlite3_step(pStmt);
    if ( result != SQLITE_DONE ) {
      kWarning() << "sqlite3_step() error " << errorMessage( result, mIndexDb );
      break;
    }

    if ( !updating ) { // save pkey value of the just inserted record
      const sqlite3_int64 newId = sqlite3_last_insert_rowid( mIndexDb );
      if ( newId == 0 ) {
        result = SQLITE_NOTFOUND;
        kWarning() << "sqlite3_last_insert_rowid() returned 0: error " << errorMessage( result, mIndexDb );
        break;
      }
      msgBase->setDbId( newId );
    }

    result = sqlite3_reset(pStmt);
    if ( result != SQLITE_OK ) {
      kWarning() << "sqlite3_reset() error " << errorMessage( result, mIndexDb );
      break;
    }
    if ( msg )
      break; // only one
  } // for

  // free the resources
  if ( pInsertStmt ) {
    int prevResult = result;
    result = sqlite3_finalize( pInsertStmt );
    if( result == SQLITE_OK && prevResult != SQLITE_OK )
      result = prevResult; // rollback if sqlite3_finalize() or prev. command failed
  }
  if ( pUpdateStmt ) {
    int prevResult = result;
    result = sqlite3_finalize( pUpdateStmt );
    if( result == SQLITE_OK && prevResult != SQLITE_OK )
      result = prevResult; // rollback if sqlite3_finalize() or prev. command failed
  }

  // commit or rollback
  if ( result != SQLITE_OK ) {
    executeQuery( mIndexDb, "ROLLBACK" );
    return 1;
  }
  if ( !executeQuery( mIndexDb, "COMMIT" ) )
    return 1;
  return 0;
}

void KMFolderIndex::msgStatusChanged( const MessageStatus& oldStatus,
                                      const MessageStatus& newStatus,
                                      int idx )
{
  mDirty = true;
  FolderStorage::msgStatusChanged(oldStatus, newStatus, idx);
}

KMMsgBase * KMFolderIndex::takeIndexEntry(int idx)
{
  KMMsgBase* msg = mMsgList.take( idx );
  if ( msg->dbId() > 0 ) {
    QList<sqlite3_int64> l;
    l << msg->dbId();
    deleteIndexRows( l );
  }
  return msg;
}
