/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmailicalifaceimpl.h"
#include "kmfoldertree.h"
#include "kmfolderdir.h"
#include "kmgroupware.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "kmfolderindex.h"
#include "kmmsgdict.h"
#include "kmfolderimap.h"
#include "globalsettings.h"
#include "kmacctmgr.h"

#include <mimelib/enum.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <qmap.h>

// Local helper methods
static void vPartMicroParser( const QString& str, QString& s );
static void reloadFolderTree();

// The index in this array is the KMail::FolderContentsType enum
static const struct {
  const char* contentsTypeStr; // the string used in the DCOP interface
} s_folderContentsType[] = {
  { "Mail" },
  { "Calendar" },
  { "Contact" },
  { "Note" },
  { "Task" },
  { "Journal" }
};

static QString folderContentsType( KMail::FolderContentsType type )
{
  return s_folderContentsType[type].contentsTypeStr;
}

static KMail::FolderContentsType folderContentsType( const QString& type )
{
  for ( uint i = 0 ; i < sizeof s_folderContentsType / sizeof *s_folderContentsType; ++i )
    if ( type == s_folderContentsType[i].contentsTypeStr )
      return static_cast<KMail::FolderContentsType>( i );
  return KMail::ContentsTypeMail;
}

/*
  This interface have three parts to it - libkcal interface;
  kmail interface; and helper functions.

  The libkcal interface and the kmail interface have the same three
  methods: add, delete and refresh. The only difference is that the
  libkcal interface is used from the IMAP resource in libkcal and
  the kmail interface is used from the groupware object in kmail.
*/

KMailICalIfaceImpl::KMailICalIfaceImpl()
  : DCOPObject( "KMailICalIface" ), QObject( 0, "KMailICalIfaceImpl" ),
    mContacts( 0 ), mCalendar( 0 ), mNotes( 0 ), mTasks( 0 ), mJournals( 0 ),
    mFolderLanguage( 0 ), mUseResourceIMAP( false ), mHideFolders( true )
{
  // Listen to config changes
  connect( kmkernel, SIGNAL( configChanged() ), this, SLOT( readConfig() ) );

  mExtraFolders.setAutoDelete( true );
  mAccumulators.setAutoDelete( true );
}

// Receive an iCal or vCard from the resource
bool KMailICalIfaceImpl::addIncidence( const QString& type,
                                       const QString& folder,
                                       const QString& uid,
                                       const QString& ical )
{
  kdDebug(5006) << "KMailICalIfaceImpl::addIncidence( " << type << ", "
            << uid << ", " << ical << " )" << endl;

  if( !mUseResourceIMAP )
    return false;

  bool rc = false;

  if ( !mInTransit.contains( uid ) ) {
    mInTransit.insert( uid, true );
  }

  // Find the folder
  KMFolder* f = folderFromType( type, folder );
  if( f ) {
    // Make a new message for the incidence
    KMMessage* msg = new KMMessage();
    msg->initHeader();
    msg->setType( DwMime::kTypeText );
    if( f == mContacts ) {
      msg->setSubtype( DwMime::kSubtypeXVCard );
      msg->setHeaderField( "Content-Type", "Text/X-VCard; charset=\"utf-8\"" );
      msg->setSubject( "vCard " + uid );
    } else {
      msg->setSubtype( DwMime::kSubtypeVCal );
      msg->setHeaderField("Content-Type",
                          "text/calendar; method=REQUEST; charset=\"utf-8\"");
      msg->setSubject( "iCal " + uid );
    }
    msg->setBodyEncoded( ical.utf8() );

    // Mark the message as read and store it in the folder
    msg->touch();
    f->addMsg( msg );
    rc = true;
  } else
    kdError(5006) << "Not an IMAP resource folder" << endl;

  return rc;
}

