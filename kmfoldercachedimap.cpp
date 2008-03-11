/**
 *  kmfoldercachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@sonofthor.dk>
 *  Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>

#include <qvaluevector.h>

#include "kmkernel.h"
#include "kmfoldercachedimap.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmacctcachedimap.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmailicalifaceimpl.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "acljobs.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"

using KMail::CachedImapJob;
#include "imapaccountbase.h"
using KMail::ImapAccountBase;
#include "listjob.h"
using KMail::ListJob;

#include "kmfolderseldlg.h"
#include "kmcommands.h"
#include "kmmainwidget.h"

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kio/scheduler.h>
#include <qbuffer.h>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qvaluelist.h>
#include "annotationjobs.h"
#include "quotajobs.h"
using namespace KMail;
#include <globalsettings.h>

#define UIDCACHE_VERSION 1
#define MAIL_LOSS_DEBUGGING 0

static QString incidencesForToString( KMFolderCachedImap::IncidencesFor r ) {
  switch (r) {
  case KMFolderCachedImap::IncForNobody: return "nobody";
  case KMFolderCachedImap::IncForAdmins: return "admins";
  case KMFolderCachedImap::IncForReaders: return "readers";
  }
  return QString::null; // can't happen
}

static KMFolderCachedImap::IncidencesFor incidencesForFromString( const QString& str ) {
  if ( str == "nobody" ) return KMFolderCachedImap::IncForNobody;
  if ( str == "admins" ) return KMFolderCachedImap::IncForAdmins;
  if ( str == "readers" ) return KMFolderCachedImap::IncForReaders;
  return KMFolderCachedImap::IncForAdmins; // by default
}

DImapTroubleShootDialog::DImapTroubleShootDialog( QWidget* parent,
                                                  const char* name )
  : KDialogBase( Plain, i18n( "Troubleshooting IMAP Cache" ),
                 Ok | Cancel, Cancel, parent, name, true ),
    rc( None )
{
  QFrame* page = plainPage();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0 );
  // spell "lose" correctly. but don't cause a fuzzy.
  QString txt = i18n( "<p><b>Troubleshooting the IMAP cache.</b></p>"
                      "<p>If you have problems with synchronizing an IMAP "
                      "folder, you should first try rebuilding the index "
                      "file. This will take some time to rebuild, but will "
                      "not cause any problems.</p><p>If that is not enough, "
                      "you can try refreshing the IMAP cache. If you do this, "
                      "you will loose all your local changes for this folder "
                      "and all its subfolders.</p>",
                      "<p><b>Troubleshooting the IMAP cache.</b></p>"
                      "<p>If you have problems with synchronizing an IMAP "
                      "folder, you should first try rebuilding the index "
                      "file. This will take some time to rebuild, but will "
                      "not cause any problems.</p><p>If that is not enough, "
                      "you can try refreshing the IMAP cache. If you do this, "
                      "you will lose all your local changes for this folder "
                      "and all its subfolders.</p>" );
  topLayout->addWidget( new QLabel( txt, page ) );

  QButtonGroup *group = new QButtonGroup( 0 );

  mIndexButton = new QRadioButton( page );
  mIndexButton->setText( i18n( "Rebuild &Index" ) );
  group->insert( mIndexButton );
  topLayout->addWidget( mIndexButton );

  QHBox *hbox = new QHBox( page );
  QLabel *scopeLabel = new QLabel( i18n( "Scope:" ), hbox );
  scopeLabel->setEnabled( false );
  mIndexScope = new QComboBox( hbox );
  mIndexScope->insertItem( i18n( "Only current folder" ) );
  mIndexScope->insertItem( i18n( "Current folder and all subfolders" ) );
  mIndexScope->insertItem( i18n( "All folders of this account" ) );
  mIndexScope->setEnabled( false );
  topLayout->addWidget( hbox );

  mCacheButton = new QRadioButton( page );
  mCacheButton->setText( i18n( "Refresh &Cache" ) );
  group->insert( mCacheButton );
  topLayout->addWidget( mCacheButton );

  enableButtonSeparator( true );

  connect ( mIndexButton, SIGNAL(toggled(bool)), mIndexScope, SLOT(setEnabled(bool)) );
  connect ( mIndexButton, SIGNAL(toggled(bool)), scopeLabel, SLOT(setEnabled(bool)) );

  connect( this, SIGNAL( okClicked () ), this, SLOT( slotDone() ) );
}

int DImapTroubleShootDialog::run()
{
  DImapTroubleShootDialog d;
  d.exec();
  return d.rc;
}

void DImapTroubleShootDialog::slotDone()
{
  rc = None;
  if ( mIndexButton->isOn() )
    rc = mIndexScope->currentItem();
  else if ( mCacheButton->isOn() )
    rc = RefreshCache;
  done( Ok );
}

KMFolderCachedImap::KMFolderCachedImap( KMFolder* folder, const char* aName )
  : KMFolderMaildir( folder, aName ),
    mSyncState( SYNC_STATE_INITIAL ), mContentState( imapNoInformation ),
    mSubfolderState( imapNoInformation ),
    mIncidencesFor( IncForAdmins ),
    mIsSelected( false ),
    mCheckFlags( true ), mReadOnly( false ), mAccount( NULL ), uidMapDirty( true ),
    uidWriteTimer( -1 ), mLastUid( 0 ), mTentativeHighestUid( 0 ),
    mFoundAnIMAPDigest( false ),
    mUserRights( 0 ), mOldUserRights( 0 ), mSilentUpload( false ),
    /*mHoldSyncs( false ),*/
    mFolderRemoved( false ),
    mRecurse( true ),
    mStatusChangedLocally( false ), mAnnotationFolderTypeChanged( false ),
    mIncidencesForChanged( false ), mPersonalNamespacesCheckDone( true ),
    mQuotaInfo(), mAlarmsBlocked( false ),
    mRescueCommandCount( 0 ),
    mPermanentFlags( 31 ) // assume standard flags by default (see imap4/imapinfo.h for bit fields values)
{
  setUidValidity("");
  // if we fail to read a uid file but there is one, nuke it
  if ( readUidCache() == -1 ) {
    if ( QFile::exists( uidCacheLocation() ) ) {
        KMessageBox::error( 0,
        i18n( "The UID cache file for folder %1 could not be read. There "
              "could be a problem with file system permission, or it is corrupted."
              ).arg( folder->prettyURL() ) );
        // try to unlink it, in case it was corruped. If it couldn't be read
        // because of permissions, this will fail, which is fine
        unlink( QFile::encodeName( uidCacheLocation() ) );
    }
  }

  mProgress = 0;
}

KMFolderCachedImap::~KMFolderCachedImap()
{
  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed( folder() );
}

void KMFolderCachedImap::reallyDoClose( const char* owner )
{
  if( !mFolderRemoved ) {
    writeUidCache();
  }
  KMFolderMaildir::reallyDoClose( owner );
}

void KMFolderCachedImap::initializeFrom( KMFolderCachedImap* parent )
{
  setAccount( parent->account() );
  // Now that we have an account, tell it that this folder was created:
  // if this folder was just removed, then we don't really want to remove it from the server.
  mAccount->removeDeletedFolder( imapPath() );
  setUserRights( parent->userRights() );
}

void KMFolderCachedImap::readConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver( config, "Folder-" + folder()->idString() );
  if( mImapPath.isEmpty() ) mImapPath = config->readEntry( "ImapPath" );
  if( QString( name() ).upper() == "INBOX" && mImapPath == "/INBOX/" )
  {
    folder()->setLabel( i18n( "inbox" ) );
    // for the icon
    folder()->setSystemFolder( true );
  }
  mNoContent = config->readBoolEntry( "NoContent", false );
  mReadOnly = config->readBoolEntry( "ReadOnly", false );
  if ( !config->readEntry( "FolderAttributes" ).isEmpty() )
    mFolderAttributes = config->readEntry( "FolderAttributes" );

  if ( mAnnotationFolderType != "FROMSERVER" ) {
    mAnnotationFolderType = config->readEntry( "Annotation-FolderType" );
    // if there is an annotation, it has to be XML
    if ( !mAnnotationFolderType.isEmpty() && !mAnnotationFolderType.startsWith( "mail" ) )
      kmkernel->iCalIface().setStorageFormat( folder(), KMailICalIfaceImpl::StorageXML );
//    kdDebug(5006) << ( mImapPath.isEmpty() ? label() : mImapPath )
//                  << " readConfig: mAnnotationFolderType=" << mAnnotationFolderType << endl;
  }
  mIncidencesFor = incidencesForFromString( config->readEntry( "IncidencesFor" ) );
  mAlarmsBlocked = config->readBoolEntry( "AlarmsBlocked", false );
//  kdDebug(5006) << ( mImapPath.isEmpty() ? label() : mImapPath )
//                << " readConfig: mIncidencesFor=" << mIncidencesFor << endl;

  mUserRights = config->readNumEntry( "UserRights", 0 ); // default is we don't know
  mOldUserRights = mUserRights;

  int storageQuotaUsage = config->readNumEntry( "StorageQuotaUsage", -1 );
  int storageQuotaLimit = config->readNumEntry( "StorageQuotaLimit", -1 );
  QString storageQuotaRoot = config->readEntry( "StorageQuotaRoot", QString::null );
  if ( !storageQuotaRoot.isNull() ) { // isEmpty() means we know there is no quota set
      mQuotaInfo.setName( "STORAGE" );
      mQuotaInfo.setRoot( storageQuotaRoot );

      if ( storageQuotaUsage > -1 )
        mQuotaInfo.setCurrent( storageQuotaUsage );
      if ( storageQuotaLimit > -1 )
        mQuotaInfo.setMax( storageQuotaLimit );
  }

  KMFolderMaildir::readConfig();

  mStatusChangedLocally =
    config->readBoolEntry( "StatusChangedLocally", false );

  mAnnotationFolderTypeChanged = config->readBoolEntry( "AnnotationFolderTypeChanged", false );
  mIncidencesForChanged = config->readBoolEntry( "IncidencesForChanged", false );
  if ( mImapPath.isEmpty() ) {
    mImapPathCreation = config->readEntry("ImapPathCreation");
  }

  QStringList uids = config->readListEntry( "UIDSDeletedSinceLastSync" );
#if MAIL_LOSS_DEBUGGING
  kdDebug( 5006 ) << "READING IN UIDSDeletedSinceLastSync: " << folder()->prettyURL() << endl << uids << endl;
#endif
  for ( QStringList::iterator it = uids.begin(); it != uids.end(); it++ ) {
      mDeletedUIDsSinceLastSync.insert( (*it).toULong(), 0);
  }
}

void KMFolderCachedImap::writeConfig()
{
  KConfigGroup configGroup( KMKernel::config(), "Folder-" + folder()->idString() );
  configGroup.writeEntry( "ImapPath", mImapPath );
  configGroup.writeEntry( "NoContent", mNoContent );
  configGroup.writeEntry( "ReadOnly", mReadOnly );
  configGroup.writeEntry( "FolderAttributes", mFolderAttributes );
  configGroup.writeEntry( "StatusChangedLocally", mStatusChangedLocally );
  if ( !mImapPathCreation.isEmpty() ) {
    if ( mImapPath.isEmpty() ) {
      configGroup.writeEntry( "ImapPathCreation", mImapPathCreation );
    } else {
      configGroup.deleteEntry( "ImapPathCreation" );
    }
  }
  if ( !mDeletedUIDsSinceLastSync.isEmpty() ) {
      QValueList<ulong> uids = mDeletedUIDsSinceLastSync.keys();
      QStringList uidstrings;
      for( QValueList<ulong>::iterator it = uids.begin(); it != uids.end(); it++ ) {
          uidstrings.append(  QString::number( (*it) ) );
      }
      configGroup.writeEntry( "UIDSDeletedSinceLastSync", uidstrings );
#if MAIL_LOSS_DEBUGGING
      kdDebug( 5006 ) << "WRITING OUT UIDSDeletedSinceLastSync in: " << folder( )->prettyURL( ) << endl << uidstrings << endl;
#endif
  } else {
    configGroup.deleteEntry( "UIDSDeletedSinceLastSync" );
  }
  writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();
  KMFolderMaildir::writeConfig();
}

void KMFolderCachedImap::writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig()
{
  KConfigGroup configGroup( KMKernel::config(), "Folder-" + folder()->idString() );
  if ( !folder()->noContent() )
  {
    configGroup.writeEntry( "AnnotationFolderTypeChanged", mAnnotationFolderTypeChanged );
    configGroup.writeEntry( "Annotation-FolderType", mAnnotationFolderType );
    configGroup.writeEntry( "IncidencesForChanged", mIncidencesForChanged );
    configGroup.writeEntry( "IncidencesFor", incidencesForToString( mIncidencesFor ) );
    configGroup.writeEntry( "AlarmsBlocked", mAlarmsBlocked );
    configGroup.writeEntry( "UserRights", mUserRights );

    configGroup.deleteEntry( "StorageQuotaUsage");
    configGroup.deleteEntry( "StorageQuotaRoot");
    configGroup.deleteEntry( "StorageQuotaLimit");

    if ( mQuotaInfo.isValid() ) {
      if ( mQuotaInfo.current().isValid() ) {
        configGroup.writeEntry( "StorageQuotaUsage", mQuotaInfo.current().toInt() );
      }
      if ( mQuotaInfo.max().isValid() ) {
        configGroup.writeEntry( "StorageQuotaLimit", mQuotaInfo.max().toInt() );
      }
      configGroup.writeEntry( "StorageQuotaRoot", mQuotaInfo.root() );
    }
  }
}

int KMFolderCachedImap::create()
{
  int rc = KMFolderMaildir::create();
  // FIXME why the below? - till
  readConfig();
  mUnreadMsgs = -1;
  return rc;
}

void KMFolderCachedImap::remove()
{
  mFolderRemoved = true;

  QString part1 = folder()->path() + "/." + dotEscape(name());
  QString uidCacheFile = part1 + ".uidcache";
  // This is the account folder of an account that was just removed
  // When this happens, be sure to delete all traces of the cache
  if( QFile::exists(uidCacheFile) )
    unlink( QFile::encodeName( uidCacheFile ) );

  FolderStorage::remove();
}

QString KMFolderCachedImap::uidCacheLocation() const
{
  QString sLocation(folder()->path());
  if (!sLocation.isEmpty()) sLocation += '/';
  return sLocation + '.' + dotEscape(fileName()) + ".uidcache";
}

