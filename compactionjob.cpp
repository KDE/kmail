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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "kmfolder.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermbox.h"
#include "kmfoldermaildir.h"

#include <kdebug.h>
#include <klocale.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

#include <sys/types.h>
#include <sys/stat.h>

using namespace KMail;

// Look at this number of messages in each slotDoWork call
#define COMPACTIONJOB_NRMESSAGES 100
// And wait this number of milliseconds before calling it again
#define COMPACTIONJOB_TIMERINTERVAL 100

MboxCompactionJob::MboxCompactionJob( KMFolder* folder, bool immediate )
 : FolderJob( 0, tOther, folder ), mTimer( this ), mTmpFile( 0 ),
   mCurrentIndex( 0 ), mImmediate( immediate ), mFolderOpen( false )
{
  mCancellable = true;
  mSrcFolder = folder;
}

MboxCompactionJob::~MboxCompactionJob()
{
}

void MboxCompactionJob::kill()
{
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mSrcFolder && mSrcFolder->storage() )
    mSrcFolder->storage()->close();

  if ( mTmpFile )
    fclose( mTmpFile );
  mTmpFile = 0;
  if ( !mTempName.isEmpty() )
    QFile::remove( mTempName );
  FolderJob::kill();
}

int MboxCompactionJob::executeNow()
{
  FolderStorage* storage = mSrcFolder->storage();
  KMFolderMbox* mbox = static_cast<KMFolderMbox *>( storage );
  if (!storage->compactable()) {
    kdDebug(5006) << storage->location() << " compaction skipped." << endl;
    return 0;
  }
  kdDebug(5006) << "Compacting " << mSrcFolder->idString() << endl;

  if (KMFolderIndex::IndexOk != mbox->indexStatus()) {
      kdDebug(5006) << "Critical error: " << storage->location() <<
          " has been modified by an external application while KMail was running." << endl;
      //      exit(1); backed out due to broken nfs
  }

  mTempName = mSrcFolder->path() + "/." + mSrcFolder->name() + ".compacted";
  mode_t old_umask = umask(077);
  mTmpFile = fopen(QFile::encodeName(mTempName), "w");
  umask(old_umask);
  if (!mTmpFile) {
    kdWarning(5006) << "Couldn't start compacting " << mSrcFolder->label() << " : " << strerror( errno ) << endl;
    return errno;
  }
  storage->open();
  mFolderOpen = true;
  mOffset = 0;
  mCurrentIndex = 0;

  kdDebug(5006) << "MboxCompactionJob: starting to compact in folder " << mSrcFolder->location() << endl;
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  if ( !mImmediate )
    mTimer.start( COMPACTIONJOB_TIMERINTERVAL );
  slotDoWork();
  return mErrorCode;
}

void MboxCompactionJob::slotDoWork()
{
  // No need to worry about mSrcFolder==0 here. The FolderStorage deletes the jobs on destruction.
  KMFolderMbox* mbox = static_cast<KMFolderMbox *>( mSrcFolder->storage() );
  bool bDone = false;
  int nbMessages = mImmediate ? -1 /*all*/ : COMPACTIONJOB_NRMESSAGES;
  int rc = mbox->compact( mCurrentIndex, nbMessages,
                          mTmpFile, mOffset /*in-out*/, bDone /*out*/ );
  if ( !mImmediate )
    mCurrentIndex += COMPACTIONJOB_NRMESSAGES;
  if ( rc || bDone ) // error, or finished
    done( rc );
}