// The resource orders a deletion
bool KMailICalIfaceImpl::deleteIncidence( const QString& type,
                                          const QString& folder,
                                          const QString& uid )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "KMailICalIfaceImpl::deleteIncidence( " << type << ", "
            << uid << " )" << endl;

  bool rc = false;

  // Find the folder and the incidence in it
  KMFolder* f = folderFromType( type, folder );
  if( f ) {
    KMMessage* msg = findMessageByUID( uid, f );
    if( msg ) {
      // Message found - delete it and return happy
      deleteMsg( msg );
      rc = true;
      mUIDToSerNum.remove( uid );
    } else
      kdDebug(5006) << type << " not found, cannot remove uid " << uid << endl;
  } else
    kdError(5006) << "Not an IMAP resource folder" << endl;

  return rc;
}

// The resource asks for a full list of incidences
QStringList KMailICalIfaceImpl::incidences( const QString& type,
                                            const QString& folder )
{
  if( !mUseResourceIMAP )
    return QStringList();

  kdDebug(5006) << "KMailICalIfaceImpl::incidences( " << type << ", "
                << folder << " )" << endl;
  QStringList ilist;

  KMFolder* f = folderFromType( type, folder );
  if ( f ) {
    f->open();
    QString s;
    for( int i=0; i<f->count(); ++i ) {
      bool unget = !f->isMessage(i);
      KMMessage *msg = f->getMsg( i );
      Q_ASSERT( msg );
      if( msg->isComplete() ) {
        if( vPartFoundAndDecoded( msg, s ) ) {
          QString uid( "UID" );
          vPartMicroParser( s, uid );
          const Q_UINT32 sernum = msg->getMsgSerNum();
          kdDebug(5006) << "Insert uid: " << uid << endl;
          mUIDToSerNum.insert( uid, sernum );
          ilist << s;
        }
        if( unget ) f->unGetMsg(i);
      } else {
        // message needs to be gotten first, once it arrives, we'll
        // accumulate it and add it to the resource
        if ( !mAccumulators[ folder ] )
          mAccumulators.insert( folder, new Accumulator( type, folder, f->count() ));
        if ( unget ) mTheUnGetMes.insert( msg->getMsgSerNum(), true );
        FolderJob *job = msg->parent()->createJob( msg );
        connect( job, SIGNAL( messageRetrieved( KMMessage* ) ),
                 this, SLOT( slotMessageRetrieved( KMMessage* ) ) );
        job->start();
      }
    }
  }
  return ilist;
}

void KMailICalIfaceImpl::slotMessageRetrieved( KMMessage* msg )
{
  if( !msg ) return;

  KMFolder *parent = msg->parent();
  Q_ASSERT( parent );
  Q_UINT32 sernum = msg->getMsgSerNum();

  // do we have an accumulator for this folder?
  Accumulator *ac = mAccumulators.find( parent->location() );
  if( ac ) {
    QString s;
    if ( !vPartFoundAndDecoded( msg, s ) ) return;
    QString uid( "UID" );
    vPartMicroParser( s, uid );
    const Q_UINT32 sernum = msg->getMsgSerNum();
    mUIDToSerNum.insert( uid, sernum );
    ac->add( s );
    if( ac->isFull() ) {
      /* if this was the last one we were waiting for, tell the resource
       * about the new incidences and clean up. */
      asyncLoadResult( ac->incidences, ac->type, ac->folder );
      mAccumulators.remove( ac->folder ); // autodelete
    }
  } else {
    /* We are not accumulating for this folder, so this one was added
     * by KMail. Do your thang. */
     slotIncidenceAdded( msg->parent(), msg->getMsgSerNum() );
  }

  if ( mTheUnGetMes.contains( sernum ) ) {
    mTheUnGetMes.remove( sernum );
    int i = 0;
    KMFolder* folder = 0;
    kmkernel->msgDict()->getLocation( sernum, &folder, &i );
    folder->unGetMsg( i );
  }
}