int KMFolderCachedImap::readUidCache()
{
  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_ReadOnly ) ) {
    char buf[1024];
    int len = uidcache.readLine( buf, sizeof(buf) );
    if( len > 0 ) {
      int cacheVersion;
      sscanf( buf, "# KMail-UidCache V%d\n",  &cacheVersion );
      if( cacheVersion == UIDCACHE_VERSION ) {
        len = uidcache.readLine( buf, sizeof(buf) );
        if( len > 0 ) {
          setUidValidity( QString::fromLocal8Bit(buf).stripWhiteSpace() );
          len = uidcache.readLine( buf, sizeof(buf) );
          if( len > 0 ) {
#if MAIL_LOSS_DEBUGGING
            kdDebug(5006) << "Reading in last uid from cache: " << QString::fromLocal8Bit(buf).stripWhiteSpace() << " in " << folder()->prettyURL() << endl;
#endif
            // load the last known highest uid from the on disk cache
            setLastUid( QString::fromLocal8Bit(buf).stripWhiteSpace().toULong() );
            return 0;
          }
        }
      }
    }
  }
  return -1;
}

int KMFolderCachedImap::writeUidCache()
{
  if( uidValidity().isEmpty() || uidValidity() == "INVALID" ) {
    // No info from the server yet, remove the file.
    if( QFile::exists( uidCacheLocation() ) )
      return unlink( QFile::encodeName( uidCacheLocation() ) );
    return 0;
  }
#if MAIL_LOSS_DEBUGGING
  kdDebug(5006) << "Writing out UID cache lastuid: " << lastUid()  << " in: " << folder()->prettyURL() << endl;
#endif
  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_WriteOnly ) ) {
    QTextStream str( &uidcache );
    str << "# KMail-UidCache V" << UIDCACHE_VERSION << endl;
    str << uidValidity() << endl;
    str << lastUid() << endl;
    uidcache.flush();
    if ( uidcache.status() == IO_Ok ) {
      fsync( uidcache.handle() ); /* this is probably overkill */
      uidcache.close();
      if ( uidcache.status() == IO_Ok )
        return 0;
    }
  }
  KMessageBox::error( 0,
        i18n( "The UID cache file for folder %1 could not be written. There "
              "could be a problem with file system permission." ).arg( folder()->prettyURL() ) );

  return -1;
}

void KMFolderCachedImap::reloadUidMap()
{
  //kdDebug(5006) << "Reloading Uid Map " << endl;
  uidMap.clear();
  open("reloadUdi");
  for( int i = 0; i < count(); ++i ) {
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue;
    ulong uid = msg->UID();
    //kdDebug(5006) << "Inserting: " << i << " with uid: " << uid << endl;
    uidMap.insert( uid, i );
  }
  close("reloadUdi");
  uidMapDirty = false;
}

/* Reimplemented from KMFolderMaildir */
KMMessage* KMFolderCachedImap::take(int idx)
{
  uidMapDirty = true;
  rememberDeletion( idx );
  return KMFolderMaildir::take(idx);
}

// Add a message without clearing it's X-UID field.
int KMFolderCachedImap::addMsgInternal( KMMessage* msg, bool newMail,
                                        int* index_return )
{
  // Possible optimization: Only dirty if not filtered below
  ulong uid = msg->UID();
  if( uid != 0 ) {
    uidMapDirty = true;
  }

  KMFolderOpener openThis(folder(), "KMFolderCachedImap::addMsgInternal");
  int rc = openThis.openResult();
  if ( rc ) {
    kdDebug(5006) << k_funcinfo << "open: " << rc << " of folder: " << label() << endl;
    return rc;
  }

  // Add the message
  rc = KMFolderMaildir::addMsg(msg, index_return);

  if( newMail && ( imapPath() == "/INBOX/" || ( !GlobalSettings::self()->filterOnlyDIMAPInbox()
      && (userRights() <= 0 || userRights() & ACLJobs::Administer )
      && (contentsType() == ContentsTypeMail || GlobalSettings::self()->filterGroupwareFolders()) ) ) )
  {
    // This is a new message. Filter it - maybe
    bool filter = false;
    if ( GlobalSettings::filterSourceFolders().isEmpty() ) {
      if ( imapPath() == "/INBOX/" )
        filter = true;
    } else {
      if ( GlobalSettings::filterSourceFolders().contains( folder()->id() ) )
        filter = true;
    }
    if ( filter )
      mAccount->processNewMsg( msg );
  }

  return rc;
}

/* Reimplemented from KMFolderMaildir */
int KMFolderCachedImap::addMsg(KMMessage* msg, int* index_return)
{
  if ( !canAddMsgNow( msg, index_return ) ) return 0;
  // Add it to storage
  int rc = KMFolderMaildir::addMsgInternal(msg, index_return, true /*stripUID*/);
  return rc;
}

void KMFolderCachedImap::rememberDeletion( int idx )
{
  KMMsgBase *msg = getMsgBase( idx );
  assert(msg);
  long uid = msg->UID();
  assert(uid>=0);
  mDeletedUIDsSinceLastSync.insert(uid, 0);
  kdDebug(5006) << "Explicit delete of UID " << uid << " at index: " << idx << " in " << folder()->prettyURL() << endl;
}

/* Reimplemented from KMFolderMaildir */
void KMFolderCachedImap::removeMsg(int idx, bool imapQuiet)
{
  uidMapDirty = true;
  rememberDeletion( idx );
  // Remove it from disk
  KMFolderMaildir::removeMsg(idx,imapQuiet);
}

bool KMFolderCachedImap::canRemoveFolder() const {
  // If this has subfolders it can't be removed
  if( folder() && folder()->child() && folder()->child()->count() > 0 )
    return false;

#if 0
  // No special condition here, so let base class decide
  return KMFolderMaildir::canRemoveFolder();
#endif
  return true;
}

/* Reimplemented from KMFolderDir */
int KMFolderCachedImap::rename( const QString& aName,
                                KMFolderDir* /*aParent*/ )
{
  QString oldName = mAccount->renamedFolder( imapPath() );
  if ( oldName.isEmpty() ) oldName = name();
  if ( aName == oldName )
    // Stupid user trying to rename it to it's old name :)
    return 0;

  if( account() == 0 || imapPath().isEmpty() ) { // I don't think any of this can happen anymore
    QString err = i18n("You must synchronize with the server before renaming IMAP folders.");
    KMessageBox::error( 0, err );
    return -1;
  }

  // Make the change appear to the user with setLabel, but we'll do the change
  // on the server during the next sync. The name() is the name at the time of
  // the last sync. Only rename if the new one is different. If it's the same,
  // don't rename, but also make sure the rename is reset, in the case of
  // A -> B -> A renames.
  if ( name() != aName )
    mAccount->addRenamedFolder( imapPath(), folder()->label(), aName );
  else
    mAccount->removeRenamedFolder( imapPath() );

  folder()->setLabel( aName );
  emit nameChanged(); // for kmailicalifaceimpl

  return 0;
}

KMFolder* KMFolderCachedImap::trashFolder() const
{
  QString trashStr = account()->trash();
  return kmkernel->dimapFolderMgr()->findIdString( trashStr );
}

void KMFolderCachedImap::setLastUid( ulong uid )
{
#if MAIL_LOSS_DEBUGGING
  kdDebug(5006) << "Setting mLastUid to: " << uid  <<  " in " << folder()->prettyURL() << endl;
#endif
  mLastUid = uid;
  if( uidWriteTimer == -1 )
    // Write in one minute
    uidWriteTimer = startTimer( 60000 );
}

void KMFolderCachedImap::timerEvent( QTimerEvent* )
{
  killTimer( uidWriteTimer );
  uidWriteTimer = -1;
  if ( writeUidCache() == -1 )
    unlink( QFile::encodeName( uidCacheLocation() ) );
}

ulong KMFolderCachedImap::lastUid()
{
  return mLastUid;
}

KMMsgBase* KMFolderCachedImap::findByUID( ulong uid )
{
  bool mapReloaded = false;
  if( uidMapDirty ) {
    reloadUidMap();
    mapReloaded = true;
  }

  QMap<ulong,int>::Iterator it = uidMap.find( uid );
  if( it != uidMap.end() ) {
    KMMsgBase *msg = getMsgBase( *it );
#if MAIL_LOSS_DEBUGGING
    kdDebug(5006) << "Folder: " << folder()->prettyURL() << endl;
    kdDebug(5006) << "UID " << uid << " is supposed to be in the map" << endl;
    kdDebug(5006) << "UID's index is to be " << *it << endl;
    kdDebug(5006) << "There is a message there? " << (msg != 0) << endl;
    if ( msg ) {
      kdDebug(5006) << "Its UID is: " << msg->UID() << endl;
    }
#endif

    if( msg && msg->UID() == uid )
      return msg;
    kdDebug(5006) << "########## Didn't find uid: " << uid << "in cache athough it's supposed to be there!" << endl;
  } else {
#if MAIL_LOSS_DEBUGGING
    kdDebug(5006) << "Didn't find uid: " << uid << "in cache!" << endl;
#endif
  }
  // Not found by now
 // if( mapReloaded )
    // Not here then
    return 0;
  // There could be a problem in the maps. Rebuild them and try again
  reloadUidMap();
  it = uidMap.find( uid );
  if( it != uidMap.end() )
    // Since the uid map is just rebuilt, no need for the sanity check
    return getMsgBase( *it );
#if MAIL_LOSS_DEBUGGING
  else
    kdDebug(5006) << "Reloaded, but stil didn't find uid: " << uid << endl;
#endif
  // Then it's not here
  return 0;
}

// This finds and sets the proper account for this folder if it has
// not been done
KMAcctCachedImap *KMFolderCachedImap::account() const
{
  if( (KMAcctCachedImap *)mAccount == 0 && kmkernel && kmkernel->acctMgr() ) {
    // Find the account
    mAccount = static_cast<KMAcctCachedImap *>( kmkernel->acctMgr()->findByName( name() ) );
  }

  return mAccount;
}

void KMFolderCachedImap::slotTroubleshoot()
{
  const int rc = DImapTroubleShootDialog::run();

  if( rc == DImapTroubleShootDialog::RefreshCache ) {
    // Refresh cache
    if( !account() ) {
      KMessageBox::sorry( 0, i18n("No account setup for this folder.\n"
                                  "Please try running a sync before this.") );
      return;
    }
    QString str = i18n("Are you sure you want to refresh the IMAP cache of "
                       "the folder %1 and all its subfolders?\nThis will "
                       "remove all changes you have done locally to your "
                       "folders.").arg( label() );
    QString s1 = i18n("Refresh IMAP Cache");
    QString s2 = i18n("&Refresh");
    if( KMessageBox::warningContinueCancel( 0, str, s1, s2 ) ==
        KMessageBox::Continue )
      account()->invalidateIMAPFolders( this );
  } else {
    // Rebuild index file
    switch ( rc ) {
      case DImapTroubleShootDialog::ReindexAll:
      {
        KMFolderCachedImap *rootStorage = dynamic_cast<KMFolderCachedImap*>( account()->rootFolder() );
        if ( rootStorage )
          rootStorage->createIndexFromContentsRecursive();
        break;
      }
      case DImapTroubleShootDialog::ReindexCurrent:
        createIndexFromContents();
        break;
      case DImapTroubleShootDialog::ReindexRecursive:
        createIndexFromContentsRecursive();
        break;
      default:
        return;
    }
    KMessageBox::information( 0, i18n( "The index of this folder has been "
                                       "recreated." ) );
    writeIndex();
    kmkernel->getKMMainWidget()->folderSelected();
  }
}

void KMFolderCachedImap::serverSync( bool recurse )
{
  if( mSyncState != SYNC_STATE_INITIAL ) {
    if( KMessageBox::warningYesNo( 0, i18n("Folder %1 is not in initial sync state (state was %2). Do you want to reset it to initial sync state and sync anyway?" ).arg( imapPath() ).arg( mSyncState ), QString::null, i18n("Reset && Sync"), KStdGuiItem::cancel() ) == KMessageBox::Yes ) {
      mSyncState = SYNC_STATE_INITIAL;
    } else return;
  }

  mRecurse = recurse;
  assert( account() );

  ProgressItem *progressItem = mAccount->mailCheckProgressItem();
  if ( progressItem ) {
    progressItem->reset();
    progressItem->setTotalItems( 100 );
  }
  mProgress = 0;

#if 0
  if( mHoldSyncs ) {
    // All done for this folder.
    account()->mailCheckProgressItem()->setProgress( 100 );
    mProgress = 100; // all done
    newState( mProgress, i18n("Synchronization skipped"));
    mSyncState = SYNC_STATE_INITIAL;
    emit folderComplete( this, true );
    return;
  }
#endif
  mTentativeHighestUid = 0; // reset, last sync could have been canceled

  serverSyncInternal();
}

QString KMFolderCachedImap::state2String( int state ) const
{
  switch( state ) {
  case SYNC_STATE_INITIAL:           return "SYNC_STATE_INITIAL";
  case SYNC_STATE_GET_USERRIGHTS:    return "SYNC_STATE_GET_USERRIGHTS";
  case SYNC_STATE_PUT_MESSAGES:      return "SYNC_STATE_PUT_MESSAGES";
  case SYNC_STATE_UPLOAD_FLAGS:      return "SYNC_STATE_UPLOAD_FLAGS";
  case SYNC_STATE_CREATE_SUBFOLDERS: return "SYNC_STATE_CREATE_SUBFOLDERS";
  case SYNC_STATE_LIST_SUBFOLDERS:   return "SYNC_STATE_LIST_SUBFOLDERS";
  case SYNC_STATE_LIST_NAMESPACES:   return "SYNC_STATE_LIST_NAMESPACES";
  case SYNC_STATE_LIST_SUBFOLDERS2:  return "SYNC_STATE_LIST_SUBFOLDERS2";
  case SYNC_STATE_DELETE_SUBFOLDERS: return "SYNC_STATE_DELETE_SUBFOLDERS";
  case SYNC_STATE_LIST_MESSAGES:     return "SYNC_STATE_LIST_MESSAGES";
  case SYNC_STATE_DELETE_MESSAGES:   return "SYNC_STATE_DELETE_MESSAGES";
  case SYNC_STATE_GET_MESSAGES:      return "SYNC_STATE_GET_MESSAGES";
  case SYNC_STATE_EXPUNGE_MESSAGES:  return "SYNC_STATE_EXPUNGE_MESSAGES";
  case SYNC_STATE_HANDLE_INBOX:      return "SYNC_STATE_HANDLE_INBOX";
  case SYNC_STATE_TEST_ANNOTATIONS:  return "SYNC_STATE_TEST_ANNOTATIONS";
  case SYNC_STATE_GET_ANNOTATIONS:   return "SYNC_STATE_GET_ANNOTATIONS";
  case SYNC_STATE_SET_ANNOTATIONS:   return "SYNC_STATE_SET_ANNOTATIONS";
  case SYNC_STATE_GET_ACLS:          return "SYNC_STATE_GET_ACLS";
  case SYNC_STATE_SET_ACLS:          return "SYNC_STATE_SET_ACLS";
  case SYNC_STATE_GET_QUOTA:         return "SYNC_STATE_GET_QUOTA";
  case SYNC_STATE_FIND_SUBFOLDERS:   return "SYNC_STATE_FIND_SUBFOLDERS";
  case SYNC_STATE_SYNC_SUBFOLDERS:   return "SYNC_STATE_SYNC_SUBFOLDERS";
  case SYNC_STATE_RENAME_FOLDER:     return "SYNC_STATE_RENAME_FOLDER";
  case SYNC_STATE_CHECK_UIDVALIDITY: return "SYNC_STATE_CHECK_UIDVALIDITY";
  default:                           return "Unknown state";
  }
}

