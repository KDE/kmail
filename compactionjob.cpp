/**
 * Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "compactionjob.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include <kdebug.h>
#include <kde_file.h>
#include <klocale.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

using namespace KMail;

// Look at this number of messages in each slotDoWork call
#define COMPACTIONJOB_NRMESSAGES 100
// And wait this number of milliseconds before calling it again
#define COMPACTIONJOB_TIMERINTERVAL 100

MboxCompactionJob::MboxCompactionJob( const Akonadi::Collection& folder, bool immediate )
 : ScheduledJob( folder, immediate ), mTimer( this ), mTmpFile( 0 ),
   mCurrentIndex( 0 ), mFolderOpen( false ), mSilent( false )
{
}

MboxCompactionJob::~MboxCompactionJob()
{
}

void MboxCompactionJob::kill()
{
#if 0
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mSrcFolder && mSrcFolder->storage() ) {
    mSrcFolder->storage()->close( "mboxcompact" );
  }

  if ( mTmpFile ) {
    fclose( mTmpFile );
  }
  mTmpFile = 0;
  if ( !mTempName.isEmpty() ) {
    QFile::remove( mTempName );
  }
  FolderJob::kill();
#endif
}

QString MboxCompactionJob::realLocation() const
{
#if 0
  QString location = mSrcFolder->location();
  QFileInfo inf( location );
  if (inf.isSymLink()) {
    KUrl u; u.setPath( location );
    // follow (and resolve) symlinks so that the final KDE_rename() always works
    // KUrl gives us support for absolute and relative links transparently.
    return KUrl( u, inf.readLink() ).toLocalFile();
  }
  return location;
#endif
  return "";
}

int MboxCompactionJob::executeNow( bool silent )
{
#if 0 //TODO port to akonadi
  mSilent = silent;
  FolderStorage *storage = mSrcFolder->storage();
  KMFolderMbox *mbox = static_cast<KMFolderMbox *>( storage );
  if ( !storage->compactable() ) {
    kDebug() << storage->location() << "compaction skipped.";
    if ( !mSilent ) {
      QString str = i18n( "For safety reasons, compaction has been disabled for %1", mbox->label() );
      BroadcastStatus::instance()->setStatusMsg( str );
    }
    return 0;
  }
  kDebug() << "Compacting" << mSrcFolder->idString();

  if ( KMFolderIndex::IndexOk != mbox->indexStatus() ) {
    kDebug() << "Critical error:" << storage->location()
                 << "has been modified by an external application while KMail was running.";
    //      exit(1); backed out due to broken nfs
  }

  const QFileInfo pathInfo( realLocation() );
  // Use /dir/.mailboxname.compacted so that it's hidden, and doesn't show up after restarting kmail
  // (e.g. due to an unfortunate crash while compaction is happening)
  mTempName = pathInfo.path() + "/." + pathInfo.fileName() + ".compacted";

  mode_t old_umask = umask( 077 );
  mTmpFile = KDE_fopen( QFile::encodeName( mTempName ), "w" );
  umask( old_umask );
  if (!mTmpFile) {
    kWarning() <<"Couldn't start compacting" << mSrcFolder->label()
                   << ":" << strerror( errno )
                   << "while creating" << mTempName;
    return errno;
  }
  mOpeningFolder = true; // Ignore open-notifications while opening the folder
  storage->open( "mboxcompact" );
  mOpeningFolder = false;
  mFolderOpen = true;
  mOffset = 0;
  mCurrentIndex = 0;

  kDebug() << "MboxCompactionJob: starting to compact folder"
               << mSrcFolder->location() << "into" << mTempName;
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  if ( !mImmediate ) {
    mTimer.start( COMPACTIONJOB_TIMERINTERVAL );
  }
  slotDoWork();
  return mErrorCode;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
	return 0;
#endif
}

void MboxCompactionJob::slotDoWork()
{
#if 0 //TODO port to akonadi
  // No need to worry about mSrcFolder==0 here. The FolderStorage deletes the jobs on destruction.
  KMFolderMbox *mbox = static_cast<KMFolderMbox *>( mSrcFolder->storage() );
  bool bDone = false;
  int nbMessages = mImmediate ? -1 /*all*/ : COMPACTIONJOB_NRMESSAGES;
  int rc = mbox->compact( mCurrentIndex, nbMessages,
                          mTmpFile, mOffset /*in-out*/, bDone /*out*/ );
  if ( !mImmediate )
    mCurrentIndex += COMPACTIONJOB_NRMESSAGES;
  if ( rc || bDone ) // error, or finished
    done( rc );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void MboxCompactionJob::done( int rc )
{
#if 0 //TODO port to akonadi
  mTimer.stop();
  mCancellable = false;
  KMFolderMbox *mbox = static_cast<KMFolderMbox *>( mSrcFolder->storage() );
  if ( !rc ) {
    rc = fflush( mTmpFile );
  }
  if ( !rc ) {
    rc = fsync( fileno( mTmpFile ) );
  }
  rc |= fclose( mTmpFile );
  QString str;
  if ( !rc ) {
    bool autoCreate = mbox->autoCreateIndex();
    QString box( realLocation() );
    rc = KDE_rename( QFile::encodeName( mTempName ), QFile::encodeName( box ) );
    if ( rc != 0 )
      return;
    mbox->writeIndex();
    mbox->writeConfig();
    mbox->setAutoCreateIndex( false );
    mbox->close( "mboxcompact", true );
    mbox->setAutoCreateIndex( autoCreate );
    mbox->setNeedsCompacting( false );            // We are clean now
    str = i18n( "Folder \"%1\" successfully compacted", mSrcFolder->label() );
    kDebug() << str;
  } else {
    mbox->close( "mboxcompact" );
    str = i18n( "Error occurred while compacting \"%1\". Compaction aborted.", mSrcFolder->label() );
    kDebug() << "Error occurred while compacting" << mbox->location();
    kDebug() << "Compaction aborted.";
    QFile::remove( mTempName );
  }
  mErrorCode = rc;

  if ( !mSilent )
    BroadcastStatus::instance()->setStatusMsg( str );

  mFolderOpen = false;
  deleteLater(); // later, because of the "return mErrorCode"
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

////

MaildirCompactionJob::MaildirCompactionJob( const Akonadi::Collection& folder, bool immediate )
 : ScheduledJob( folder, immediate ), mTimer( this ),
   mCurrentIndex( 0 ), mFolderOpen( false ), mSilent( false )
{
}

MaildirCompactionJob::~MaildirCompactionJob()
{
}

void MaildirCompactionJob::kill()
{
#if 0
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mSrcFolder && mSrcFolder->storage() ) {
    mSrcFolder->storage()->close( "maildircompact" );
  }

  FolderJob::kill();
#endif
}

