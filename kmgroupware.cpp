// kmgroupware.cpp
// Author: Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klibloader.h>
#include <kpopupmenu.h>
#include <krfcdate.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kparts/part.h>

#include "kmgroupware.h"
#include "kfileio.h"
#include "kmkernel.h"
#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmmsgpart.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"
#include "kmfoldertree.h"
#include "kmheaders.h"
#include "kmreaderwin.h"
#include "kmcomposewin.h"
#include "kmmimeparttree.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "kmacctmgr.h"
#include "kmgroupwarefuncs.h"
#include "kmcommands.h"
#include "kmfolderindex.h"
#include "kmmsgdict.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;

#include <ktnef/ktnefparser.h>
#include <ktnef/ktnefattach.h>
#include <ktnef/ktnefproperty.h>
#include <ktnef/ktnefmessage.h>
#include <ktnef/ktnefdefs.h>

#include <mimelib/enum.h>
#include <mimelib/headers.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>
#include <mimelib/text.h>

#include <assert.h>

#include <qdatetime.h>
#include <qstring.h>
#include <qsplitter.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qfile.h>
#include <qinputdialog.h>

// groupware folder icons:
QPixmap* KMGroupware::pixContacts = 0;
QPixmap* KMGroupware::pixCalendar = 0;
QPixmap* KMGroupware::pixNotes    = 0;
QPixmap* KMGroupware::pixTasks    = 0;

// global status flag:
static bool ignore_GroupwareDataChangeSlots = false;

// Global tables of foldernames is different languages
// For now: 0->English, 1->German
static QMap<KFolderTreeItem::Type,QString> folderNames[2];

//-----------------------------------------------------------------------------
KMGroupware::KMGroupware( QObject* parent, const char* name ) :
    QObject(parent, name),
    mMainWin(0),
    mUseGroupware(false),
    mGroupwareIsHidingMimePartTree(false),
    mFolderLanguage(0),
    mKOrgPart(0),
    mFolderParent(0),
    mFolderType(KMFolderTypeUnknown),
    mContactsLocked(false),
    mContacts(0),
    mCalendar(0),
    mNotes(0),
    mTasks(0)
{
}

void KMGroupware::readConfig()
{
  KConfigGroup options( KMKernel::config(), "Groupware" );

  // Do not read the config for this, if it's not setup at all
  if( options.readEntry( "Enabled", "notset" ) == "notset" )
    return;

  bool enabled = options.readBoolEntry( "Enabled", true );

  mUseGroupware = enabled;

  // Set the menus on the main window
  setupActions();

  // Make the folder tree show the icons or not
  if( mMainWin && mMainWin->mainKMWidget()->folderTree() )
    mMainWin->mainKMWidget()->folderTree()->reload();

  if( !mUseGroupware ) {
    slotGroupwareHide();

    // FIXME: This doesn't work :-(
    // delete (KParts::ReadOnlyPart*)mKOrgPart;
    // mKOrgPart = 0;

    if( mContacts ) mContacts->setType("øh");
    mContacts = mCalendar = mNotes = mTasks = 0;
    if( mReader ) mReader->setUseGroupware( false );

    return;
  }

  mFolderLanguage = options.readNumEntry( "FolderLanguage", 0 );
  if( mFolderLanguage > 1 ) mFolderLanguage = 0; // Just for safety

  QString parentName( options.readEntry("GroupwareFolder") );
  KMFolder* folderParent = kernel->folderMgr()->findIdString( parentName );
  if( folderParent == 0 )
    folderParent = kernel->imapFolderMgr()->findIdString( parentName );

  if( folderParent == 0 ) {
    // Maybe nothing was configured?
    mFolderParent = &(kernel->folderMgr()->dir());
    mFolderType = KMFolderTypeMaildir;
  } else {
    KMFolderDir* oldParent = mFolderParent;

    mFolderParent = folderParent->createChildFolder();
    mFolderType = folderParent->folderType();

    if( oldParent && oldParent != mFolderParent ) {
      // Some other folder was previously set as the parent
      // TODO: Do something!
    }
  }

  if( !checkFolders() ) {
    assert( mFolderParent );
    if( KMessageBox::questionYesNo( 0, i18n("KMail will now create the required groupware folders"
					    " as subfolders of %1. If you dont want this, press \"No\","
					    " and the groupware functions will be disabled").arg(mFolderParent->name()),
				    i18n("Groupware Folders") ) == KMessageBox::No ) {
      mUseGroupware = false;
    }
  }

  initFolders();

  internalCreateKOrgPart();

  // Make KOrganizer re-read everything
  emit signalRefreshAll();
}

//-----------------------------------------------------------------------------
KMGroupware::~KMGroupware()
{
  delete mKOrgPart;
}

//-----------------------------------------------------------------------------
// Returns true if folder is a groupware folder. If groupware mode isn't enabled
// this always returns false
bool KMGroupware::isGroupwareFolder( KMFolder* folder ) const
{
  return mUseGroupware && folder && ( folder == mCalendar || folder == mContacts ||
				      folder == mNotes || folder == mTasks );
}

//-----------------------------------------------------------------------------
KFolderTreeItem::Type KMGroupware::folderType( KMFolder* folder ) const
{
  if( mUseGroupware && folder ) {
    if( folder == mCalendar )
      return KFolderTreeItem::Calendar;
    else if( folder == mContacts )
      return KFolderTreeItem::Contacts;
    else if( folder == mNotes )
      return KFolderTreeItem::Notes;
    else if( folder == mTasks )
      return KFolderTreeItem::Tasks;
  }

  return KFolderTreeItem::Other;
}


//-----------------------------------------------------------------------------
QString KMGroupware::folderName( KFolderTreeItem::Type type, int language ) const
{
  static bool folderNamesSet = false;
  if( !folderNamesSet ) {
    folderNamesSet = true;
    /*
      Logical (enum)     English         German
      ------------------------------------------------------
      Contacts           Contacts        Kontakte
      Calendar           Calendar        Kalender
      Notes              Notes           Notizen
      Tasks              Tasks           Aufgaben
      Inbox              inbox           Posteingang
      Outbox             outbox          Postausgang
    */
    /* NOTE: If you add something here, you also need to update
       GroupwarePage in configuredialog.cpp */
    // English
    folderNames[0][KFolderTreeItem::Calendar] = QString::fromLatin1("Calendar");
    folderNames[0][KFolderTreeItem::Contacts] = QString::fromLatin1("Contacts");
    folderNames[0][KFolderTreeItem::Notes] = QString::fromLatin1("Notes");
    folderNames[0][KFolderTreeItem::Tasks] = QString::fromLatin1("Tasks");
    //folderNames[0][KFolderTreeItem::Journal] = QString::fromLatin1("Journals");

    // German
    folderNames[1][KFolderTreeItem::Calendar] = QString::fromLatin1("Kalender");
    folderNames[1][KFolderTreeItem::Contacts] = QString::fromLatin1("Kontakte");
    folderNames[1][KFolderTreeItem::Notes] = QString::fromLatin1("Notizen");
    folderNames[1][KFolderTreeItem::Tasks] = QString::fromLatin1("Aufgaben");
  }

  if( language == -1 ) return folderNames[mFolderLanguage][type];
  else return folderNames[language][type];
}

bool KMGroupware::checkFolders() const
{
  if( !mUseGroupware ) return true;
  if( !mFolderParent ) return false;

  KMFolderNode* node;
  node = mFolderParent->hasNamedFolder( folderName( KFolderTreeItem::Contacts ) );
  if( !node || node->isDir() ) return false;
  node = mFolderParent->hasNamedFolder( folderName( KFolderTreeItem::Calendar ) );
  if( !node || node->isDir() ) return false;
  node = mFolderParent->hasNamedFolder( folderName( KFolderTreeItem::Notes ) );
  if( !node || node->isDir() ) return false;
  node = mFolderParent->hasNamedFolder( folderName( KFolderTreeItem::Tasks ) );
  if( !node || node->isDir() ) return false;
  return true;
}

//-----------------------------------------------------------------------------
KMFolder* KMGroupware::initFolder( KFolderTreeItem::Type itemType, const char* typeString )
{
  // Figure out what type of folder this is supposed to be
  KMFolderType type = mFolderType;
  if( type == KMFolderTypeUnknown ) type = KMFolderTypeMaildir;

  KMFolder* folder = 0;

  KMFolderNode* node = mFolderParent->hasNamedFolder( folderName( itemType ) );
  if( node && !node->isDir() ) folder = static_cast<KMFolder*>(node);
  if( !folder ) folder = mFolderParent->createFolder( folderName( itemType ), false, type );
  if( folder->canAccess() != 0 ) {
    KMessageBox::sorry(0, i18n("You do not have read/write permission to your %1 folder.")
		       .arg( folderName( itemType ) ) );
    return 0;
  }
  folder->setType( typeString );
  folder->setSystemFolder( true );
  folder->open();

  return folder;
}

void KMGroupware::initFolders()
{
  // Close the previous folders
  cleanup();

  // Set the new folders
  if( mUseGroupware && mFolderParent ) {
    mContacts = initFolder( KFolderTreeItem::Contacts, "GCo" );
    mCalendar = initFolder( KFolderTreeItem::Calendar, "GCa" );
    mNotes = initFolder( KFolderTreeItem::Notes, "GNo" );
    mTasks = initFolder( KFolderTreeItem::Tasks, "GTa" );

    // TODO: Change Notes and Contacts to work like calendar and tasks and add Journals
    // Connect the notes folder. This is the old way to do it :-(
    connect( mNotes, SIGNAL( changed() ), this, SLOT( slotNotesFolderChanged() ) );
    connect( mNotes, SIGNAL( msgAdded(int) ), this, SLOT( slotNotesFolderChanged() ) );
    connect( mNotes, SIGNAL( msgRemoved(int, QString) ), this, SLOT( slotNotesFolderChanged() ) );
    slotNotesFolderChanged();

    connect( mCalendar, SIGNAL( msgAdded( KMFolder*, int ) ), this, SLOT( slotIncidenceAdded( KMFolder*, int ) ) );
    connect( mCalendar, SIGNAL( expunged() ), this, SIGNAL( signalCalendarFolderExpunged() ) );
    connect( mTasks, SIGNAL( msgAdded( KMFolder*, int ) ), this, SLOT( slotIncidenceAdded( KMFolder*, int ) ) );
    connect( mTasks, SIGNAL( expunged() ), this, SIGNAL( signalTasksFolderExpunged() ) );

    // Make the folder tree show the icons or not
    if( mMainWin && mMainWin->mainKMWidget()->folderTree() )
      mMainWin->mainKMWidget()->folderTree()->reload();
  }
}


static void cleanupFolder( KMFolder* folder, KMGroupware* gw )
{
  if( folder ) {
    folder->setType( "plain" );
    folder->setSystemFolder( false );
    folder->disconnect( gw );
    folder->close( true );
  }
}