/*
  Progress calculation: each step is assigned a span. Initially the total is 100.
  But if we skip a step, don't increase the progress.
  This leaves more room for the step a with variable size (get_messages)
   connecting 5
   getuserrights 5
   rename 5
   check_uidvalidity 5
   create_subfolders 5
   put_messages 10 (but it can take a very long time, with many messages....)
   upload_flags 5
   list_subfolders 5
   list_subfolders2 0 (all local)
   delete_subfolders 5
   list_messages 10
   delete_messages 10
   expunge_messages 5
   get_messages variable (remaining-5) i.e. minimum 15.
   check_annotations 0 (rare)
   set_annotations 0 (rare)
   get_annotations 2
   set_acls 0 (rare)
   get_acls 3

  noContent folders have only a few of the above steps
  (permissions, and all subfolder stuff), so its steps should be given more span

 */

// While the server synchronization is running, mSyncState will hold
// the state that should be executed next
void KMFolderCachedImap::serverSyncInternal()
{
  // This is used to stop processing when we're about to exit
  // and the current job wasn't cancellable.
  // For user-requested abort, we'll use signalAbortRequested instead.
  if( kmkernel->mailCheckAborted() ) {
    resetSyncState();
    emit folderComplete( this, false );
    return;
  }

  //kdDebug(5006) << label() << ": " << state2String( mSyncState ) << endl;
  switch( mSyncState ) {
  case SYNC_STATE_INITIAL:
  {
    mProgress = 0;
    foldersForDeletionOnServer.clear();
    newState( mProgress, i18n("Synchronizing"));

    open("cachedimap");
    if ( !noContent() )
        mAccount->addLastUnreadMsgCount( this, countUnread() );

    // Connect to the server (i.e. prepare the slave)
    ImapAccountBase::ConnectionState cs = mAccount->makeConnection();
    if ( cs == ImapAccountBase::Error ) {
      // Cancelled by user, or slave can't start
      // kdDebug(5006) << "makeConnection said Error, aborting." << endl;
      // We stop here. We're already in SYNC_STATE_INITIAL for the next time.
      newState( mProgress, i18n( "Error connecting to server %1" ).arg( mAccount->host() ) );
      close("cachedimap");
      emit folderComplete(this, false);
      break;
    } else if ( cs == ImapAccountBase::Connecting ) {
      mAccount->setAnnotationCheckPassed( false );
      // kdDebug(5006) << "makeConnection said Connecting, waiting for signal." << endl;
      newState( mProgress, i18n("Connecting to %1").arg( mAccount->host() ) );
      // We'll wait for the connectionResult signal from the account.
      connect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
               this, SLOT( slotConnectionResult(int, const QString&) ) );
      break;
    } else {
      // Connected
      // kdDebug(5006) << "makeConnection said Connected, proceeding." << endl;
      mSyncState = SYNC_STATE_GET_USERRIGHTS;
      // Fall through to next state
    }
  }


  case SYNC_STATE_GET_USERRIGHTS:
    //kdDebug(5006) << "===== Syncing " << ( mImapPath.isEmpty() ? label() : mImapPath ) << endl;

    mSyncState = SYNC_STATE_RENAME_FOLDER;

    if( !noContent() && mAccount->hasACLSupport() ) {
      // Check the user's own rights. We do this every time in case they changed.
      mOldUserRights = mUserRights;
      newState( mProgress, i18n("Checking permissions"));
      connect( mAccount, SIGNAL( receivedUserRights( KMFolder* ) ),
               this, SLOT( slotReceivedUserRights( KMFolder* ) ) );
      mAccount->getUserRights( folder(), imapPath() ); // after connecting, due to the INBOX case
      break;
    }

  case SYNC_STATE_RENAME_FOLDER:
  {
    mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;
    // Returns the new name if the folder was renamed, empty otherwise.
    bool isResourceFolder = kmkernel->iCalIface().isStandardResourceFolder( folder() );
    QString newName = mAccount->renamedFolder( imapPath() );
    if ( !newName.isEmpty() && !folder()->isSystemFolder() && !isResourceFolder ) {
      newState( mProgress, i18n("Renaming folder") );
      CachedImapJob *job = new CachedImapJob( newName, CachedImapJob::tRenameFolder, this );
      connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    }
  }

  case SYNC_STATE_CHECK_UIDVALIDITY:
    mSyncState = SYNC_STATE_CREATE_SUBFOLDERS;
    if( !noContent() ) {
      checkUidValidity();
      break;
    }
    // Else carry on

  case SYNC_STATE_CREATE_SUBFOLDERS:
    mSyncState = SYNC_STATE_PUT_MESSAGES;
    createNewFolders();
    break;

  case SYNC_STATE_PUT_MESSAGES:
    mSyncState = SYNC_STATE_UPLOAD_FLAGS;
    if( !noContent() ) {
      uploadNewMessages();
      break;
    }
    // Else carry on
  case SYNC_STATE_UPLOAD_FLAGS:
    mSyncState = SYNC_STATE_LIST_NAMESPACES;
    if( !noContent() ) {
       // We haven't downloaded messages yet, so we need to build the map.
       if( uidMapDirty )
         reloadUidMap();
       // Upload flags, unless we know from the ACL that we're not allowed
       // to do that or they did not change locally
       if ( mUserRights <= 0 || ( mUserRights & (KMail::ACLJobs::WriteFlags ) ) ) {
         if ( mStatusChangedLocally ) {
           uploadFlags();
           break;
         } else {
           //kdDebug(5006) << "Skipping flags upload, folder unchanged: " << label() << endl;
         }
       } else if ( mUserRights & KMail::ACLJobs::WriteSeenFlag ) {
         if ( mStatusChangedLocally ) {
           uploadSeenFlags();
           break;
         }
       }
    }
    // Else carry on

  case SYNC_STATE_LIST_NAMESPACES:
    if ( this == mAccount->rootFolder() ) {
      listNamespaces();
      break;
    }
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS;
    // Else carry on

  case SYNC_STATE_LIST_SUBFOLDERS:
    newState( mProgress, i18n("Retrieving folderlist"));
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS2;
    if( !listDirectory() ) {
      mSyncState = SYNC_STATE_INITIAL;
      KMessageBox::error(0, i18n("Error while retrieving the folderlist"));
    }
    break;

  case SYNC_STATE_LIST_SUBFOLDERS2:
    mSyncState = SYNC_STATE_DELETE_SUBFOLDERS;
    mProgress += 10;
    newState( mProgress, i18n("Retrieving subfolders"));
    listDirectory2();
    break;

  case SYNC_STATE_DELETE_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_MESSAGES;
    if( !foldersForDeletionOnServer.isEmpty() ) {
      newState( mProgress, i18n("Deleting folders from server"));
      CachedImapJob* job = new CachedImapJob( foldersForDeletionOnServer,
                                                  CachedImapJob::tDeleteFolders, this );
      connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
      connect( job, SIGNAL( finished() ), this, SLOT( slotFolderDeletionOnServerFinished() ) );
      job->start();
      break;
    }
    // Not needed, the next step emits newState very quick
    //newState( mProgress, i18n("No folders to delete from server"));
      // Carry on

  case SYNC_STATE_LIST_MESSAGES:
    mSyncState = SYNC_STATE_DELETE_MESSAGES;
    if( !noContent() ) {
      newState( mProgress, i18n("Retrieving message list"));
      listMessages();
      break;
    }
    // Else carry on

  case SYNC_STATE_DELETE_MESSAGES:
    mSyncState = SYNC_STATE_EXPUNGE_MESSAGES;
    if( !noContent() ) {
      if( deleteMessages() ) {
        // Fine, we will continue with the next state
      } else {
        // No messages to delete, skip to GET_MESSAGES
        newState( mProgress, i18n("No messages to delete..."));
        mSyncState = SYNC_STATE_GET_MESSAGES;
        serverSyncInternal();
      }
      break;
    }
    // Else carry on

  case SYNC_STATE_EXPUNGE_MESSAGES:
    mSyncState = SYNC_STATE_GET_MESSAGES;
    if( !noContent() ) {
      newState( mProgress, i18n("Expunging deleted messages"));
      CachedImapJob *job = new CachedImapJob( QString::null,
                                              CachedImapJob::tExpungeFolder, this );
      connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    }
    // Else carry on

  case SYNC_STATE_GET_MESSAGES:
    mSyncState = SYNC_STATE_HANDLE_INBOX;
    if( !noContent() ) {
      if( !mMsgsForDownload.isEmpty() ) {
        newState( mProgress, i18n("Retrieving new messages"));
        CachedImapJob *job = new CachedImapJob( mMsgsForDownload,
                                                CachedImapJob::tGetMessage,
                                                this );
        connect( job, SIGNAL( progress(unsigned long, unsigned long) ),
                 this, SLOT( slotProgress(unsigned long, unsigned long) ) );
        connect( job, SIGNAL( finished() ), this, SLOT( slotUpdateLastUid() ) );
        connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
        job->start();
        mMsgsForDownload.clear();
        break;
      } else {
        newState( mProgress, i18n("No new messages from server"));
        /* There were no messages to download, but it could be that we uploaded some
           which we didn't need to download again because we already knew the uid.
           Now that we are sure there is nothing to download, and everything that had
           to be deleted on the server has been deleted, adjust our local notion of the
           highes uid seen thus far. */
        slotUpdateLastUid();
        if( mLastUid == 0 && uidWriteTimer == -1 ) {
          // This is probably a new and empty folder. Write the UID cache
          if ( writeUidCache() == -1 ) {
            resetSyncState();
            emit folderComplete( this, false );
            return;
          }
        }
      }
    }

    // Else carry on

  case SYNC_STATE_HANDLE_INBOX:
    // Wrap up the 'download emails' stage. We always end up at 95 here.
    mProgress = 95;
    mSyncState = SYNC_STATE_TEST_ANNOTATIONS;

  #define KOLAB_FOLDERTEST "/vendor/kolab/folder-test"
  case SYNC_STATE_TEST_ANNOTATIONS:
    mSyncState = SYNC_STATE_GET_ANNOTATIONS;
    // The first folder with user rights to write annotations
    if( !mAccount->annotationCheckPassed() &&
         ( mUserRights <= 0 || ( mUserRights & ACLJobs::Administer ) )
         && !imapPath().isEmpty() && imapPath() != "/" ) {
      kdDebug(5006) << "Setting test attribute on folder: "<< folder()->prettyURL() << endl;
      newState( mProgress, i18n("Checking annotation support"));

      KURL url = mAccount->getUrl();
      url.setPath( imapPath() );
      KMail::AnnotationList annotations; // to be set

      KMail::AnnotationAttribute attr( KOLAB_FOLDERTEST, "value.shared", "true" );
      annotations.append( attr );

      kdDebug(5006) << "Setting test attribute to "<< url << endl;
      KIO::Job* job = AnnotationJobs::multiSetAnnotation( mAccount->slave(),
          url, annotations );
      ImapAccountBase::jobData jd( url.url(), folder() );
      jd.cancellable = true; // we can always do so later
      mAccount->insertJob(job, jd);
       connect(job, SIGNAL(result(KIO::Job *)),
              SLOT(slotTestAnnotationResult(KIO::Job *)));
      break;
    }

  case SYNC_STATE_GET_ANNOTATIONS: {
#define KOLAB_FOLDERTYPE "/vendor/kolab/folder-type"
#define KOLAB_INCIDENCESFOR "/vendor/kolab/incidences-for"
//#define KOLAB_FOLDERTYPE "/comment"  //for testing, while cyrus-imap doesn't support /vendor/*
    mSyncState = SYNC_STATE_SET_ANNOTATIONS;

    bool needToGetInitialAnnotations = false;
    if ( !noContent() ) {
      // for a folder we didn't create ourselves: get annotation from server
      if ( mAnnotationFolderType == "FROMSERVER" ) {
        needToGetInitialAnnotations = true;
        mAnnotationFolderType = QString::null;
      } else {
        updateAnnotationFolderType();
      }
    }

    // First retrieve the annotation, so that we know we have to set it if it's not set.
    // On the other hand, if the user changed the contentstype, there's no need to get first.
    if ( !noContent() && mAccount->hasAnnotationSupport() &&
        ( kmkernel->iCalIface().isEnabled() || needToGetInitialAnnotations ) ) {
      QStringList annotations; // list of annotations to be fetched
      if ( !mAnnotationFolderTypeChanged || mAnnotationFolderType.isEmpty() )
        annotations << KOLAB_FOLDERTYPE;
      if ( !mIncidencesForChanged )
        annotations << KOLAB_INCIDENCESFOR;
      if ( !annotations.isEmpty() ) {
        newState( mProgress, i18n("Retrieving annotations"));
        KURL url = mAccount->getUrl();
        url.setPath( imapPath() );
        AnnotationJobs::MultiGetAnnotationJob* job =
          AnnotationJobs::multiGetAnnotation( mAccount->slave(), url, annotations );
        ImapAccountBase::jobData jd( url.url(), folder() );
        jd.cancellable = true;
        mAccount->insertJob(job, jd);

        connect( job, SIGNAL(annotationResult(const QString&, const QString&, bool)),
                 SLOT(slotAnnotationResult(const QString&, const QString&, bool)) );
        connect( job, SIGNAL(result(KIO::Job *)),
                 SLOT(slotGetAnnotationResult(KIO::Job *)) );
        break;
      }
    }
  } // case
  case SYNC_STATE_SET_ANNOTATIONS:

    mSyncState = SYNC_STATE_SET_ACLS;
    if ( !noContent() && mAccount->hasAnnotationSupport() &&
         ( mUserRights <= 0 || ( mUserRights & ACLJobs::Administer ) ) ) {
      newState( mProgress, i18n("Setting annotations"));
      KURL url = mAccount->getUrl();
      url.setPath( imapPath() );
      KMail::AnnotationList annotations; // to be set
      if ( mAnnotationFolderTypeChanged && !mAnnotationFolderType.isEmpty() ) {
        KMail::AnnotationAttribute attr( KOLAB_FOLDERTYPE, "value.shared", mAnnotationFolderType );
        annotations.append( attr );
        kdDebug(5006) << "Setting folder-type annotation for " << label() << " to " << mAnnotationFolderType << endl;
      }
      if ( mIncidencesForChanged ) {
        const QString val = incidencesForToString( mIncidencesFor );
        KMail::AnnotationAttribute attr( KOLAB_INCIDENCESFOR, "value.shared", val );
        annotations.append( attr );
        kdDebug(5006) << "Setting incidences-for annotation for " << label() << " to " << val << endl;
      }
      if ( !annotations.isEmpty() ) {
        KIO::Job* job =
          AnnotationJobs::multiSetAnnotation( mAccount->slave(), url, annotations );
        ImapAccountBase::jobData jd( url.url(), folder() );
        jd.cancellable = true; // we can always do so later
        mAccount->insertJob(job, jd);

        connect(job, SIGNAL(annotationChanged( const QString&, const QString&, const QString& ) ),
                SLOT( slotAnnotationChanged( const QString&, const QString&, const QString& ) ));
        connect(job, SIGNAL(result(KIO::Job *)),
                SLOT(slotSetAnnotationResult(KIO::Job *)));
        break;
      }
    }

  case SYNC_STATE_SET_ACLS:
    mSyncState = SYNC_STATE_GET_ACLS;

    if( !noContent() && mAccount->hasACLSupport() &&
      ( mUserRights <= 0 || ( mUserRights & ACLJobs::Administer ) ) ) {
      bool hasChangedACLs = false;
      ACLList::ConstIterator it = mACLList.begin();
      for ( ; it != mACLList.end() && !hasChangedACLs; ++it ) {
        hasChangedACLs = (*it).changed;
      }
      if ( hasChangedACLs ) {
        newState( mProgress, i18n("Setting permissions"));
        KURL url = mAccount->getUrl();
        url.setPath( imapPath() );
        KIO::Job* job = KMail::ACLJobs::multiSetACL( mAccount->slave(), url, mACLList );
        ImapAccountBase::jobData jd( url.url(), folder() );
        mAccount->insertJob(job, jd);

        connect(job, SIGNAL(result(KIO::Job *)),
                SLOT(slotMultiSetACLResult(KIO::Job *)));
        connect(job, SIGNAL(aclChanged( const QString&, int )),
                SLOT(slotACLChanged( const QString&, int )) );
        break;
      }
    }

  case SYNC_STATE_GET_ACLS:
    mSyncState = SYNC_STATE_GET_QUOTA;

    if( !noContent() && mAccount->hasACLSupport() ) {
      newState( mProgress, i18n( "Retrieving permissions" ) );
      mAccount->getACL( folder(), mImapPath );
      connect( mAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
               this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );
      break;
    }
  case SYNC_STATE_GET_QUOTA:
    // Continue with the subfolders
    mSyncState = SYNC_STATE_FIND_SUBFOLDERS;
    if( !noContent() && mAccount->hasQuotaSupport() ) {
      newState( mProgress, i18n("Getting quota information"));
      KURL url = mAccount->getUrl();
      url.setPath( imapPath() );
      KIO::Job* job = KMail::QuotaJobs::getStorageQuota( mAccount->slave(), url );
      ImapAccountBase::jobData jd( url.url(), folder() );
      mAccount->insertJob(job, jd);
      connect( job, SIGNAL( storageQuotaResult( const QuotaInfo& ) ),
          SLOT( slotStorageQuotaResult( const QuotaInfo& ) ) );
      connect( job, SIGNAL(result(KIO::Job *)),
          SLOT(slotQuotaResult(KIO::Job *)) );
      break;
    }
  case SYNC_STATE_FIND_SUBFOLDERS:
    {
      mProgress = 98;
      newState( mProgress, i18n("Updating cache file"));

      mSyncState = SYNC_STATE_SYNC_SUBFOLDERS;
      mSubfoldersForSync.clear();
      mCurrentSubfolder = 0;
      if( folder() && folder()->child() ) {
        KMFolderNode *node = folder()->child()->first();
        while( node ) {
          if( !node->isDir() ) {
            KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
            // Only sync folders that have been accepted by the server
            if ( !storage->imapPath().isEmpty()
                 // and that were not just deleted from it
                 && !foldersForDeletionOnServer.contains( storage->imapPath() ) ) {
              mSubfoldersForSync << storage;
            } else {
              kdDebug(5006) << "Do not add " << storage->label()
                << " to synclist" << endl;
            }
          }
          node = folder()->child()->next();
        }
      }

    // All done for this folder.
    mProgress = 100; // all done
    newState( mProgress, i18n("Synchronization done"));
      KURL url = mAccount->getUrl();
      url.setPath( imapPath() );
      kmkernel->iCalIface().folderSynced( folder(), url );
    }

    if ( !mRecurse ) // "check mail for this folder" only
      mSubfoldersForSync.clear();

    // Carry on
  case SYNC_STATE_SYNC_SUBFOLDERS:
    {
      if( mCurrentSubfolder ) {
        disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                    this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );
        mCurrentSubfolder = 0;
      }

      if( mSubfoldersForSync.isEmpty() ) {
        mSyncState = SYNC_STATE_INITIAL;
        mAccount->addUnreadMsgCount( this, countUnread() ); // before closing
        close("cachedimap");
        emit folderComplete( this, true );
      } else {
        mCurrentSubfolder = mSubfoldersForSync.front();
        mSubfoldersForSync.pop_front();
        connect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                 this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );

        //kdDebug(5006) << "Sync'ing subfolder " << mCurrentSubfolder->imapPath() << endl;
        assert( !mCurrentSubfolder->imapPath().isEmpty() );
        mCurrentSubfolder->setAccount( account() );
        bool recurse = mCurrentSubfolder->noChildren() ? false : true;
        mCurrentSubfolder->serverSync( recurse );
      }
    }
    break;

  default:
    kdDebug(5006) << "KMFolderCachedImap::serverSyncInternal() WARNING: no such state "
              << mSyncState << endl;
  }
}

