/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

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
    mFolderLanguage( 0 ), mUseResourceIMAP( false ), mResourceQuiet( false )
{
  // Listen to config changes
  connect( kmkernel, SIGNAL( configChanged() ), this, SLOT( readConfig() ) );
}

// Receive an iCal or vCard from the resource
bool KMailICalIfaceImpl::addIncidence( const QString& type, 
                                       const QString& uid, 
                                       const QString& ical )
{
  kdDebug() << "KMailICalIfaceImpl::addIncidence( " << type << ", "
            << uid << ", " << ical << " )" << endl;

  if( !mUseResourceIMAP )
    return false;

  bool rc = false;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  // Find the folder
  KMFolder* folder = folderFromType( type );
  if( folder ) {
    // Make a new message for the incidence
    KMMessage* msg = new KMMessage();
    msg->initHeader();
    msg->setType( DwMime::kTypeText );
    if( folder == mContacts ) {
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
    folder->addMsg( msg );  

    rc = true;
  } else
    kdError() << "Not an IMAP resource folder" << endl;

  mResourceQuiet = quiet;
  return false;
}

// The resource orders a deletion
bool KMailICalIfaceImpl::deleteIncidence( const QString& type,
                                          const QString& uid )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug() << "KMailICalIfaceImpl::deleteIncidence( " << type << ", "
            << uid << " )" << endl;

  bool rc = false;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  // Find the folder and the incidence in it
  KMFolder* folder = folderFromType( type );
  if( folder ) {
    KMMessage* msg = findMessageByUID( uid, folder );
    if( msg ) {
      // Message found - delete it and return happy
      deleteMsg( msg );
      rc = true;
    } else
      kdDebug(5006) << type << " not found, cannot remove uid " << uid << endl;
  } else
    kdError() << "Not an IMAP resource folder" << endl;

  mResourceQuiet = quiet;
  return true;
}

// The resource asks for a full list of incidences
QStringList KMailICalIfaceImpl::incidences( const QString& type )
{
  if( !mUseResourceIMAP )
    return QStringList();

  kdDebug() << "KMailICalIfaceImpl::incidences( " << type << " )" << endl;
  QStringList ilist;

  KMFolder* folder = folderFromType( type );
  if( folder ) {
    QString s;
    for( int i=0; i<folder->count(); ++i ) {
      bool unget = !folder->isMessage(i);
      if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
        if( folder == mContacts ) {
          kdDebug(5006) << "vCard for KAB: " << s << endl;
        }
        ilist << s;
      }
      if( unget ) folder->unGetMsg(i);
    }
  } else
    kdError() << "Not an IMAP resource folder" << endl;

  return ilist;
}

bool KMailICalIfaceImpl::update( const QString& type,
                                 const QStringList& entries )
{
  if( !mUseResourceIMAP )
    return false;

  if( entries.count() & 2 == 1 )
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

    if( !update( type, uid, entry ) )
      // Some error happened
      return false;
  }

  return true;
}

bool KMailICalIfaceImpl::update( const QString& type, const QString& uid,
                                 const QString& entry )
{
  if( !mUseResourceIMAP )
    return false;

  bool rc = true;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  // Find the folder and the incidence in it
  KMFolder* folder = folderFromType( type );
  if( folder ) {
    KMMessage* msg = findMessageByUID( uid, folder );
    if( msg ) {
      // Message found - update it
      deleteMsg( msg );
      addIncidence( type, uid, entry );
      rc = true;
    } else {
      kdDebug(5006) << type << " not found, cannot update uid " << uid << endl;
      // Since it doesn't seem to be there, save it instead
      addIncidence( type, uid, entry );
    }
  } else {
    kdError() << "Not an IMAP resource folder" << endl;
    rc = false;
  }

  mResourceQuiet = quiet;
  return rc;
}

// KMail added a file to one of the groupware folders
void KMailICalIfaceImpl::slotIncidenceAdded( KMFolder* folder,
                                             Q_UINT32 sernum )
{
  if( mResourceQuiet || !mUseResourceIMAP )
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
    if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
      QByteArray data;
      QDataStream arg(data, IO_WriteOnly );
      arg << type << s;
      kdDebug() << "Emitting DCOP signal incidenceAdded( " << type
                << ", " << s << " )" << endl;
      emitDCOPSignal( "incidenceAdded(QString,QString)", data );
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError() << "Not an IMAP resource folder" << endl;
}

// KMail deleted a file
void KMailICalIfaceImpl::slotIncidenceDeleted( KMFolder* folder,
                                               Q_UINT32 sernum )
{
  if( mResourceQuiet || !mUseResourceIMAP )
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
    if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
      QString uid( "UID" );
      vPartMicroParser( s, uid );
      QByteArray data;
      QDataStream arg(data, IO_WriteOnly );
      arg << type << uid;
      kdDebug() << "Emitting DCOP signal incidenceDeleted( "
                << type << ", " << uid << " )" << endl;
      emitDCOPSignal( "incidenceDeleted(QString,QString)", data );
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError() << "Not a groupware folder" << endl;
}

// KMail orders a refresh
void KMailICalIfaceImpl::slotRefresh( const QString& type )
{
  if( mUseResourceIMAP ) {
    QByteArray data;
    QDataStream arg(data, IO_WriteOnly );
    arg << type;
    kdDebug() << "Emitting DCOP signal signalRefresh( " << type << " )"
              << endl;
    emitDCOPSignal( "signalRefresh(QString)", data );
  }
}