QStringList KMailICalIfaceImpl::subresources( const QString& type )
{
  QStringList lst;

  // Add the default one
  KMFolder* f = folderFromType( type, QString::null );
  if ( f )
    lst << f->location();

  // Add the extra folders
  KMail::FolderContentsType t = folderContentsType( type );
  QDictIterator<ExtraFolder> it( mExtraFolders );
  for ( ; it.current(); ++it )
    if ( it.current()->type == t )
      lst << it.current()->folder->location();

  return lst;
}

bool KMailICalIfaceImpl::isWritableFolder( const QString& type,
                                           const QString& resource )
{
  KMFolder* f = folderFromType( type, resource );
  if ( !f )
    // Definitely not writable
    return false;

  return !f->isReadOnly();
}

bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QStringList& entries )
{
  if( !mUseResourceIMAP )
    return false;

  if( entries.count() % 2 == 1 )
    // Something is wrong - an odd amount of strings should not happen
    return false;

  QStringList::ConstIterator it = entries.begin();
  while( true ) {
    // Read them in pairs and call the single update method
    QString uid, entry;
    if( it == entries.end() )
      break;
    uid = *it;
    ++it;
    if( it == entries.end() )
      break;
    entry = *it;
    ++it;

    if( !update( type, folder, uid, entry ) )
      // Some error happened
      return false;
  }

  return true;
}

bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QString& uid, const QString& entry )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "Update( " << type << ", " << folder << ", " << uid << ")\n";
  bool rc = true;

  if ( !mInTransit.contains( uid ) ) {
    mInTransit.insert( uid, true );
  } else {
    // this is reentrant, if a new update comes in, we'll just
    // replace older ones
    mPendingUpdates.insert( uid, entry );
    return rc;
  }

  // Find the folder and the incidence in it
  KMFolder* f = folderFromType( type, folder );
  if( f ) {
    KMMessage* msg = findMessageByUID( uid, f );
    if( msg ) {
      // Message found - update it
      deleteMsg( msg );
      mUIDToSerNum.remove( uid );
    } else {
      kdDebug(5006) << type << " not found, cannot update uid " << uid << endl;
    }
    addIncidence( type, folder, uid, entry );
  } else {
    kdError(5006) << "Not an IMAP resource folder" << endl;
    rc = false;
  }
  return rc;
}