void MboxCompactionJob::done( int rc )
{
  mTimer.stop();
  mCancellable = false;
  KMFolderMbox* mbox = static_cast<KMFolderMbox *>( mSrcFolder->storage() );
  if (!rc)
      rc = fflush(mTmpFile);
  if (!rc)
      rc = fsync(fileno(mTmpFile));
  rc |= fclose(mTmpFile);
  QString str;
  if (!rc) {
    bool autoCreate = mbox->autoCreateIndex();
    QFileInfo inf(mbox->location());
    QString box;
    if (inf.isSymLink())
      box = inf.readLink();
    if (!box)
      box = mbox->location();
    ::rename(QFile::encodeName(mTempName), QFile::encodeName(box));
    mbox->writeIndex();
    mbox->writeConfig();
    mbox->setAutoCreateIndex( false );
    mbox->close(true);
    mbox->setAutoCreateIndex( autoCreate );
    mbox->setNeedsCompacting( false );            // We are clean now
    str = i18n( "Mailbox %1 successfully compacted" ).arg( mSrcFolder->label() );
  } else {
    mbox->close();
    str = i18n( "Error occurred while compacting %1. Compaction aborted." ).arg( mSrcFolder->label() );
    kdDebug(5006) << "Error occurred while compacting " << mbox->location() << endl;
    kdDebug(5006) << "Compaction aborted." << endl;
  }
  mErrorCode = rc;

  KMBroadcastStatus::instance()->setStatusMsg( str );

  mFolderOpen = false;
  deleteLater(); // later, because of the "return mErrorCode"
}

////

MaildirCompactionJob::MaildirCompactionJob( KMFolder* folder, bool immediate )
 : FolderJob( 0, tOther, folder ), mTimer( this ),
   mCurrentIndex( 0 ), mImmediate( immediate ), mFolderOpen( false )
{
  mCancellable = true;
  mSrcFolder = folder;
}

MaildirCompactionJob::~MaildirCompactionJob()
{
}

void MaildirCompactionJob::kill()
{
  Q_ASSERT( mCancellable );
  // We must close the folder if we opened it and got interrupted
  if ( mFolderOpen && mSrcFolder && mSrcFolder->storage() )
    mSrcFolder->storage()->close();

  FolderJob::kill();
}

int MaildirCompactionJob::executeNow()
{
  KMFolderMaildir* storage = static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  kdDebug(5006) << "Compacting " << mSrcFolder->idString() << endl;

  storage->open();
  mFolderOpen = true;
  QString subdirNew(storage->location() + "/new/");
  QDir d(subdirNew);
  mEntryList = d.entryList();
  mCurrentIndex = 0;

  kdDebug(5006) << "MaildirCompactionJob: starting to compact in folder " << mSrcFolder->location() << endl;
  connect( &mTimer, SIGNAL( timeout() ), SLOT( slotDoWork() ) );
  if ( !mImmediate )
    mTimer.start( COMPACTIONJOB_TIMERINTERVAL );
  slotDoWork();
  return mErrorCode;
}

void MaildirCompactionJob::slotDoWork()
{
  // No need to worry about mSrcFolder==0 here. The FolderStorage deletes the jobs on destruction.
  KMFolderMaildir* storage = static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  bool bDone = false;
  int nbMessages = mImmediate ? -1 /*all*/ : COMPACTIONJOB_NRMESSAGES;
  int rc = storage->compact( mCurrentIndex, nbMessages, mEntryList, bDone /*out*/ );
  if ( !mImmediate )
    mCurrentIndex += COMPACTIONJOB_NRMESSAGES;
  if ( rc || bDone ) // error, or finished
    done( rc );
}

void MaildirCompactionJob::done( int rc )
{
  KMFolderMaildir* storage = static_cast<KMFolderMaildir *>( mSrcFolder->storage() );
  mTimer.stop();
  mCancellable = false;
  QString str;
  if ( !rc ) {
    str = i18n( "Mailbox %1 successfully compacted" ).arg( mSrcFolder->label() );
  } else {
    str = i18n( "Error occurred while compacting %1. Compaction aborted." ).arg( mSrcFolder->label() );
  }
  mErrorCode = rc;
  storage->setNeedsCompacting( false );
  storage->close();
  KMBroadcastStatus::instance()->setStatusMsg( str );

  mFolderOpen = false;
  deleteLater(); // later, because of the "return mErrorCode"
}

////

FolderJob* ScheduledCompactionTask::run()
{
  if ( !folder() || !folder()->needsCompacting() )
    return 0;
  switch( folder()->storage()->folderType() ) {
  case KMFolderTypeMbox:
    return new MboxCompactionJob( folder(), mImmediate );
  case KMFolderTypeCachedImap:
  case KMFolderTypeMaildir:
    return new MaildirCompactionJob( folder(), mImmediate );
  default: // imap, search, unknown...
    return 0;
  }
}

#include "compactionjob.moc"