void KMGroupware::cleanup()
{
  cleanupFolder( mContacts, this );
  cleanupFolder( mCalendar, this );
  cleanupFolder( mNotes, this );
  cleanupFolder( mTasks, this );

  mContacts = mCalendar = mNotes = mTasks = 0;
}


bool KMGroupware::vPartFoundAndDecoded( KMMessage* msg,
                                        int& aUpdateCounter,
                                        QString& s )
{
  assert( msg );
//   kdDebug(5006) << "KMGroupware::vPartFoundAndDecoded( " << msg->subject() << endl;
//   kdDebug(5006) << msg->type() << ", " << msg->subtype() << ")" << endl;

  if( ( DwMime::kTypeText == msg->type() && ( DwMime::kSubtypeVCal   == msg->subtype() ||
					      DwMime::kSubtypeXVCard == msg->subtype() ) ) ||
      ( DwMime::kTypeApplication == msg->type() &&
	DwMime::kSubtypeOctetStream == msg->subtype() ) )
  {
    s = QString::fromUtf8( msg->bodyDecoded() );
    return true;
  } else if( DwMime::kTypeMultipart == msg->type() &&
	    DwMime::kSubtypeMixed  == msg->subtype() )
  {
    // kdDebug(5006) << "KMGroupware looking for TNEF data" << endl;
    DwBodyPart* dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
					      DwMime::kSubtypeMsTNEF );
    if( !dwPart )
      dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
				    DwMime::kSubtypeOctetStream );
    if( dwPart ){
      // kdDebug(5006) << "KMGroupware analyzing TNEF data" << endl;
      KMMessagePart msgPart;
      KMMessage::bodyPart(dwPart, &msgPart);
      return KMGroupware::msTNEFToVPart( msgPart.bodyDecodedBinary(), aUpdateCounter, s );
    }
  }else if( DwMime::kTypeMultipart == msg->type() &&
	    DwMime::kSubtypeMixed  == msg->subtype() ){
  }

  return false;
}


void KMGroupware::slotCalendarFolderChanged()
{
  QStringList lEvents;
  QString s;
  int iDummy;
  if( mCalendar )
    for( int i=0; i<mCalendar->count(); ++i ){
      bool unget = !mCalendar->isMessage(i);
      if( KMGroupware::vPartFoundAndDecoded( mCalendar->getMsg( i ), iDummy, s ) )
        lEvents << s;
      if( unget ) mCalendar->unGetMsg(i);
    }

  ignore_GroupwareDataChangeSlots = true;
  emit signalRefreshEvents( lEvents );
  ignore_GroupwareDataChangeSlots = false;
}


void KMGroupware::slotContactsFolderChanged()
{
  //pending(khz): remove this methode if it is no longer needed
}


void KMGroupware::slotTasksFolderChanged()
{
  QStringList lTasks;
  QString s;
  int iDummy;
  if( mTasks )
    for( int i=0; i<mTasks->count(); ++i ){
      bool unget = !mTasks->isMessage(i);
      if( vPartFoundAndDecoded( mTasks->getMsg( i ), iDummy, s ) )
        lTasks << s;
      if( unget ) mTasks->unGetMsg(i);
    }

  ignore_GroupwareDataChangeSlots = true;
  emit signalRefreshTasks( lTasks );
  ignore_GroupwareDataChangeSlots = false;
}


void KMGroupware::slotNotesFolderChanged()
{
  QStringList lNotes;
  KMMessage* m;
  if( mNotes )
    for( int i=0; i<mNotes->count(); ++i ){
      bool unget = !mNotes->isMessage(i);
      m = mNotes->getMsg( i );
      if( m ){
        lNotes << QString::fromUtf8( m->asDwString().data(), m->asDwString().length() );
	m->touch();
      }
      if( unget ) mNotes->unGetMsg(i);
    }

  ignore_GroupwareDataChangeSlots = true;
  emit signalRefreshNotes( lNotes );
  ignore_GroupwareDataChangeSlots = false;
}


void KMGroupware::slotInvalidateIMAPFolders()
{
  QString str = i18n("Are you sure you want to refresh the IMAP cache?\n"
		     "This will remove all changes you have done locally to your folders");
  QString s1 = i18n("Refresh IMAP Cache");
  QString s2 = i18n("&Refresh");
  if( KMessageBox::warningContinueCancel(mMainWin, str, s1, s2 ) == KMessageBox::Continue)
    kernel->acctMgr()->invalidateIMAPFolders();
}

//-----------------------------------------------------------------------------
bool KMGroupware::setFolderPixmap(const KMFolder& folder, KMFolderTreeItem& fti) const
{
  if( !mUseGroupware )
    return false;

  // Make sure they are set
  loadPixmaps();

  if( folder.label() == folderName( KFolderTreeItem::Contacts ) )
    fti.setPixmap( 0, *pixContacts );
  else if( folder.label() == folderName( KFolderTreeItem::Calendar ) )
    fti.setPixmap( 0, *pixCalendar );
  else if( folder.label() == folderName( KFolderTreeItem::Notes ) )
    fti.setPixmap( 0, *pixNotes );
  else if( folder.label() == folderName( KFolderTreeItem::Tasks ) )
    fti.setPixmap( 0, *pixTasks );
  else
    return false;

  return true;
}


//-----------------------------------------------------------------------------
void KMGroupware::setupKMReaderWin(KMReaderWin* reader)
{
  mReader = reader;
  mReader->setUseGroupware( mUseGroupware );

  // connect KMReaderWin's signals to our slots
  connect( mReader, SIGNAL( signalGroupwareShow( bool ) ),
           this,      SLOT(   slotGroupwareShow( bool ) ) );

  // HACK (Bo): Don't receive events while showing groupware widgets
  mReader->installEventFilter( this );
}


void KMGroupware::setHeaders( KMHeaders* headers )
{
  mHeaders = headers;

  // HACK (Bo): Don't receive events while showing groupware widgets
  mHeaders->installEventFilter( this );
}

//-----------------------------------------------------------------------------
void KMGroupware::setMimePartTree(KMMimePartTree* mimePartTree)
{
  mMimePartTree = mimePartTree;
}


//-----------------------------------------------------------------------------
void KMGroupware::createKOrgPart(QWidget* parent)
{
  if( mKOrgPart && mKOrgPartParent.operator->() == parent )
    // The part is already set up.
    return;

  // We must at least remember this.
  mKOrgPartParent = parent;

  if( !mUseGroupware )
    // Groupware mode is disabled, so hold setting it up until user enables it
    return;

  // Ok, we can proceed with actually making the part
  internalCreateKOrgPart();
}

void KMGroupware::internalCreateKOrgPart()
{
  if( !mKOrgPartParent || mKOrgPart )
    // Don't set this up before the parent is set or if the parent is destructed
    return;

  // create a KOrganizer KPart and embedd it into the messageParent
  KLibFactory *factory = KLibLoader::self()->factory( "libkorganizer" );
  if( !factory ) {
    KMessageBox::error(mMainWin, "No libkorganizer found !");
    mUseGroupware = false;
    mReader->setUseGroupware( false );
    return;
  }

  // Create the part
  QStringList partArgs;
  const KMIdentity& identity = kernel->identityManager()->defaultIdentity();
  partArgs << QString( "name=%1" ).arg( identity.fullName() );
  partArgs << QString( "email=%1" ).arg( identity.emailAddr() );
  partArgs << "storage=imap";
  mKOrgPart = (KParts::ReadOnlyPart *)factory->create( (QWidget*)mKOrgPartParent,
						       "korganizerpart",
						       "KParts::ReadOnlyPart", partArgs );

  // initially hide the KOrganizer part
  mKOrgPart->widget()->hide();

  // connect our signals to KOrgPart's slots
  connect( this, SIGNAL( signalSetKroupwareCommunicationEnabled( QObject* ) ),
	   mKOrgPart, SLOT( slotSetKroupwareCommunicationEnabled( QObject* ) ) );

  connect( this, SIGNAL( signalShowCalendarView() ), mKOrgPart, SLOT( slotShowNextXView() ) );
  connect( this, SIGNAL( signalShowNotesView() ), mKOrgPart, SLOT( slotShowNotesView() ) );
  connect( this, SIGNAL( signalShowTodoView() ), mKOrgPart, SLOT( slotShowTodoView() ) );
  // exception: Contacts are handled by KAddressbook - so call it via KMMainWin
  connect( this, SIGNAL( signalShowContactsView() ), mMainWin, SLOT( slotAddrBook() ) );

  connect( this, SIGNAL( signalCalendarUpdateView( const QDateTime&, const QDateTime& ) ),
	   mKOrgPart, SLOT( slotUpdateView( const QDateTime&, const QDateTime& ) ) );
  connect( this, SIGNAL( signalRefreshEvents( const QStringList& ) ),
	   mKOrgPart, SLOT( slotRefreshEvents( const QStringList& ) ) );
  connect( this, SIGNAL( signalRefreshNotes( const QStringList& ) ),
	   mKOrgPart, SLOT( slotRefreshNotes( const QStringList& ) ) );
  connect( this, SIGNAL( signalRefreshTasks( const QStringList& ) ),
	   mKOrgPart, SLOT( slotRefreshTasks( const QStringList& ) ) );

  connect( this, SIGNAL( signalEventRequest( const QCString&, const QString&, bool&,
					     QString&, QString&, bool& ) ),
	   mKOrgPart, SLOT(slotEventRequest( const QCString&, const QString&, bool&,
					     QString&, QString&, bool& ) ) );
  connect( this, SIGNAL( signalResourceRequest( const QValueList<QPair<QDateTime,QDateTime> >&,
						const QCString&, const QString&, bool&,
						QString&, bool&, bool&,
						QDateTime&, QDateTime& ) ),
	   mKOrgPart,
	   SLOT( slotResourceRequest( const QValueList<QPair<QDateTime, QDateTime> >&,
				      const QCString&, const QString&, bool&,
				      QString&, bool&, bool&, QDateTime&, QDateTime& ) ) );
  connect( this, SIGNAL( signalAcceptedEvent( bool, const QCString&, const QString&,
					      bool&, QString&, bool& ) ),
	   mKOrgPart, SLOT(slotAcceptedEvent( bool, const QCString&, const QString&,
					      bool&, QString&, bool& ) ) );
  connect( this, SIGNAL( signalRejectedEvent( const QCString&, const QString&, bool&,
					      QString&, bool& ) ),
	   mKOrgPart, SLOT(slotRejectedEvent( const QCString&, const QString&, bool&,
					      QString&, bool& ) ) );
  connect( this, SIGNAL( signalIncidenceAnswer( const QCString&, const QString&,
						QString& ) ),
	   mKOrgPart, SLOT( slotIncidenceAnswer( const QCString&, const QString&,
						 QString& ) ) );
  connect( this, SIGNAL( signalEventDeleted( const QString& ) ),
	   mKOrgPart, SLOT( slotEventDeleted( const QString& ) ) );

  connect( this, SIGNAL( signalTaskDeleted( const QString& ) ),
	   mKOrgPart, SLOT( slotTaskDeleted( const QString& ) ) );

  connect( this, SIGNAL( signalNoteDeleted( const QString& ) ),
	   mKOrgPart, SLOT( slotNoteDeleted( const QString& ) ) );

  connect( mKOrgPart,SIGNAL( signalKOrganizerShow( bool ) ),
	   this, SLOT( slotGroupwareShow( bool ) ) );

  emit signalSetKroupwareCommunicationEnabled( this );

  // initialize Groupware using data stored in our folders
  // ignore_GroupwareDataChangeSlots = true;
  // Calendar
  slotCalendarFolderChanged();
  // Notes
  slotNotesFolderChanged();
  // Tasks
  slotTasksFolderChanged();
  // ignore_GroupwareDataChangeSlots = false;
}