// KMail added a file to one of the groupware folders
void KMailICalIfaceImpl::slotIncidenceAdded( KMFolder* folder,
                                             Q_UINT32 sernum )
{
  if( !mUseResourceIMAP )
    return;

  QString type = icalFolderType( folder );
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    // Read the iCal or vCard
    bool unget = !folder->isMessage( i );
    QString s;
    KMMessage *msg = folder->getMsg( i );
    if( !msg ) return;
    if( msg->isComplete() ) {
      if ( !vPartFoundAndDecoded( msg, s ) ) return;
      kdDebug(5006) << "Emitting DCOP signal incidenceAdded( " << type
                    << ", " << folder->location() << ", " << s << " )" << endl;
      QString uid( "UID" );
      vPartMicroParser( s, uid );
      const Q_UINT32 sernum = msg->getMsgSerNum();
      kdDebug(5006) << "Insert uid: " << uid << endl;
      mUIDToSerNum.insert( uid, sernum );
      // tell the resource if we didn't trigger this ourselves
      if( !mInTransit.contains( uid ) )
        incidenceAdded( type, folder->location(), s );
      else
        mInTransit.remove( uid );

      // Check if new updates have since arrived, if so, trigger them
      if ( mPendingUpdates.contains( uid ) ) {
        kdDebug(5006) << "KMailICalIfaceImpl::slotIncidenceAdded - Pending Update" << endl;
        QString entry = mPendingUpdates[ uid ];
        mPendingUpdates.remove( uid );
        update( type, folder->location(), uid, entry );
      }
    } else {
      // go get the rest of it, then try again
      if ( unget ) mTheUnGetMes.insert( msg->getMsgSerNum(), true );
      FolderJob *job = msg->parent()->createJob( msg );
      connect( job, SIGNAL( messageRetrieved( KMMessage* ) ),
               this, SLOT( slotMessageRetrieved( KMMessage* ) ) );
      job->start();
      return;
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError(5006) << "Not an IMAP resource folder" << endl;
}

// KMail deleted a file
void KMailICalIfaceImpl::slotIncidenceDeleted( KMFolder* folder,
                                               Q_UINT32 sernum )
{
  if( !mUseResourceIMAP )
    return;

  QString type = icalFolderType( folder );
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    // Read the iCal or vCard
    bool unget = !folder->isMessage( i );
    QString s;
    if( vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
      QString uid( "UID" );
      vPartMicroParser( s, uid );
      kdDebug(5006) << "Emitting DCOP signal incidenceDeleted( "
                    << type << ", " << folder->location() << ", " << uid
                    << " )" << endl;
      if( !mInTransit.contains( uid ) ) // we didn't delete it ourselves
        incidenceDeleted( type, folder->location(), uid );
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError(5006) << "Not a groupware folder" << endl;
}

// KMail orders a refresh
void KMailICalIfaceImpl::slotRefresh( const QString& type )
{
  if( mUseResourceIMAP ) {
    signalRefresh( type, QString::null /* PENDING(bo) folder->location() */ );
    kdDebug(5006) << "Emitting DCOP signal signalRefresh( " << type << " )" << endl;
  }
}


/****************************
 * The folder and message stuff code
 */

KMFolder* KMailICalIfaceImpl::folderFromType( const QString& type,
                                              const QString& folder )
{
  if( mUseResourceIMAP ) {
    KMFolder* f = extraFolder( type, folder );
    if ( f )
      return f;

    if( type == "Calendar" ) f = mCalendar;
    else if( type == "Contact" ) f = mContacts;
    else if( type == "Note" ) f = mNotes;
    else if( type == "Task" || type == "Todo" ) f = mTasks;
    else if( type == "Journal" ) f = mJournals;

    if ( f && ( folder.isEmpty() || folder == f->location() ) )
      return f;

    kdError(5006) << "No folder ( " << type << ", " << folder << " )\n";
  }

  return 0;
}


// Returns true if folder is a resource folder. If the resource isn't enabled
// this always returns false
bool KMailICalIfaceImpl::isResourceImapFolder( KMFolder* folder ) const
{
  return mUseResourceIMAP && folder &&
    ( folder == mCalendar || folder == mTasks || folder == mJournals ||
      folder == mNotes || folder == mContacts );
}

bool KMailICalIfaceImpl::hideResourceImapFolder( KMFolder* folder ) const
{
  return mHideFolders && isResourceImapFolder( folder );
}

KFolderTreeItem::Type KMailICalIfaceImpl::folderType( KMFolder* folder ) const
{
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar )
      return KFolderTreeItem::Calendar;
    else if( folder == mContacts )
      return KFolderTreeItem::Contacts;
    else if( folder == mNotes )
      return KFolderTreeItem::Notes;
    else if( folder == mTasks )
      return KFolderTreeItem::Tasks;
    else if( folder == mJournals )
      return KFolderTreeItem::Journals;
  }

  return KFolderTreeItem::Other;
}


QString KMailICalIfaceImpl::icalFolderType( KMFolder* folder ) const
{
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar )
      return "Calendar";
    else if( folder == mContacts )
      return "Contact";
    else if( folder == mNotes )
      return "Note";
    else if( folder == mTasks )
      return "Task";
    else if( folder == mJournals )
      return "Journal";
    else {
      ExtraFolder* ef = mExtraFolders.find( folder->location() );
      if ( ef != 0 )
        return folderContentsType( ef->type );
    }
  }

  return QString::null;
}