/* Connected to the imap account's connectionResult signal.
   Emitted when the slave connected or failed to connect.
*/
void KMFolderCachedImap::slotConnectionResult( int errorCode, const QString& errorMsg )
{
  disconnect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( slotConnectionResult(int, const QString&) ) );
  if ( !errorCode ) {
    // Success
    mSyncState = SYNC_STATE_GET_USERRIGHTS;
    mProgress += 5;
    serverSyncInternal();
  } else {
    // Error (error message already shown by the account)
    newState( mProgress, KIO::buildErrorString( errorCode, errorMsg ));
    emit folderComplete(this, false);
  }
}

/* find new messages (messages without a UID) */
QValueList<unsigned long> KMFolderCachedImap::findNewMessages()
{
  QValueList<unsigned long> result;
  for( int i = 0; i < count(); ++i ) {
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if ( msg->UID() == 0 )
      result.append( msg->getMsgSerNum() );
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QValueList<unsigned long> newMsgs = findNewMessages();
  if( !newMsgs.isEmpty() ) {
    if ( mUserRights <= 0 || ( mUserRights & ( KMail::ACLJobs::Insert ) ) ) {
      newState( mProgress, i18n("Uploading messages to server"));
      CachedImapJob *job = new CachedImapJob( newMsgs, CachedImapJob::tPutMessage, this );
      connect( job, SIGNAL( progress( unsigned long, unsigned long) ),
               this, SLOT( slotPutProgress(unsigned long, unsigned long) ) );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      return;
    } else {
      KMCommand *command = rescueUnsyncedMessages();
      connect( command, SIGNAL( completed( KMCommand * ) ),
               this, SLOT( serverSyncInternal() ) );
    }
  } else { // nothing to upload
    if ( mUserRights != mOldUserRights && (mOldUserRights & KMail::ACLJobs::Insert)
         && !(mUserRights & KMail::ACLJobs::Insert) ) {
      // write access revoked
      KMessageBox::information( 0, i18n("<p>Your access rights to folder <b>%1</b> have been restricted, "
          "it will no longer be possible to add messages to this folder.</p>").arg( folder()->prettyURL() ),
          i18n("Acces rights revoked"), "KMailACLRevocationNotification" );
    }
  }
  newState( mProgress, i18n("No messages to upload to server"));
  serverSyncInternal();
}

/* Progress info during uploadNewMessages */
void KMFolderCachedImap::slotPutProgress( unsigned long done, unsigned long total )
{
  // (going from mProgress to mProgress+10)
  int progressSpan = 10;
  newState( mProgress + (progressSpan * done) / total, QString::null );
  if ( done == total ) // we're done
    mProgress += progressSpan;
}

/* Upload message flags to server */
void KMFolderCachedImap::uploadFlags()
{
  if ( !uidMap.isEmpty() ) {
    mStatusFlagsJobs = 0;
    newState( mProgress, i18n("Uploading status of messages to server"));

    // FIXME DUPLICATED FROM KMFOLDERIMAP
    QMap< QString, QStringList > groups;
    //open(); //already done
    for( int i = 0; i < count(); ++i ) {
      KMMsgBase* msg = getMsgBase( i );
      if( !msg || msg->UID() == 0 )
        // Either not a valid message or not one that is on the server yet
        continue;

      QString flags = KMFolderImap::statusToFlags(msg->status(), mPermanentFlags);
      // Collect uids for each typem of flags.
      QString uid;
      uid.setNum( msg->UID() );
      groups[flags].append(uid);
    }
    QMapIterator< QString, QStringList > dit;
    for( dit = groups.begin(); dit != groups.end(); ++dit ) {
      QCString flags = dit.key().latin1();
      QStringList sets = KMFolderImap::makeSets( (*dit), true );
      mStatusFlagsJobs += sets.count(); // ### that's not in kmfolderimap....
      // Send off a status setting job for each set.
      for( QStringList::Iterator slit = sets.begin(); slit != sets.end(); ++slit ) {
        QString imappath = imapPath() + ";UID=" + ( *slit );
        mAccount->setImapStatus(folder(), imappath, flags);
      }
    }
    // FIXME END DUPLICATED FROM KMFOLDERIMAP

    if ( mStatusFlagsJobs ) {
      connect( mAccount, SIGNAL( imapStatusChanged(KMFolder*, const QString&, bool) ),
               this, SLOT( slotImapStatusChanged(KMFolder*, const QString&, bool) ) );
      return;
    }
  }
  newState( mProgress, i18n("No messages to upload to server"));
  serverSyncInternal();
}

void KMFolderCachedImap::uploadSeenFlags()
{
  if ( !uidMap.isEmpty() ) {
    mStatusFlagsJobs = 0;
    newState( mProgress, i18n("Uploading status of messages to server"));

    QValueList<ulong> seenUids, unseenUids;
    for( int i = 0; i < count(); ++i ) {
      KMMsgBase* msg = getMsgBase( i );
      if( !msg || msg->UID() == 0 )
        // Either not a valid message or not one that is on the server yet
        continue;

      if ( msg->status() & KMMsgStatusOld || msg->status() & KMMsgStatusRead )
        seenUids.append( msg->UID() );
      else
        unseenUids.append( msg->UID() );
    }
    if ( !seenUids.isEmpty() ) {
      QStringList sets = KMFolderImap::makeSets( seenUids, true );
      mStatusFlagsJobs += sets.count();
      for( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it ) {
        QString imappath = imapPath() + ";UID=" + ( *it );
        mAccount->setImapSeenStatus( folder(), imappath, true );
      }
    }
    if ( !unseenUids.isEmpty() ) {
      QStringList sets = KMFolderImap::makeSets( unseenUids, true );
      mStatusFlagsJobs += sets.count();
      for( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it ) {
        QString imappath = imapPath() + ";UID=" + ( *it );
        mAccount->setImapSeenStatus( folder(), imappath, false );
      }
    }

    if ( mStatusFlagsJobs ) {
      connect( mAccount, SIGNAL( imapStatusChanged(KMFolder*, const QString&, bool) ),
               this, SLOT( slotImapStatusChanged(KMFolder*, const QString&, bool) ) );
      return;
    }
  }
  newState( mProgress, i18n("No messages to upload to server"));
  serverSyncInternal();
}

void KMFolderCachedImap::slotImapStatusChanged(KMFolder* folder, const QString&, bool cont)
{
  if ( mSyncState == SYNC_STATE_INITIAL ){
      //kdDebug(5006) << "IMAP status changed but reset " << endl;
      return; // we were reset
  }
  //kdDebug(5006) << "IMAP status changed for folder: " << folder->prettyURL() << endl;
  if ( folder->storage() == this ) {
    --mStatusFlagsJobs;
    if ( mStatusFlagsJobs == 0 || !cont ) // done or aborting
      disconnect( mAccount, SIGNAL( imapStatusChanged(KMFolder*, const QString&, bool) ),
                  this, SLOT( slotImapStatusChanged(KMFolder*, const QString&, bool) ) );
    if ( mStatusFlagsJobs == 0 && cont ) {
      mProgress += 5;
      serverSyncInternal();
      //kdDebug(5006) << "Proceeding with mailcheck." << endl;
    }
  }
}

// This is not perfect, what if the status didn't really change? Oh well ...
void KMFolderCachedImap::setStatus( int idx, KMMsgStatus status, bool toggle)
{
  KMFolderMaildir::setStatus( idx, status, toggle );
  mStatusChangedLocally = true;
}

void KMFolderCachedImap::setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle)
{
  KMFolderMaildir::setStatus(ids, status, toggle);
  mStatusChangedLocally = true;
}