//-----------------------------------------------------------------------------
void KMGroupware::reparent(QSplitter* panner)
{
  mPanner = panner;
  if( mKOrgPart )
    mKOrgPart->widget()->reparent( mPanner, 0, QPoint( 0, 0 ) );
  else
    // Since the creation of the part is possibly deferred, we should remember this
    mKOrgPartParent = panner;
}


//-----------------------------------------------------------------------------
void KMGroupware::moveToLast()
{
  if( mKOrgPart )
    mPanner->moveToLast( mKOrgPart->widget() );
}


//-----------------------------------------------------------------------------
void KMGroupware::setupActions()
{
  static bool actionsSetup = false;

  if( !actionsSetup && mMainWin ) {
    actionsSetup = true;

    // file menu: some entries
    new KAction( i18n("Merge Calendar"), 0, mKOrgPart, SLOT(slotFileMerge()),
		 mMainWin->actionCollection(), "file_korganizermerge_calendar" );
    new KAction( i18n("Archive Old Entries"), 0, mKOrgPart, SLOT(slotFileArchive()),
		 mMainWin->actionCollection(), "file_korganizerarchive_old_entries" );
    new KAction( i18n("Export iCal"), 0, mKOrgPart, SLOT(slotExportICalendar()),
		 mMainWin->actionCollection(), "file_korganizerexport_ical" );
    new KAction( i18n("Export vCal"), 0, mKOrgPart, SLOT(slotExportVCalendar()),
		 mMainWin->actionCollection(), "file_korganizerexport_vcal" );
    new KAction( i18n("delete completed To-Dos","Purge Completed To-Dos"), 0, mKOrgPart,
		 SLOT(slotPurgeCompleted()), mMainWin->actionCollection(),
		 "file_korganizerpurge_completed" );
    new KAction( i18n("refresh local imap cache", "Refresh Local IMAP Cache"), 0,
		 this, SLOT(slotInvalidateIMAPFolders()), mMainWin->actionCollection(),
		 "invalidate_imap_cache" );
    // view menu: some entries
    new KAction( i18n("What's Next"), "whatsnext", 0, mKOrgPart, SLOT(slotShowWhatsNextView()),
		 mMainWin->actionCollection(), "view_korganizerwhats_next" );
    new KAction( i18n("List"), "list", 0, mKOrgPart, SLOT(slotShowListView()),
		 mMainWin->actionCollection(), "view_korganizerlist" );
    new KAction( i18n("Day"), "1day", 0, mKOrgPart, SLOT(slotShowDayView()),
		 mMainWin->actionCollection(), "view_korganizerday" );
    new KAction( i18n("Work Week"), "5days", 0, mKOrgPart, SLOT(slotShowWorkWeekView()),
		 mMainWin->actionCollection(), "view_korganizerwork_week" );
    new KAction( i18n("Week"), "7days", 0, mKOrgPart, SLOT(slotShowWeekView()),
		 mMainWin->actionCollection(), "view_korganizerweek" );
    new KAction( i18n("Next 3 Days"), 0, mKOrgPart, SLOT(slotShowNextXView()),
		 mMainWin->actionCollection(), "view_korganizernext_three_days" );
    new KAction( i18n("Month"), "month", 0, mKOrgPart, SLOT(slotShowMonthView()),
		 mMainWin->actionCollection(), "view_korganizermonth" );
    new KAction( i18n("To-Do List"), "todo", 0, mKOrgPart, SLOT(slotShowTodoView()),
		 mMainWin->actionCollection(), "view_korganizertodo_list" );
    new KAction( i18n("Notes"), "notes", 0, mKOrgPart, SLOT(slotShowNotesView()),
		 mMainWin->actionCollection(), "view_korganizernotes" );
    // FIXME (Bo): Disable Journals for now
//     new KAction( i18n("Journal"), 0, mKOrgPart, SLOT(slotShowJournalView()),
// 		 mMainWin->actionCollection(), "view_korganizerjournal" );
    new KAction( i18n("Update"), 0, mKOrgPart, SLOT(slotUpdate()),
		 mMainWin->actionCollection(), "view_korganizerupdate" );
    // FIXME (Bo): IMHO this doesn't make sense
//     new KAction( i18n("Hide Organizer"), 0, this, SLOT(slotGroupwareHide()),
// 		 mMainWin->actionCollection(), "view_korganizerhide_groupware" );
    // go menu: some entries
    new KAction( i18n("Go Backward in Calendar"), "1leftarrow", 0, mKOrgPart,
		 SLOT(slotGoPrevious()), mMainWin->actionCollection(),
		 "go_korganizerbackward" );
    new KAction( i18n("Go Forward in Calendar"), "1rightarrow", 0, mKOrgPart,
		 SLOT(slotGoNext()), mMainWin->actionCollection(), "go_korganizerforward" );
    new KAction( i18n("Go to Today"), "today", 0, mKOrgPart, SLOT(slotGoToday()),
		 mMainWin->actionCollection(), "go_korganizertoday" );
    // actions menu: complete menu
    new KAction( i18n("New Event"), "appointment", 0, mKOrgPart, SLOT(slotAppointment_new()),
		 mMainWin->actionCollection(), "korganizeractions_new_event" );
    new KAction( i18n("New To-Do"), "newtodo", 0, mKOrgPart, SLOT(slotNewTodo()),
		 mMainWin->actionCollection(), "korganizeractions_new_todo" );
    new KAction( i18n("New Sub-To-Do"), 0, mKOrgPart, SLOT(slotSubTodo()),
		 mMainWin->actionCollection(), "korganizeractions_new_subtodo" );
    new KAction( i18n("New Note"), 0, mKOrgPart, SLOT(slotNewNote()),
		 mMainWin->actionCollection(), "korganizeractions_new_note" );
    new KAction( i18n("Delete"), 0, mKOrgPart, SLOT(slotDeleteIncidence()),
		 mMainWin->actionCollection(), "korganizeractions_new_delete" );
    new KAction( i18n("Edit"), 0, mKOrgPart, SLOT(slotEditIncidence()),
		 mMainWin->actionCollection(), "korganizeractions_edit" );
    new KAction( i18n("Make Sub-To-Do Independent"), 0, mKOrgPart, SLOT(slotTodo_unsub()),
		 mMainWin->actionCollection(), "korganizeractions_make_subtodo_independent" );
    new KAction( i18n("Organizer Print Preview"), 0, mKOrgPart, SLOT(slotPrintPreview()),
		 mMainWin->actionCollection(), "korganizeractions_printpreview" );
    new KAction( i18n( "Organizer Print" ), 0, mKOrgPart, SLOT( slotPrint() ),
		 mMainWin->actionCollection(), "korganizeractions_print" );
    // schedule menu: complete menu
    new KAction( i18n("Publish Free Busy Information"), 0, mKOrgPart,
		 SLOT(slotPublishFreeBusy()), mMainWin->actionCollection(),
		 "korganizerschedule_publish_free_busy_information" );
    // settings menu: some entries
    new KAction( i18n("Configure KOrganizer"), "korganizer", 0, mKOrgPart,
		 SLOT(slotConfigure()), mMainWin->actionCollection(),
		 "settings_korganizerKOrganizer" );
    new KAction( i18n("Configure Date && Time..."), 0,
		 mKOrgPart, SLOT(slotConfigureDateTime()),
		 mMainWin->actionCollection(), "settings_korganizerdatetime" );
    new KAction( i18n("Edit Filters"), 0, mKOrgPart, SLOT(slotEditFilters()),
		 mMainWin->actionCollection(), "settings_korganizeredit_filters" );
    new KAction( i18n("Edit Categories"), 0, mKOrgPart, SLOT(slotShowCategoryEditDialog()),
		 mMainWin->actionCollection(), "settings_korganizeredit_categories" );
  }

  emit signalMenusChanged();
}

// find message matching a given UID and return it in msg* or trash it
KMMessage *KMGroupware::findMessageByUID( const QString& uid, KMFolder* folder,
					  bool takeMessage )
{
  assert( folder );

  KMMessage* msg = 0;
  KMMessage* m;
  for( int i=0; i<folder->count() && !msg; ++i ){
    bool unget = !folder->isMessage(i);
    m = folder->getMsg( i );
    if( m ){
      int iDummy;
      QString vCalOld;
      if( vPartFoundAndDecoded( m, iDummy, vCalOld ) ){
	QString uidOld( "UID" );
	vPartMicroParser( vCalOld.utf8(), uidOld );
	if( uidOld == uid ){
	  if( takeMessage ) {
	    msg = folder->take(i);
	    msg->removeHeaderField("X-UID");
	  } else
	    msg = m;
	}
      }
    }
    if( !msg && unget )
      folder->unGetMsg(i);
  }

  return msg;
}

void KMGroupware::deleteMsg( KMMessage *msg )
{
  assert( msg );
  QPtrList<KMMsgBase> mList;
  mList.append(msg);
  ( new KMDeleteMsgCommand( msg->parent(), mList ) )->start();
}

//-----------------------------------------------------------------------------
//   Special Contacts methods called by KMKernel's DCOP functions
//-----------------------------------------------------------------------------
void KMGroupware::requestAddresses( QString fname )
{
  QFile file( fname );
  if( file.open( IO_WriteOnly ) ) {
    QTextStream ts( &file );
    ts.setEncoding( QTextStream::UnicodeUTF8 );
    if( mContacts ){
      int iDummy;
      QString s;
      for( int i=0; i<mContacts->count(); ++i ){
        bool unget = !mContacts->isMessage(i);
        if( KMGroupware::vPartFoundAndDecoded( mContacts->getMsg( i ), iDummy, s ) ){
          ts << s;
          s.replace('\n', "\\n");
          s.truncate(65);
        }
        if( unget ) mContacts->unGetMsg(i);
      }
    }else{
      kdDebug(5006) << "+++KMGroupware::requestAddresses(): Contacts folder does not exist" << endl;
    }
    file.close();
  }else{
    kdDebug(5006) << "+++KMGroupware::requestAddresses(): could not open file" << endl;
  }
}