// Global tables of foldernames is different languages
// For now: 0->English, 1->German, 2->French, 3->Dutch
static QMap<KFolderTreeItem::Type,QString> folderNames[4];
QString KMailICalIfaceImpl::folderName( KFolderTreeItem::Type type, int language ) const
{
  static bool folderNamesSet = false;
  if( !folderNamesSet ) {
    folderNamesSet = true;
    /* NOTE: If you add something here, you also need to update
       GroupwarePage in configuredialog.cpp */

    // English
    folderNames[0][KFolderTreeItem::Calendar] = QString::fromLatin1("Calendar");
    folderNames[0][KFolderTreeItem::Tasks] = QString::fromLatin1("Tasks");
    folderNames[0][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[0][KFolderTreeItem::Contacts] = QString::fromLatin1("Contacts");
    folderNames[0][KFolderTreeItem::Notes] = QString::fromLatin1("Notes");

    // German
    folderNames[1][KFolderTreeItem::Calendar] = QString::fromLatin1("Kalender");
    folderNames[1][KFolderTreeItem::Tasks] = QString::fromLatin1("Aufgaben");
    folderNames[1][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[1][KFolderTreeItem::Contacts] = QString::fromLatin1("Kontakte");
    folderNames[1][KFolderTreeItem::Notes] = QString::fromLatin1("Notizen");

    // French
    folderNames[2][KFolderTreeItem::Calendar] = QString::fromLatin1("Calendrier");
    folderNames[2][KFolderTreeItem::Tasks] = QString::fromLatin1("Tâches");
    folderNames[2][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[2][KFolderTreeItem::Contacts] = QString::fromLatin1("Contacts");
    folderNames[2][KFolderTreeItem::Notes] = QString::fromLatin1("Notes");

    // Dutch
    folderNames[3][KFolderTreeItem::Calendar] = QString::fromLatin1("Agenda");
    folderNames[3][KFolderTreeItem::Tasks] = QString::fromLatin1("Taken");
    folderNames[3][KFolderTreeItem::Journals] = QString::fromLatin1("Logboek");
    folderNames[3][KFolderTreeItem::Contacts] = QString::fromLatin1("Contactpersonen");
    folderNames[3][KFolderTreeItem::Notes] = QString::fromLatin1("Notities");
  }

  if( language < 0 || language > 3 ) {
    return folderNames[mFolderLanguage][type];
  }
  else {
    return folderNames[language][type];
  }
}


// Find message matching a given UID
KMMessage *KMailICalIfaceImpl::findMessageByUID( const QString& uid, KMFolder* folder )
{
  if( !folder || !mUIDToSerNum.contains( uid ) ) return 0;
  int i;
  KMFolder *aFolder;
  kmkernel->msgDict()->getLocation( mUIDToSerNum[uid], &aFolder, &i );
  Q_ASSERT( aFolder == folder );
  return folder->getMsg( i );
}

void KMailICalIfaceImpl::deleteMsg( KMMessage *msg )
{
  if( !msg ) return;
  // Commands are now delayed; can't use that anymore, we need immediate deletion
  //( new KMDeleteMsgCommand( msg->parent(), msg ) )->start();
  KMFolder *srcFolder = msg->parent();
  int idx = srcFolder->find(msg);
  assert(idx != -1);
  srcFolder->removeMsg(idx);
  delete msg;
}

void KMailICalIfaceImpl::folderContentsTypeChanged( KMFolder* folder,
                                                    KMail::FolderContentsType contentsType )
{
  kdDebug(5006) << "folderContentsTypeChanged( " << folder->name()
                << ", " << contentsType << ")\n";

  // Find previous type of this folder
  ExtraFolder* ef = mExtraFolders.find( folder->location() );
  if ( ( ef && ef->type == contentsType ) || ( !ef && contentsType == 0 ) )
    // Nothing to tell!
    return;

  if ( ef ) {
    // Notify that the old folder resource is no longer available
    subresourceDeleted(folderContentsType( ef->type ), folder->location() );

    if ( contentsType == 0 ) {
      // Delete the old entry, stop listening and stop here
      mExtraFolders.remove( folder->location() );
      folder->disconnect( this );
      return;
    }

    // So the type changed to another groupware type.
    // Set the entry to the new type
    ef->type = contentsType;
  } else {
    // Make a new entry for the list
    ef = new ExtraFolder( folder, contentsType );
    mExtraFolders.insert( folder->location(), ef );

    // avoid multiple connections
    disconnect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
                this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    disconnect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
                this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );

    // And listen to changes from it
    connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  }

  // Tell about the new resource
  subresourceAdded( folderContentsType( contentsType ), folder->location() );
}

KMFolder* KMailICalIfaceImpl::extraFolder( const QString& type,
                                           const QString& folder )
{
  // If an extra folder exists that match the type and folder location,
  // use that
  int t = folderContentsType( type );
  if ( t < 1 || t > 5 )
    return 0;

  QDictIterator<ExtraFolder> it( mExtraFolders );
  for ( ; it.current(); ++it )
    if ( it.current()->type == t &&
         it.current()->folder->location() == folder )
      return it.current()->folder;

  return 0;
}


/****************************
 * The config stuff
 */

void KMailICalIfaceImpl::readConfig()
{
  bool enabled = GlobalSettings::theIMAPResourceEnabled();

  if( !enabled ) {
    if( mUseResourceIMAP == true ) {
      // Shutting down
      mUseResourceIMAP = false;
      cleanup();
      reloadFolderTree();
    }
    return;
  }

  // Read remaining options
  const bool hideFolders = GlobalSettings::hideGroupwareFolders();
  unsigned int folderLanguage = GlobalSettings::theIMAPResourceFolderLanguage();
  if( folderLanguage > 3 ) folderLanguage = 0;
  QString parentName = GlobalSettings::theIMAPResourceFolderParent();

  // Find the folder parent
  KMFolderDir* folderParentDir;
  KMFolderType folderType;
  KMFolder* folderParent = kmkernel->findFolderById( parentName );
  if( folderParent == 0 ) {
    // Parent folder not found. It was probably deleted. The user will have to
    // configure things again.
    kdDebug(5006) << "Groupware folder " << parentName << " not found. Groupware functionality disabled" << endl;
    // Or maybe the inbox simply wasn't created on the first startup
    KMAccount* account = kmkernel->acctMgr()->find( GlobalSettings::theIMAPResourceAccount() );
    Q_ASSERT( account );
    if ( account ) {
      // just in case we were connected already
      disconnect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
               this, SLOT( slotCheckDone() ) );
      connect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
               this, SLOT( slotCheckDone() ) );
    }
    mUseResourceIMAP = false;
    return;
  } else {
    folderParentDir = folderParent->createChildFolder();
    folderType = folderParent->folderType();
  }

  // Make sure the folder parent has the subdirs
  bool makeSubFolders = false;
  KMFolderNode* node;
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Calendar, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mCalendar = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Tasks, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mTasks = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Journals, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mJournals = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Contacts, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mContacts = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Notes, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mNotes = 0;
  }
  if( makeSubFolders ) {
    // Not all subfolders were there, so ask if we can make them
    if( KMessageBox::questionYesNo( 0, i18n("KMail will now create the required folders for the IMAP resource"
                                            " as subfolders of %1; if you do not want this, press \"No\","
                                            " and the IMAP resource will be disabled").arg(folderParent!=0?folderParent->name():folderParentDir->name()),
                                    i18n("IMAP Resource Folders") ) == KMessageBox::No ) {

      GlobalSettings::setTheIMAPResourceEnabled( false );
      mUseResourceIMAP = false;
      mFolderParentDir = 0;
      mFolderParent = 0;
      reloadFolderTree();
      return;
    }
  }

  // Check if something changed
  if( mUseResourceIMAP && !makeSubFolders && mFolderParentDir == folderParentDir
      && mFolderType == folderType ) {
    // Nothing changed
    if ( hideFolders != mHideFolders ) {
      // Well, the folder hiding has changed
      mHideFolders = hideFolders;
      reloadFolderTree();
    }
    return;
  }

  // Make the new settings work
  mUseResourceIMAP = true;
  mFolderLanguage = folderLanguage;
  mFolderParentDir = folderParentDir;
  mFolderParent = folderParent;
  mFolderType = folderType;
  mHideFolders = hideFolders;

  // Close the previous folders
  cleanup();

  // Set the new folders
  mCalendar = initFolder( KFolderTreeItem::Calendar, "GCa" );
  mTasks    = initFolder( KFolderTreeItem::Tasks, "GTa" );
  mJournals = initFolder( KFolderTreeItem::Journals, "GTa" );
  mContacts = initFolder( KFolderTreeItem::Contacts, "GCo" );
  mNotes    = initFolder( KFolderTreeItem::Notes, "GNo" );

  // Connect the expunged signal
  connect( mCalendar, SIGNAL( expunged( KMFolder* ) ), this, SLOT( slotRefreshCalendar() ) );
  connect( mTasks,    SIGNAL( expunged( KMFolder* ) ), this, SLOT( slotRefreshTasks() ) );
  connect( mJournals, SIGNAL( expunged( KMFolder* ) ), this, SLOT( slotRefreshJournals() ) );
  connect( mContacts, SIGNAL( expunged( KMFolder* ) ), this, SLOT( slotRefreshContacts() ) );
  connect( mNotes,    SIGNAL( expunged( KMFolder* ) ), this, SLOT( slotRefreshNotes() ) );

  // Bad hack
  connect( mNotes,    SIGNAL( changed() ),  this, SLOT( slotRefreshNotes() ) );

  // Make KOrganizer re-read everything
  slotRefresh( "Calendar" );
  slotRefresh( "Task" );
  slotRefresh( "Journal" );
  slotRefresh( "Contact" );
  slotRefresh( "Notes" );

  reloadFolderTree();
}