/* Upload new folders to server */
void KMFolderCachedImap::createNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders = findNewFolders();
  //kdDebug(5006) << label() << " createNewFolders:" << newFolders.count() << " new folders." << endl;
  if( !newFolders.isEmpty() ) {
    newState( mProgress, i18n("Creating subfolders on server"));
    CachedImapJob *job = new CachedImapJob( newFolders, CachedImapJob::tAddSubfolders, this );
    connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  } else {
    serverSyncInternal();
  }
}

QValueList<KMFolderCachedImap*> KMFolderCachedImap::findNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders;
  if( folder() && folder()->child() ) {
    KMFolderNode *node = folder()->child()->first();
    while( node ) {
      if( !node->isDir() ) {
        if( static_cast<KMFolder*>(node)->folderType() != KMFolderTypeCachedImap ) {
          kdError(5006) << "KMFolderCachedImap::findNewFolders(): ARGH!!! "
                        << node->name() << " is not an IMAP folder\n";
          node = folder()->child()->next();
          assert(0);
        }
        KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if( folder->imapPath().isEmpty() ) {
          newFolders << folder;
        }
      }
      node = folder()->child()->next();
    }
  }
  return newFolders;
}

bool KMFolderCachedImap::deleteMessages()
{
  /* Delete messages from cache that are gone from the server */
  QPtrList<KMMessage> msgsForDeletion;

  // It is not possible to just go over all indices and remove
  // them one by one because the index list can get resized under
  // us. So use msg pointers instead

  QStringList uids;
  QMap<ulong,int>::const_iterator it = uidMap.constBegin();
  for( ; it != uidMap.end(); it++ ) {
    ulong uid ( it.key() );
    if( uid!=0 && !uidsOnServer.find( uid ) ) {
      uids << QString::number( uid );
      msgsForDeletion.append( getMsg( *it ) );
    }
  }

  if( !msgsForDeletion.isEmpty() ) {
#if MAIL_LOSS_DEBUGGING
      if ( KMessageBox::warningYesNo(
             0, i18n( "<qt><p>Mails on the server in folder <b>%1</b> were deleted. "
                 "Do you want to delete them locally?<br>UIDs: %2</p></qt>" )
             .arg( folder()->prettyURL() ).arg( uids.join(",") ) ) == KMessageBox::Yes )
#endif
        removeMsg( msgsForDeletion );
  }

  if ( mUserRights > 0 && !( mUserRights & KMail::ACLJobs::Delete ) )
    return false;

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    newState( mProgress, i18n("Deleting removed messages from server"));
    QStringList sets = KMFolderImap::makeSets( uidsForDeletionOnServer, true );
    uidsForDeletionOnServer.clear();
    kdDebug(5006) << "Deleting " << sets.count() << " sets of messages from server folder " << imapPath() << endl;
    CachedImapJob *job = new CachedImapJob( sets, CachedImapJob::tDeleteMessage, this );
    connect( job, SIGNAL( result(KMail::FolderJob *) ),
             this, SLOT( slotDeleteMessagesResult(KMail::FolderJob *) ) );
    job->start();
    return true;
  } else {
    return false;
  }
}

void KMFolderCachedImap::slotDeleteMessagesResult( KMail::FolderJob* job )
{
  if ( job->error() ) {
    // Skip the EXPUNGE state if deleting didn't work, no need to show two error messages
    mSyncState = SYNC_STATE_GET_MESSAGES;
  } else {
    // deleting on the server went fine, clear the pending deletions cache
    mDeletedUIDsSinceLastSync.clear();
  }
  mProgress += 10;
  serverSyncInternal();
}

void KMFolderCachedImap::checkUidValidity() {
  // IMAP root folders don't seem to have a UID validity setting.
  // Also, don't try the uid validity on new folders
  if( imapPath().isEmpty() || imapPath() == "/" )
    // Just proceed
    serverSyncInternal();
  else {
    newState( mProgress, i18n("Checking folder validity"));
    CachedImapJob *job = new CachedImapJob( FolderJob::tCheckUidValidity, this );
    connect( job, SIGNAL(permanentFlags(int)), SLOT(slotPermanentFlags(int)) );
    connect( job, SIGNAL( result( KMail::FolderJob* ) ),
             this, SLOT( slotCheckUidValidityResult( KMail::FolderJob* ) ) );
    job->start();
  }
}

void KMFolderCachedImap::slotCheckUidValidityResult( KMail::FolderJob* job )
{
  if ( job->error() ) { // there was an error and the user chose "continue"
    // We can't continue doing anything in the same folder though, it would delete all mails.
    // But we can continue to subfolders if any. Well we can also try annotation/acl stuff...
    mSyncState = SYNC_STATE_HANDLE_INBOX;
  }
  mProgress += 5;
  serverSyncInternal();
}

void KMFolderCachedImap::slotPermanentFlags(int flags)
{
  mPermanentFlags = flags;
}

/* This will only list the messages in a folder.
   No directory listing done*/
void KMFolderCachedImap::listMessages() {
  bool groupwareOnly = GlobalSettings::self()->showOnlyGroupwareFoldersForGroupwareAccount()
               && GlobalSettings::self()->theIMAPResourceAccount() == (int)mAccount->id()
               && folder()->isSystemFolder()
               && mImapPath == "/INBOX/";
  // Don't list messages on the root folder, and skip the inbox, if this is
  // the inbox of a groupware-only dimap account
  if( imapPath() == "/" || groupwareOnly ) {
    serverSyncInternal();
    return;
  }

  if( !mAccount->slave() ) { // sync aborted
    resetSyncState();
    emit folderComplete( this, false );
    return;
  }
  uidsOnServer.clear();
  uidsOnServer.resize( count() * 2 );
  uidsForDeletionOnServer.clear();
  mMsgsForDownload.clear();
  mUidsForDownload.clear();
  // listing is only considered successful if saw a syntactically correct imapdigest
  mFoundAnIMAPDigest = false;

  CachedImapJob* job = new CachedImapJob( FolderJob::tListMessages, this );
  connect( job, SIGNAL( result(KMail::FolderJob *) ),
           this, SLOT( slotGetLastMessagesResult(KMail::FolderJob *) ) );
  job->start();
}

void KMFolderCachedImap::slotGetLastMessagesResult(KMail::FolderJob *job)
{
  getMessagesResult(job, true);
}