//--------------
bool KMGroupware::storeAddresses( QString fname, QStringList delUIDs )
{
  if( mContacts ){
    QFile file( fname );
    QStringList vCards;
    if( file.open( IO_ReadOnly ) ) {
      QTextStream ts( &file );
      ts.setEncoding( QTextStream::UnicodeUTF8 );
      QString currentVCard;
      while( !ts.eof() ) {
        QString line;
        line = ts.readLine();
        if( line.isEmpty() ) {
          // New vCard
          vCards << currentVCard;
          currentVCard = "";
        } else {
          // Continue current vCard
          currentVCard += line + "\r\n";
        }
      }
      file.close();
    }else{
      kdDebug(5006) << "+++KMGroupware::storeAddresses(): could not open file" << endl;
      return false;
    }

    for( QStringList::iterator it = delUIDs.begin(); it != delUIDs.end(); ++it ) {
      KMMessage* msg = findMessageByUID( *it, mContacts );
      if( msg )
        deleteMsg( msg );
      else
        kdDebug(5006) << "vCard not found, cannot remove: " << *it << endl;
    }

    for( QStringList::iterator it2 = vCards.begin(); it2 != vCards.end(); ++it2 ) {
      QCString vCard( (*it2).utf8() );
      QString uid( "UID" );
      QString name( "NAME" );
      vPartMicroParser( vCard, uid, name );
      KMMessage* msg = findMessageByUID( uid, mContacts, false );
      if( !msg ) {
        // process a new event:
        msg = new KMMessage(); // makes a "Content-Type=text/plain" message
        msg->initHeader();
        msg->setType( DwMime::kTypeText );
        msg->setSubtype( DwMime::kSubtypeXVCard );
        msg->setHeaderField( "Content-Type", "Text/X-VCard; charset=\"utf-8\"" );
	msg->setSubject( "Contact" );
	msg->setTo( name );

	// add missing headers/content:
	msg->setBodyEncoded( vCard );

	// mark the message as read and store it in our Contacts folder
	msg->touch();
	mContacts->addMsg( msg );
      } else {
	// Figure out if the contact have been changed
	int iDummy;
	QString s;
	if( vPartFoundAndDecoded( msg, iDummy, s ) && s.utf8() != vCard ) {
	  msg->setBodyEncoded( vCard );
	  msg->setTo( name );
	}
      }
    }
  }else{
    kdDebug(5006) << "+++KMGroupware::storeAddresses(): Contacts folder does not exist" << endl;
  }
  return true;
}

//--------------
bool KMGroupware::lockContactsFolder()
{
  if( mContactsLocked )
    return false;
  mContactsLocked = true;
  return true;
}
//--------------
bool KMGroupware::unlockContactsFolder()
{
  if( !mContactsLocked )
    return false;
  mContactsLocked = false;
  return true;
}


//-----------------------------------------------------------------------------
void KMGroupware::slotNewOrUpdatedNote( const QString& id, const QString& geometry, const QColor& color,
					const QString& text)
{
  if( ignore_GroupwareDataChangeSlots ) return;
  blockSignals(true);
  KMFolder* folder = mNotes;
  KMMessage* msgNew = 0;

  for( int i=0; i<folder->count(); ++i ){
    bool unget = !folder->isMessage(i);
    KMMessage* m = folder->getMsg( i );
    if( m && (id == m->headerField("X-KOrg-Note-Id")) ) {
      msgNew = folder->take(i);
      break;
    }
    if( unget ) folder->unGetMsg(i);
  }

  if( !msgNew ) msgNew = new KMMessage();

  msgNew->setFrom( kernel->identityManager()->defaultIdentity().fullEmailAddr() );
  msgNew->setTo( kernel->identityManager()->defaultIdentity().fullEmailAddr() );
  msgNew->setHeaderField("Content-Type",
                         "text/plain; charset=\"utf-8\"");
  msgNew->setHeaderField("X-KOrg-Note-Id", id);
  if( !geometry.isEmpty() ) msgNew->setHeaderField("X-KOrg-Note-Geometry", geometry);
  if( color.isValid() ) msgNew->setHeaderField("X-KOrg-Note-Color", color.name() );
  msgNew->setBodyEncoded(text.utf8());
  folder->addMsg(msgNew);
  blockSignals(false);
}

void KMGroupware::slotDeleteNote( const QString& _id )
{
  QString id = _id.stripWhiteSpace();
  //if( ignore_GroupwareDataChangeSlots ) return;
  //blockSignals(true);
  mNotes->open();
  for( int i=0; i<mNotes->count(); ++i ) {
    bool unget = !mNotes->isMessage(i);
    KMMessage* msg = mNotes->getMsg( i );
    if( msg && id == msg->headerField("X-KOrg-Note-Id").stripWhiteSpace() ) {
      deleteMsg( msg );
      break;
    } else if( unget )
      mNotes->unGetMsg(i);
  }
  mNotes->close();
  //blockSignals(false);
}

void internal_directlySendMessage(KMMessage* msg)
{
  // important: We create a composer, but don't want to show it,
  //            so we can *not* call mMainWin->slotCompose().
  KMComposeWin win( msg );
  win.mNeverSign    = true;
  win.mNeverEncrypt = true;
  win.slotSendNow();
  //mMainWin->slotCompose( msgNew, 0 );
}

bool KMGroupware::addIncidence( const QString& type, 
				const QString& uid, 
				const QString& ical )
{
  KMFolder* folder = 0;

  if( type == "Contact" ) {
    folder = mCalendar;
  } else if( type == "Calendar" ) {
    folder = mCalendar;
  } else if( type == "Note" ) {
    folder = mNotes;
  } else if( type == "Task" ) {
    folder = mTasks;
  } else {
    kdError() << "No folder type \"" << type << "\"" << endl;
    assert(0);
  }

  // process a new event:
  KMMessage* msg = new KMMessage(); // makes a "Content-Type=text/plain" message
  msg->initHeader();
  msg->setType(    DwMime::kTypeText );
  msg->setSubtype( DwMime::kSubtypeVCal );
  msg->setHeaderField("Content-Type",
		      "text/calendar; method=REQUEST; charset=\"utf-8\"");
  msg->setSubject( uid ); // we could use the uid as subj
  msg->setTo( msg->from() );
  msg->setBodyEncoded( ical.utf8() );

  msg->touch();
  folder->addMsg( msg );  
  return true;
}

bool KMGroupware::deleteIncidence( const QString& type, const QString& uid )
{
  KMFolder* folder = 0;

  if( type == "Contact" ) {
    folder = mCalendar;
  } else if( type == "Calendar" ) {
    folder = mCalendar;
  } else if( type == "Note" ) {
    folder = mNotes;
  } else if( type == "Task" ) {
    folder = mTasks;
  } else {
    kdError() << "No such folder" << endl;
    return false;
  }

  KMMessage* msg = findMessageByUID( uid, folder, false );
  if( !msg ) return false;
  
  deleteMsg( msg );
  return true;
}

QStringList KMGroupware::incidences( const QString& type )
{
  KMFolder* folder = 0;

  if( type == "Contact" ) {
    folder = mCalendar;
  } else if( type == "Calendar" ) {
    folder = mCalendar;
  } else if( type == "Note" ) {
    folder = mNotes;
  } else if( type == "Task" ) {
    folder = mTasks;
  } else {
    assert(0);
  }

  QStringList ilist;
  QString s;
  int iDummy;  
  for( int i=0; i<folder->count(); ++i ){
    bool unget = !folder->isMessage(i);
    if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), iDummy, s ) )
      ilist << s;
    if( unget ) folder->unGetMsg(i);
  }
  return ilist;
}

void KMGroupware::slotIncidenceAdded( KMFolder* folder, Q_UINT32 sernum )
{
  QString type;
  if( folder == mContacts ) {
    type = "Contact";
  } else if( folder == mCalendar ) {
    type = "Calendar";
  } else if( folder == mTasks ) {
    type = "Task";
  } else if( folder == mNotes ) {
    type = "Note";
  } else {
    kdError() << "Not a groupware folder" << endl;
    return;
  }

  int i = 0;
  KMFolder* aFolder = 0;
  KMKernel::self()->msgDict()->getLocation( sernum, &aFolder, &i );
  assert( folder == aFolder );

  bool unget = !folder->isMessage( i );
  int iDummy;
  QString s;
  if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), iDummy, s ) )
    emit incidenceAdded( type, s );
  if( unget ) folder->unGetMsg(i);
}

void KMGroupware::slotIncidenceDeleted( KMFolder* folder, Q_UINT32 sernum )
{
  QString type;
  if( folder == mContacts ) {
    type = "Contact";
  } else if( folder == mCalendar ) {
    type = "Calendar";
  } else if( folder == mTasks ) {
    type = "Task";
  } else if( folder == mNotes ) {
    type = "Note";
  } else {
    kdError() << "Not a groupware folder" << endl;
    return;
  }

  int i = 0;
  KMFolder* aFolder = 0;
  KMKernel::self()->msgDict()->getLocation( sernum, &aFolder, &i );
  assert( folder == aFolder );

  bool unget = !folder->isMessage( i );
  int iDummy;
  QString s;
  if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), iDummy, s ) ) {
    QString uid( "UID" );
    vPartMicroParser( s.utf8(), uid );
    emit incidenceDeleted( type, uid );
  }
  if( unget ) folder->unGetMsg(i);
}

void KMGroupware::slotNewOrUpdatedIncident( const QString& type,
                                            const QString& vCalNew,
                                            const QString& uid,
                                            const QStringList& recipients,
                                            const QString& subject )
{
  if( ignore_GroupwareDataChangeSlots || vCalNew.isEmpty() ) return;

  KMFolder* folder = 0;

  if( type == "Contact" ) {
    folder = mCalendar;
  } else if( type == "Calendar" ) {
    folder = mCalendar;
  } else if( type == "Note" ) {
    folder = mNotes;
  } else if( type == "Task" ) {
    folder = mTasks;
  } else {
    assert(0);
  }

  const QString deleteMe( "DELETE ME:" );
  const bool bDeleteMe = uid.startsWith( deleteMe );
  const QString uidNew( bDeleteMe ? uid.mid(deleteMe.length()) : uid );

  QStringList lEvents, lNotes, lTasks;

  // Calendar
  KMMessage* msgNew = findMessageByUID( uidNew, folder );
  bool bWasOld = (msgNew != 0);

  // NOTE: We send *no* message to recipients
  //       when user deletes an event from her Calendar.
  if( bWasOld || !bDeleteMe ){
    QString subjectHeader;
    if( !bWasOld ){
      // process a new event:
      msgNew = new KMMessage(); // makes a "Content-Type=text/plain" message
      msgNew->initHeader();
      msgNew->setType(    DwMime::kTypeText );
      msgNew->setSubtype( DwMime::kSubtypeVCal );
      msgNew->setHeaderField("Content-Type",
                             "text/calendar; method=REQUEST; charset=\"utf-8\"");
    }
    if( bDeleteMe )
      subjectHeader = i18n("DELETED: ");
    else if( bWasOld )
      subjectHeader = i18n("UPDATE: ");
    subjectHeader.append( subject );
    msgNew->setSubject( subjectHeader );
    if( bWasOld && recipients.count() ){
      msgNew->setType(    DwMime::kTypeText );
      msgNew->setSubtype( DwMime::kSubtypeVCal );
      msgNew->setHeaderField("Content-Type",
                             "text/calendar; method=REPLY; charset=\"utf-8\"");
    }
    // add missing headers/content:
    if( recipients.count() )
      msgNew->setTo( recipients.join(",") );
    else
      msgNew->setTo( msgNew->from() );
    msgNew->setBodyEncoded( vCalNew.utf8() );

    // send the message to the recipients, but only if there are any
    if( recipients.count() ){
      KMMessage* msgSend = new KMMessage( *msgNew );
      internal_directlySendMessage( msgSend );
    }

    if( bDeleteMe ){
      // we have informed any recipients and put the entry into the paper bin now
      if( bWasOld ){
        deleteMsg( msgNew );
      }
    }else{
      // mark the message as read and store it in our Calendar folder
      msgNew->touch();
      folder->addMsg( msgNew );
    }
  }
}