int MaildirCompactionJob::executeNow( bool silent )
{
#if 0 //TODO port to akonadi
  mSilent = silent;
  KMFolderMaildir *storage =
    static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  kDebug() << "Compacting" << mSrcFolder->idString();

  mOpeningFolder = true; // Ignore open-notifications while opening the folder
  storage->open( "maildircompact" );
  mOpeningFolder = false;
  mFolderOpen = true;
  QString subdirNew( storage->location() + "/new/" );
  QDir d( subdirNew );
  mEntryList = d.entryList();
  mCurrentIndex = 0;

  kDebug() << "MaildirCompactionJob: starting to compact in folder"
               << mSrcFolder->location();
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  if ( !mImmediate ) {
    mTimer.start( COMPACTIONJOB_TIMERINTERVAL );
  }
  slotDoWork();
  return mErrorCode;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
	return 0;
#endif
}

void MaildirCompactionJob::slotDoWork()
{
#if 0 //TODO port to akonadi
  // No need to worry about mSrcFolder==0 here. The FolderStorage deletes the jobs on destruction.
  KMFolderMaildir *storage =
    static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  bool bDone = false;
  int nbMessages = mImmediate ? -1 /*all*/ : COMPACTIONJOB_NRMESSAGES;
  int rc = storage->compact( mCurrentIndex, nbMessages, mEntryList, bDone /*out*/ );
  if ( !mImmediate )
    mCurrentIndex += COMPACTIONJOB_NRMESSAGES;
  if ( rc || bDone ) // error, or finished
    done( rc );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void MaildirCompactionJob::done( int rc )
{
#if 0 //TODO port to akonadi
  KMFolderMaildir *storage =
    static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  mTimer.stop();
  mCancellable = false;
  QString str;
  if ( !rc ) {
    str = i18n( "Folder \"%1\" successfully compacted", mSrcFolder->label() );
  } else {
    str = i18n( "Error occurred while compacting \"%1\". Compaction aborted.", mSrcFolder->label() );
  }
  mErrorCode = rc;
  storage->setNeedsCompacting( false );
  storage->close( "maildircompact" );
  if ( storage->isOpened() ) {
    storage->updateIndex();
  }
  if ( !mSilent ) {
    BroadcastStatus::instance()->setStatusMsg( str );
  }

  mFolderOpen = false;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  deleteLater(); // later, because of the "return mErrorCode"
}

ScheduledJob *ScheduledCompactionTask::run()
{
#if 0
  if ( !folder() || !folder()->needsCompacting() )
    return 0;
  switch( folder()->storage()->folderType() ) {
  case KMFolderTypeMbox:
    return new MboxCompactionJob( folder(), isImmediate() );
  case KMFolderTypeCachedImap:
  case KMFolderTypeMaildir:
    return new MaildirCompactionJob( folder(), isImmediate() );
  default: // imap, search, unknown...
    return 0;
  }
#endif
return 0;
}

#include "compactionjob.moc"