// Connected to the listMessages job in CachedImapJob
void KMFolderCachedImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    // be sure to reset the sync state, if the listing was partial we would
    // otherwise delete not-listed mail locally, and on the next sync on the server
    // as well
    mSyncState = SYNC_STATE_HANDLE_INBOX;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }
  (*it).cdata += QCString(data, data.size() + 1);
  int pos = (*it).cdata.find("\r\n--IMAPDIGEST");
  if (pos > 0) {
    int a = (*it).cdata.find("\r\nX-uidValidity:");
    if (a != -1) {
      int b = (*it).cdata.find("\r\n", a + 17);
      setUidValidity((*it).cdata.mid(a + 17, b - a - 17));
    }
    a = (*it).cdata.find("\r\nX-Access:");
    // Only trust X-Access (i.e. the imap select info) if we don't know mUserRights.
    // The latter is more accurate (checked on every sync) whereas X-Access is only
    // updated when selecting the folder again, which might not happen if using
    // RMB / Check Mail in this folder. We don't need two (potentially conflicting)
    // sources for the readonly setting, in any case.
    if (a != -1 && mUserRights == -1 ) {
      int b = (*it).cdata.find("\r\n", a + 12);
      const QString access = (*it).cdata.mid(a + 12, b - a - 12);
      setReadOnly( access == "Read only" );
    }
    (*it).cdata.remove(0, pos);
    mFoundAnIMAPDigest = true;
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  // Start with something largish when rebuilding the cache
  if ( uidsOnServer.size() == 0 )
    uidsOnServer.resize( KMail::nextPrime( 2000 ) );
  const int v = 42;
  while (pos >= 0) {
      /*
    KMMessage msg;
    msg.fromString((*it).cdata.mid(16, pos - 16));
    const int flags = msg.headerField("X-Flags").toInt();
    const ulong size = msg.headerField("X-Length").toULong();
    const ulong uid = msg.UID();
       */
    // The below is optimized for speed, not prettiness. The commented out chunk
    // above was the solution copied from kmfolderimap, and it's 15-20% slower.
    const QCString& entry( (*it).cdata );
    const int indexOfUID = entry.find("X-UID", 16);
    const int startOfUIDValue = indexOfUID  + 7;
    const int indexOfLength = entry.find("X-Length", startOfUIDValue ); // we know length comes after UID
    const int startOfLengthValue = indexOfLength + 10;
    const int indexOfFlags = entry.find("X-Flags", startOfLengthValue ); // we know flags comes last
    const int startOfFlagsValue = indexOfFlags + 9;

    const int flags = entry.mid( startOfFlagsValue, entry.find( '\r', startOfFlagsValue ) - startOfFlagsValue ).toInt();
    const ulong size = entry.mid( startOfLengthValue, entry.find( '\r', startOfLengthValue ) - startOfLengthValue ).toULong();
    const ulong uid = entry.mid( startOfUIDValue, entry.find( '\r', startOfUIDValue ) - startOfUIDValue ).toULong();

    const bool deleted = ( flags & 8 );
    if ( !deleted ) {
      if( uid != 0 ) {
        if ( uidsOnServer.count() == uidsOnServer.size() ) {
          uidsOnServer.resize( KMail::nextPrime( uidsOnServer.size() * 2 ) );
          //kdDebug( 5006 ) << "Resizing to: " << uidsOnServer.size() << endl;
        }
        uidsOnServer.insert( uid, &v );
      }
      bool redownload = false;
      if (  uid <= lastUid() ) {
       /*
        * If this message UID is not present locally, then it must
        * have been deleted by the user, so we delete it on the
        * server also. If we don't have delete permissions on the server,
        * re-download the message, it must have vanished by some error, or
        * while we still thought we were allowed to delete (ACL change).
        *
        * This relies heavily on lastUid() being correct at all times.
        */
        // kdDebug(5006) << "KMFolderCachedImap::slotGetMessagesData() : folder "<<label()<<" already has msg="<<msg->headerField("Subject") << ", UID="<<uid << ", lastUid = " << mLastUid << endl;
        KMMsgBase *existingMessage = findByUID(uid);
        if( !existingMessage ) {
#if MAIL_LOSS_DEBUGGING
           kdDebug(5006) << "Looking at uid " << uid << " high water is: " << lastUid() << " we should delete it" << endl;
#endif
          // double check we deleted it since the last sync
           if ( mDeletedUIDsSinceLastSync.contains(uid) ) {
               if ( mUserRights <= 0 || ( mUserRights & KMail::ACLJobs::Delete ) ) {
#if MAIL_LOSS_DEBUGGING
                   kdDebug(5006) << "message with uid " << uid << " is gone from local cache. Must be deleted on server!!!" << endl;
#endif
                   uidsForDeletionOnServer << uid;
               } else {
                   redownload = true;
               }
           } else {
               kdDebug(5006) << "WARNING: ####### " << endl;
               kdDebug(5006) << "Message locally missing but not deleted in folder: " << folder()->prettyURL() << endl;
               kdDebug(5006) << "The missing UID: " << uid << ". It will be redownloaded " << endl;
               redownload = true;
           }

        } else {
          // if this is a read only folder, ignore status updates from the server
          // since we can't write our status back our local version is what has to
          // be considered correct.
          if ( !mReadOnly || !GlobalSettings::allowLocalFlags() ) {
            /* The message is OK, update flags */
            KMFolderImap::flagsToStatus( existingMessage, flags,  false, mReadOnly ? INT_MAX : mPermanentFlags );
          } else if ( mUserRights & KMail::ACLJobs::WriteSeenFlag ) {
            KMFolderImap::seenFlagToStatus( existingMessage, flags );
          }
        }
        // kdDebug(5006) << "message with uid " << uid << " found in the local cache. " << endl;
      }
      if ( uid > lastUid() || redownload ) {
#if MAIL_LOSS_DEBUGGING
        kdDebug(5006) << "Looking at uid " << uid << " high water is: " << lastUid() << " we should download it" << endl;
#endif
        // The message is new since the last sync, but we might have just uploaded it, in which case
        // the uid map already contains it.
        if ( !uidMap.contains( uid ) ) {
          mMsgsForDownload << KMail::CachedImapJob::MsgForDownload(uid, flags, size);
          if( imapPath() == "/INBOX/" )
            mUidsForDownload << uid;
        }
        // Remember the highest uid and once the download is completed, update mLastUid
        if ( uid > mTentativeHighestUid ) {
#if MAIL_LOSS_DEBUGGING
          kdDebug(5006) << "Setting the tentative highest UID to: " << uid << endl;
#endif
          mTentativeHighestUid = uid;
        }
      }
    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  }
}

void KMFolderCachedImap::getMessagesResult( KMail::FolderJob *job, bool lastSet )
{
  mProgress += 10;
  if ( !job->error() && !mFoundAnIMAPDigest ) {
      kdWarning(5006) << "######## Folderlisting did not complete, but there was no error! "
          "Aborting sync of folder: " << folder()->prettyURL() << endl;
#if MAIL_LOSS_DEBUGGING
      kmkernel->emergencyExit( i18n("Folder listing failed in interesting ways." ) );
#endif
  }
  if( job->error() ) { // error listing messages but the user chose to continue
    mContentState = imapNoInformation;
    mSyncState = SYNC_STATE_HANDLE_INBOX; // be sure not to continue in this folder
  } else {
    if( lastSet ) { // always true here (this comes from online-imap...)
      mContentState = imapFinished;
      mStatusChangedLocally = false; // we are up to date again
    }
  }
  serverSyncInternal();
}

void KMFolderCachedImap::slotProgress(unsigned long done, unsigned long total)
{
  int progressSpan = 100 - 5 - mProgress;
  //kdDebug(5006) << "KMFolderCachedImap::slotProgress done=" << done << " total=" << total << "=> mProgress=" << mProgress + ( progressSpan * done ) / total << endl;
  // Progress info while retrieving new emails
  // (going from mProgress to mProgress+progressSpan)
  newState( mProgress + (progressSpan * done) / total, QString::null );
}

void KMFolderCachedImap::setAccount(KMAcctCachedImap *aAccount)
{
  assert( aAccount->isA("KMAcctCachedImap") );
  mAccount = aAccount;
  if( imapPath()=="/" ) aAccount->setFolder( folder() );

  // Folder was renamed in a previous session, and the user didn't sync yet
  QString newName = mAccount->renamedFolder( imapPath() );
  if ( !newName.isEmpty() )
    folder()->setLabel( newName );

  if( !folder() || !folder()->child() || !folder()->child()->count() ) return;
  for( KMFolderNode* node = folder()->child()->first(); node;
       node = folder()->child()->next() )
    if (!node->isDir())
      static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->setAccount(aAccount);
}

void KMFolderCachedImap::listNamespaces()
{
  ImapAccountBase::ListType type = ImapAccountBase::List;
  if ( mAccount->onlySubscribedFolders() )
    type = ImapAccountBase::ListSubscribed;

  kdDebug(5006) << "listNamespaces " << mNamespacesToList << endl;
  if ( mNamespacesToList.isEmpty() ) {
    mSyncState = SYNC_STATE_DELETE_SUBFOLDERS;
    mPersonalNamespacesCheckDone = true;

    QStringList ns = mAccount->namespaces()[ImapAccountBase::OtherUsersNS];
    ns += mAccount->namespaces()[ImapAccountBase::SharedNS];
    mNamespacesToCheck = ns.count();
    for ( QStringList::Iterator it = ns.begin(); it != ns.end(); ++it )
    {
      if ( (*it).isEmpty() ) {
        // ignore empty listings as they have been listed before
        --mNamespacesToCheck;
        continue;
      }
      KMail::ListJob* job = new KMail::ListJob( mAccount, type, this, mAccount->addPathToNamespace( *it ) );
      job->setHonorLocalSubscription( true );
      connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
              const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
          this, SLOT(slotCheckNamespace(const QStringList&, const QStringList&,
              const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
      job->start();
    }
    if ( mNamespacesToCheck == 0 ) {
      serverSyncInternal();
    }
    return;
  }
  mPersonalNamespacesCheckDone = false;

  QString ns = mNamespacesToList.front();
  mNamespacesToList.pop_front();

  mSyncState = SYNC_STATE_LIST_SUBFOLDERS2;
  newState( mProgress, i18n("Retrieving folders for namespace %1").arg(ns));
  KMail::ListJob* job = new KMail::ListJob( mAccount, type, this,
      mAccount->addPathToNamespace( ns ) );
  job->setNamespace( ns );
  job->setHonorLocalSubscription( true );
  connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
      this, SLOT(slotListResult(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
  job->start();
}

void KMFolderCachedImap::slotCheckNamespace( const QStringList& subfolderNames,
                                             const QStringList& subfolderPaths,
                                             const QStringList& subfolderMimeTypes,
                                             const QStringList& subfolderAttributes,
                                             const ImapAccountBase::jobData& jobData )
{
  Q_UNUSED( subfolderPaths );
  Q_UNUSED( subfolderMimeTypes );
  Q_UNUSED( subfolderAttributes );
  --mNamespacesToCheck;
  kdDebug(5006) << "slotCheckNamespace " << subfolderNames << ",remain=" <<
   mNamespacesToCheck << endl;

  // get a correct foldername:
  // strip / and make sure it does not contain the delimiter
  QString name = jobData.path.mid( 1, jobData.path.length()-2 );
  name.remove( mAccount->delimiterForNamespace( name ) );
  if ( name.isEmpty() ) {
    // should not happen
    kdWarning(5006) << "slotCheckNamespace: ignoring empty folder!" << endl;
    return;
  }

  folder()->createChildFolder();
  KMFolderNode *node = 0;
  for ( node = folder()->child()->first(); node;
        node = folder()->child()->next())
  {
    if ( !node->isDir() && node->name() == name )
      break;
  }
  if ( !subfolderNames.isEmpty() ) {
    if ( node ) {
      // folder exists so we have nothing to do - it will be listed later
      kdDebug(5006) << "found namespace folder " << name << endl;
    } else
    {
      // create folder
      kdDebug(5006) << "create namespace folder " << name << endl;
      KMFolder* newFolder = folder()->child()->createFolder( name, false,
          KMFolderTypeCachedImap );
      if ( newFolder ) {
        KMFolderCachedImap *f = static_cast<KMFolderCachedImap*>( newFolder->storage() );
        f->setImapPath( mAccount->addPathToNamespace( name ) );
        f->setNoContent( true );
        f->setAccount( mAccount );
        f->close("cachedimap");
        kmkernel->dimapFolderMgr()->contentsChanged();
      }
    }
  } else {
    if ( node ) {
      kdDebug(5006) << "delete namespace folder " << name << endl;
      KMFolder* fld = static_cast<KMFolder*>(node);
      kmkernel->dimapFolderMgr()->remove( fld );
    }
  }

  if ( mNamespacesToCheck == 0 ) {
    // all namespaces are done so continue with the next step
    serverSyncInternal();
  }
}

// This lists the subfolders on the server
// and (in slotListResult) takes care of folders that have been removed on the server
bool KMFolderCachedImap::listDirectory()
{
  if( !mAccount->slave() ) { // sync aborted
    resetSyncState();
    emit folderComplete( this, false );
    return false;
  }
  mSubfolderState = imapInProgress;

  // get the folders
  ImapAccountBase::ListType type = ImapAccountBase::List;
  if ( mAccount->onlySubscribedFolders() )
    type = ImapAccountBase::ListSubscribed;
  KMail::ListJob* job = new KMail::ListJob( mAccount, type, this );
  job->setHonorLocalSubscription( true );
  connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
      this, SLOT(slotListResult(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
  job->start();

  return true;
}

void KMFolderCachedImap::slotListResult( const QStringList& folderNames,
                                         const QStringList& folderPaths,
                                         const QStringList& folderMimeTypes,
                                         const QStringList& folderAttributes,
                                         const ImapAccountBase::jobData& jobData )
{
  Q_UNUSED( jobData );
  //kdDebug(5006) << label() << ": folderNames=" << folderNames << " folderPaths="
  //<< folderPaths << " mimeTypes=" << folderMimeTypes << endl;
  mSubfolderNames = folderNames;
  mSubfolderPaths = folderPaths;
  mSubfolderMimeTypes = folderMimeTypes;
  mSubfolderState = imapFinished;
  mSubfolderAttributes = folderAttributes;
  kdDebug(5006) << "##### setting subfolder attributes: " << mSubfolderAttributes << endl;

  folder()->createChildFolder();
  KMFolderNode *node = folder()->child()->first();
  bool root = ( this == mAccount->rootFolder() );

  QPtrList<KMFolder> toRemove;
  bool emptyList = ( root && mSubfolderNames.empty() );
  if ( !emptyList ) {
    while (node) {
      if (!node->isDir() ) {
        KMFolderCachedImap *f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());

        if ( mSubfolderNames.findIndex(node->name()) == -1 ) {
          QString name = node->name();
          // as more than one namespace can be listed in the root folder we need to make sure
          // that the folder is within the current namespace
          bool isInNamespace = ( jobData.curNamespace.isEmpty() ||
              jobData.curNamespace == mAccount->namespaceForFolder( f ) );
          // ignore some cases
          bool ignore = root && ( f->imapPath() == "/INBOX/" ||
              mAccount->isNamespaceFolder( name ) || !isInNamespace );

          // This subfolder isn't present on the server
          if( !f->imapPath().isEmpty() && !ignore  ) {
            // The folder has an imap path set, so it has been
            // on the server before. Delete it locally.
            toRemove.append( f->folder() );
            kdDebug(5006) << node->name() << " isn't on the server. It has an imapPath -> delete it locally" << endl;
          }
        } else { // folder both local and on server
          //kdDebug(5006) << node->name() << " is on the server." << endl;

          /**
           * Store the folder attributes for every subfolder.
           */
          int index = mSubfolderNames.findIndex( node->name() );
          f->mFolderAttributes = folderAttributes[ index ];
        }
      } else {
        //kdDebug(5006) << "skipping dir node:" << node->name() << endl;
      }
      node = folder()->child()->next();
    }
  }

  for ( KMFolder* doomed=toRemove.first(); doomed; doomed = toRemove.next() ) {
    rescueUnsyncedMessagesAndDeleteFolder( doomed );
  }

  mProgress += 5;

  // just in case there is nothing to rescue
  slotRescueDone( 0 );
}

// This synchronizes the local folders as needed (creation/deletion). No network communication here.
void KMFolderCachedImap::listDirectory2()
{
  QString path = folder()->path();
  kmkernel->dimapFolderMgr()->quiet(true);

  bool root = ( this == mAccount->rootFolder() );
  if ( root && !mAccount->hasInbox() )
  {
    KMFolderCachedImap *f = 0;
    KMFolderNode *node;
    // create the INBOX
    for (node = folder()->child()->first(); node; node = folder()->child()->next())
      if (!node->isDir() && node->name() == "INBOX") break;
    if (node) {
      f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
    } else {
      KMFolder* newFolder = folder()->child()->createFolder("INBOX", true, KMFolderTypeCachedImap);
      if ( newFolder ) {
        f = static_cast<KMFolderCachedImap*>(newFolder->storage());
      }
    }
    if ( f ) {
      f->setAccount( mAccount );
      f->setImapPath( "/INBOX/" );
      f->folder()->setLabel( i18n("inbox") );
    }
    if (!node) {
      if ( f )
        f->close("cachedimap");
      kmkernel->dimapFolderMgr()->contentsChanged();
    }
    // so we have an INBOX
    mAccount->setHasInbox( true );
  }

  if ( root && !mSubfolderNames.isEmpty() ) {
    KMFolderCachedImap* parent =
      findParent( mSubfolderPaths.first(), mSubfolderNames.first() );
    if ( parent ) {
      kdDebug(5006) << "KMFolderCachedImap::listDirectory2 - pass listing to "
        << parent->label() << endl;
      mSubfolderNames.clear();
    }
  }

  // Find all subfolders present on server but not on disk
  QValueVector<int> foldersNewOnServer;
  for (uint i = 0; i < mSubfolderNames.count(); i++) {

    // Find the subdir, if already present
    KMFolderCachedImap *f = 0;
    KMFolderNode *node = 0;
    for (node = folder()->child()->first(); node;
         node = folder()->child()->next())
      if (!node->isDir() && node->name() == mSubfolderNames[i]) break;

    if (!node) {
      // This folder is not present here
      // Either it's new on the server, or we just deleted it.
      QString subfolderPath = mSubfolderPaths[i];
      // The code used to look at the uidcache to know if it was "just deleted".
      // But this breaks with noContent folders and with shared folders.
      // So instead we keep a list in the account.
      bool locallyDeleted = mAccount->isDeletedFolder( subfolderPath );
      // That list is saved/restored across sessions, but to avoid any mistake,
      // ask for confirmation if the folder was deleted in a previous session
      // (could be that the folder was deleted & recreated meanwhile from another client...)
      if ( !locallyDeleted && mAccount->isPreviouslyDeletedFolder( subfolderPath ) ) {
           locallyDeleted = KMessageBox::warningYesNo(
             0, i18n( "<qt><p>It seems that the folder <b>%1</b> was deleted. Do you want to delete it from the server?</p></qt>" ).arg( mSubfolderNames[i] ), QString::null, KStdGuiItem::del(), KStdGuiItem::cancel() ) == KMessageBox::Yes;
      }

      if ( locallyDeleted ) {
        kdDebug(5006) << subfolderPath << " was deleted locally => delete on server." << endl;
        foldersForDeletionOnServer += mAccount->deletedFolderPaths( subfolderPath ); // grab all subsubfolders too
      } else {
        kdDebug(5006) << subfolderPath << " is a new folder on the server => create local cache" << endl;
        foldersNewOnServer.append( i );
      }
    } else { // Folder found locally
      if( static_cast<KMFolder*>(node)->folderType() == KMFolderTypeCachedImap )
        f = dynamic_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
      if( f ) {
        // kdDebug(5006) << "folder("<<f->name()<<")->imapPath()=" << f->imapPath()
        //               << "\nSetting imapPath " << mSubfolderPaths[i] << endl;
        // Write folder settings
        f->setAccount(mAccount);
        f->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
        f->setNoChildren(mSubfolderMimeTypes[i] == "message/digest");
        f->setImapPath(mSubfolderPaths[i]);
      }
    }
  }

  /* In case we are ignoring non-groupware folders, and this is the groupware
   * main account, find out the contents types of folders that have newly
   * appeared on the server. Otherwise just create them and finish listing.
   * If a folder is already known to be locally unsubscribed, it won't be
   * listed at all, on this level, so these are only folders that we are
   * seeing for the first time. */

  /*  Note: We ask the globalsettings, and not the current state of the
   *  kmkernel->iCalIface().isEnabled(), since that is false during the
   *  very first sync, where we already want to filter. */
  if ( GlobalSettings::self()->showOnlyGroupwareFoldersForGroupwareAccount()
     && GlobalSettings::self()->theIMAPResourceAccount() == (int)mAccount->id()
     && mAccount->hasAnnotationSupport()
     && GlobalSettings::self()->theIMAPResourceEnabled()
     && !foldersNewOnServer.isEmpty() ) {

    QStringList paths;
    for ( uint i = 0; i < foldersNewOnServer.count(); ++i )
      paths << mSubfolderPaths[ foldersNewOnServer[i] ];

    AnnotationJobs::MultiUrlGetAnnotationJob* job =
      AnnotationJobs::multiUrlGetAnnotation( mAccount->slave(), mAccount->getUrl(), paths, KOLAB_FOLDERTYPE );
    ImapAccountBase::jobData jd( QString::null, folder() );
    jd.cancellable = true;
    mAccount->insertJob(job, jd);
    connect( job, SIGNAL(result(KIO::Job *)),
        SLOT(slotMultiUrlGetAnnotationResult(KIO::Job *)) );

  } else {
    createFoldersNewOnServerAndFinishListing( foldersNewOnServer );
  }
}

void KMFolderCachedImap::createFoldersNewOnServerAndFinishListing( const QValueVector<int> foldersNewOnServer )
{
  for ( uint i = 0; i < foldersNewOnServer.count(); ++i ) {
    int idx = foldersNewOnServer[i];
    KMFolder* newFolder = folder()->child()->createFolder( mSubfolderNames[idx], false, KMFolderTypeCachedImap);
    if (newFolder) {
      KMFolderCachedImap *f = dynamic_cast<KMFolderCachedImap*>(newFolder->storage());
      kdDebug(5006) << " ####### Locally creating folder " << mSubfolderNames[idx] <<endl;
      f->close("cachedimap");
      f->setAccount(mAccount);
      f->mAnnotationFolderType = "FROMSERVER";
      f->setNoContent(mSubfolderMimeTypes[idx] == "inode/directory");
      f->setNoChildren(mSubfolderMimeTypes[idx] == "message/digest");
      f->setImapPath(mSubfolderPaths[idx]);
      f->mFolderAttributes = mSubfolderAttributes[idx];
      kdDebug(5006) << " ####### Attributes: " << f->mFolderAttributes <<endl;
      //kdDebug(5006) << subfolderPath << ": mAnnotationFolderType set to FROMSERVER" << endl;
      kmkernel->dimapFolderMgr()->contentsChanged();
    } else {
      kdDebug(5006) << "can't create folder " << mSubfolderNames[idx] <<endl;
    }
  }

  kmkernel->dimapFolderMgr()->quiet(false);
  emit listComplete(this);
  if ( !mPersonalNamespacesCheckDone ) {
    // we're not done with the namespaces
    mSyncState = SYNC_STATE_LIST_NAMESPACES;
  }
  serverSyncInternal();
}

//-----------------------------------------------------------------------------
KMFolderCachedImap* KMFolderCachedImap::findParent( const QString& path,
                                                    const QString& name )
{
  QString parent = path.left( path.length() - name.length() - 2 );
  if ( parent.length() > 1 )
  {
    // extract name of the parent
    parent = parent.right( parent.length() - 1 );
    if ( parent != label() )
    {
      KMFolderNode *node = folder()->child()->first();
      // look for a better parent
      while ( node )
      {
        if ( node->name() == parent )
        {
          KMFolder* fld = static_cast<KMFolder*>(node);
          KMFolderCachedImap* imapFld =
            static_cast<KMFolderCachedImap*>( fld->storage() );
          return imapFld;
        }
        node = folder()->child()->next();
      }
    }
  }
  return 0;
}

void KMFolderCachedImap::slotSubFolderComplete(KMFolderCachedImap* sub, bool success)
{
  Q_UNUSED(sub);
  //kdDebug(5006) << label() << " slotSubFolderComplete: " << sub->label() << endl;
  if ( success ) {
    serverSyncInternal();
  }
  else
  {
    // success == false means the sync was aborted.
    if ( mCurrentSubfolder ) {
      Q_ASSERT( sub == mCurrentSubfolder );
      disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                  this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );
      mCurrentSubfolder = 0;
    }

    mSubfoldersForSync.clear();
    mSyncState = SYNC_STATE_INITIAL;
    close("cachedimap");
    emit folderComplete( this, false );
  }
}

void KMFolderCachedImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if (it == mAccount->jobsEnd()) return;
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}

FolderJob*
KMFolderCachedImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                 QString, const AttachmentStrategy* ) const
{
  QPtrList<KMMessage> msgList;
  msgList.append( msg );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

FolderJob*
KMFolderCachedImap::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                 FolderJob::JobType jt, KMFolder *folder ) const
{
  //FIXME: how to handle sets here?
  Q_UNUSED( sets );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

void
KMFolderCachedImap::setUserRights( unsigned int userRights )
{
  mUserRights = userRights;
  writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();
}

void
KMFolderCachedImap::slotReceivedUserRights( KMFolder* folder )
{
  if ( folder->storage() == this ) {
    disconnect( mAccount, SIGNAL( receivedUserRights( KMFolder* ) ),
                this, SLOT( slotReceivedUserRights( KMFolder* ) ) );
    if ( mUserRights == 0 ) // didn't work
      mUserRights = -1; // error code (used in folderdia)
    else
      setReadOnly( ( mUserRights & KMail::ACLJobs::Insert ) == 0 );
    mProgress += 5;
    serverSyncInternal();
  }
}

void
KMFolderCachedImap::setReadOnly( bool readOnly )
{
  if ( readOnly != mReadOnly ) {
    mReadOnly = readOnly;
    emit readOnlyChanged( folder() );
  }
}

void
KMFolderCachedImap::slotReceivedACL( KMFolder* folder, KIO::Job*, const KMail::ACLList& aclList )
{
  if ( folder->storage() == this ) {
    disconnect( mAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
                this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );
    mACLList = aclList;
    serverSyncInternal();
  }
}

void
KMFolderCachedImap::slotStorageQuotaResult( const QuotaInfo& info )
{
  setQuotaInfo( info );
}

void KMFolderCachedImap::setQuotaInfo( const QuotaInfo & info )
{
    if ( info != mQuotaInfo ) {
      mQuotaInfo = info;
      writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();
      emit folderSizeChanged();
    }
}

void
KMFolderCachedImap::setACLList( const ACLList& arr )
{
  mACLList = arr;
}

void
KMFolderCachedImap::slotMultiSetACLResult(KIO::Job *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  if ( job->error() )
    // Display error but don't abort the sync just for this
    // PENDING(dfaure) reconsider using handleJobError now that it offers continue/cancel
    job->showErrorDialog();
  else
    kmkernel->iCalIface().addFolderChange( folder(), KMailICalIfaceImpl::ACL );

  if (mAccount->slave()) mAccount->removeJob(job);
  serverSyncInternal();
}

void
KMFolderCachedImap::slotACLChanged( const QString& userId, int permissions )
{
  // The job indicates success in changing the permissions for this user
  // -> we note that it's been done.
  for( ACLList::Iterator it = mACLList.begin(); it != mACLList.end(); ++it ) {
    if ( (*it).userId == userId && (*it).permissions == permissions ) {
      if ( permissions == -1 ) // deleted
        mACLList.erase( it );
      else // added/modified
        (*it).changed = false;
      return;
    }
  }
}

// called by KMAcctCachedImap::killAllJobs
void KMFolderCachedImap::resetSyncState()
{
  if ( mSyncState == SYNC_STATE_INITIAL ) return;
  mSubfoldersForSync.clear();
  mSyncState = SYNC_STATE_INITIAL;
  close("cachedimap");
  // Don't use newState here, it would revert to mProgress (which is < current value when listing messages)
  KPIM::ProgressItem *progressItem = mAccount->mailCheckProgressItem();
  QString str = i18n("Aborted");
  if (progressItem)
     progressItem->setStatus( str );
  emit statusMsg( str );
}

void KMFolderCachedImap::slotIncreaseProgress()
{
  mProgress += 5;
}

void KMFolderCachedImap::newState( int progress, const QString& syncStatus )
{
  //kdDebug() << k_funcinfo << folder() << " " << mProgress << " " << syncStatus << endl;
  KPIM::ProgressItem *progressItem = mAccount->mailCheckProgressItem();
  if( progressItem )
    progressItem->setCompletedItems( progress );
  if ( !syncStatus.isEmpty() ) {
    QString str;
    // For a subfolder, show the label. But for the main folder, it's already shown.
    if ( mAccount->imapFolder() == this )
      str = syncStatus;
    else
      str = QString( "%1: %2" ).arg( label() ).arg( syncStatus );
    if( progressItem )
      progressItem->setStatus( str );
    emit statusMsg( str );
  }
  if( progressItem )
    progressItem->updateProgress();
}

void KMFolderCachedImap::setSubfolderState( imapState state )
{
  mSubfolderState = state;
  if ( state == imapNoInformation && folder()->child() )
  {
    // pass through to childs
    KMFolderNode* node;
    QPtrListIterator<KMFolderNode> it( *folder()->child() );
    for ( ; (node = it.current()); )
    {
      ++it;
      if (node->isDir()) continue;
      KMFolder *folder = static_cast<KMFolder*>(node);
      static_cast<KMFolderCachedImap*>(folder->storage())->setSubfolderState( state );
    }
  }
}

void KMFolderCachedImap::setImapPath(const QString &path)
{
  mImapPath = path;
}

// mAnnotationFolderType is the annotation as known to the server (and stored in kmailrc)
// It is updated from the folder contents type and whether it's a standard resource folder.
// This happens during the syncing phase and during initFolder for a new folder.
// Don't do it earlier, e.g. from setContentsType:
// on startup, it's too early there to know if this is a standard resource folder.
void KMFolderCachedImap::updateAnnotationFolderType()
{
  QString oldType = mAnnotationFolderType;
  QString oldSubType;
  int dot = oldType.find( '.' );
  if ( dot != -1 ) {
    oldType.truncate( dot );
    oldSubType = mAnnotationFolderType.mid( dot + 1 );
  }

  QString newType, newSubType;
  // We want to store an annotation on the folder only if using the kolab storage.
  if ( kmkernel->iCalIface().storageFormat( folder() ) == KMailICalIfaceImpl::StorageXML ) {
    newType = KMailICalIfaceImpl::annotationForContentsType( mContentsType );
    if ( kmkernel->iCalIface().isStandardResourceFolder( folder() ) )
      newSubType = "default";
    else
      newSubType = oldSubType; // preserve unknown subtypes, like drafts etc. And preserve ".default" too.
  }

  //kdDebug(5006) << mImapPath << ": updateAnnotationFolderType: " << newType << " " << newSubType << endl;
  if ( newType != oldType || newSubType != oldSubType ) {
    mAnnotationFolderType = newType + ( newSubType.isEmpty() ? QString::null : "."+newSubType );
    mAnnotationFolderTypeChanged = true; // force a "set annotation" on next sync
    kdDebug(5006) << mImapPath << ": updateAnnotationFolderType: '" << mAnnotationFolderType << "', was (" << oldType << " " << oldSubType << ") => mAnnotationFolderTypeChanged set to TRUE" << endl;
  }
  // Ensure that further readConfig()s don't lose mAnnotationFolderType
  writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();
}

void KMFolderCachedImap::setIncidencesFor( IncidencesFor incfor )
{
  if ( mIncidencesFor != incfor ) {
    mIncidencesFor = incfor;
    mIncidencesForChanged = true;
  }
}

void KMFolderCachedImap::slotAnnotationResult(const QString& entry, const QString& value, bool found)
{
  if ( entry == KOLAB_FOLDERTYPE ) {
    // There are four cases.
    // 1) no content-type on server -> set it
    // 2) different content-type on server, locally changed -> set it (we don't even come here)
    // 3) different (known) content-type on server, no local change -> get it
    // 4) different unknown content-type on server, probably some older version -> set it
    if ( found ) {
      QString type = value;
      QString subtype;
      int dot = value.find( '.' );
      if ( dot != -1 ) {
        type.truncate( dot );
        subtype = value.mid( dot + 1 );
      }
      bool foundKnownType = false;
      for ( uint i = 0 ; i <= ContentsTypeLast; ++i ) {
        FolderContentsType contentsType = static_cast<KMail::FolderContentsType>( i );
        if ( type == KMailICalIfaceImpl::annotationForContentsType( contentsType ) ) {
          // Case 3: known content-type on server, get it
          //kdDebug(5006) << mImapPath << ": slotGetAnnotationResult: found known type of annotation" << endl;
          if ( contentsType != ContentsTypeMail )
            kmkernel->iCalIface().setStorageFormat( folder(), KMailICalIfaceImpl::StorageXML );
          mAnnotationFolderType = value;
          if ( folder()->parent()->owner()->idString() != GlobalSettings::self()->theIMAPResourceFolderParent()
               && GlobalSettings::self()->theIMAPResourceEnabled()
               && subtype == "default" ) {
            // Truncate subtype if this folder can't be a default resource folder for us,
            // although it apparently is for someone else.
            mAnnotationFolderType = type;
            kdDebug(5006) << mImapPath << ": slotGetAnnotationResult: parent folder is " << folder()->parent()->owner()->idString() << " => truncating annotation to " << value << endl;
          }
          setContentsType( contentsType );
          mAnnotationFolderTypeChanged = false; // we changed it, not the user
          foundKnownType = true;

          // Users don't read events/contacts/etc. in kmail, so mark them all as read.
          // This is done in cachedimapjob when getting new messages, but do it here too,
          // for the initial set of messages when we didn't know this was a resource folder yet,
          // for old folders, etc.
          if ( contentsType != ContentsTypeMail )
            markUnreadAsRead();

          // Ensure that further readConfig()s don't lose mAnnotationFolderType
          writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();
          break;
        }
      }
      if ( !foundKnownType && !mReadOnly ) {
        //kdDebug(5006) << "slotGetAnnotationResult: no known type of annotation found, will need to set it" << endl;
        // Case 4: server has strange content-type, set it to what we need
        mAnnotationFolderTypeChanged = true;
      }
      // TODO handle subtype (inbox, drafts, sentitems, junkemail)
    }
    else if ( !mReadOnly ) {
      // Case 1: server doesn't have content-type, set it
      //kdDebug(5006) << "slotGetAnnotationResult: no annotation found, will need to set it" << endl;
      mAnnotationFolderTypeChanged = true;
    }
  } else if ( entry == KOLAB_INCIDENCESFOR ) {
    if ( found ) {
      mIncidencesFor = incidencesForFromString( value );
      Q_ASSERT( mIncidencesForChanged == false );
    }
  }
}

void KMFolderCachedImap::slotGetAnnotationResult( KIO::Job* job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  Q_ASSERT( it != mAccount->jobsEnd() );
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  Q_ASSERT( (*it).parent == folder() );
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  AnnotationJobs::GetAnnotationJob* annjob = static_cast<AnnotationJobs::GetAnnotationJob *>( job );
  if ( annjob->error() ) {
    if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION ) {
      // that's when the imap server doesn't support annotations
      if ( GlobalSettings::self()->theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML
           && (uint)GlobalSettings::self()->theIMAPResourceAccount() == mAccount->id() )
	KMessageBox::error( 0, i18n( "The IMAP server %1 does not have support for IMAP annotations. The XML storage cannot be used on this server; please re-configure KMail differently." ).arg( mAccount->host() ) );
      mAccount->setHasNoAnnotationSupport();
    }
    else
      kdWarning(5006) << "slotGetAnnotationResult: " << job->errorString() << endl;
  }

  if (mAccount->slave()) mAccount->removeJob(job);
  mProgress += 2;
  serverSyncInternal();
}

void KMFolderCachedImap::slotMultiUrlGetAnnotationResult( KIO::Job* job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  Q_ASSERT( it != mAccount->jobsEnd() );
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  Q_ASSERT( (*it).parent == folder() );
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  QValueVector<int> folders;
  AnnotationJobs::MultiUrlGetAnnotationJob* annjob
    = static_cast<AnnotationJobs::MultiUrlGetAnnotationJob *>( job );
  if ( annjob->error() ) {
    if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION ) {
      // that's when the imap server doesn't support annotations
      if ( GlobalSettings::self()->theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML
           && (uint)GlobalSettings::self()->theIMAPResourceAccount() == mAccount->id() )
        KMessageBox::error( 0, i18n( "The IMAP server %1 doesn't have support for imap annotations. The XML storage cannot be used on this server, please re-configure KMail differently" ).arg( mAccount->host() ) );
      mAccount->setHasNoAnnotationSupport();
    }
    else
      kdWarning(5006) << "slotGetMultiUrlAnnotationResult: " << job->errorString() << endl;
  } else {
    // we got the annotation allright, let's filter out the ones with the wrong type
    QMap<QString, QString> annotations = annjob->annotations();
    QMap<QString, QString>::Iterator it = annotations.begin();
    for ( ; it != annotations.end(); ++it ) {
      const QString folderPath = it.key();
      const QString annotation = it.data();
      kdDebug(5006) << k_funcinfo << "Folder: " << folderPath << " has type: " << annotation << endl;
      // we're only interested in the main type
      QString type(annotation);
      int dot = annotation.find( '.' );
      if ( dot != -1 ) type.truncate( dot );
      type = type.simplifyWhiteSpace();

      const int idx = mSubfolderPaths.findIndex( folderPath );
      const bool isNoContent =  mSubfolderMimeTypes[idx] == "inode/directory";
      if ( ( isNoContent && type.isEmpty() )
        || ( !type.isEmpty() && type != KMailICalIfaceImpl::annotationForContentsType( ContentsTypeMail ) ) ) {
        folders.append( idx );
        kdDebug(5006) << k_funcinfo << " subscribing to: " << folderPath << endl;
      } else {
        kdDebug(5006) << k_funcinfo << " automatically unsubscribing from: " << folderPath << endl;
        mAccount->changeLocalSubscription( folderPath, false );
      }
    }
  }

  if (mAccount->slave()) mAccount->removeJob(job);
  createFoldersNewOnServerAndFinishListing( folders );
}

void KMFolderCachedImap::slotQuotaResult( KIO::Job* job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  Q_ASSERT( it != mAccount->jobsEnd() );
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  Q_ASSERT( (*it).parent == folder() );
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  QuotaJobs::GetStorageQuotaJob* quotajob = static_cast<QuotaJobs::GetStorageQuotaJob *>( job );
  QuotaInfo empty;
  if ( quotajob->error() ) {
    if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION ) {
      // that's when the imap server doesn't support quota
      mAccount->setHasNoQuotaSupport();
      setQuotaInfo( empty );
    }
    else
      kdWarning(5006) << "slotGetQuotaResult: " << job->errorString() << endl;
  }

  if (mAccount->slave()) mAccount->removeJob(job);
  mProgress += 2;
  serverSyncInternal();
}

void
KMFolderCachedImap::slotAnnotationChanged( const QString& entry, const QString& attribute, const QString& value )
{
  Q_UNUSED( attribute );
  Q_UNUSED( value );
  //kdDebug(5006) << k_funcinfo << entry << " " << attribute << " " << value << endl;
  if ( entry == KOLAB_FOLDERTYPE )
    mAnnotationFolderTypeChanged = false;
  else if ( entry == KOLAB_INCIDENCESFOR ) {
    mIncidencesForChanged = false;
    // The incidences-for changed, we must trigger the freebusy creation.
    // HACK: in theory we would need a new enum value for this.
    kmkernel->iCalIface().addFolderChange( folder(), KMailICalIfaceImpl::ACL );
  }
}

void KMFolderCachedImap::slotTestAnnotationResult(KIO::Job *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  Q_ASSERT( it != mAccount->jobsEnd() );
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  Q_ASSERT( (*it).parent == folder() );
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  mAccount->setAnnotationCheckPassed( true );
  if ( job->error() ) {
    kdDebug(5006) << "Test Annotation was not passed, disabling annotation support" << endl;
    mAccount->setHasNoAnnotationSupport( );
  } else {
    kdDebug(5006) << "Test Annotation was passed   OK" << endl;
  }
  if (mAccount->slave()) mAccount->removeJob(job);
  serverSyncInternal();
}

void
KMFolderCachedImap::slotSetAnnotationResult(KIO::Job *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  bool cont = true;
  if ( job->error() ) {
    // Don't show error if the server doesn't support ANNOTATEMORE and this folder only contains mail
    if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION && contentsType() == ContentsTypeMail )
      if (mAccount->slave()) mAccount->removeJob(job);
    else
      cont = mAccount->handleJobError( job, i18n( "Error while setting annotation: " ) + '\n' );
  } else {
    if (mAccount->slave()) mAccount->removeJob(job);
  }
  if ( cont )
    serverSyncInternal();
}