void KMailICalIfaceImpl::slotCheckDone()
{
  QString parentName = GlobalSettings::theIMAPResourceFolderParent();
  KMFolder* folderParent = kmkernel->findFolderById( parentName );
  if ( folderParent )  // cool it exists now
  {
    KMAccount* account = kmkernel->acctMgr()->find( GlobalSettings::theIMAPResourceAccount() );
    if ( account )
      disconnect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                  this, SLOT( slotCheckDone() ) );
    readConfig();
  }
}

void KMailICalIfaceImpl::slotRefreshCalendar() { slotRefresh( "Calendar" ); }
void KMailICalIfaceImpl::slotRefreshTasks() { slotRefresh( "Task" ); }
void KMailICalIfaceImpl::slotRefreshJournals() { slotRefresh( "Journal" ); }
void KMailICalIfaceImpl::slotRefreshContacts() { slotRefresh( "Contact" ); }
void KMailICalIfaceImpl::slotRefreshNotes() { slotRefresh( "Notes" ); }

KMFolder* KMailICalIfaceImpl::initFolder( KFolderTreeItem::Type itemType,
                                          const char* typeString )
{
  // Figure out what type of folder this is supposed to be
  KMFolderType type = mFolderType;
  if( type == KMFolderTypeUnknown ) type = KMFolderTypeMaildir;

  // Find the folder
  KMFolder* folder = 0;
  KMFolderNode* node = mFolderParentDir->hasNamedFolder( folderName( itemType ) );
  if( node && !node->isDir() ) folder = static_cast<KMFolder*>(node);
  if( !folder ) {
    // The folder isn't there yet - create it
    folder =
      mFolderParentDir->createFolder( folderName( itemType ), false, type );
    if( mFolderType == KMFolderTypeImap ) {
      KMFolderImap* parentFolder = static_cast<KMFolderImap*>( mFolderParent->storage() );
      parentFolder->createFolder( folderName( itemType ) );
      static_cast<KMFolderImap*>( folder->storage() )->setAccount( parentFolder->account() );
    }
  }

  if( folder->canAccess() != 0 ) {
    KMessageBox::sorry(0, i18n("You do not have read/write permission to your %1 folder.")
                       .arg( folderName( itemType ) ) );
    return 0;
  }
  folder->setType( typeString );
  folder->setSystemFolder( true );
  folder->open();
  // avoid multiple connections
  disconnect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
              this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
  disconnect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
              this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  // Setup the signals to listen for changes
  connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
  connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );

  return folder;
}