KMGroupware::VCalType KMGroupware::getVCalType( const QString &vCal )
{
  // This is ugly: We can't even use vPartMicroParser() here, because
  // we are actually looking for the _second_ BEGIN: line.
  // PENDING(kalle) We might need to look for even more things here,
  // like journals.
  if( vCal.find( QRegExp( "BEGIN:\\s*VEVENT" ) ) != -1 )
    return vCalEvent;
  else if( vCal.find( QRegExp( "BEGIN:\\s*VTODO" ) ) != -1 )
    return vCalTodo;
  return vCalUnknown;
}

//-----------------------------------------------------------------------------
void KMGroupware::processVCalRequest( const QCString& receiver,
				      const QString& vCalIn,
                                      QString& choice )
{
  ignore_GroupwareDataChangeSlots = true;
  bool inOK = false, outOK = false;
  QString outVCal;

  VCalType type = getVCalType( vCalIn );
  if( type == vCalUnknown ) {
    kdDebug(5006) << "processVCalRequest called with something that is not a vCal\n";
    return;
  }

  // If we are in legacy mode, and there is more than one receiver, we
  // need to ask the user which address to use
  KMMessage* msgOld = mMainWin->mainKMWidget()->headers()->currentMsg();
  KConfigGroup options( KMKernel::config(), "Groupware" );
  QString fromAddress; // this variable is only used in legacy mode
  if( options.readBoolEntry( "LegacyMangleFromToHeaders", false ) ) {
      QStringList toAddresses = KMMessage::splitEmailAddrList( msgOld->to() );
      if( toAddresses.count() <= 1 )
          // only one address: no problem, we can spare the user the dialog
          // and just take the from address
          fromAddress = msgOld->to();
      else {
          // We have more than one To: address and are in legacy mode. Next
          // try is to search the identities for one of the email addresses
          // in the toAddresses list.
          for( QStringList::Iterator sit = toAddresses.begin();
               sit != toAddresses.end(); ++sit ) {
              if( KMMessage::getEmailAddr( *sit ) == 
                  kernel->identityManager()->defaultIdentity().emailAddr().local8Bit() ) {
                  // our default identity was contained in the To: list,
                  // copy that from To: to From:
                  fromAddress = *sit;
                  break; // We are done
              }
          }
          
          // If we still haven't found anything, we have to ask the user
          // what to do.
          if( fromAddress.isEmpty() ) {
              bool bOk;
              fromAddress = QInputDialog::getItem( i18n( "Select Address" ),
                                                   i18n( "In order to let Outlook(tm) recognize you as the receiver, you need to indicate which one of the following addresses is your email address" ),
                                                   toAddresses, 0, false, &bOk,
                                                   mMainWin );
              if( !bOk )
                  // If the user didn't select anything, just take the
                  // first one so that we have something at all.
                  fromAddress = toAddresses.first();
          }
      }
  }

  // step 1: call Organizer
  if("accept" == choice ){
    emit signalAcceptedEvent( false, receiver, vCalIn, inOK, outVCal, outOK );
  }else if("accept conditionally" == choice ){
    emit signalAcceptedEvent( true, receiver, vCalIn, inOK, outVCal, outOK );
  }else if("decline" == choice ){
    emit signalRejectedEvent( receiver, vCalIn, inOK, outVCal, outOK );
  }else if("check" == choice ){
    emit signalShowCalendarView();
    slotGroupwareShow( true );
    // try to find out the start and end time
    QString sDtStart( "DTSTART" );
    QString sDtEnd( "DTEND" );
    vPartMicroParser( vCalIn.utf8(), sDtStart, sDtEnd );
    if( !sDtStart.isEmpty() && !sDtEnd.isEmpty() ) {
      sDtStart = ISOToLocalQDateTime( sDtStart );
      sDtEnd = ISOToLocalQDateTime( sDtEnd );
      QDateTime start = QDateTime::fromString( sDtStart.left(sDtStart.find('@')), Qt::ISODate );
      QDateTime end = QDateTime::fromString( sDtEnd.left( sDtEnd.find(  '@')), Qt::ISODate );
      emit signalCalendarUpdateView( start, end );
    }
    emit signalEventRequest( receiver, vCalIn, inOK, choice, outVCal, outOK );
  }
  // step 2: process vCal returned by Organizer
  if( outOK && mMainWin && mCalendar ){
    // mMainWin->slotNewBodyReplyToMsg( outVCal );
    KMMessage* msgNew = 0;
    if( msgOld ){
      msgNew = msgOld->createReply( false, false, outVCal, false, true, TRUE );

      // This is really, really, really ugly, but Outlook will only
      // understand the reply if the From: header is the same as the
      // To: header of the invitation message.
      KConfigGroup options( KMKernel::config(), "Groupware" );
      if( options.readBoolEntry( "LegacyMangleFromToHeaders", false ) )
          msgNew->setFrom( fromAddress );

      msgNew->setType(    DwMime::kTypeText );
      msgNew->setSubtype( DwMime::kSubtypeVCal );
      msgNew->setHeaderField("Content-Type",
                             "text/calendar; method=REPLY; charset=\"utf-8\"");
      internal_directlySendMessage( msgNew );
    }
    if( "accept" == choice || "accept conditionally" == choice ) {
      if( type == vCalTodo )
	// This is a task
	mMainWin->mainKMWidget()->slotMoveMsgToFolder( mTasks );
      else
	// This is an appointment
	mMainWin->mainKMWidget()->slotMoveMsgToFolder( mCalendar );
    } else if("decline" == choice )
      mMainWin->mainKMWidget()->slotTrashMsg();
  }
  slotGroupwareHide();
  ignore_GroupwareDataChangeSlots = false;
}


//-----------------------------------------------------------------------------
void KMGroupware::processVCalReply( const QCString& sender, const QString& vCalIn,
                                    const QString& choice )
{
  VCalType type = getVCalType( vCalIn );
  if( type == vCalUnknown ) {
    kdDebug(5006) << "processVCalReply called with something that is not a vCal\n";
    return;
  }

  if("enter" == choice ){
    // step 1: call Organizer
    QString vCalOut;
    ignore_GroupwareDataChangeSlots = true;
    // emit signal...
    emit signalIncidenceAnswer( sender, vCalIn, vCalOut );

    // Check for the user stopping this transaction or some error happening
    if( vCalOut == "false" ) {
      kdDebug(5006) << "Problem in processing the iCal reply\n";
      ignore_GroupwareDataChangeSlots = false;
      return;
    }

    // note: If we do not get a vCalOut back, we just store the vCalIn into mCalendar

    QString uid( "UID" );
    vPartMicroParser( vCalOut.isEmpty() ? vCalIn.utf8() : vCalOut.utf8(), uid );
    KMMessage* msgNew = findMessageByUID( uid, mCalendar );
    if( !msgNew ){
      // process a new event:
      msgNew = new KMMessage(); // makes a "Content-Type=text/plain" message
      msgNew->initHeader();
      msgNew->setType(    DwMime::kTypeText );
      msgNew->setSubtype( DwMime::kSubtypeVCal );
      msgNew->setHeaderField("Content-Type",
                             "text/calendar; method=REPLY; charset=\"utf-8\"");
      if( type == vCalEvent )
	msgNew->setSubject( "Meeting" );
      else if( type == vCalTodo )
	msgNew->setSubject( "Task" );
    }
    // add missing headers/content:
    msgNew->setTo( msgNew->from() );
    msgNew->setBodyEncoded( vCalOut.isEmpty() ? vCalIn.utf8() : vCalOut.utf8() );

    // mark the message as read and store it in a folder
    msgNew->touch();
    if( type == vCalEvent )
      mCalendar->addMsg( msgNew );
    else if( type == vCalTodo )
      mTasks->addMsg( msgNew );

    // step 2: inform user that Organizer was updated
    KMessageBox::information( mMainWin, (type == vCalEvent ?
					 i18n("The answer was registered in your calendar.") :
					 i18n("The answer was registered in your task list.")),
			      QString::null, "groupwareBox");
    ignore_GroupwareDataChangeSlots = false;
  } else if( "cancel" == choice ) {
    QString uid( "UID" );
    QString descr("DESCRIPTION");
    QString summary("SUMMARY"); 

    vPartMicroParser( vCalIn.utf8(), uid, descr, summary );
    if( type == vCalEvent ) {
      emit signalEventDeleted( uid );
      KMessageBox::information( mMainWin, i18n("<qt>The event <b>%1</b> was deleted from your calendar.</qt>")
				.arg( descr) );
    } else if( type == vCalTodo ) {
      emit signalTaskDeleted( uid );
      KMessageBox::information( mMainWin, i18n("The task was deleted from your tasks")
				.arg( summary ) );
    }
  } else {
    // Don't know what to do, so better not delete the mail
    return;
  }

  // An answer was saved, so trash the message
  mMainWin->mainKMWidget()->slotTrashMsg();
}


//-----------------------------------------------------------------------------
bool KMGroupware::isContactsFolder( KMFolder* folder ) const
{
  return mContacts && folder == mContacts;
};


//-----------------------------------------------------------------------------
bool KMGroupware::folderSelected( KMFolder* folder )
{
  bool bFound = mUseGroupware;
  if( mUseGroupware ){
    if( folder == mCalendar ){
      slotGroupwareShow( true );
      emit signalShowCalendarView();
    }
    else if( folder == mContacts ){
      // note: We do *not* show the KOrganizer plugin here!
      //       Contacts are done via KAddressbook.
      emit signalShowContactsView();
    }
    else if( folder == mNotes ){
      slotGroupwareShow( true );
      emit signalShowNotesView();
    }
    else if( folder == mTasks ){
      slotGroupwareShow( true );
      emit signalShowTodoView();
    }
    else{
      slotGroupwareHide();
      bFound = false;
    }
  }
  return bFound;
}