void KMFolderCachedImap::slotUpdateLastUid()
{
  if( mTentativeHighestUid != 0 ) {

      // Sanity checking:
      // By now all new mails should be downloaded, which means
      // that iteration over the folder should yield only UIDs
      // lower or equal to what we think the highes ist, and the
      // highest one as well. If not, our notion of the highest
      // uid we've seen thus far is wrong, which is dangerous, so
      // don't update the mLastUid, then.
      bool sane = false;

      for (int i=0;i<count(); i++ ) {
          ulong uid = getMsgBase(i)->UID();
          if ( uid > mTentativeHighestUid && uid > lastUid() ) {
              kdWarning(5006) << "DANGER: Either the server listed a wrong highest uid, "
                  "or we parsed it wrong. Send email to adam@kde.org, please, and include this log." << endl;
              kdWarning(5006) << "uid: " << uid << " mTentativeHighestUid: " << mTentativeHighestUid << endl;
              assert( false );
              break;
          } else if ( uid == mTentativeHighestUid || lastUid() ) {
              // we've found our highest uid, all is well
              sane = true;
          } else {
              // must be smaller, that's ok, let's wait for bigger fish
          }
      }
      if (sane) {
#if MAIL_LOSS_DEBUGGING
          kdDebug(5006) << "Tentative highest UID test was sane, writing out: " << mTentativeHighestUid << endl;
#endif
          setLastUid( mTentativeHighestUid );
      }
  }
  mTentativeHighestUid = 0;
}