static void cleanupFolder( KMFolder* folder, KMailICalIfaceImpl* _this )
{
  if( folder ) {
    folder->setType( "plain" );
    folder->setSystemFolder( false );
    folder->disconnect( _this );
    folder->close();
  }
}

void KMailICalIfaceImpl::cleanup()
{
  cleanupFolder( mContacts, this );
  cleanupFolder( mCalendar, this );
  cleanupFolder( mNotes, this );
  cleanupFolder( mTasks, this );
  cleanupFolder( mJournals, this );

  mContacts = mCalendar = mNotes = mTasks = mJournals = 0;
}

void KMailICalIfaceImpl::loadPixmaps() const
{
  static bool pixmapsLoaded = false;

  if( mUseResourceIMAP && !pixmapsLoaded ) {
    pixmapsLoaded = true;
    pixContacts = new QPixmap( UserIcon("kmgroupware_folder_contacts"));
    pixCalendar = new QPixmap( UserIcon("kmgroupware_folder_calendar"));
    pixNotes    = new QPixmap( UserIcon("kmgroupware_folder_notes"));
    pixTasks    = new QPixmap( UserIcon("kmgroupware_folder_tasks"));
    pixJournals = new QPixmap( UserIcon("kmgroupware_folder_journals"));
  }
}