/* View->Groupware menu */
void KMGroupware::slotGroupwareHide()
{
  if( mKOrgPart ){
    mKOrgPart->widget()->hide();
    mHeaders->show();
    mReader->show();
    if( mGroupwareIsHidingMimePartTree ){
      mGroupwareIsHidingMimePartTree = false;
      mMimePartTree->show();
    }
  }
}


/* additional groupware slots */
void KMGroupware::slotGroupwareShow(bool visible)
{
  if( mKOrgPart ){
    if( visible ){
      mHeaders->hide();
      mReader->hide();
      if( !mMimePartTree->isHidden() ){
        mMimePartTree->hide();
        mGroupwareIsHidingMimePartTree = true;
      }
      mKOrgPart->widget()->show();
    }
    else
      slotGroupwareHide();
  }
}

bool KMGroupware::eventFilter( QObject *o, QEvent *e ) const {
  if( o ) {
    if( o == mReader || o == mHeaders )
      // When a groupware widget is shown, these two must not get any events
      return mKOrgPart->widget()->isShown();

    if( o == mMainWin )
      // Only filter keypresses from main win
      return mKOrgPart->widget()->isShown() && e->type() == QEvent::KeyPress;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool KMGroupware::vPartToHTML( int aUpdateCounter, const QString& vCal, QString fname,
                               bool useGroupware, QString& prefix, QString& postfix )
{
  VCalType type = getVCalType( vCal );
  if( type == vCalUnknown ) {
    kdDebug(5006) << "Unknown incidence!\n";
    return false;
  }

  // Read the vCal
  QString sLocation( "LOCATION" );
  QString sDtStart( "DTSTART" );
  QString sDtEnd( "DTEND" );
  QString sDescr( "DESCRIPTION" );
  QString sMethod( "METHOD");
  QString sAttendee( "ATTENDEE" );
  QString sSummary( "SUMMARY" );
  vPartMicroParser( vCal.utf8(), sLocation, sDtStart, sDtEnd, sDescr, sMethod, sAttendee, sSummary );
  string2HTML( sLocation );
  while( sDescr.endsWith("\\n") )
    sDescr.truncate( sDescr.length()-2 );
  string2HTML( sDescr );
  sDtStart = ISOToLocalQDateTime( sDtStart );
  sDtEnd = ISOToLocalQDateTime( sDtEnd );
  sDtStart = sDtStart.right( sDtStart.length() - sDtStart.find( '@' ) - 1 ) ;
  sDtEnd = sDtEnd.right( sDtEnd.length() - sDtEnd.find( '@' ) - 1 );
  sMethod = sMethod.lower();
  sAttendee = sAttendee.upper();

  QString typeString;
  if( type == vCalEvent )
    typeString = i18n("calendar");
  else
    typeString = i18n("tasks");

  if( sMethod == "request" ) {
    if( type == vCalEvent ) {
      if( aUpdateCounter == 0 )
	prefix = i18n("You have been invited to a meeting");
      else
	prefix = i18n("This is an update of a previous invitation.");
      prefix += "<br>";
      if( !sLocation.isEmpty() )
	prefix.append( i18n( "The meeting will take place in %1 from %2 to %3" )
		       .arg( sLocation ).arg( sDtStart ).arg( sDtEnd ) );
      else
	prefix.append( i18n( "The meeting will take place from %1 to %2" )
		       .arg( sDtStart )
		       .arg( sDtEnd ) );
    } else {
      prefix = i18n( "You have been assigned a task:<br>%1" ).arg( sSummary );
    }
  } else if( sMethod == "reply" ){
    if( 0 < sAttendee.contains("PARTSTAT=ACCEPTED") ) {
      if( type == vCalEvent )
	prefix = i18n("Sender <b>accepts</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( type == vCalTodo )
	prefix = i18n( "Sender <b>accepts</b> the task <b>%1</b>." ).arg(sSummary );
    } else if( 0 < sAttendee.contains("PARTSTAT=TENTATIVE") ) {
      if( type == vCalEvent )
	prefix = i18n("Sender <b>tentatively accepts</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( type == vCalTodo )
	prefix = i18n( "Sender <b>tentatively accepts</b> the task <b>%1</b>." ).
	  arg(sSummary );
    } else if( 0 < sAttendee.contains("PARTSTAT=DECLINED") ) {
      if( type == vCalEvent )
	prefix = i18n("Sender <b>declines</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( vCalTodo )
	prefix = i18n( "Sender <b>declines</b> the task %1." ).arg( sSummary );
    } else {
      if( type == vCalEvent ) {
	prefix = i18n("This is an unknown reply to the event in %1 from %2 to %3")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      } else if( type == vCalTodo ) {
	prefix = i18n("This is an unknown reply to the task %1").arg(sSummary);
      }
    }
  } else if( sMethod == "cancel" ) {
    if( type == vCalEvent ) {
      prefix = i18n("The event %1 was cancelled").arg(sSummary);
    } else if( type == vCalTodo ) {
      prefix = i18n("The task %1 was cancelled").arg(sSummary);
    }
  }

  // show the 'buttons' (only if in groupware mode)
  if( useGroupware ) {
    prefix.append( "<br>&nbsp;<br>&nbsp;<br><table border=\"0\" cellspacing=\"0\"><tr><td>&nbsp;</td><td>" );
    if( sMethod == "request" || sMethod == "update" ) {
      // Accept
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_accept#%1\"><b>")
		     .arg(fname) );
      prefix.append( i18n("[Accept]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>") );
      // Accept conditionally
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_accept conditionally#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Accept&nbsp;cond.]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>") );
      // Decline
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_decline#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Decline]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>" ) );
      if( type == vCalEvent ) {
	// Check my calendar...
	prefix.append(QString("<a href=\"kmail:groupware_vCal_request_check#%1\"><b>")
		      .arg(fname));
	prefix.append(i18n("[Check&nbsp;my&nbsp;calendar...]"));
	prefix.append(QString("</b></a>"));
      }
    } else if( sMethod == "reply" ) {
      // Enter this into my calendar
      prefix.append(QString("<a href=\"kmail:groupware_vCal_reply_enter#%1\"><b>")
		    .arg(fname));
      if( type == vCalEvent )
	prefix.append(i18n("[Enter&nbsp;this&nbsp;into&nbsp;my&nbsp;calendar]"));
      else
	prefix.append(i18n("[Enter&nbsp;this&nbsp;into&nbsp;my&nbsp;tasks]"));
      prefix.append(QString("</b></a>"));
    } else if( sMethod == "cancel" ) {
      // Cancel event from my calendar
      prefix.append( QString("<a href=\"kmail:groupware_vCal_cancel_enter#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Remove&nbsp;this&nbsp;from&nbsp;my&nbsp;calendar]"));
      prefix.append(QString("</b></a>"));
    }
    prefix.append( "</td></tr></table>" );
  }

  if( sMethod == "request" || sMethod == "cancel" ) {
    sDescr.prepend( "<br>&nbsp;<br>&nbsp;<br><u>" + i18n("Description:")
		    + "</u><br><table border=\"0\"><tr><td>&nbsp;</td><td>" );
    sDescr.append( "</td></tr></table>" );
    prefix.append( sDescr );
  }
  prefix.append("&nbsp;<br>&nbsp;<br><u>");
  prefix.append(i18n("Original message:"));
  prefix.append("</u><br><table border=\"0\"><tr><td>&nbsp;</td><td>");
  // postfix:
  postfix = "</td></tr></table>";

  return true;
}


//-----------------------------------------------------------------------------
QString attendeeLine( const QString& name, const QString& mail, bool bIsRSVP,
                      bool bIsReply, bool bAccepted, bool bAcceptedCond, bool bDeclined )
{
  QString line( "ATTENDEE;" );
  if( !name.isEmpty() ){
    line.append( "CN=\"" );
    line.append( name );
    line.append( "\";" );
  }
  line.append( "RSVP=" );
  line.append( bIsRSVP ? "TRUE" : "FALSE" );
  line.append( ";" );
  if( bIsReply ){
    line.append( "PARTSTAT=" );
    if( bAccepted )
      line.append( "ACCEPTED" );
    else if( bAcceptedCond )
      line.append( "TENTATIVE" );
    else if( bDeclined )
      line.append( "DECLINED" );
    else
      line.append( "TENTATIVE" ); // use this as fallback ?   (khz, 2002/10/16)
    line.append( ':' );
  }else{
    line.append( "PARTSTAT=NEEDS-ACTION;" );
    line.append( "ROLE=REQ-PARTICIPANT;" );
  }
  line.append( "MAILTO:" );
  line.append( mail );
  return line;
}

QString stringProp( KTNEFMessage* tnefMsg, const QString&  prefix, const QString&  title,
                    const Q_UINT32& key, const QString&  fallback )
{
  QString res;
  QString value( tnefMsg->findProp( key < 0x10000 ? key & 0xFFFF : key >> 16,
                                    fallback ) );
  if( !value.isEmpty() ){
    res = prefix;
    if( !title.isEmpty() ){
      res.append( title );
      res.append( ':'   );
    }
    res.append( value );
  }
  return res;
}

QString sNamedProp( KTNEFMessage* tnefMsg, const QString& prefix, const QString& title,
                    const QString& name, const QString& fallback )
{
  QString res;
  QString value( tnefMsg->findNamedProp( name, fallback ) );
  if( !value.isEmpty() ){
    res = prefix;
    if( !title.isEmpty() ){
      res.append( title );
      res.append( ':'   );
    }
    res.append( value );
  }
  return res;
}

//-----------------------------------------------------------------------------
bool KMGroupware::msTNEFToVPart( const QByteArray& tnef,
                                 int& aUpdateCounter,
                                 QString& vPart )
{
  // Note: vPart is not erased but
  //       keeps it's initial data if it cannot be decoded
  bool bOk = false;

  KTNEFParser parser;
  QBuffer buf( tnef );
  if( parser.openDevice( &buf ) )
  {
    KTNEFMessage* tnefMsg = parser.message();
    //QMap<int,KTNEFProperty*> props = parser.message()->properties();

    // everything depends from property PR_MESSAGE_CLASS
    // (this is added by KTNEFParser):
    QString msgClass = tnefMsg->findProp(0x001A, "", true).upper();
    if( !msgClass.isEmpty() ){
      // Match the old class names that might be used by Outlook for
      // compatibility with Microsoft Mail for Windows for Workgroups 3.1.
      bool bCompatClassAppointment   = false;
      bool bCompatMethodRequest      = false;
      bool bCompatMethodCancled      = false;
      bool bCompatMethodAccepted     = false;
      bool bCompatMethodAcceptedCond = false;
      bool bCompatMethodDeclined     = false;
      if( msgClass.startsWith( "IPM.MICROSOFT SCHEDULE." ) ){
        bCompatClassAppointment = true;
        if( msgClass.endsWith( ".MTGREQ" ) )
          bCompatMethodRequest = true;
        if( msgClass.endsWith( ".MTGCNCL" ) )
          bCompatMethodCancled = true;
        if( msgClass.endsWith( ".MTGRESPP" ) )
          bCompatMethodAccepted = true;
        if( msgClass.endsWith( ".MTGRESPA" ) )
          bCompatMethodAcceptedCond = true;
        if( msgClass.endsWith( ".MTGRESPN" ) )
          bCompatMethodDeclined = true;
      }
      bool bCompatClassNote = (msgClass == "IPM.MICROSOFT MAIL.NOTE");

      if( bCompatClassAppointment || "IPM.APPOINTMENT" == msgClass ){

        // retrieve the update counter
        aUpdateCounter = tnefMsg->findNamedProp("0x8201", "0").toInt();
        // compose a vCal
        bool bIsReply = false;
        vPart = "BEGIN:VCALENDAR\n";
        vPart += "PRODID:-//Microsoft Corporation//Outlook ";
        vPart += tnefMsg->findNamedProp("0x8554", "9.0");
        vPart += "MIMEDIR/EN\n";
        vPart += "VERSION:2.0\n";

        vPart += "METHOD:";
        if( bCompatMethodRequest )
          vPart += "REQUEST";
        else if( bCompatMethodCancled )
          vPart += "CANCEL";
        else if( bCompatMethodAccepted ||
                 bCompatMethodAcceptedCond ||
                 bCompatMethodDeclined ){
          bIsReply = true;
          vPart += "REPLY";
        }
        else{
          // pending(khz): verify whether "0x0c17" is the right tag ???
          //
          // at the moment we think there are REQUESTS and UPDATES
          //
          // but WHAT ABOUT REPLIES ???
          //
          //
          if( tnefMsg->findProp(0x0c17, "") == "1" )
            bIsReply = true;
          vPart += "REQUEST";
        }
        vPart += '\n';

        QString sSenderSearchKeyEmail( tnefMsg->findProp(0x0C1D, "") );
        if( !sSenderSearchKeyEmail.isEmpty() ){
          int colon = sSenderSearchKeyEmail.find(':');
          if( -1 < colon ) // may be e.g. "SMTP:KHZ@KDE.ORG"
            sSenderSearchKeyEmail.remove(0, colon+1);
        }

        vPart += "BEGIN:VEVENT\n";
        QString s( tnefMsg->findProp(0x0e04, "") );
        QStringList attendees( QStringList::split(';', s) );
        if( attendees.count() ){
          for ( QStringList::Iterator it = attendees.begin(); it != attendees.end(); ++it ) {
            // skip all entries that have no '@' since these are no mail addresses
            if( -1 < (*it).find('@') ){
              s = (*it).stripWhiteSpace();
              vPart += attendeeLine( QString(),
                                     s,
                                     true,
                                     bIsReply,
                                     bCompatMethodAccepted,
                                     bCompatMethodAcceptedCond,
                                     bCompatMethodDeclined );
              vPart += '\n';
            }
          }
        }else{
          // Oops, no attendees?
          // This must be old style, let us use the PR_SENDER_SEARCH_KEY.
          s = sSenderSearchKeyEmail;
          if( !s.isEmpty() ){
            vPart += attendeeLine( QString(),
                                   s,
                                   true,
                                   bIsReply,
                                   bCompatMethodAccepted,
                                   bCompatMethodAcceptedCond,
                                   bCompatMethodDeclined );
            vPart += '\n';
          }
        }
        s = tnefMsg->findProp(0x0c1f, ""); // look for organizer property
        if( s.isEmpty() && !bIsReply )
          s = sSenderSearchKeyEmail;
        if( !s.isEmpty() ){
          vPart += "ORGANIZER;MAILTO:";
          vPart += s;
          vPart += '\n';
        }
        s = tnefMsg->findNamedProp("0x8516", "")
              .replace(QChar('-'), "")
              .replace(QChar(':'), "");
        vPart += "DTSTART:";
        vPart += s;
        vPart += "Z\n";
        s = tnefMsg->findNamedProp("0x8517", "")
              .replace(QChar('-'), "")
              .replace(QChar(':'), "");
        vPart += "DTEND:";
        vPart += s;
        vPart += "Z\n";
        vPart += "LOCATION:";
        vPart += tnefMsg->findNamedProp("0x8208", "");
        vPart += '\n';

        // is it OK to set this to OPAQUE always ??
        vPart += "TRANSP:OPAQUE\n";

        vPart += "SEQUENCE:0\n";

        // is "0x0023" OK  -  or should we look for "0x0003" ??
        vPart += "UID:";
        vPart += tnefMsg->findNamedProp("0x0023", "");
        vPart += '\n';

        vPart += "DTSTAMP:";
        // pending(khz): is this value in local timezone ??   must it be adjusted ??
        // most likely this is a bug in the server or in Outlook - we ignore it for now.
        vPart += tnefMsg->findNamedProp("0x8202", "")
              .replace(QChar('-'), "")
              .replace(QChar(':'), "");
        vPart += '\n';

        vPart += "CATEGORIES:";
        vPart += tnefMsg->findNamedProp("Keywords", "");
        vPart += '\n';
        vPart += "DESCRIPTION:";
        vPart += tnefMsg->findProp(0x1000, "");
        vPart += '\n';
        vPart += "SUMMARY:";
        vPart += tnefMsg->findProp(0x0070, "");
        vPart += '\n';

        vPart += "PRIORITY:";
        s = tnefMsg->findProp(0x0026, "");
        if( "1" == s )
          vPart += "URGENT";
        else if( ("0" == s) || ("-1" == s) )
          vPart += "NORMAL";
        vPart += '\n';

        // is reminder flag set ?
        if( "TRUE" == tnefMsg->findProp(0x8503, "").upper() ){
          vPart += "CLASS:PUBLIC\n";
          vPart += "BEGIN:VALARM\n";
          QDateTime highNoonTime(
                      pureISOToLocalQDateTime( tnefMsg->findProp(0x8502, "")
                                                    .replace(QChar('-'), "")
                                                    .replace(QChar(':'), "") ) );
          QDateTime wakeMeUpTime(
                      pureISOToLocalQDateTime( tnefMsg->findProp(0x8560, "")
                                                    .replace(QChar('-'), "")
                                                    .replace(QChar(':'), "") ) );
          vPart += "TRIGGER:PT";
          if( highNoonTime.isValid() && wakeMeUpTime.isValid() )
            vPart += QString::number( wakeMeUpTime.secsTo( highNoonTime ) / 60 );
          else
            vPart += "15"; // default: wake them up 15 minutes before the appointment
          vPart += "M\n";

          // sorry: the different action types are not known (yet)
          //        so we allways set 'DISPLAY' (no sounds, no images...)
          vPart += "ACTION:DISPLAY\n";
          vPart += "DESCRIPTION:";
          vPart += i18n("Reminder");
          vPart += '\n';
          vPart += "END:VALARM\n";
        }
        vPart += "END:VEVENT\n";
        vPart += "END:CALENDAR\n";
        bOk = true;
        // we finished composing a vCal

      }else if( bCompatClassNote || "IPM.CONTACT" == msgClass ){

        vPart =  stringProp(tnefMsg, "\n","UID", attMSGID, "" );
        vPart += stringProp(tnefMsg, "\n","FN", MAPI_TAG_PR_DISPLAY_NAME, "" );
        vPart += sNamedProp(tnefMsg, "\n","EMAIL", MAPI_TAG_CONTACT_EMAIL1EMAILADDRESS, "" );
        vPart += sNamedProp(tnefMsg, "\n","EMAIL", MAPI_TAG_CONTACT_EMAIL2EMAILADDRESS, "" );
        vPart += sNamedProp(tnefMsg, "\n","EMAIL", MAPI_TAG_CONTACT_EMAIL3EMAILADDRESS, "" );
        vPart += sNamedProp(tnefMsg, "\n","X-KADDRESSBOOK-X-IMAddress", MAPI_TAG_CONTACT_IMADDRESS, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-SpousesName", MAPI_TAG_PR_SPOUSE_NAME, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-ManagersName", MAPI_TAG_PR_MANAGER_NAME, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-AssistantsName", MAPI_TAG_PR_ASSISTANT, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-Department", MAPI_TAG_PR_DEPARTMENT_NAME, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-Office", MAPI_TAG_PR_OFFICE_LOCATION, "" );
        vPart += stringProp(tnefMsg, "\n","X-KADDRESSBOOK-X-Profession", MAPI_TAG_PR_PROFESSION, "" );
        QString s( tnefMsg->findProp( MAPI_TAG_PR_WEDDING_ANNIVERSARY, "")
                      .replace(QChar('-'), "")
                      .replace(QChar(':'), "") );
        if( !s.isEmpty() ){
          vPart += "\nX-KADDRESSBOOK-X-Anniversary:";
          vPart += s;
        }
        vPart += sNamedProp(tnefMsg, "\n","URL", MAPI_TAG_CONTACT_WEBPAGE, "" );
        // collect parts of Name entry
        s = stringProp(tnefMsg, "","", MAPI_TAG_PR_SURNAME, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_GIVEN_NAME, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_MIDDLE_NAME, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_DISPLAY_NAME_PREFIX, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_GENERATION, "" );
        if( s != ";;;;" ){
          vPart += "\nN:";
          vPart += s;
        }
        vPart += stringProp(tnefMsg, "\n","NICKNAME", MAPI_TAG_PR_NICKNAME, "" );
        vPart += stringProp(tnefMsg, "\n","ROLE", MAPI_TAG_PR_TITLE, "" );
        vPart += stringProp(tnefMsg, "\n","ORG", MAPI_TAG_PR_COMPANY_NAME, "" );
        /*
        the MAPI property ID of this (multiline) )field is unknown:
        vPart += stringProp(tnefMsg, "\n","NOTE", ... , "" );
        */

        s = stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_PO_BOX, "" );
        s += ";";
        //s += stringProp(tnefMsg, "","", don't know, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_STREET, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_CITY, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_STATE_OR_PROVINCE, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_POSTAL_CODE, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_HOME_ADDRESS_COUNTRY, "" );
        // note: If no HOME address properties were found
        //       we use the POSTAL address as home address.
        if( s == ";;;;;;" ){
          s = stringProp(tnefMsg, "","", MAPI_TAG_PR_PO_BOX, "" );
          s += ";";
          //s += stringProp(tnefMsg, "","", don't know, "" );
          s += ";";
          s += stringProp(tnefMsg, "","", MAPI_TAG_PR_STREET_ADDRESS, "" );
          s += ";";
          s += stringProp(tnefMsg, "","", MAPI_TAG_PR_LOCALITY, "" );
          s += ";";
          s += stringProp(tnefMsg, "","", MAPI_TAG_PR_STATE_OR_PROVINCE, "" );
          s += ";";
          s += stringProp(tnefMsg, "","", MAPI_TAG_PR_POSTAL_CODE, "" );
          s += ";";
          s += stringProp(tnefMsg, "","", MAPI_TAG_PR_STATE_OR_PROVINCE, "" );
        }
        if( s != ";;;;;;" ){
          vPart += "\nADR;TYPE=home:";
          vPart += s;
        }
        s = sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSPOBOX, "" );
        s += ";";
        //s += sNamedProp(tnefMsg, "","", don't know, "" );
        s += ";";
        s += sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSSTREET, "" );
        s += ";";
        s += sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSCITY, "" );
        s += ";";
        s += sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSSTATE, "" );
        s += ";";
        s += sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSPOSTALCODE, "" );
        s += ";";
        s += sNamedProp(tnefMsg, "","", MAPI_TAG_CONTACT_BUSINESSADDRESSCOUNTRY, "" );
        if( s != ";;;;;;" ){
          vPart += "\nADR;TYPE=work:";
          vPart += s;
        }
        s = stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_PO_BOX, "" );
        s += ";";
        //s += stringProp(tnefMsg, "","", don't know, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_STREET, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_CITY, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_STATE_OR_PROVINCE, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_POSTAL_CODE, "" );
        s += ";";
        s += stringProp(tnefMsg, "","", MAPI_TAG_PR_OTHER_ADDRESS_COUNTRY, "" );
        if( s != ";;;;;;" ){
          vPart += "\nADR;TYPE=dom:";
          vPart += s;
        }
        // problem: the 'other' address was stored by KOrganizer in
        //          a line looking like the following one:
        // vPart += "\nADR;TYPE=dom;TYPE=intl;TYPE=parcel;TYPE=postal;TYPE=work;TYPE=home:other_pobox;;other_str1\nother_str2;other_loc;other_region;other_pocode;other_country

        vPart += stringProp(tnefMsg, "\n","TEL;TYPE=home", MAPI_TAG_PR_HOME_TELEPHONE_NUMBER, "" );
        vPart += stringProp(tnefMsg, "\n","TEL;TYPE=work", MAPI_TAG_PR_BUSINESS_TELEPHONE_NUMBER, "" );
        vPart += stringProp(tnefMsg, "\n","TEL;TYPE=cell", MAPI_TAG_PR_MOBILE_TELEPHONE_NUMBER, "" );
        vPart += stringProp(tnefMsg, "\n","TEL;TYPE=home;TYPE=fax", MAPI_TAG_PR_HOME_FAX_NUMBER, "" );
        vPart += stringProp(tnefMsg, "\n","TEL;TYPE=work;TYPE=fax", MAPI_TAG_PR_BUSINESS_FAX_NUMBER, "" );
        s = tnefMsg->findProp( MAPI_TAG_PR_BIRTHDAY, "")
                      .replace(QChar('-'), "")
                      .replace(QChar(':'), "");
        if( !s.isEmpty() ){
          vPart += "\nBDAY:";
          vPart += s;
        }
        // add the vPart's header and footer
        if( !vPart.isEmpty() ){
          vPart.prepend("BEGIN:VCARD"
                        "\nVERSION:3.0"  );
          vPart.append( "\nCLASS:PRIVATE"
                        "\nEND:VCARD"    );
          bOk = true;
        }
      }else if( "IPM.NOTE" == msgClass ){

      } // else if ... and so on ...
    }
  }

  return bOk;
}