/****************************
 * The folder and message stuff code
 */

KMFolder* KMailICalIfaceImpl::folderFromType( const QString& type )
{
  if( mUseResourceIMAP ) {
    if( type == "Calendar" ) return mCalendar;
    else if( type == "Contact" ) return mContacts;
    else if( type == "Note" ) return mNotes;
    else if( type == "Task" || type == "Todo" ) return mTasks;
    else if( type == "Journal" ) return mJournals;

    kdError() << "No folder type \"" << type << "\"" << endl;
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
  if( !folder ) return 0;

  for( int i=0; i<folder->count(); ++i ) {
    bool unget = !folder->isMessage(i);
    KMMessage* msg = folder->getMsg( i );
    if( msg ) {
      QString vCal;
      if( KMGroupware::vPartFoundAndDecoded( msg, vCal ) ) {
        QString msgUid( "UID" );
        vPartMicroParser( vCal, msgUid );
        if( msgUid == uid )
          return msg;
      }
    }
    if( unget ) folder->unGetMsg(i);
  }

  return 0;
}

void KMailICalIfaceImpl::deleteMsg( KMMessage *msg )
{
  if( !msg ) return;
  ( new KMDeleteMsgCommand( msg->parent(), msg ) )->start();
}

/****************************
 * The config stuff
 */

void KMailICalIfaceImpl::readConfig()
{
  // Read the options
  KConfigGroup options( KMKernel::config(), "IMAP Resource" );
  bool enabled = options.readBoolEntry( "Enabled", false );

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
  unsigned int folderLanguage = options.readNumEntry( "Folder Language", 0 );
  if( folderLanguage > 3 ) folderLanguage = 0;
  QString parentName = options.readEntry("Folder Parent");

  // Find the folder parent
  KMFolderDir* folderParentDir;
  KMFolderType folderType;
  KMFolder* folderParent = kmkernel->folderMgr()->findIdString( parentName );
  if( folderParent == 0 )
    folderParent = kmkernel->dimapFolderMgr()->findIdString( parentName );
  if( folderParent == 0 )
    folderParent = kmkernel->imapFolderMgr()->findIdString( parentName );
  if( folderParent == 0 ) {
    // Maybe nothing was configured?
    folderParentDir = &(kmkernel->folderMgr()->dir());
    folderType = KMFolderTypeMaildir;
  } else {
    folderParentDir = folderParent->createChildFolder();
    folderType = folderParent->folderType();
  }

  // Make sure the folder parent has the subdirs
  bool makeSubFolders = false;
  KMFolderNode* node;
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Calendar ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mCalendar = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Tasks ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mTasks = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Journals ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mJournals = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Contacts ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mContacts = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Notes ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mNotes = 0;
  }
  if( makeSubFolders ) {
    // Not all subfolders were there, so ask if we can make them
    if( KMessageBox::questionYesNo( 0, i18n("KMail will now create the required folders for the IMAP resource"
                                            " as subfolders of %1. If you don't want this, press \"No\","
                                            " and the IMAP resource will be disabled").arg(folderParent!=0?folderParent->name():folderParentDir->name()),
                                    i18n("IMAP Resource Folders") ) == KMessageBox::No ) {
      mUseResourceIMAP = false;
      mFolderParent = 0;
      reloadFolderTree();
      return;
    }
  }

  // Check if something changed
  if( mUseResourceIMAP && !makeSubFolders && mFolderParent == folderParentDir && mFolderType == folderType )
    // Nothing changed
    return;

  // Make the new settings work
  mUseResourceIMAP = true;
  mFolderLanguage = folderLanguage;
  mFolderParent = folderParentDir;
  mFolderType = folderType;

  // Close the previous folders
  cleanup();

  // Set the new folders
  mCalendar = initFolder( KFolderTreeItem::Calendar, "GCa" );
  mTasks    = initFolder( KFolderTreeItem::Tasks, "GTa" );
  mJournals = initFolder( KFolderTreeItem::Journals, "GTa" );
  mContacts = initFolder( KFolderTreeItem::Contacts, "GCo" );
  mNotes    = initFolder( KFolderTreeItem::Notes, "GNo" );

  // Connect the expunged signal
  connect( mCalendar, SIGNAL( expunged() ), this, SLOT( slotRefreshCalendar() ) );
  connect( mTasks,    SIGNAL( expunged() ), this, SLOT( slotRefreshTasks() ) );
  connect( mJournals, SIGNAL( expunged() ), this, SLOT( slotRefreshJournals() ) );
  connect( mContacts, SIGNAL( expunged() ), this, SLOT( slotRefreshContacts() ) );
  connect( mNotes,    SIGNAL( expunged() ), this, SLOT( slotRefreshNotes() ) );

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
  KMFolderNode* node = mFolderParent->hasNamedFolder( folderName( itemType ) );
  if( node && !node->isDir() ) folder = static_cast<KMFolder*>(node);
  // It should never be possible that it's not there, but better safe than sorry
  if( !folder ) folder = mFolderParent->createFolder( folderName( itemType ), false, type );
  if( folder->canAccess() != 0 ) {
    KMessageBox::sorry(0, i18n("You do not have read/write permission to your %1 folder.")
                       .arg( folderName( itemType ) ) );
    return 0;
  }
  folder->setType( typeString );
  folder->setSystemFolder( true );
  folder->open();

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
    folder->close( true );
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
    // TODO: Find a pixmap for journals
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