QString KMailICalIfaceImpl::folderPixmap( KFolderTreeItem::Type type ) const
{
  if( !mUseResourceIMAP )
    return QString::null;

  if( type == KFolderTreeItem::Contacts )
    return QString::fromLatin1( "kmgroupware_folder_contacts" );
  else if( type == KFolderTreeItem::Calendar )
    return QString::fromLatin1( "kmgroupware_folder_calendar" );
  else if( type == KFolderTreeItem::Notes )
    return QString::fromLatin1( "kmgroupware_folder_notes" );
  else if( type == KFolderTreeItem::Tasks )
    return QString::fromLatin1( "kmgroupware_folder_tasks" );
  else if( type == KFolderTreeItem::Journals )
    return QString::fromLatin1( "kmgroupware_folder_journals" );

  return QString::null;
}

QPixmap* KMailICalIfaceImpl::pixContacts;
QPixmap* KMailICalIfaceImpl::pixCalendar;
QPixmap* KMailICalIfaceImpl::pixNotes;
QPixmap* KMailICalIfaceImpl::pixTasks;
QPixmap* KMailICalIfaceImpl::pixJournals;

static void reloadFolderTree()
{
  // Make the folder tree show the icons or not
  kmkernel->folderMgr()->contentsChanged();
}

// This is a very light-weight and fast 'parser' to retrieve
// a data entry from a vCal taking continuation lines
// into account
static void vPartMicroParser( const QString& str, QString& s )
{
  QString line;
  uint len = str.length();

  for( uint i=0; i<len; ++i){
    if( str[i] == '\r' || str[i] == '\n' ){
      if( str[i] == '\r' )
        ++i;
      if( i+1 < len && str[i+1] == ' ' ){
        // found a continuation line, skip it's leading blanc
        ++i;
      }else{
        // found a logical line end, process the line
        if( line.startsWith( s ) ) {
          s = line.mid( s.length() + 1 );
          return;
        }
        line = "";
      }
    } else {
      line += str[i];
    }
  }

  // Not found. Clear it
  s.truncate(0);
}


#include "kmailicalifaceimpl.moc"