//-----------------------------------------------------------------------------
bool KMGroupware::msTNEFToHTML( KMReaderWin* reader, QString& vPart, QString fname,
                                bool useGroupware, QString& prefix, QString& postfix )
{
  QByteArray tnef( kFileToBytes( fname, false ) );
  if( tnef.count() ) {
    int updateCounter;
    if( msTNEFToVPart( tnef, updateCounter, vPart ) ){
      QByteArray theBody( vPart.utf8() );
      QString fname2( ObjectTreeParser::byteArrayToTempFile( reader,
                                                        "groupware",
                                                        "vPart_decoded.raw",
                                                        theBody ) );
      if( !fname2.isEmpty() )
        return vPartToHTML( updateCounter, vPart, fname2,
			    useGroupware, prefix, postfix );
    }
  }else{
    KMessageBox::error(0, i18n("<qt>Unable to open file <b>%1</b>.</qt>").arg(fname));
  }
  return false;
}


//-----------------------------------------------------------------------------
bool KMGroupware::foundGroupwareLink( const QString aUrl, QString& gwType, QString& gwAction,
                                      QString& gwAction2, QString& gwData )
{
  static QString gwPrefix("groupware_");
  gwType    = "";
  gwAction  = "";
  gwAction2 = "";
  gwData    = "";

  int i1 = aUrl.find( gwPrefix );
  if( -1 < i1 ) {
    i1 += gwPrefix.length();

    int i2 = aUrl.find("_", i1);
    if( i1 <= i2 )
    {
      // retrieve gwType
      gwType = aUrl.mid( i1, i2-i1 );
      i1 = i2+1;
      i2 = aUrl.find("_", i1);
      if( i1 <= i2 )
      {
        // retrieve gwAction
        gwAction = aUrl.mid( i1, i2-i1 );
        i1 = i2+1;
        i2 = aUrl.find("#", i1);
        if( i1 <= i2 )
        {
          // retrieve gwAction2
          gwAction2 = aUrl.mid( i1, i2-i1 );
          i2 += 1;
          // retrieve gwData
          gwData = aUrl.mid( i2 );
        }
      }
    }
  }
  return !gwType.isEmpty();
}