bool KMFolderCachedImap::isMoveable() const
{
  return ( hasChildren() == HasNoChildren &&
      !folder()->isSystemFolder() ) ? true : false;
}

void KMFolderCachedImap::slotFolderDeletionOnServerFinished()
{
  for ( QStringList::const_iterator it = foldersForDeletionOnServer.constBegin();
      it != foldersForDeletionOnServer.constEnd(); ++it ) {
    KURL url( mAccount->getUrl() );
    url.setPath( *it );
    kmkernel->iCalIface().folderDeletedOnServer( url );
  }
  serverSyncInternal();
}

int KMFolderCachedImap::createIndexFromContentsRecursive()
{
  if ( !folder() || !folder()->child() )
    return 0;

  KMFolderNode *node = 0;
  for( QPtrListIterator<KMFolderNode> it( *folder()->child() ); (node = it.current()); ++it ) {
    if( !node->isDir() ) {
      KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
      kdDebug() << k_funcinfo << "Re-indexing: " << storage->folder()->label() << endl;
      int rv = storage->createIndexFromContentsRecursive();
      if ( rv > 0 )
        return rv;
    }
  }

  return createIndexFromContents();
}

void KMFolderCachedImap::setAlarmsBlocked( bool blocked )
{
  mAlarmsBlocked = blocked;
}

bool KMFolderCachedImap::alarmsBlocked() const
{
  return mAlarmsBlocked;
}

bool KMFolderCachedImap::isCloseToQuota() const
{
  bool closeToQuota = false;
  if ( mQuotaInfo.isValid() && mQuotaInfo.max().toInt() > 0 ) {
    const int ratio = mQuotaInfo.current().toInt() * 100  / mQuotaInfo.max().toInt();
    //kdDebug(5006) << "Quota ratio: " << ratio << "% " << mQuotaInfo.toString() << endl;
    closeToQuota = ( ratio > 0 && ratio >= GlobalSettings::closeToQuotaThreshold() );
  }
  //kdDebug(5006) << "Folder: " << folder()->prettyURL() << " is over quota: " << closeToQuota << endl;
  return closeToQuota;
}

KMCommand* KMFolderCachedImap::rescueUnsyncedMessages()
{
  QValueList<unsigned long> newMsgs = findNewMessages();
  kdDebug() << k_funcinfo << newMsgs << " of " << count() << endl;
  if ( newMsgs.isEmpty() )
    return 0;
  KMFolder *dest = 0;
  bool manualMove = true;
  while ( GlobalSettings::autoLostFoundMove() ) {
    // find the inbox of this account
    KMFolder *inboxFolder = kmkernel->findFolderById( QString(".%1.directory/INBOX").arg( account()->id() ) );
    if ( !inboxFolder ) {
      kdWarning(5006) << k_funcinfo << "inbox not found!" << endl;
      break;
    }
    KMFolderDir *inboxDir = inboxFolder->child();
    if ( !inboxDir && !inboxFolder->storage() )
      break;
    assert( inboxFolder->storage()->folderType() == KMFolderTypeCachedImap );

    // create lost+found folder if needed
    KMFolderNode *node;
    KMFolder *lfFolder = 0;
    if ( !(node = inboxDir->hasNamedFolder( i18n("lost+found") )) ) {
      kdDebug(5006) << k_funcinfo << "creating lost+found folder" << endl;
      KMFolder* folder = kmkernel->dimapFolderMgr()->createFolder(
          i18n("lost+found"), false, KMFolderTypeCachedImap, inboxDir );
      if ( !folder || !folder->storage() )
        break;
      static_cast<KMFolderCachedImap*>( folder->storage() )->initializeFrom(
        static_cast<KMFolderCachedImap*>( inboxFolder->storage() ) );
      folder->storage()->setContentsType( KMail::ContentsTypeMail );
      folder->storage()->writeConfig();
      lfFolder = folder;
    } else {
      kdDebug(5006) << k_funcinfo << "found lost+found folder" << endl;
      lfFolder = dynamic_cast<KMFolder*>( node );
    }
    if ( !lfFolder || !lfFolder->createChildFolder() || !lfFolder->storage() )
      break;

    // create subfolder for this incident
    QDate today = QDate::currentDate();
    QString baseName = folder()->label() + "-" + QString::number( today.year() )
        + (today.month() < 10 ? "0" : "" ) + QString::number( today.month() )
        + (today.day() < 10 ? "0" : "" ) + QString::number( today.day() );
    QString name = baseName;
    int suffix = 0;
    while ( (node = lfFolder->child()->hasNamedFolder( name )) ) {
      ++suffix;
      name = baseName + '-' + QString::number( suffix );
    }
    kdDebug(5006) << k_funcinfo << "creating lost+found folder " << name << endl;
    dest = kmkernel->dimapFolderMgr()->createFolder( name, false, KMFolderTypeCachedImap, lfFolder->child() );
    if ( !dest || !dest->storage() )
        break;
    static_cast<KMFolderCachedImap*>( dest->storage() )->initializeFrom(
      static_cast<KMFolderCachedImap*>( lfFolder->storage() ) );
    dest->storage()->setContentsType( contentsType() );
    dest->storage()->writeConfig();

    KMessageBox::sorry( 0, i18n("<p>There are new messages in folder <b>%1</b>, which "
          "have not been uploaded to the server yet, but the folder has been deleted "
          "on the server or you do not "
          "have sufficient access rights on the folder to upload them.</p>"
          "<p>All affected messages will therefore be moved to <b>%2</b> "
          "to avoid data loss.</p>").arg( folder()->prettyURL() ).arg( dest->prettyURL() ),
          i18n("Insufficient access rights") );
    manualMove = false;
    break;
  }

  if ( manualMove ) {
    const QString msg ( i18n( "<p>There are new messages in this folder (%1), which "
          "have not been uploaded to the server yet, but the folder has been deleted "
          "on the server or you do not "
          "have sufficient access rights on the folder now to upload them. "
          "Please contact your administrator to allow upload of new messages "
          "to you, or move them out of this folder.</p> "
          "<p>Do you want to move these messages to another folder now?</p>").arg( folder()->prettyURL() ) );
    if ( KMessageBox::warningYesNo( 0, msg, QString::null, i18n("Move"), i18n("Do Not Move") ) == KMessageBox::Yes ) {
      KMail::KMFolderSelDlg dlg( kmkernel->getKMMainWidget(),
          i18n("Move Messages to Folder"), true );
      if ( dlg.exec() ) {
        dest = dlg.folder();
      }
    }
  }
  if ( dest ) {
    QPtrList<KMMsgBase> msgs;
    for( int i = 0; i < count(); ++i ) {
      KMMsgBase *msg = getMsgBase( i );
      if( !msg ) continue; /* what goes on if getMsg() returns 0? */
      if ( msg->UID() == 0 )
        msgs.append( msg );
    }
    KMCommand *command = new KMMoveCommand( dest, msgs );
    command->start();
    return command;
  }
  return 0;
}

void KMFolderCachedImap::rescueUnsyncedMessagesAndDeleteFolder( KMFolder *folder, bool root )
{
  kdDebug() << k_funcinfo << folder << " " << root << endl;
  if ( root )
    mToBeDeletedAfterRescue.append( folder );
  folder->open("cachedimap");
  KMFolderCachedImap* storage = dynamic_cast<KMFolderCachedImap*>( folder->storage() );
  if ( storage ) {
    KMCommand *command = storage->rescueUnsyncedMessages();
    if ( command ) {
      connect( command, SIGNAL(completed(KMCommand*)),
               SLOT(slotRescueDone(KMCommand*)) );
      ++mRescueCommandCount;
    } else {
      // nothing to rescue, close folder
      // (we don't need to close it in the other case, it will be deleted anyway)
      folder->close("cachedimap");
    }
  }
  if ( folder->child() ) {
    KMFolderNode *node = folder->child()->first();
    while (node) {
      if (!node->isDir() ) {
        KMFolder *subFolder = static_cast<KMFolder*>( node );
        rescueUnsyncedMessagesAndDeleteFolder( subFolder, false );
      }
      node = folder->child()->next();
    }
  }
}

void KMFolderCachedImap::slotRescueDone(KMCommand * command)
{
  // FIXME: check command result
  if ( command )
    --mRescueCommandCount;
  if ( mRescueCommandCount > 0 )
    return;
  for ( QValueList<KMFolder*>::ConstIterator it = mToBeDeletedAfterRescue.constBegin();
        it != mToBeDeletedAfterRescue.constEnd(); ++it ) {
    kmkernel->dimapFolderMgr()->remove( *it );
  }
  mToBeDeletedAfterRescue.clear();
  serverSyncInternal();
}

#include "kmfoldercachedimap.moc"