bool KMGroupware::handleLink( const KURL &aUrl, KMMessage* msg )
{
  QString gwType, gwAction, gwAction2, gwData;

  if( !aUrl.hasRef() || !foundGroupwareLink( aUrl.path()+"#"+aUrl.ref(), gwType,
					     gwAction, gwAction2, gwData ) )
    // No groupware link to handle here
    return false;

  if( gwType != "vCal" || gwData.isEmpty()
      || ( "request" != gwAction && "reply" != gwAction && "cancel" != gwAction ) )
    // Then we can't handle it. But it is a groupware link, so we return true
    return true;

  // Read the vCal
  QFile file( gwData );
  if( !file.open( IO_ReadOnly ) ) {
    kdDebug(5006) << "Could not open file " << gwData << endl;
    return true;
  }
  QTextStream ts( &file );
  ts.setEncoding( QTextStream::UnicodeUTF8 );
  QString vCal = ts.read();
  file.close();

  // Find the receiver if we can
  QString receiver;
  if( msg ) {
    KMIdentity ident = kernel->identityManager()->identityForAddress( msg->to() );
    if( ident != KMIdentity::null ) {
      receiver = ident.emailAddr();
    } else {
      QStringList addrs = KMMessage::splitEmailAddrList( msg->to() );
      bool ok;
      receiver = QInputDialog::getItem( i18n("Select Address"), 
					i18n("None of your identities match the receiver "
					     "of this message,<br> please choose which of "
					     "the following addresses is yours:"), 
					addrs, 0, FALSE, &ok, mMainWin );
      if( !ok ) return false;
    }
  }

  // Find the sender if we can
  QCString sender = KMMessage::getEmailAddr( msg->from() );

  if( "request" == gwAction )
    processVCalRequest( receiver.utf8(), vCal, gwAction2 );
  else if( "reply" == gwAction )
    processVCalReply( sender, vCal, gwAction2 );
  else if( "cancel" == gwAction )
    /* Note, we pass gwAction here, not gwAction2 */
    processVCalReply( sender, vCal, gwAction );

  return true;
}


/*!
  This method handles incoming resource requests. It sends them off to
  KOrganizer for answering, records the result and sends an answer
  back.
*/
bool KMGroupware::incomingResourceMessage( KMAccount* acct, KMMessage* msg )
{
  if( !mUseGroupware)
    return false;

  int updateCounter;
  QString vCalIn;
  if( vPartFoundAndDecoded( msg, updateCounter, vCalIn ) )
    return false;

  bool vCalInOK, vCalOutOK, isFree;
  QString vCalOut;
  QDateTime start, end;
  emit( signalResourceRequest( acct->intervals(), KMMessage::getEmailAddr( msg->to() ),
			       vCalIn, vCalInOK, vCalOut, vCalOutOK, isFree, start, end ) );
  if( !vCalInOK || !vCalOutOK )
    return false; // parsing or generation error somewhere

  // Check whether we are supposed to answer automatically at all
  KConfigGroup options( KMKernel::config(), "Groupware" );
  if( isFree && options.readBoolEntry( "AutoAccept", false ) )
    return false;
  if( !isFree && options.readBoolEntry( "AutoDeclConflict", false ) )
    return false;

  // Everything went fine so far, now attach the answer
  KMMessage* msgNew = 0;
  if( msg ){
    msgNew = msg->createReply( false, false, vCalOut, false, true, TRUE );
    msgNew->setType( DwMime::kTypeText );
    msgNew->setSubtype( DwMime::kSubtypeVCal );
    msgNew->setHeaderField("Content-Type", "text/calendar; method=REPLY; charset=\"utf-8\"");
    internal_directlySendMessage( msgNew );
  }

  // And also record in the account.
  acct->addInterval( qMakePair( start, end ) );

  return true;
}

/*!
  This method checks whether the folder is one of Calendar, Notes, and
  Tasks and informs KOrganizer accordingly about the deleted object.
*/
void KMGroupware::msgRemoved( KMFolder* folder, KMMessage* msg )
{
  assert( msg );
  assert( msg->isMessage() );

  int iDummy;
  QString vCal;

  // Let's try for a note
  QString noteId = msg->headerField( "X-KOrg-Note-Id" );
  if( !noteId.isEmpty() ) {
    kdDebug(5006) << "%%% Deleting note with id: " << noteId << endl;
    emit signalNoteDeleted( noteId );
  } if( vPartFoundAndDecoded( msg, iDummy, vCal ) ) {
    QString uid( "UID" );
    vPartMicroParser( vCal.utf8(), uid );
    if( !uid.isEmpty() ){
      // We have found something with an UID, now tell KOrganizer if
      // this was a relevant folder.
      if( folder == mCalendar )
	emit signalEventDeleted( uid );
      else if( folder == mTasks )
	emit signalTaskDeleted( uid );
    }
  } else
    kdDebug(5006) << "%%% Unknown groupware deletion\n";
}


void KMGroupware::loadPixmaps() const
{
  static bool pixmapsLoaded = false;

  if( mUseGroupware && !pixmapsLoaded ) {
    pixmapsLoaded = true;
    pixContacts = new QPixmap( UserIcon("kmgroupware_folder_contacts"));
    pixCalendar = new QPixmap( UserIcon("kmgroupware_folder_calendar"));
    pixNotes    = new QPixmap( UserIcon("kmgroupware_folder_notes"));
    pixTasks    = new QPixmap( UserIcon("kmgroupware_folder_tasks"));
  }
}


#include "kmgroupware.moc"
