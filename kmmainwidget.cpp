// -*- mode: C++; c-file-style: "gnu" -*-
// kmmainwidget.cpp
//#define MALLOC_DEBUG 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <config-kmail.h>
#include <kicon.h>
#include <kwin.h>
#include <krun.h>
#ifdef MALLOC_DEBUG
#include <malloc.h>
#endif

#undef Unsorted // X headers...
#include <q3accel.h>
#include <QLayout>


#include <QLabel>
#include <QList>
#include <QVBoxLayout>
#include <QByteArray>
#include <QToolBar>
//Added by qt3to4:
#include <q3popupmenu.h>
#include <q3ptrlist.h>

#include <kmessagebox.h>

#include <kactionmenu.h>
#include <kmenu.h>
#include <kacceleratormanager.h>
#include <kglobalsettings.h>
#include <kstandardshortcut.h>
#include <kshortcutsdialog.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <ktip.h>
#include <kstandarddirs.h>
#include <kstandardaction.h>
#include <kaddrbook.h>
#include <ktoggleaction.h>
#include <knotification.h>
#include <knotifyconfigwidget.h>
#include <kstringhandler.h>
#include <kconfiggroup.h>

#include <qsignalmapper.h>

#include "globalsettings.h"
#include "kcursorsaver.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldermgr.h"
#include "kmfolderdia.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmfilter.h"
#include "kmfoldertree.h"
#include "kmreadermainwin.h"
#include "kmfoldercachedimap.h"
#include "kmfolderimap.h"
#include "kmacctcachedimap.h"
#include "composer.h"
#include "kmfolderseldlg.h"
#include "kmfiltermgr.h"
#include "messagesender.h"
#include "kmaddrbook.h"
#include "kmversion.h"
#include "searchwindow.h"
using KMail::SearchWindow;
#include "kmacctfolder.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "imapaccountbase.h"
#include "transportmanager.h"
using KMail::ImapAccountBase;
#include "vacation.h"
using KMail::Vacation;

#include "subscriptiondialog.h"
using KMail::SubscriptionDialog;
#include "localsubscriptiondialog.h"
using KMail::LocalSubscriptionDialog;
#include "attachmentstrategy.h"
using KMail::AttachmentStrategy;
#include "headerstrategy.h"
using KMail::HeaderStrategy;
#include "headerstyle.h"
using KMail::HeaderStyle;
#include "folderjob.h"
using KMail::FolderJob;
#include "mailinglist-magic.h"
#include "antispamwizard.h"
using KMail::AntiSpamWizard;
#include "filterlogdlg.h"
using KMail::FilterLogDialog;
#include <headerlistquicksearch.h>
using KMail::HeaderListQuickSearch;
#include "kmheaders.h"
#include "mailinglistpropertiesdialog.h"
#include "templateparser.h"
using KMail::TemplateParser;

#if !defined(NDEBUG)
    #include "sievedebugdialog.h"
    using KMail::SieveDebugDialog;
#endif

#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>

#include <ktoolinvocation.h>
#include <kmenu.h>
#include <kxmlguifactory.h>

#include <QSplitter>

#include <assert.h>
#include <kstatusbar.h>
#include <kstaticdeleter.h>
#include <kaction.h>

#include <kmime/kmime_mdn.h>
#include <kmime/kmime_header_parsing.h>
using namespace KMime;
using KMime::Types::AddrSpecList;

#include "progressmanager.h"
using KPIM::ProgressManager;

#include "managesievescriptsdialog.h"
#include <q3stylesheet.h>
#include <kvbox.h>
#include <QTextDocument>
#include <QDBusConnection>
#include <QDBusMessage>
#include "customtemplates.h"
#include "customtemplates_kfg.h"

#include "kmmainwidget.moc"

QList<KMMainWidget*>* KMMainWidget::s_mainWidgetList = 0;
static KStaticDeleter<QList<KMMainWidget*> > mwlsd;

//-----------------------------------------------------------------------------
KMMainWidget::KMMainWidget(QWidget *parent, const char *name,
                           KXMLGUIClient *aGUIClient,
                           KActionCollection *actionCollection, KConfig* config ) :
    QWidget(parent),
    mQuickSearchLine( 0 ),
    mShowBusySplashTimer( 0 ),
    mShowingOfflineScreen( false ),
    mAccel( 0 )
{
  // must be the first line of the constructor:
  mStartupDone = false;
  mSearchWin = 0;
  mIntegrated  = true;
  mFolder = 0;
  mTemplateFolder = 0;
  mFolderThreadPref = false;
  mFolderThreadSubjPref = true;
  mReaderWindowActive = true;
  mReaderWindowBelow = true;
  mFolderHtmlPref = false;
  mFolderHtmlLoadExtPref = false;
  mSystemTray = 0;
  mDestructed = false;
  mActionCollection = actionCollection;
  mTopLayout = new QVBoxLayout(this);
  mFolderShortcutCommands.setAutoDelete(true);
  mJob = 0;
  mConfig = config;
  mGUIClient = aGUIClient;
  mOpenedImapFolder = false;

  mCustomReplyActionMenu = 0;
  mCustomReplyAllActionMenu = 0;
  mCustomForwardActionMenu = 0;
  mCustomReplyMapper = 0;
  mCustomReplyAllMapper = 0;
  mCustomForwardMapper = 0;

  setObjectName( name );
  // FIXME This should become a line separator as soon as the API
  // is extended in kdelibs.
  mToolbarActionSeparator = new QAction( this );
  mToolbarActionSeparator->setSeparator( true );

  if( !s_mainWidgetList )
    mwlsd.setObject( s_mainWidgetList, new QList<KMMainWidget*>() );
  s_mainWidgetList->append( this );

  mPanner1Sep << 1 << 1;
  mPanner2Sep << 1 << 1;

  setMinimumSize(400, 300);

  readPreConfig();
  createWidgets();

  setupActions();

  readConfig();

  activatePanners();

  QTimer::singleShot( 0, this, SLOT( slotShowStartupFolder() ));

  connect( kmkernel->acctMgr(), SIGNAL( checkedMail( bool, bool, const QMap<QString, int> & ) ),
           this, SLOT( slotMailChecked( bool, bool, const QMap<QString, int> & ) ) );

  connect( kmkernel->acctMgr(), SIGNAL( accountAdded( KMAccount* ) ),
           this, SLOT( initializeIMAPActions() ) );
  connect( kmkernel->acctMgr(), SIGNAL( accountRemoved( KMAccount* ) ),
           this, SLOT( initializeIMAPActions() ) );

  connect(kmkernel, SIGNAL( configChanged() ),
          this, SLOT( slotConfigChanged() ));

  // display the full path to the folder in the caption
  connect(mFolderTree, SIGNAL(currentChanged(Q3ListViewItem*)),
      this, SLOT(slotChangeCaption(Q3ListViewItem*)));
  connect(mFolderTree, SIGNAL(selectionChanged()),
          SLOT(updateFolderMenu()) );

  connect(kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->searchFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect( kmkernel, SIGNAL( onlineStatusChanged( GlobalSettings::EnumNetworkState::type ) ),
           this, SLOT( slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type ) ) );

  toggleSystemTray();

  // must be the last line of the constructor:
  mStartupDone = true;
}


//-----------------------------------------------------------------------------
//The kernel may have already been deleted when this method is called,
//perform all cleanup that requires the kernel in destruct()
KMMainWidget::~KMMainWidget()
{
  s_mainWidgetList->removeAll( this );
  qDeleteAll( mFilterCommands );
  destruct();
}


//-----------------------------------------------------------------------------
//This method performs all cleanup that requires the kernel to exist.
void KMMainWidget::destruct()
{
  if (mDestructed)
    return;
  if (mSearchWin)
    mSearchWin->close();
  writeConfig();
  writeFolderConfig();
  delete mHeaders;
  delete mFolderTree;
  delete mSystemTray;
  delete mMsgView;
  mDestructed = true;
}


//-----------------------------------------------------------------------------
void KMMainWidget::readPreConfig(void)
{
  const KConfigGroup geometry( KMKernel::config(), "Geometry" );
  const KConfigGroup general( KMKernel::config(), "General" );
  const KConfigGroup reader( KMKernel::config(), "Reader" );

  mLongFolderList = geometry.readEntry( "FolderList", "long" ) != "short";
  mReaderWindowActive = geometry.readEntry( "readerWindowMode", "below" ) != "hide";
  mReaderWindowBelow = geometry.readEntry( "readerWindowMode", "below" ) == "below";
  mThreadPref = geometry.readEntry( "nestedMessages", false );

  mHtmlPref = reader.readEntry( "htmlMail", false );
  mHtmlLoadExtPref = reader.readEntry( "htmlLoadExternal", false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::readFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + mFolder->idString());
  mFolderThreadPref =
      group.readEntry( "threadMessagesOverride", false );
  mFolderThreadSubjPref =
      group.readEntry( "threadMessagesBySubject", true );
  mFolderHtmlPref =
      group.readEntry( "htmlMailOverride", false );
  mFolderHtmlLoadExtPref =
      group.readEntry( "htmlLoadExternalOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + mFolder->idString());
  group.writeEntry( "threadMessagesOverride", mFolderThreadPref );
  group.writeEntry( "threadMessagesBySubject", mFolderThreadSubjPref );
  group.writeEntry( "htmlMailOverride", mFolderHtmlPref );
  group.writeEntry( "htmlLoadExternalOverride", mFolderHtmlLoadExtPref );
}


//-----------------------------------------------------------------------------
void KMMainWidget::readConfig(void)
{
  KConfig *config = KMKernel::config();

  bool oldLongFolderList =  mLongFolderList;
  bool oldReaderWindowActive = mReaderWindowActive;
  bool oldReaderWindowBelow = mReaderWindowBelow;

  QString str;
  QSize siz;

  if (mStartupDone)
  {
    writeConfig();

    readPreConfig();
    mHeaders->refreshNestedState();

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );


    if( layoutChanged ) {
      hide();
      // delete all panners
      delete mPanner1; // will always delete the others
      createWidgets();
    }

  }

  { // area for config group "Geometry"
    KConfigGroup group(config, "Geometry");
    // size of the mainwin
    QSize defaultSize(750,560);
    siz = group.readEntry("MainWin", defaultSize );
    if (!siz.isEmpty())
      resize(siz);
    // default width of the foldertree
    static const int folderpanewidth = 250;

    const int folderW = group.readEntry( "FolderPaneWidth", folderpanewidth );
    const int headerW = group.readEntry( "HeaderPaneWidth", width()-folderpanewidth );
    const int headerH = group.readEntry( "HeaderPaneHeight", 180 );
    const int readerH = group.readEntry( "ReaderPaneHeight", 280 );

    mPanner1Sep.clear();
    mPanner2Sep.clear();
    QList<int> & widths = mLongFolderList ? mPanner1Sep : mPanner2Sep ;
    QList<int> & heights = mLongFolderList ? mPanner2Sep : mPanner1Sep ;

    widths << folderW << headerW;
    heights << headerH << readerH;

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );

    if (!mStartupDone || layoutChanged )
    {
      /** unread / total columns
       * as we have some dependencies in this widget
       * it's better to manage these here */
      // The columns are shown by default.

      const int unreadColumn =
          group.readEntry( "UnreadColumn", 1 );
      const int totalColumn =
          group.readEntry( "TotalColumn", 2 );

      /* we need to _activate_ them in the correct order
      * this is ugly because we can't use header()->moveSection
      * but otherwise the restoreLayout from KMFolderTree
      * doesn't know that to do */
      if (unreadColumn != -1 && unreadColumn < totalColumn)
        mFolderTree->addUnreadColumn( i18n("Unread"), 70 );
      if (totalColumn != -1)
        mFolderTree->addTotalColumn( i18n("Total"), 70 );
      if (unreadColumn != -1 && unreadColumn > totalColumn)
        mFolderTree->addUnreadColumn( i18n("Unread"), 70 );
      mUnreadColumnToggle->setChecked( mFolderTree->isUnreadActive() );
      mUnreadTextToggle->setChecked( !mFolderTree->isUnreadActive() );
      mTotalColumnToggle->setChecked( mFolderTree->isTotalActive() );

      mFolderTree->updatePopup();
    }
  }

  if (mMsgView)
    mMsgView->readConfig();

  mHeaders->readConfig();
  mHeaders->restoreLayout(KMKernel::config(), "Header-Geometry");

  mFolderTree->readConfig();

  { // area for config group "General"
    KConfigGroup group(config, "General");
    mBeepOnNew = group.readEntry("beep-on-mail", false );
    mConfirmEmpty = group.readEntry("confirm-before-empty", true );
    // startup-Folder, defaults to system-inbox
	mStartupFolder = group.readEntry("startupFolder", kmkernel->inboxFolder()->idString());
    if (!mStartupDone)
    {
      // check mail on startup
      if ( group.readEntry("checkmail-startup", false ) )
        // do it after building the kmmainwin, so that the progressdialog is available
        QTimer::singleShot( 0, this, SLOT( slotCheckMail() ) );
    }
  }

  // reload foldertree
  mFolderTree->reload();

  // Re-activate panners
  if (mStartupDone)
  {
    // Update systray
    toggleSystemTray();

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );
    if ( layoutChanged ) {
      activatePanners();
    }

    mFolderTree->showFolder( mFolder );

    // sanders - New code
    mHeaders->setFolder(mFolder);
    if (mMsgView) {
      int aIdx = mHeaders->currentItemIndex();
      if (aIdx != -1)
        mMsgView->setMsg( mFolder->getMsg(aIdx), true );
      else
        mMsgView->clear( true );
    }
    updateMessageActions();
    show();
    // sanders - Maybe this fixes a bug?

  }
  updateMessageMenu();
  updateFileMenu();
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeConfig(void)
{
  QString s;
  KConfig *config = KMKernel::config();
  KConfigGroup geometry( config, "Geometry" );
  KConfigGroup general( config, "General" );

  if (mMsgView)
    mMsgView->writeConfig();

  mFolderTree->writeConfig();

  geometry.writeEntry( "MainWin", this->geometry().size() );

  const QList<int> widths = ( mLongFolderList ? mPanner1 : mPanner2 )->sizes();
  const QList<int> heights = ( mLongFolderList ? mPanner2 : mPanner1 )->sizes();

  geometry.writeEntry( "FolderPaneWidth", widths[0] );
  geometry.writeEntry( "HeaderPaneWidth", widths[1] );

  // Only save when the widget is shown (to avoid saving a wrong value)
  if ( mSearchAndHeaders && !mSearchAndHeaders->isHidden() ) {
    geometry.writeEntry( "HeaderPaneHeight", heights[0] );
    if (mReaderWindowActive) {
      geometry.writeEntry( "ReaderPaneHeight", heights[1] );
    }
  }

  // save the state of the unread/total-columns
  geometry.writeEntry( "UnreadColumn", mFolderTree->unreadIndex() );
  geometry.writeEntry( "TotalColumn", mFolderTree->totalIndex() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::createWidgets(void)
{
  mAccel = new Q3Accel(this, "createWidgets()");

  // Create the splitters according to the layout settings
  QWidget *headerParent = 0, *folderParent = 0,
            *mimeParent = 0, *messageParent = 0;

  const bool opaqueResize = KGlobalSettings::opaqueResize();
  if ( mLongFolderList ) {
    // superior splitter: folder tree vs. rest
    // inferior splitter: headers vs. message vs. mime tree
    mPanner1 = new QSplitter( Qt::Horizontal, this );
    mPanner1->setObjectName( "panner 1" );
    mPanner1->setOpaqueResize( opaqueResize );
    Qt::Orientation orientation = mReaderWindowBelow ? Qt::Vertical : Qt::Horizontal;
    mPanner2 = new QSplitter( orientation, mPanner1 );
    mPanner2->setObjectName( "panner 2" );
    mPanner2->setOpaqueResize( opaqueResize );
    folderParent = mPanner1;
    headerParent = mimeParent = messageParent = mPanner2;
  } else /* !mLongFolderList */ {
    // superior splitter: ( folder tree + headers ) vs. message vs. mime
    // inferior splitter: folder tree vs. headers
    mPanner1 = new QSplitter( Qt::Vertical, this );
    mPanner1->setObjectName( "panner 1" );
    mPanner1->setOpaqueResize( opaqueResize );
    mPanner2 = new QSplitter( Qt::Horizontal, mPanner1 );
    mPanner2->setObjectName( "panner 2" );
    mPanner2->setOpaqueResize( opaqueResize );
    headerParent = folderParent = mPanner2;
    mimeParent = messageParent = mPanner1;
  }

#ifndef NDEBUG
  mPanner1->dumpObjectTree();
  mPanner2->dumpObjectTree();
#endif

  mTopLayout->setMargin (0);
  mTopLayout->addWidget( mPanner1 );

  // BUG -sanders these accelerators stop working after switching
  // between long/short folder layout
  // Probably need to disconnect them first.

  // create list of messages
#ifndef NDEBUG
  headerParent->dumpObjectTree();
#endif
  mSearchAndHeaders = new KVBox( headerParent );
  mSearchToolBar = new QToolBar( mSearchAndHeaders);
  mSearchToolBar->setObjectName( "search toolbar" );
  mSearchToolBar->setMovable( false );
  mSearchToolBar->layout()->setSpacing( KDialog::spacingHint() );

  mHeaders = new KMHeaders( this, mSearchAndHeaders );
  mHeaders->setObjectName( "headers" );
  mQuickSearchLine = new HeaderListQuickSearch( mSearchToolBar, mHeaders,
                                                actionCollection() );
  mQuickSearchLine->setObjectName( "headers quick search line" );

  mSearchToolBar->addWidget( mQuickSearchLine );
    connect( mHeaders, SIGNAL( messageListUpdated() ),
           mQuickSearchLine, SLOT( updateSearch() ) );
  if ( !GlobalSettings::self()->quickSearchActive() ) mSearchToolBar->hide();

  if (mReaderWindowActive) {
    connect(mHeaders, SIGNAL(selected(KMMessage*)),
            this, SLOT(slotMsgSelected(KMMessage*)));
  }
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
          this, SLOT(slotMsgActivated(KMMessage*)));
  connect( mHeaders, SIGNAL( selectionChanged() ),
           SLOT( startUpdateMessageActionsTimer() ) );
  mAccel->connectItem(mAccel->insertItem(Qt::SHIFT+Qt::Key_Left),
                     mHeaders, SLOT(selectPrevMessage()));
  mAccel->connectItem(mAccel->insertItem(Qt::SHIFT+Qt::Key_Right),
                     mHeaders, SLOT(selectNextMessage()));

  if (mReaderWindowActive) {
    mMsgView = new KMReaderWin(messageParent, this, actionCollection(), 0 );

    connect(mMsgView, SIGNAL(replaceMsgByUnencryptedVersion()),
        this, SLOT(slotReplaceMsgByUnencryptedVersion()));
    connect(mMsgView, SIGNAL(popupMenu(KMMessage&,const KUrl&,const QPoint&)),
        this, SLOT(slotMsgPopup(KMMessage&,const KUrl&,const QPoint&)));
    connect(mMsgView, SIGNAL(urlClicked(const KUrl&,int)),
        mMsgView, SLOT(slotUrlClicked()));
    connect(mHeaders, SIGNAL(maybeDeleting()),
        mMsgView, SLOT(clearCache()));
    connect(mMsgView, SIGNAL(noDrag()),
        mHeaders, SLOT(slotNoDrag()));
    mAccel->connectItem(mAccel->insertItem(Qt::Key_Up),
        mMsgView, SLOT(slotScrollUp()));
    mAccel->connectItem(mAccel->insertItem(Qt::Key_Down),
        mMsgView, SLOT(slotScrollDown()));
    mAccel->connectItem(mAccel->insertItem(Qt::Key_PageUp),
        mMsgView, SLOT(slotScrollPrior()));
    mAccel->connectItem(mAccel->insertItem(Qt::Key_PageDown),
        mMsgView, SLOT(slotScrollNext()));
  } else {
    mMsgView = NULL;
  }

  QAction *action  = new KAction(i18n("Move Message to Folder"), this);
  actionCollection()->addAction("move_message_to_folder", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotMoveMsg()));
  action->setShortcut(QKeySequence(Qt::Key_M));
  action  = new KAction(i18n("Copy Message to Folder"), this);
  actionCollection()->addAction("copy_message_to_folder", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCopyMsg()));
  action->setShortcut(QKeySequence(Qt::Key_C));
  action  = new KAction(i18n("Jump to Folder"), this);
  actionCollection()->addAction("jump_to_folder", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotJumpToFolder()));
  action->setShortcut(QKeySequence(Qt::Key_J));
  mAccel->connectItem(mAccel->insertItem(Qt::Key_M),
		     this, SLOT(slotMoveMsg()) );
  mAccel->connectItem(mAccel->insertItem(Qt::Key_C),
		     this, SLOT(slotCopyMsg()) );
  mAccel->connectItem(mAccel->insertItem(Qt::Key_J),
                      this, SLOT(slotJumpToFolder()) );

  // create list of folders
  mFolderTree = new KMFolderTree(this, folderParent);
  mFolderTree->setObjectName( "folderTree" );
  mFolderTree->setFrameStyle( QFrame::NoFrame );

  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));
  connect( mFolderTree, SIGNAL( folderSelected( KMFolder* ) ),
           mQuickSearchLine, SLOT( reset() ) );
  connect(mFolderTree, SIGNAL(folderSelectedUnread(KMFolder*)),
	  this, SLOT(folderSelectedUnread(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDrop(KMFolder*)),
	  this, SLOT(slotMoveMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDropCopy(KMFolder*)),
          this, SLOT(slotCopyMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(columnsChanged()),
          this, SLOT(slotFolderTreeColumnsChanged()));

  //Commands not worthy of menu items, but that deserve configurable keybindings
  mRemoveDuplicatesAction  = new KAction(i18n("Remove Duplicate Messages"), this);
  actionCollection()->addAction("remove_duplicate_messages", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(removeDuplicates()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Asterisk));

  action  = new KAction(i18n("Abort Current Operation"), this);
  actionCollection()->addAction("cancel", action );
  connect(action, SIGNAL(triggered(bool) ), ProgressManager::instance(), SLOT(slotAbortAll()));
  action->setShortcut(QKeySequence(Qt::Key_Escape));
  mAccel->connectItem(mAccel->insertItem(Qt::Key_Escape),
                     ProgressManager::instance(), SLOT(slotAbortAll()));

  action  = new KAction(i18n("Focus on Next Folder"), this);
  actionCollection()->addAction("inc_current_folder", action );
  connect(action, SIGNAL(triggered(bool) ), mFolderTree, SLOT(incCurrentFolder()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Right));
  mAccel->connectItem(mAccel->insertItem(Qt::CTRL+Qt::Key_Right),
                     mFolderTree, SLOT(incCurrentFolder()));

  action  = new KAction(i18n("Focus on Previous Folder"), this);
  actionCollection()->addAction("dec_current_folder", action );
  connect(action, SIGNAL(triggered(bool) ), mFolderTree, SLOT(decCurrentFolder()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Left));
  mAccel->connectItem(mAccel->insertItem(Qt::CTRL+Qt::Key_Left),
                     mFolderTree, SLOT(decCurrentFolder()));

  action  = new KAction(i18n("Select Folder with Focus"), this);
  actionCollection()->addAction("select_current_folder", action );
  connect(action, SIGNAL(triggered(bool) ), mFolderTree, SLOT(selectCurrentFolder()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Space));
  mAccel->connectItem(mAccel->insertItem(Qt::CTRL+Qt::Key_Space),
                     mFolderTree, SLOT(selectCurrentFolder()));
  action  = new KAction(i18n("Focus on Next Message"), this);
  actionCollection()->addAction("inc_current_message", action );
  connect(action, SIGNAL(triggered(bool) ), mHeaders, SLOT(incCurrentMessage()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Right));
  mAccel->connectItem( mAccel->insertItem( Qt::ALT+Qt::Key_Right ),
                        mHeaders, SLOT( incCurrentMessage() ) );

  action  = new KAction(i18n("Focus on Previous Message"), this);
  actionCollection()->addAction("dec_current_message", action );
  connect(action, SIGNAL(triggered(bool) ), mHeaders, SLOT(decCurrentMessage()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Left));
  mAccel->connectItem( mAccel->insertItem( Qt::ALT+Qt::Key_Left ),
                        mHeaders, SLOT( decCurrentMessage() ) );

  action  = new KAction(i18n("Select Message with Focus"), this);
  actionCollection()->addAction("select_current_message", action );
  connect(action, SIGNAL(triggered(bool) ), mHeaders, SLOT( selectCurrentMessage() ));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Space));
  mAccel->connectItem( mAccel->insertItem( Qt::ALT+Qt::Key_Space ),
                        mHeaders, SLOT( selectCurrentMessage() ) );

  connect( kmkernel->outboxFolder(), SIGNAL( msgRemoved(int, QString) ),
           SLOT( startUpdateMessageActionsTimer() ) );
  connect( kmkernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( startUpdateMessageActionsTimer() ) );
}


//-----------------------------------------------------------------------------
void KMMainWidget::activatePanners(void)
{
  if (mMsgView) {
    QObject::disconnect( mMsgView->copyAction(),
        SIGNAL( activated() ),
        mMsgView, SLOT( slotCopySelectedText() ));
  }
  if ( mLongFolderList ) {
    mSearchAndHeaders->setParent( mPanner2 );
    if (mMsgView) {
      mMsgView->setParent( mPanner2 );
      mPanner2->addWidget( mMsgView );
    }
    mFolderTree->setParent( mPanner1 );
    mPanner1->addWidget( mPanner2 );
    mPanner1->setSizes( mPanner1Sep );
    mPanner1->setStretchFactor( mPanner1->indexOf(mFolderTree), 0 );
    mPanner2->setSizes( mPanner2Sep );
    mPanner2->setStretchFactor( mPanner2->indexOf(mSearchAndHeaders), 0 );
  } else /* !mLongFolderList */ {
    mFolderTree->setParent( mPanner2 );
    mSearchAndHeaders->setParent( mPanner2 );
    mPanner2->addWidget( mSearchAndHeaders );
    mPanner1->insertWidget( 0, mPanner2 );
    if (mMsgView) {
      mMsgView->setParent( mPanner1 );
      mPanner1->addWidget( mMsgView );
    }
    mPanner1->setSizes( mPanner1Sep );
    mPanner2->setSizes( mPanner2Sep );
    mPanner1->setStretchFactor( mPanner1->indexOf(mPanner2), 0 );
    mPanner2->setStretchFactor( mPanner2->indexOf(mFolderTree), 0 );
  }

  if (mMsgView) {
    QObject::connect( mMsgView->copyAction(),
		    SIGNAL( activated() ),
		    mMsgView, SLOT( slotCopySelectedText() ));
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::hide()
{
  QWidget::hide();
}


//-----------------------------------------------------------------------------
void KMMainWidget::show()
{
  QWidget::show();
}

//-------------------------------------------------------------------------
void KMMainWidget::slotSearch()
{
  if(!mSearchWin)
  {
    mSearchWin = new SearchWindow(this, mFolder);
    mSearchWin->setModal( false );
    mSearchWin->setObjectName( "Search" );
    connect(mSearchWin, SIGNAL(destroyed()),
	    this, SLOT(slotSearchClosed()));
  }
  else
  {
    mSearchWin->activateFolder(mFolder);
  }

  mSearchWin->show();
#ifdef Q_OS_UNIX
  KWin::activateWindow( mSearchWin->winId() );
#endif
}


//-------------------------------------------------------------------------
void KMMainWidget::slotSearchClosed()
{
  mSearchWin = 0;
}


//-------------------------------------------------------------------------
void KMMainWidget::slotFind()
{
  if( mMsgView )
    mMsgView->slotFind();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotHelp()
{
  KToolInvocation::invokeHelp();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotFilter()
{
  kmkernel->filterMgr()->openDialog( this );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPopFilter()
{
  kmkernel->popFilterMgr()->openDialog( this );
}

void KMMainWidget::slotManageSieveScripts()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }
  KMail::ManageSieveScriptsDialog * dlg = new KMail::ManageSieveScriptsDialog( this );
  dlg->show();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotAddrBook()
{
  KAddrBookExternal::openAddressBook(this);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotImport()
{
  KRun::runCommand("kmailcvt");
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckMail()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }
  kmkernel->acctMgr()->checkMail(true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckOneAccount(QAction* item)
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  KMAccount* t = kmkernel->acctMgr()->findByName(item->text());

  if (!t)
    kmkernel->acctMgr()->singleCheckMail(t);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailChecked( bool newMail, bool sendOnCheck,
                                    const QMap<QString, int> & newInFolder )
{
  const bool sendOnAll =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnAllChecks;
  const bool sendOnManual =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnManualChecks;
  if( sendOnAll || (sendOnManual && sendOnCheck ) )
    slotSendQueued();

  if ( !newMail || newInFolder.isEmpty() )
    return;

  QDBusMessage message =
      QDBusMessage::createSignal("/KMail", "org.kde.kmail.kmail", "unreadCountChanged");
  QDBusConnection::sessionBus().send(message);

  // build summary for new mail message
  bool showNotification = false;
  QString summary;
  QStringList keys( newInFolder.keys() );
  keys.sort();
  for ( QStringList::const_iterator it = keys.begin();
        it != keys.end();
        ++it ) {
    kDebug(5006) << newInFolder.find( *it ).value() << " new message(s) in "
                  << *it << endl;

    KMFolder *folder = kmkernel->findFolderById( *it );

    if ( folder && !folder->ignoreNewMail() ) {
      showNotification = true;
      if ( GlobalSettings::self()->verboseNewMailNotification() ) {
        summary += "<br>" + i18np( "1 new message in %2",
                                  "%1 new messages in %2",
                                  newInFolder.find( *it ).value() ,
                              folder->prettyUrl() );
      }
    }
  }

  // update folder menus in case some mail got filtered to trash/current folder
  // and we can enable "empty trash/move all to trash" action etc.
  updateFolderMenu();

  if ( !showNotification )
    return;

  if ( GlobalSettings::self()->verboseNewMailNotification() ) {
    summary = i18nc( "%1 is a list of the number of new messages per folder",
                    "<b>New mail arrived</b><br>%1" ,
                summary );
  }
  else {
    summary = i18n( "New mail arrived" );
  }

  if(kmkernel->xmlGuiInstance().isValid()) {
#ifdef __GNUC__
#warning "kde4 : port ?"
#endif
    //KNotifyClient::Instance instance(kmkernel->xmlGuiInstance());
    KNotification::event( "new-mail-arrived",
                          summary,QPixmap() ,topLevelWidget() );
  }
  else
    KNotification::event( "new-mail-arrived",
                          summary,QPixmap() ,topLevelWidget() );

  if (mBeepOnNew) {
    KNotification::beep();
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCompose()
{
  KMail::Composer * win;
  KMMessage* msg = new KMMessage;

  if ( mFolder ) {
      msg->initHeader( mFolder->identity() );
      TemplateParser parser( msg, TemplateParser::NewMessage,
			     "", false, false, false, false );
      parser.process( NULL, mFolder );
      win = KMail::makeComposer( msg, mFolder->identity() );
  } else {
      msg->initHeader();
      TemplateParser parser( msg, TemplateParser::NewMessage,
			     "", false, false, false, false );
      parser.process( NULL, NULL );
      win = KMail::makeComposer( msg );
  }

  win->show();

}


//-----------------------------------------------------------------------------
// TODO: do we want the list sorted alphabetically?
void KMMainWidget::slotShowNewFromTemplate()
{
  if ( mFolder ) {
    const KPIM::Identity & ident =
      kmkernel->identityManager()->identityForUoidOrDefault( mFolder->identity() );
    mTemplateFolder = kmkernel->folderMgr()->findIdString( ident.templates() );
  }
  else mTemplateFolder = kmkernel->templatesFolder();
  if ( !mTemplateFolder )
    return;

  mTemplateMenu->popupMenu()->clear();
  for ( int idx = 0; idx<mTemplateFolder->count(); ++idx ) {
    KMMsgBase *mb = mTemplateFolder->getMsgBase( idx );

    QString subj = mb->subject();
    if ( subj.isEmpty() ) subj = i18n("No Subject");
    mTemplateMenu->popupMenu()->insertItem(
      KStringHandler::rsqueeze( subj.replace( "&", "&&" ) ), idx );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNewFromTemplate( int id )
{
  if ( !mTemplateFolder )
    return;
  newFromTemplate(mTemplateFolder->getMsg( id ) );
}


//-----------------------------------------------------------------------------
void KMMainWidget::newFromTemplate( KMMessage *msg )
{
  if ( !msg )
    return;
  KMCommand *command = new KMUseTemplateCommand( this, msg );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPostToML()
{
  if ( mFolder && mFolder->isMailingListEnabled() ) {
    KMCommand *command = new KMMailingListPostCommand( this, mFolder );
    command->start();
  }
  else
    slotCompose();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderMailingListProperties()
{
  if (!mFolderTree) return;
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( mFolderTree->currentItem() );
  if ( !item ) return;
  KMFolder* folder = item->folder();
  if ( folder ) {
    ( new KMail::MailingListFolderPropertiesDialog( this, folder ) )->show();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderShortcutCommand()
{
  if (!mFolderTree) return;
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( mFolderTree->currentItem() );
  if ( item )
    item->assignShortcut();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotModifyFolder()
{
  if (!mFolderTree) return;
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( mFolderTree->currentItem() );
  if ( item )
    modifyFolder( item );
}

//-----------------------------------------------------------------------------
void KMMainWidget::modifyFolder( KMFolderTreeItem* folderItem )
{
  KMFolder* folder = folderItem->folder();
  KMFolderTree* folderTree = static_cast<KMFolderTree *>( folderItem->listView() );
  KMFolderDialog props( folder, folder->parent(), folderTree,
                        i18n("Properties of Folder %1", folder->label() ) );
  props.exec();
  updateFolderMenu();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireFolder()
{
  QString     str;
  bool        canBeExpired = true;

  if (!mFolder) return;

  if (!mFolder->isAutoExpire()) {
    canBeExpired = false;
  } else if (mFolder->getUnreadExpireUnits()==expireNever &&
	     mFolder->getReadExpireUnits()==expireNever) {
    canBeExpired = false;
  }

  if (!canBeExpired) {
    str = i18n("This folder does not have any expiry options set");
    KMessageBox::information(this, str);
    return;
  }
  KConfig           *config = KMKernel::config();
  KConfigGroup group(config, "General");

  if (group.readEntry("warn-before-expire", true ) ) {
    str = i18n("<qt>Are you sure you want to expire the folder <b>%1</b>?</qt>", Qt::escape( mFolder->label() ));
    if (KMessageBox::warningContinueCancel(this, str, i18n("Expire Folder"),
					   KGuiItem(i18n("&Expire")))
	!= KMessageBox::Continue) return;
  }

  mFolder->expireOldMessages( true /*immediate*/);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEmptyFolder()
{
  QString str;

  if (!mFolder) return;
  bool isTrash = kmkernel->folderIsTrash(mFolder);

  if (mConfirmEmpty)
  {
    QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
    QString text = (isTrash) ?
      i18n("Are you sure you want to empty the trash folder?") :
      i18n("<qt>Are you sure you want to move all messages from "
           "folder <b>%1</b> to the trash?</qt>", Qt::escape( mFolder->label() ) );

    if (KMessageBox::warningContinueCancel(this, text, title, KGuiItem( title, "edit-trash"))
      != KMessageBox::Continue) return;
  }
  KCursorSaver busy(KBusyPtr::busy());
  slotMarkAll();
  if (isTrash) {
    /* Don't ask for confirmation again when deleting, the user has already
       confirmed. */
    slotDeleteMsg( false );
  }
  else
    slotTrashMsg();

  if (mMsgView) mMsgView->clearCache();

  if ( !isTrash )
    BroadcastStatus::instance()->setStatusMsg(i18n("Moved all messages to the trash"));

  updateMessageActions();

  // Disable empty trash/move all to trash action - we've just deleted/moved
  // all folder contents.
  mEmptyFolderAction->setEnabled( false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if ( !mFolder ) return;
  if ( mFolder->isSystemFolder() ) return;
  if ( mFolder->isReadOnly() ) return;

  QString title;
  if ( mFolder->folderType() == KMFolderTypeSearch ) {
    title = i18n("Delete Search");
    str = i18n("<qt>Are you sure you want to delete the search <b>%1</b>?<br>"
                "Any messages it shows will still be available in their original folder.</qt>",
             Qt::escape( mFolder->label() ) );
  } else {
    title = i18n("Delete Folder");
    if ( mFolder->count() == 0 ) {
      if ( !mFolder->child() || mFolder->child()->isEmpty() ) {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<b>%1</b>?</qt>",
                Qt::escape( mFolder->label() ) );
      }
      else {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<b>%1</b> and all its subfolders? Those subfolders might "
                   "not be empty and their contents will be discarded as well. "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</qt>",
                Qt::escape( mFolder->label() ) );
      }
    } else {
      if ( !mFolder->child() || mFolder->child()->isEmpty() ) {
        str = i18n("<qt>Are you sure you want to delete the folder "
                   "<b>%1</b>, discarding its contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</qt>",
                Qt::escape( mFolder->label() ) );
      }
      else {
        str = i18n("<qt>Are you sure you want to delete the folder <b>%1</b> "
                   "and all its subfolders, discarding their contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</qt>",
              Qt::escape( mFolder->label() ) );
      }
    }
  }

  if (KMessageBox::warningContinueCancel(this, str, title,
                                         KGuiItem( i18n("&Delete"), "edit-delete"))
      == KMessageBox::Continue)
  {
    if ( mFolder->hasAccounts() ) {
      // this folder has an account, so we need to change that to the inbox
      for ( AccountList::Iterator it (mFolder->acctList()->begin() ),
             end( mFolder->acctList()->end() ); it != end; ++it ) {
        (*it)->setFolder( kmkernel->inboxFolder() );
        KMessageBox::information(this,
            i18n("<qt>The folder you deleted was associated with the account "
              "<b>%1</b> which delivered mail into it. The folder the account "
              "delivers new mail into was reset to the main Inbox folder.</qt>", (*it)->name()));
      }
    }
    if (mFolder->folderType() == KMFolderTypeImap)
      kmkernel->imapFolderMgr()->remove(mFolder);
    else if (mFolder->folderType() == KMFolderTypeCachedImap) {
      // Deleted by user -> tell the account (see KMFolderCachedImap::listDirectory2)
      KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>( mFolder->storage() );
      KMAcctCachedImap* acct = storage->account();
      if ( acct )
        acct->addDeletedFolder( mFolder );

      kmkernel->dimapFolderMgr()->remove(mFolder);
    }
    else if (mFolder->folderType() == KMFolderTypeSearch)
      kmkernel->searchFolderMgr()->remove(mFolder);
    else
      kmkernel->folderMgr()->remove(mFolder);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAllAsRead()
{
  if (!mFolder)
    return;
  mFolder->markUnreadAsRead();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompactFolder()
{
  if (mFolder) {
    int idx = mHeaders->currentItemIndex();
    KCursorSaver busy(KBusyPtr::busy());
    mFolder->compact( KMFolder::CompactNow );
    // setCurrentItemByIndex will override the statusbar message, so save/restore it
    QString statusMsg = BroadcastStatus::instance()->statusMsg();
    mHeaders->setCurrentItemByIndex(idx);
    BroadcastStatus::instance()->setStatusMsg( statusMsg );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRefreshFolder()
{
  if (mFolder)
  {
    if ( mFolder->folderType() == KMFolderTypeImap || mFolder->folderType() == KMFolderTypeCachedImap ) {
      if ( !kmkernel->askToGoOnline() ) {
        return;
      }
    }

    if (mFolder->folderType() == KMFolderTypeImap)
    {
      KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
      imap->getAndCheckFolder();
    } else if ( mFolder->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap* f = static_cast<KMFolderCachedImap*>( mFolder->storage() );
      f->account()->processNewMailSingleFolder( mFolder );
    }
  }
}

void KMMainWidget::slotTroubleshootFolder()
{
  if (mFolder)
  {
    if ( mFolder->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap* f = static_cast<KMFolderCachedImap*>( mFolder->storage() );
      f->slotTroubleshoot();
    }
  }
}

void KMMainWidget::slotInvalidateIMAPFolders() {
  if ( KMessageBox::warningContinueCancel( this,
          i18n("Are you sure you want to refresh the IMAP cache?\n"
	       "This will remove all changes that you have done "
	       "locally to your IMAP folders."),
	  i18n("Refresh IMAP Cache"), KGuiItem(i18n("&Refresh")) ) == KMessageBox::Continue )
    kmkernel->acctMgr()->invalidateIMAPFolders();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireAll() {
  KConfig    *config = KMKernel::config();
  int        ret = 0;

  KConfigGroup group(config, "General");

  if (group.readEntry("warn-before-expire", true ) ) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList().first(),
			 i18n("Are you sure you want to expire all old messages?"),
			 i18n("Expire Old Messages?"), KGuiItem(i18n("Expire")));
    if (ret != KMessageBox::Continue) {
      return;
    }
  }

  kmkernel->expireAllFoldersNow();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompactAll()
{
  KCursorSaver busy(KBusyPtr::busy());
  kmkernel->compactAllFolders();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideHtml()
{
  if( mHtmlPref == mFolderHtmlPref ) {
    int result = KMessageBox::warningContinueCancel( this,
      // the warning text is taken from configuredialog.cpp:
      i18n( "Use of HTML in mail will make you more vulnerable to "
        "\"spam\" and may increase the likelihood that your system will be "
        "compromised by other present and anticipated security exploits." ),
      i18n( "Security Warning" ),
      KGuiItem(i18n( "Use HTML" )),
      KStandardGuiItem::cancel(),
      "OverrideHtmlWarning", false);
    if( result == KMessageBox::Cancel ) {
      mPreferHtmlAction->setChecked( false );
      return;
    }
  }
  mFolderHtmlPref = !mFolderHtmlPref;
  if (mMsgView) {
    mMsgView->setHtmlOverride(mFolderHtmlPref);
    mMsgView->update( true );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideHtmlLoadExt()
{
  if( mHtmlLoadExtPref == mFolderHtmlLoadExtPref ) {
    int result = KMessageBox::warningContinueCancel( this,
      // the warning text is taken from configuredialog.cpp:
      i18n( "Loading external references in html mail will make you more vulnerable to "
        "\"spam\" and may increase the likelihood that your system will be "
        "compromised by other present and anticipated security exploits." ),
      i18n( "Security Warning" ),
      KGuiItem(i18n( "Load External References" )),
      KStandardGuiItem::cancel(),
      "OverrideHtmlLoadExtWarning", false);
    if( result == KMessageBox::Cancel ) {
      mPreferHtmlLoadExtAction->setChecked( false );
      return;
    }
  }
  mFolderHtmlLoadExtPref = !mFolderHtmlLoadExtPref;
  if (mMsgView) {
    mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPref);
    mMsgView->update( true );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideThread()
{
  mFolderThreadPref = !mFolderThreadPref;
  mHeaders->setNestedOverride(mFolderThreadPref);
  mThreadBySubjectAction->setEnabled(mThreadMessagesAction->isChecked());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleSubjectThreading()
{
  mFolderThreadSubjPref = !mFolderThreadSubjPref;
  mHeaders->setSubjectThreading(mFolderThreadSubjPref);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleShowQuickSearch()
{
  GlobalSettings::self()->setQuickSearchActive( !GlobalSettings::self()->quickSearchActive() );
  if ( GlobalSettings::self()->quickSearchActive() )
    mSearchToolBar->show();
  else {
    mQuickSearchLine->reset();
    mSearchToolBar->hide();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMessageQueuedOrDrafted()
{
  if (!kmkernel->folderIsDraftOrOutbox(mFolder))
      return;
  if (mMsgView)
    mMsgView->update(true);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardMsg()
{
  KMMessageList* selected = mHeaders->selectedMsgs();
  KMCommand *command = 0L;
  if(selected && !selected->isEmpty()) {
    command = new KMForwardCommand( this, *selected, mFolder->identity() );
  } else {
    command = new KMForwardCommand( this, mHeaders->currentMsg(), mFolder->identity() );
  }

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMsg()
{
  KMMessageList* selected = mHeaders->selectedMsgs();
  KMCommand *command = 0L;
  if(selected && !selected->isEmpty()) {
    command = new KMForwardAttachedCommand( this, *selected, mFolder->identity() );
  } else {
    command = new KMForwardAttachedCommand( this, mHeaders->currentMsg(), mFolder->identity() );
  }

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotEditMsg()
{
  KMCommand *command = new KMEditMsgCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUseTemplate()
{
  newFromTemplate( mHeaders->currentMsg() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotResendMsg()
{
  KMCommand *command = new KMResendMessageCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotTrashMsg()
{
  mHeaders->deleteMsg();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotDeleteMsg( bool confirmDelete )
{
  mHeaders->moveMsgToFolder( 0, confirmDelete );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotTrashThread()
{
  mHeaders->highlightCurrentThread();
  mHeaders->deleteMsg();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotDeleteThread( bool confirmDelete )
{
  mHeaders->highlightCurrentThread();
  mHeaders->moveMsgToFolder( 0, confirmDelete );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyToCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyAuthorToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyAuthorCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyAllToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyToAllCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( this, mHeaders->currentMsg() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyListToMsg()
{

  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyListCommand( this, mHeaders->currentMsg(),
					       text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyToMsg( int tid )
{
  QString text = mMsgView? mMsgView->copyText() : "";
  QString tmpl = mCustomTemplates[ tid ];
  kDebug() << "Reply with template: " << tmpl << " (" << tid << ")" << endl;
  KMCommand *command = new KMCustomReplyToCommand( this,
                                                   mHeaders->currentMsg(),
                                                   text,
                                                   tmpl );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyAllToMsg( int tid )
{
  QString text = mMsgView? mMsgView->copyText() : "";
  QString tmpl = mCustomTemplates[ tid ];
  kDebug() << "Reply to All with template: " << tmpl << " (" << tid << ")" << endl;
  KMCommand *command = new KMCustomReplyAllToCommand( this,
                                                   mHeaders->currentMsg(),
                                                   text,
                                                   tmpl );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomForwardMsg( int tid )
{
  QString tmpl = mCustomTemplates[ tid ];
  kDebug() << "Forward with template: " << tmpl << " (" << tid << ")" << endl;
  KMMessageList* selected = mHeaders->selectedMsgs();
  KMCommand *command = 0L;
  if(selected && !selected->isEmpty()) {
    command = new KMCustomForwardCommand( this, *selected,
                                          mFolder->identity(), tmpl );
  } else {
    command = new KMCustomForwardCommand( this, mHeaders->currentMsg(),
                                          mFolder->identity(), tmpl );
  }
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNoQuoteReplyToMsg()
{
  KMCommand *command = new KMNoQuoteReplyToCommand( this, mHeaders->currentMsg() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubjectFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailingListFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMMailingListFilterCommand( this, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFromFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  AddrSpecList al = msg->extractAddrSpecs( "From" );
  KMCommand *command;
  if ( al.empty() )
    command = new KMFilterCommand( "From",  msg->from() );
  else
    command = new KMFilterCommand( "From",  al.front().asString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "To",  msg->to() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateListFilterAction()
{
  //Proxy the mListFilterAction to update the action text
  QByteArray name;
  QString value;
  QString lname = MailingList::name( mHeaders->currentMsg(), name, value );
  mListFilterAction->setText( i18n("Filter on Mailing-List...") );
  if ( lname.isNull() )
    mListFilterAction->setEnabled( false );
  else {
    mListFilterAction->setEnabled( true );
    mListFilterAction->setText( i18n( "Filter on Mailing-List %1...", lname ) );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUndo()
{
    mHeaders->undo();
    updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleUnread()
{
  mFolderTree->toggleColumn(KMFolderTree::unread);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleTotalColumn()
{
  mFolderTree->toggleColumn(KMFolderTree::total, true);
}

void KMMainWidget::slotJumpToFolder()
{
  KMail::KMFolderSelDlg dlg( this, i18n("Jump to Folder"), true );
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  slotSelectFolder( dest );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMoveMsg()
{
  KMail::KMFolderSelDlg dlg( this, i18n("Move Message to Folder"), true );
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMoveMsgToFolder( KMFolder *dest)
{
  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCopyMsgToFolder( KMFolder *dest)
{
  mHeaders->copyMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotApplyFilters()
{
  mHeaders->applyFiltersOnMsg();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEditVacation()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  if ( mVacation )
    return;

  mVacation = new Vacation( this );
  if ( mVacation->isUsable() ) {
    connect( mVacation, SIGNAL(result(bool)), mVacation, SLOT(deleteLater()) );
  } else {
    QString msg = i18n("KMail's Out of Office Reply functionality relies on "
                      "server-side filtering. You have not yet configured an "
                      "IMAP server for this.\n"
                      "You can do this on the \"Filtering\" tab of the IMAP "
                      "account configuration.");
    KMessageBox::sorry( this, msg, i18n("No Server-Side Filtering Configured") );

    delete mVacation; // QGuardedPtr sets itself to 0!
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotDebugSieve()
{
#if !defined(NDEBUG)
  if ( mSieveDebugDialog )
    return;

  mSieveDebugDialog = new SieveDebugDialog( this );
  mSieveDebugDialog->exec();
  delete mSieveDebugDialog;
#endif
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotStartCertManager()
{
  K3Process certManagerProc; // save to create on the heap, since
  // there is no parent
  certManagerProc << "kleopatra";

  if( !certManagerProc.start( K3Process::DontCare ) )
    KMessageBox::error( this, i18n( "Could not start certificate manager; "
                                    "please check your installation." ),
                                    i18n( "KMail Error" ) );
  else
    kDebug(5006) << "\nslotStartCertManager(): certificate manager started.\n" << endl;
  // process continues to run even after the K3Process object goes
  // out of scope here, since it is started in DontCare run mode.

}

//-----------------------------------------------------------------------------
void KMMainWidget::slotStartWatchGnuPG()
{
  K3Process certManagerProc;
  certManagerProc << "kwatchgnupg";

  if( !certManagerProc.start( K3Process::DontCare ) )
    KMessageBox::error( this, i18n( "Could not start GnuPG LogViewer (kwatchgnupg); "
                                    "please check your installation." ),
                                    i18n( "KMail Error" ) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCopyMsg()
{
  KMail::KMFolderSelDlg dlg( this, i18n("Copy Message to Folder"), true );
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->copyMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotPrintMsg()
{
  bool htmlOverride = mMsgView ? mMsgView->htmlOverride() : false;
  bool htmlLoadExtOverride = mMsgView ? mMsgView->htmlLoadExtOverride() : false;
  KConfigGroup reader( KMKernel::config(), "Reader" );
  bool useFixedFont = mMsgView ? mMsgView->isFixedFont()
                               : reader.readEntry( "useFixedFont", false );
  KMCommand *command =
    new KMPrintCommand( this, mHeaders->currentMsg(),
                        htmlOverride, htmlLoadExtOverride,
                        useFixedFont, overrideEncoding() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotConfigChanged()
{
  readConfig();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveMsg()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this,
    *mHeaders->selectedMsgs() );

  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOpenMsg()
{
  KMOpenMsgCommand *openCommand = new KMOpenMsgCommand( this, KUrl(), overrideEncoding() );

  openCommand->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveAttachments()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;
  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( this,
                                                                        *mHeaders->selectedMsgs() );
  saveCommand->start();
}

void KMMainWidget::slotOnlineStatus()
{
  // KMKernel will emit a signal when we toggle the network state that is caught by
  // KMMainWidget::slotUpdateOnlineStatus to update our GUI
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online ) {
    // if online; then toggle and set it offline.
    kmkernel->stopNetworkJobs();
  } else {
    kmkernel->resumeNetworkJobs();
  }
}

void KMMainWidget::slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type )
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online )
    actionCollection()->action( "online_status" )->setText( i18n("Work Offline") );
  else
    actionCollection()->action( "online_status" )->setText( i18n("Work Online") );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueued()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  kmkernel->msgSender()->sendQueued();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueuedVia( QAction* item )
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  QStringList availTransports= KMail::TransportManager::transportNames();
  if (availTransports.contains(item->text()))
    kmkernel->msgSender()->sendQueued( item->text() );
}

//-----------------------------------------------------------------------------

// Unused? TA20060825
// void KMMainWidget::slotViewChange()
// {
//   if(mBodyPartsMenu->isItemChecked(mBodyPartsMenu->idAt(0)))
//   {
//     mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(0),false);
//     mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(1),true);
//   }
//   else if(mBodyPartsMenu->isItemChecked(mBodyPartsMenu->idAt(1)))
//   {
//     mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(1),false);
//     mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(0),true);
//   }
//
//   //mMsgView->setInline(!mMsgView->isInline());
// }


//-----------------------------------------------------------------------------
void KMMainWidget::folderSelectedUnread( KMFolder* aFolder )
{
  folderSelected( aFolder, true );
  slotChangeCaption( mFolderTree->currentItem() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::folderSelected()
{
  folderSelected( mFolder );
  updateFolderMenu();
  // opened() before the getAndCheckFolder() in folderSelected
  if ( mFolder && mFolder->folderType() == KMFolderTypeImap &&
       mOpenedImapFolder ) {
    // the selected state is mainly a duplicate for having an open
    // "mainwidget" so keep that in sync
    KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
    imap->setSelected( false );
    mFolder->close( "mainwidget" );
    mOpenedImapFolder = false;
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::folderSelected( KMFolder* aFolder, bool forceJumpToUnread )
{
  KCursorSaver busy(KBusyPtr::busy());

  if (mMsgView)
    mMsgView->clear(true);

  if ( mFolder && mFolder->folderType() == KMFolderTypeImap && !mFolder->noContent() )
  {
    KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
    if ( mFolder->needsCompacting() && imap->autoExpunge() )
      imap->expungeFolder(imap, true);
  }

  // Re-enable the msg list and quicksearch if we're showing a splash
  // screen. This is true either if there's no active folder, or if we
  // have a timer that is no longer active (i.e. it has already fired)
  // To make the if() a bit more complicated, we suppress the hiding
  // when the new folder is also an IMAP folder, because that's an
  // async operation and we don't want flicker if it results in just
  // a new splash.
  bool newFolder = ( mFolder != aFolder );
  bool isNewImapFolder = aFolder && aFolder->folderType() == KMFolderTypeImap && newFolder;
  if( !mFolder
      || ( !isNewImapFolder && mShowBusySplashTimer )
      || ( newFolder && mShowingOfflineScreen && !( isNewImapFolder && kmkernel->isOffline() ) ) ) {
    if ( mMsgView ) {
      mMsgView->enableMsgDisplay();
      mMsgView->clear( true );
    }
    if( mSearchAndHeaders && mHeaders )
      mSearchAndHeaders->show();
    mShowingOfflineScreen = false;
  }

  // Delete any pending timer, if needed it will be recreated below
  delete mShowBusySplashTimer;
  mShowBusySplashTimer = 0;

  if ( newFolder ) {
    writeFolderConfig();
  }
  if ( mFolder ) {
    disconnect( mFolder, SIGNAL( changed() ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgRemoved( KMFolder * ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    if ( mOpenedImapFolder && newFolder &&
         mFolder->folderType() == KMFolderTypeImap ) {
      KMFolderImap *imap = static_cast<KMFolderImap*>( mFolder->storage() );
      imap->setSelected( false );
      mFolder->close( "mainwidget" );
      mOpenedImapFolder = false;
    }
  }

  mFolder = aFolder;

  if ( aFolder && aFolder->folderType() == KMFolderTypeImap ) {
    if ( kmkernel->isOffline() ) {
      showOfflinePage();
      return;
    }
    KMFolderImap *imap = static_cast<KMFolderImap*>( aFolder->storage() );
    if ( newFolder && !mFolder->noContent() ) {
      assert( !mOpenedImapFolder );
      imap->open( "mainwidget" ); // will be closed in the folderSelected slot
      mOpenedImapFolder = true;
      // first get new headers before we select the folder
      imap->setSelected( true );
      connect( imap, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
          this, SLOT( folderSelected() ) );
      imap->getAndCheckFolder();
      mHeaders->setFolder( 0 );
      updateFolderMenu();
      mForceJumpToUnread = forceJumpToUnread;

      // Set a timer to show a splash screen if fetching folder contents
      // takes more than the amount of seconds configured in the kmailrc (default 1000 msec)
      mShowBusySplashTimer = new QTimer( this );
      mShowBusySplashTimer->setSingleShot( true );
      connect( mShowBusySplashTimer, SIGNAL( timeout() ), this, SLOT( slotShowBusySplash() ) );
      mShowBusySplashTimer->start( GlobalSettings::self()->folderLoadingTimeout() );
      return;
    } else {
      // the folder is complete now - so go ahead
      disconnect( imap, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
          this, SLOT( folderSelected() ) );
      forceJumpToUnread = mForceJumpToUnread;
    }
  }

  if ( mFolder ) { // == 0 -> pointing to toplevel ("Welcome to KMail") folder
    connect( mFolder, SIGNAL( changed() ),
           this, SLOT( updateMarkAsReadAction() ) );
    connect( mFolder, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    connect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    connect( mFolder, SIGNAL( msgRemoved(KMFolder *) ),
           this, SLOT( updateMarkAsReadAction() ) );
  }
  readFolderConfig();
  if (mMsgView)
  {
    mMsgView->setHtmlOverride(mFolderHtmlPref);
    mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPref);
  }
  mHeaders->setFolder( mFolder, forceJumpToUnread );
  updateMessageActions();
  updateFolderMenu();
  if (!aFolder)
    slotIntro();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowBusySplash()
{
  if ( mReaderWindowActive )
  {
    mMsgView->displayBusyPage();
    // hide widgets that are in the way:
    if ( mSearchAndHeaders && mHeaders && mLongFolderList )
      mSearchAndHeaders->hide();
  }
}

void KMMainWidget::showOfflinePage()
{
  if ( !mReaderWindowActive ) return;
  mShowingOfflineScreen = true;

  mMsgView->displayOfflinePage();
  // hide widgets that are in the way:
  if ( mSearchAndHeaders && mHeaders && mLongFolderList )
    mSearchAndHeaders->hide();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgSelected(KMMessage *msg)
{
  if ( msg && msg->parent() && !msg->isComplete() )
  {
    if ( msg->transferInProgress() )
      return;
    mMsgView->clear();
    mMsgView->setWaitingForSerNum( msg->getMsgSerNum() );

    if ( mJob ) {
       disconnect( mJob, 0, mMsgView, 0 );
       delete mJob;
    }
    mJob = msg->parent()->createJob( msg, FolderJob::tGetMessage, 0,
          "STRUCTURE", mMsgView->attachmentStrategy() );
    connect(mJob, SIGNAL(messageRetrieved(KMMessage*)),
            mMsgView, SLOT(slotMessageArrived(KMMessage*)));
    mJob->start();
  } else {
    mMsgView->setMsg(msg);
  }
  // reset HTML override to the folder setting
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPref);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgChanged()
{
  mHeaders->msgChanged();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectFolder(KMFolder* folder)
{
  Q3ListViewItem* item = mFolderTree->indexOfFolder(folder);
  if ( item ) {
    mFolderTree->ensureItemVisible( item );
    mFolderTree->doFolderSelected( item );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectMessage(KMMessage* msg)
{
  int idx = mFolder->find(msg);
  if (idx != -1) {
    mHeaders->setCurrentMsg(idx);
    if (mMsgView)
      mMsgView->setMsg(msg);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplaceMsgByUnencryptedVersion()
{
  kDebug(5006) << "KMMainWidget::slotReplaceMsgByUnencryptedVersion()" << endl;
  KMMessage* oldMsg = mHeaders->currentMsg();
  if( oldMsg ) {
    kDebug(5006) << "KMMainWidget  -  old message found" << endl;
    if( oldMsg->hasUnencryptedMsg() ) {
      kDebug(5006) << "KMMainWidget  -  extra unencrypted message found" << endl;
      KMMessage* newMsg = oldMsg->unencryptedMsg();
      // adjust the message id
      {
        QString msgId( oldMsg->msgId() );
        QString prefix("DecryptedMsg.");
        int oldIdx = msgId.indexOf(prefix, 0, Qt::CaseInsensitive);
        if( -1 == oldIdx ) {
          int leftAngle = msgId.lastIndexOf( '<' );
          msgId = msgId.insert( (-1 == leftAngle) ? 0 : ++leftAngle, prefix );
        }
        else {
          // toggle between "DecryptedMsg." and "DeCryptedMsg."
          // to avoid same message id
          QCharRef c = msgId[ oldIdx+2 ];
          if( 'C' == c )
            c = 'c';
          else
            c = 'C';
        }
        newMsg->setMsgId( msgId );
        mMsgView->setIdOfLastViewedMessage( msgId );
      }
      // insert the unencrypted message
      kDebug(5006) << "KMMainWidget  -  adding unencrypted message to folder" << endl;
      mFolder->addMsg( newMsg );
      /* Figure out its index in the folder for selecting. This must be count()-1,
       * since we append. Be safe and do find, though, just in case. */
      int newMsgIdx = mFolder->find( newMsg );
      Q_ASSERT( newMsgIdx != -1 );
      /* we need this unget, to have the message displayed correctly initially */
      mFolder->unGetMsg( newMsgIdx );
      int idx = mFolder->find( oldMsg );
      Q_ASSERT( idx != -1 );
      /* only select here, so the old one is not un-Gotten before, which would
       * render the pointer we hold invalid so that find would fail */
      mHeaders->setCurrentItemByIndex( newMsgIdx );
      // remove the old one
      if ( idx != -1 ) {
        kDebug(5006) << "KMMainWidget  -  deleting encrypted message" << endl;
        mFolder->take( idx );
      }

      kDebug(5006) << "KMMainWidget  -  updating message actions" << endl;
      updateMessageActions();

      kDebug(5006) << "KMMainWidget  -  done." << endl;
    } else
      kDebug(5006) << "KMMainWidget  -  NO EXTRA UNENCRYPTED MESSAGE FOUND" << endl;
  } else
    kDebug(5006) << "KMMainWidget  -  PANIC: NO OLD MESSAGE FOUND" << endl;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusNew()
{
  mHeaders->setMsgStatus( MessageStatus::statusNew() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusUnread()
{
  mHeaders->setMsgStatus( MessageStatus::statusUnread() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusRead()
{
  mHeaders->setMsgStatus( MessageStatus::statusRead() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusFlag()
{
  mHeaders->setMsgStatus( MessageStatus::statusImportant(), true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusTodo()
{
  mHeaders->setMsgStatus( MessageStatus::statusTodo(), true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusSent()
{
  mHeaders->setMsgStatus( MessageStatus::statusSent(), true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusNew()
{
  mHeaders->setThreadStatus( MessageStatus::statusNew() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusUnread()
{
  mHeaders->setThreadStatus( MessageStatus::statusUnread() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusFlag()
{
  mHeaders->setThreadStatus( MessageStatus::statusImportant(), true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusRead()
{
  mHeaders->setThreadStatus( MessageStatus::statusRead());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusTodo()
{
  mHeaders->setThreadStatus( MessageStatus::statusTodo(), true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusWatched()
{
  mHeaders->setThreadStatus( MessageStatus::statusWatched(), true);
  if (mWatchThreadAction->isChecked()) {
    mIgnoreThreadAction->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusIgnored()
{
  mHeaders->setThreadStatus( MessageStatus::statusIgnored(), true);
  if (mIgnoreThreadAction->isChecked()) {
    mWatchThreadAction->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotNextMessage()       { mHeaders->nextMessage(); }
void KMMainWidget::slotNextUnreadMessage()
{
  if ( !mHeaders->nextUnreadMessage() )
    if ( GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders )
      mFolderTree->nextUnreadFolder(true);
}
void KMMainWidget::slotNextImportantMessage() {
  //mHeaders->nextImportantMessage();
}
void KMMainWidget::slotPrevMessage()       { mHeaders->prevMessage(); }
void KMMainWidget::slotPrevUnreadMessage()
{
  if ( !mHeaders->prevUnreadMessage() )
    if ( GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders )
      mFolderTree->prevUnreadFolder();
}
void KMMainWidget::slotPrevImportantMessage() {
  //mHeaders->prevImportantMessage();
}

void KMMainWidget::slotDisplayCurrentMessage()
{
  if ( mHeaders->currentMsg() )
    slotMsgActivated( mHeaders->currentMsg() );
}

//-----------------------------------------------------------------------------
//called from headers. Message must not be deleted on close
void KMMainWidget::slotMsgActivated(KMMessage *msg)
{
  if ( !msg ) return;
  if (msg->parent() && !msg->isComplete())
  {
    FolderJob *job = msg->parent()->createJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotMsgActivated(KMMessage*)));
    job->start();
    return;
  }

  if (kmkernel->folderIsDraftOrOutbox(mFolder))
  {
    slotEditMsg();
    return;
  }
  if ( kmkernel->folderIsTemplates( mFolder ) ) {
    slotUseTemplate();
    return;
  }

  assert( msg != 0 );
  KMReaderMainWin *win = new KMReaderMainWin( mFolderHtmlPref, mFolderHtmlLoadExtPref );
  KConfigGroup reader( KMKernel::config(), "Reader" );
  bool useFixedFont = mMsgView ? mMsgView->isFixedFont()
                               : reader.readEntry( "useFixedFont", false );
  win->setUseFixedFont( useFixedFont );
  KMMessage *newMessage = new KMMessage(*msg);
  newMessage->setParent( msg->parent() );
  newMessage->setMsgSerNum( msg->getMsgSerNum() );
  newMessage->setReadyToShow( true );
  win->showMsg( overrideEncoding(), newMessage );
  win->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAll()
{
  mHeaders->selectAll( true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgPopup(KMMessage&, const KUrl &aUrl, const QPoint& aPoint)
{
  KMenu * menu = new KMenu;
  updateMessageMenu();
  mUrlCurrent = aUrl;

  bool urlMenuAdded = false;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto")
    {
      // popup on a mailto URL
      menu->addAction( mMsgView->mailToComposeAction() );
      menu->addAction( mMsgView->mailToReplyAction() );
      menu->addAction( mMsgView->mailToForwardAction() );

      menu->addSeparator();
      menu->addAction( mMsgView->addAddrBookAction() );
      menu->addAction( mMsgView->openAddrBookAction() );
      menu->addAction( mMsgView->copyURLAction() );
      menu->addAction( mMsgView->startImChatAction() );
      // only enable if our KIMProxy is functional
      mMsgView->startImChatAction()->setEnabled( kmkernel->imProxy()->initialize() );

    } else {
      // popup on a not-mailto URL
      menu->addAction( mMsgView->urlOpenAction() );
      menu->addAction( mMsgView->addBookmarksAction() );
      menu->addAction( mMsgView->urlSaveAsAction() );
      menu->addAction( mMsgView->copyURLAction() );
    }
    if ( aUrl.protocol() == "im" )
    {
      // popup on an IM address
      // no need to check the KIMProxy is initialized, as these protocols will
      // only be present if it is.
      menu->addAction( mMsgView->startImChatAction() );
    }

    urlMenuAdded=true;
    kDebug( 0 ) << k_funcinfo << " URL is: " << aUrl << endl;
  }


  if(mMsgView && !mMsgView->copyText().isEmpty()) {
    if ( urlMenuAdded )
      menu->addSeparator();
    menu->addAction( mReplyActionMenu );
    menu->addSeparator();

    menu->addAction( mMsgView->copyAction() );
    menu->addAction( mMsgView->selectAllAction() );
  } else  if ( !urlMenuAdded )
  {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mHeaders->currentMsg()) // no messages
    {
      delete menu;
      return;
    }

    if ( mFolder->isDrafts() || mFolder->isOutbox() ) {
      menu->addAction( mEditAction );
    }
    else if ( mFolder->isTemplates() ) {
      menu->addAction( mUseAction );
      menu->addAction( mEditAction );
    }
    else {
      if( !mFolder->isSent() )
        menu->addAction( mReplyActionMenu );
      menu->addAction( mForwardActionMenu );
    }
    menu->addSeparator();

    menu->addAction( mCopyActionMenu );
    menu->addAction( mMoveActionMenu );

    menu->addSeparator();

    menu->addAction( mStatusMenu );
    menu->addSeparator();

    menu->addAction( viewSourceAction() );
    if(mMsgView) {
      menu->addAction( mMsgView->toggleFixFontAction() );
    }
    menu->addSeparator();
    menu->addAction( mPrintAction );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAttachmentsAction );

    menu->addSeparator();
    if( mFolder->isTrash() )
      menu->addAction( mDeleteAction );
    else
      menu->addAction( mTrashAction );
  }
  KAcceleratorManager::manage(menu);
  menu->exec(aPoint, 0);
  delete menu;
}

//-----------------------------------------------------------------------------
void KMMainWidget::getAccountMenu()
{
  QStringList actList;

  mActMenu->clear();
  actList = kmkernel->acctMgr()->getAccounts();
  QStringList::Iterator it;
  for(it = actList.begin(); it != actList.end() ; ++it)
    mActMenu->addAction((*it).replace("&", "&&"));
}

//-----------------------------------------------------------------------------
void KMMainWidget::getTransportMenu()
{
  QStringList availTransports;

  mSendMenu->clear();
  availTransports = KMail::TransportManager::transportNames();
  QStringList::Iterator it;
  for(it = availTransports.begin(); it != availTransports.end() ; ++it)
    mSendMenu->addAction((*it).replace("&", "&&"));
}


//-----------------------------------------------------------------------------
void KMMainWidget::updateCustomTemplateMenus()
{
  if ( !mCustomTemplateActions.isEmpty() ) {
    Q3PtrList<KAction>::iterator ait = mCustomTemplateActions.begin();
    for ( ; ait != mCustomTemplateActions.end() ; ++ait ) {
      //(*ait)->unplugAll();
      delete (*ait);
    }
    mCustomTemplateActions.clear();
  }

  delete mCustomReplyActionMenu;
  delete mCustomReplyAllActionMenu;
  delete mCustomForwardActionMenu;

  delete mCustomReplyMapper;
  delete mCustomReplyAllMapper;
  delete mCustomForwardMapper;

  mCustomForwardActionMenu =
    new KActionMenu( KIcon( "mail_custom_forward" ),
		     i18n("Forward with custom template"),
                     actionCollection() ); //, "custom_forward" );
  QSignalMapper *mCustomForwardMapper = new QSignalMapper( this );
  connect( mCustomForwardMapper, SIGNAL( mapped( int ) ),
           this, SLOT( slotCustomForwardMsg( int ) ) );
  mCustomForwardActionMenu->addAction( mForwardActionMenu );

  mCustomReplyActionMenu =
    new KActionMenu( KIcon( "mail_custom_reply" ),
		     i18n("Reply with custom template"),
                     actionCollection() ); //, "custom_reply" );
  QSignalMapper *mCustomReplyMapper = new QSignalMapper( this );
  connect( mCustomReplyMapper, SIGNAL( mapped( int ) ),
           this, SLOT( slotCustomReplyToMsg( int ) ) );
  mCustomReplyActionMenu->addAction( mReplyActionMenu );

  mCustomReplyAllActionMenu =
    new KActionMenu( KIcon( "mail_custom_reply_all" ),
		     i18n("Reply to All with custom template"),
                     actionCollection() ); //, "custom_reply_all" );
  QSignalMapper *mCustomReplyAllMapper = new QSignalMapper( this );
  connect( mCustomReplyAllMapper, SIGNAL( mapped( int ) ),
           this, SLOT( slotCustomReplyAllToMsg( int ) ) );
  mCustomReplyAllActionMenu->addAction( mReplyActionMenu );

  mCustomTemplates.clear();

  QStringList list = GlobalSettingsBase::self()->customTemplates();
  QStringList::iterator it = list.begin();
  int idx = 0;
  int replyc = 0;
  int replyallc = 0;
  int forwardc = 0;
  for ( ; it != list.end(); ++it ) {
    CTemplates t( *it );
    mCustomTemplates.append( *it );

    KAction *action;
    switch ( t.type() ) {
    case CustomTemplates::TReply:
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyMapper, SLOT(map()) );
      mCustomReplyMapper->setMapping( action, idx );
      mCustomReplyActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyc;
      break;
    case CustomTemplates::TReplyAll:
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyAllMapper, SLOT(map()) );
      mCustomReplyAllMapper->setMapping( action, idx );
      mCustomReplyAllActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyallc;
      break;
    case CustomTemplates::TForward:
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomForwardMapper, SLOT(map()) );
      mCustomForwardMapper->setMapping( action, idx );
      mCustomForwardActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++forwardc;
      break;
    case CustomTemplates::TUniversal:
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyMapper, SLOT(map()) );
      mCustomReplyMapper->setMapping( action, idx );
      mCustomReplyActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyc;
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyAllMapper, SLOT(map()) );
      mCustomReplyAllMapper->setMapping( action, idx );
      mCustomReplyAllActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyallc;
      action = new KAction( (*it).replace( "&", "&&" ),
                            actionCollection() );
                            // (*it).utf8() );
      connect( action, SIGNAL(triggered(bool)), mCustomForwardMapper, SLOT(map()) );
      mCustomForwardMapper->setMapping( action, idx );
      mCustomForwardActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++forwardc;
      break;
    }

    ++idx;
  }
  if ( !replyc ) {
      mCustomReplyActionMenu->popupMenu()->insertItem( i18n( "(no custom templates)" ), 0 );
      mCustomReplyActionMenu->popupMenu()->setItemEnabled( 0, false );
  }
  if ( !replyallc ) {
      mCustomReplyAllActionMenu->popupMenu()->insertItem( i18n( "(no custom templates)" ), 0 );
      mCustomReplyAllActionMenu->popupMenu()->setItemEnabled( 0, false );
  }
  if ( !forwardc ) {
      mCustomForwardActionMenu->popupMenu()->insertItem( i18n( "(no custom templates)" ), 0 );
      mCustomForwardActionMenu->popupMenu()->setItemEnabled( 0, false );
  }

}


//-----------------------------------------------------------------------------
void KMMainWidget::setupActions()
{
  //----- File Menu
  mSaveAsAction  = new KAction(KIcon("document-save"), i18n("Save &As..."), this);
  actionCollection()->addAction("file_save_as", mSaveAsAction );
  connect(mSaveAsAction, SIGNAL(triggered(bool) ), SLOT(slotSaveMsg()));
  mSaveAsAction->setShortcut(KStandardShortcut::shortcut(KStandardShortcut::Save));

  mOpenAction = KStandardAction::open( this, SLOT( slotOpenMsg() ),
                                  actionCollection() );

  QAction *action  = new KAction(i18n("&Compact All Folders"), this);
  actionCollection()->addAction("compact_all_folders", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCompactAll()));

  action  = new KAction(i18n("&Expire All Folders"), this);
  actionCollection()->addAction("expire_all_folders", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpireAll()));

  action  = new KAction(KIcon("refresh"), i18n("&Refresh Local IMAP Cache"), this);
  actionCollection()->addAction("file_invalidate_imap_cache", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotInvalidateIMAPFolders()));

  action  = new KAction(i18n("Empty All &Trash Folders"), this);
  actionCollection()->addAction("empty_trash", action );
  connect(action, SIGNAL(triggered(bool) ), KMKernel::self(), SLOT(slotEmptyTrash()));

  action  = new KAction(KIcon("mail-get"), i18n("Check &Mail"), this);
  actionCollection()->addAction("check_mail", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCheckMail()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));

  KActionMenu *actActionMenu  = new KActionMenu(KIcon("mail-get"), i18n("Check Mail &In"), this);
  actionCollection()->addAction("check_mail_in", actActionMenu );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"

  connect(actActionMenu,SIGNAL(activated()),this,SLOT(slotCheckMail()));

  mActMenu = actActionMenu->menu();
  connect(mActMenu,SIGNAL(triggered(QAction*)),
          SLOT(slotCheckOneAccount(QAction*)));
  connect(mActMenu,SIGNAL(aboutToShow()), SLOT(getAccountMenu()));

  action  = new KAction(KIcon("mail-send"), i18n("&Send Queued Messages"), this);
  actionCollection()->addAction("send_queued", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotSendQueued()));

  action  = new KAction(KIcon("online_status"), i18n("Onlinestatus (unknown)"), this);
  actionCollection()->addAction("online_status", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotOnlineStatus()));

  KActionMenu *sendActionMenu  = new KActionMenu(KIcon("mail_send_via"), i18n("Send Queued Messages Via"), this);
  actionCollection()->addAction("send_queued_via", sendActionMenu );
  sendActionMenu->setDelayed(true);

  mSendMenu = sendActionMenu->menu();
  connect(mSendMenu,SIGNAL(triggered(QAction*)), SLOT(slotSendQueuedVia(QAction*)));
  connect(mSendMenu,SIGNAL(aboutToShow()),SLOT(getTransportMenu()));

  QAction *act;
  //----- Tools menu
  if (parent()->inherits("KMMainWin")) {
    act  = new KAction(KIcon("help-contents"), i18n("&Address Book..."), this);
    actionCollection()->addAction("addressbook", act );
    connect(act, SIGNAL(triggered(bool)), SLOT(slotAddrBook()));
    if (KStandardDirs::findExe("kaddressbook").isEmpty()) act->setEnabled(false);
  }

  act  = new KAction(KIcon("pgp-keys"), i18n("Certificate Manager..."), this);
  actionCollection()->addAction("tools_start_certman", act );
  connect(act, SIGNAL(triggered(bool)), SLOT(slotStartCertManager()));
  // disable action if no certman binary is around
  if (KStandardDirs::findExe("kleopatra").isEmpty()) act->setEnabled(false);

  act  = new KAction(KIcon("pgp-keys"), i18n("GnuPG Log Viewer..."), this);
  actionCollection()->addAction("tools_start_kwatchgnupg", act );
  connect(act, SIGNAL(triggered(bool)), SLOT(slotStartWatchGnuPG()));
  // disable action if no kwatchgnupg binary is around
  if (KStandardDirs::findExe("kwatchgnupg").isEmpty()) act->setEnabled(false);

  act  = new KAction(KIcon("document-open"), i18n("&Import Messages..."), this);
  actionCollection()->addAction("import", act );
  connect(act, SIGNAL(triggered(bool)), SLOT(slotImport()));
  if (KStandardDirs::findExe("kmailcvt").isEmpty()) act->setEnabled(false);

#if !defined(NDEBUG)
  action  = new KAction(KIcon("idea"), i18n("&Debug Sieve..."), this);
  actionCollection()->addAction("tools_debug_sieve", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotDebugSieve()));
#endif

  // @TODO (marc/bo): Test
  action  = new KAction(KIcon("configure"), i18n("Edit \"Out of Office\" Replies..."), this);
  actionCollection()->addAction("tools_edit_vacation", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotEditVacation()));

  action  = new KAction(i18n("Filter &Log Viewer..."), this);
  actionCollection()->addAction("filter_log_viewer", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotFilterLogViewer()));

  action  = new KAction(i18n("&Anti-Spam Wizard..."), this);
  actionCollection()->addAction("antiSpamWizard", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotAntiSpamWizard()));
  action  = new KAction(i18n("&Anti-Virus Wizard..."), this);
  actionCollection()->addAction("antiVirusWizard", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotAntiVirusWizard()));

  //----- Edit Menu
  mTrashAction  = new KAction(i18n("&Move to Trash"), this);
  actionCollection()->addAction("move_to_trash", mTrashAction );
  mTrashAction->setIcon(KIcon("edit-trash"));
  mTrashAction->setShortcut(QKeySequence(Qt::Key_Delete));
  mTrashAction->setToolTip(i18n("Move message to trashcan"));
  connect(mTrashAction, SIGNAL(triggered(bool)), SLOT(slotTrashMsg()));

  /* The delete action is nowhere in the gui, by default, so we need to make
   * sure it is plugged into the KAccel now, since that won't happen on
   * XMLGui construction or manual ->plug(). This is only a problem when run
   * as a part, though. */
  mDeleteAction  = new KAction(KIcon("edit-delete"), i18n("&Delete"), this);
  actionCollection()->addAction("delete", mDeleteAction );
  connect(mDeleteAction, SIGNAL(triggered(bool)), SLOT(slotDeleteMsg()));
  mDeleteAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_Delete));
#ifdef __GNUC__
#warning Port me!
#endif
//  mDeleteAction->plugAccel( actionCollection()->kaccel() );

  mTrashThreadAction  = new KAction(i18n("M&ove Thread to Trash"), this);
  actionCollection()->addAction("move_thread_to_trash", mTrashThreadAction );
  mTrashThreadAction->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Delete));
  mTrashThreadAction->setIcon(KIcon("edit-trash"));
  mTrashThreadAction->setToolTip(i18n("Move thread to trashcan") );
  connect(mTrashThreadAction, SIGNAL(triggered(bool)), SLOT(slotTrashThread()));

  mDeleteThreadAction  = new KAction(KIcon("edit-delete"), i18n("Delete T&hread"), this);
  actionCollection()->addAction("delete_thread", mDeleteThreadAction );
  connect(mDeleteThreadAction, SIGNAL(triggered(bool)), SLOT(slotDeleteThread()));
  mDeleteThreadAction->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Delete));


  action  = new KAction(KIcon("mail-find"), i18n("&Find Messages..."), this);
  actionCollection()->addAction("search_messages", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotSearch()));
  action->setShortcut(QKeySequence(Qt::Key_S));

  mFindInMessageAction  = new KAction(KIcon("edit-find"), i18n("&Find in Message..."), this);
  actionCollection()->addAction("find_in_messages", mFindInMessageAction );
  connect(mFindInMessageAction, SIGNAL(triggered(bool)), SLOT(slotFind()));
  mFindInMessageAction->setShortcut(KStandardShortcut::shortcut(KStandardShortcut::Find));

  action  = new KAction(i18n("Select &All Messages"), this);
  actionCollection()->addAction("mark_all_messages", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotMarkAll()));
  action->setShortcuts(KStandardShortcut::selectAll());

  //----- Folder Menu
  mNewFolderAction  = new KAction(KIcon("folder-new"), i18n("&New Folder..."), this);
  actionCollection()->addAction("new_folder", action );
  connect(action, SIGNAL(triggered(bool)), mFolderTree, SLOT(addChildFolder()));

  mModifyFolderAction  = new KAction(KIcon("configure"), i18n("&Properties"), this);
  actionCollection()->addAction("modify", mModifyFolderAction );
  connect(mModifyFolderAction, SIGNAL(triggered(bool)), SLOT(slotModifyFolder()));

  mFolderMailingListPropertiesAction  = new KAction(i18n("&Mailing List Management..."), this);
  actionCollection()->addAction("folder_mailinglist_properties", mFolderMailingListPropertiesAction );
  connect(mFolderMailingListPropertiesAction, SIGNAL(triggered(bool)), SLOT( slotFolderMailingListProperties()));
  // mFolderMailingListPropertiesAction->setIcon(KIcon("folder_mailinglist_properties"));

  mFolderShortCutCommandAction  = new KAction(KIcon("configure-shortcuts"), i18n("&Assign Shortcut..."), this);
  actionCollection()->addAction("folder_shortcut_command", mFolderShortCutCommandAction );
  connect(mFolderShortCutCommandAction, SIGNAL(triggered(bool) ), SLOT( slotFolderShortcutCommand() ));


  mMarkAllAsReadAction  = new KAction(KIcon("goto-page"), i18n("Mark All Messages as &Read"), this);
  actionCollection()->addAction("mark_all_as_read", mMarkAllAsReadAction );
  connect(mMarkAllAsReadAction, SIGNAL(triggered(bool)), SLOT(slotMarkAllAsRead()));

  mExpireFolderAction  = new KAction(i18n("&Expiration Settings"), this);
  actionCollection()->addAction("expire", mExpireFolderAction );
  connect(mExpireFolderAction, SIGNAL(triggered(bool) ), SLOT(slotExpireFolder()));

  mCompactFolderAction  = new KAction(i18n("&Compact Folder"), this);
  actionCollection()->addAction("compact", mCompactFolderAction );
  connect(mCompactFolderAction, SIGNAL(triggered(bool) ), SLOT(slotCompactFolder()));

  mRefreshFolderAction  = new KAction(KIcon("view-refresh"), i18n("Check Mail &in This Folder"), this);
  actionCollection()->addAction("refresh_folder", mRefreshFolderAction );
  connect(mRefreshFolderAction, SIGNAL(triggered(bool) ), SLOT(slotRefreshFolder()));
  mRefreshFolderAction->setShortcut(KStandardShortcut::shortcut( KStandardShortcut::Reload ));
  mTroubleshootFolderAction = 0; // set in initializeIMAPActions

  mEmptyFolderAction  = new KAction(KIcon("edit-trash"),
                                    "foo" /*set in updateFolderMenu*/, this);
  actionCollection()->addAction("empty", mEmptyFolderAction );
  connect(mEmptyFolderAction, SIGNAL(triggered(bool)), SLOT(slotEmptyFolder()));

  mRemoveFolderAction  = new KAction(KIcon("edit-delete"),
                                     "foo" /*set in updateFolderMenu*/, this);
  actionCollection()->addAction("delete_folder", mRemoveFolderAction );
  connect(mRemoveFolderAction, SIGNAL(triggered(bool)), SLOT(slotRemoveFolder()));

  mPreferHtmlAction  = new KToggleAction(i18n("Prefer &HTML to Plain Text"), this);
  actionCollection()->addAction("prefer_html", mPreferHtmlAction );
  connect(mPreferHtmlAction, SIGNAL(triggered(bool) ), SLOT(slotOverrideHtml()));

  mPreferHtmlLoadExtAction  = new KToggleAction(i18n("Load E&xternal References"), this);
  actionCollection()->addAction("prefer_html_external_refs", mPreferHtmlLoadExtAction );
  connect(mPreferHtmlLoadExtAction, SIGNAL(triggered(bool) ), SLOT(slotOverrideHtmlLoadExt()));

  mThreadMessagesAction  = new KToggleAction(i18n("&Thread Messages"), this);
  actionCollection()->addAction("thread_messages", mThreadMessagesAction );
  connect(mThreadMessagesAction, SIGNAL(triggered(bool) ), SLOT(slotOverrideThread()));

  mThreadBySubjectAction  = new KToggleAction(i18n("Thread Messages also by &Subject"), this);
  actionCollection()->addAction("thread_messages_by_subject", mThreadBySubjectAction );
  connect(mThreadBySubjectAction, SIGNAL(triggered(bool) ), SLOT(slotToggleSubjectThreading()));

  action = new KAction(KIcon("edit-copy"), i18n("Copy Folder"), this);
  action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_C));
  actionCollection()->addAction("copy_folder", action);
  connect(action, SIGNAL(triggered(bool)), folderTree(), SLOT(copyFolder()));

  action = new KAction(KIcon("edit-cut"), i18n("Cut Folder"), this);
  action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_X));
  actionCollection()->addAction("cut_folder", action);
  connect(action, SIGNAL(triggered(bool)), folderTree(), SLOT(cutFolder()));

  action = new KAction(KIcon("edit-paste"), i18n("Paste Folder"), this);
  action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_V));
  actionCollection()->addAction("paste_folder", action);
  connect(action, SIGNAL(triggered(bool)), folderTree(), SLOT(pasteFolder()));

  action = new KAction(KIcon("edit-copy"), i18n("Copy Messages"), this);
  action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_C));
  actionCollection()->addAction("copy_messages", action);
  connect(action, SIGNAL(triggered(bool)), headers(), SLOT(copyMessages()));

  action = new KAction(KIcon("edit-cut"), i18n("Cut Messages"), this);
  action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_X));
  actionCollection()->addAction("cut_messages", action);
  connect(action, SIGNAL(triggered(bool)), headers(), SLOT(cutMessages()));

  action = new KAction(KIcon("edit-paste"), i18n("Paste Messages"), this);
  action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_V));
  actionCollection()->addAction("paste_messages", action);
  connect(action, SIGNAL(triggered(bool)), headers(), SLOT(pasteMessages()));

  //----- Message Menu
  action  = new KAction(KIcon("mail-new"), i18n("&New Message..."), this);
  actionCollection()->addAction("new_message", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotCompose()));
  action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::New));

  mTemplateMenu = new KActionMenu( KIcon( "document-new" ), i18n("Message From &Template"),
				   actionCollection() );
  mTemplateMenu->setDelayed( true );
  actionCollection()->addAction("new_from_template", mTemplateMenu );
  connect( mTemplateMenu->popupMenu(), SIGNAL( aboutToShow() ), this,
           SLOT( slotShowNewFromTemplate() ) );
  connect( mTemplateMenu->popupMenu(), SIGNAL( activated(int) ), this,
           SLOT( slotNewFromTemplate(int) ) );

  action  = new KAction(KIcon("mail_post_to"), i18n("New Message t&o Mailing-List..."), this);
  actionCollection()->addAction("post_message", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotPostToML()));
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_N));

  mForwardActionMenu  = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(activated()), this,
	   SLOT(slotForwardMsg()) );

  mForwardAttachedAction  = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","As &Attachment..."), this);
  actionCollection()->addAction("message_forward_as_attachment", mForwardAttachedAction );
  mForwardAttachedAction->setShortcut(QKeySequence(Qt::Key_F));
  connect(mForwardAttachedAction, SIGNAL(triggered(bool) ), SLOT(slotForwardAttachedMsg()));
  mForwardActionMenu->addAction( forwardAttachedAction() );
  mForwardAction  = new KAction(KIcon("mail-forward"), i18n("&Inline..."), this);
  actionCollection()->addAction("message_forward_inline", mForwardAction );
  connect(mForwardAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));
  mForwardAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F));

  mForwardActionMenu->addAction( forwardAction() );

  mSendAgainAction  = new KAction(i18n("Send A&gain..."), this);
  actionCollection()->addAction("send_again", mSendAgainAction );
  connect(mSendAgainAction, SIGNAL(triggered(bool) ), SLOT(slotResendMsg()));

  mReplyActionMenu  = new KActionMenu(KIcon("mail-reply-sender"), i18nc("Message->","&Reply"), this);
  actionCollection()->addAction("message_reply_menu", mReplyActionMenu );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
	   SLOT(slotReplyToMsg()) );

  mReplyAction  = new KAction(KIcon("mail-reply-sender"), i18n("&Reply..."), this);
  actionCollection()->addAction("reply", mReplyAction );
  connect(mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()));
  mReplyAction->setShortcut(QKeySequence(Qt::Key_R));
  mReplyActionMenu->addAction( mReplyAction );

  mReplyAuthorAction  = new KAction(KIcon("mail-reply-sender"), i18n("Reply to A&uthor..."), this);
  actionCollection()->addAction("reply_author", mReplyAuthorAction );
  connect(mReplyAuthorAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAuthorToMsg()));
  mReplyAuthorAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_A));
  mReplyActionMenu->addAction( mReplyAuthorAction );

  mReplyAllAction  = new KAction(KIcon("mail-reply-all"), i18n("Reply to &All..."), this);
  actionCollection()->addAction("reply_all", mReplyAllAction );
  connect(mReplyAllAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAllToMsg()));
  mReplyAllAction->setShortcut(QKeySequence(Qt::Key_A));
  mReplyActionMenu->addAction( mReplyAllAction );

  mReplyListAction  = new KAction(KIcon("mail-replylist"), i18n("Reply to Mailing-&List..."), this);
  actionCollection()->addAction("reply_list", mReplyListAction );
  connect(mReplyListAction, SIGNAL(triggered(bool) ), SLOT(slotReplyListToMsg()));
  mReplyListAction->setShortcut(QKeySequence(Qt::Key_L));
  mReplyActionMenu->addAction( mReplyListAction );

  mRedirectAction  = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","&Redirect..."), this);
  actionCollection()->addAction("message_forward_redirect", mRedirectAction );
  mRedirectAction->setShortcut(QKeySequence(Qt::Key_E));
  connect(mRedirectAction, SIGNAL(triggered(bool) ), SLOT(slotRedirectMsg()));
  mForwardActionMenu->addAction( redirectAction() );

  mNoQuoteReplyAction  = new KAction(i18n("Reply Without &Quote..."), this);
  actionCollection()->addAction("noquotereply", mNoQuoteReplyAction );
  connect(mNoQuoteReplyAction, SIGNAL(triggered(bool) ), SLOT(slotNoQuoteReplyToMsg()));
  mNoQuoteReplyAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_R));

  //----- Create filter actions
  mFilterMenu  = new KActionMenu(KIcon("search-filter"), i18n("&Create Filter"), this);
  actionCollection()->addAction("create_filter", mFilterMenu );
  connect( mFilterMenu, SIGNAL(activated()), this,
	   SLOT(slotFilter()) );
  mSubjectFilterAction  = new KAction(i18n("Filter on &Subject..."), this);
  actionCollection()->addAction("subject_filter", mSubjectFilterAction );
  connect(mSubjectFilterAction, SIGNAL(triggered(bool) ), SLOT(slotSubjectFilter()));
  mFilterMenu->addAction( mSubjectFilterAction );

  mFromFilterAction  = new KAction(i18n("Filter on &From..."), this);
  actionCollection()->addAction("from_filter", mFromFilterAction );
  connect(mFromFilterAction, SIGNAL(triggered(bool) ), SLOT(slotFromFilter()));
  mFilterMenu->addAction( mFromFilterAction );

  mToFilterAction  = new KAction(i18n("Filter on &To..."), this);
  actionCollection()->addAction("to_filter", mToFilterAction );
  connect(mToFilterAction, SIGNAL(triggered(bool) ), SLOT(slotToFilter()));
  mFilterMenu->addAction( mToFilterAction );

  mListFilterAction  = new KAction(i18n("Filter on Mailing-&List..."), this);
  actionCollection()->addAction("mlist_filter", mListFilterAction );
  connect(mListFilterAction, SIGNAL(triggered(bool) ), SLOT(slotMailingListFilter()));
  mFilterMenu->addAction( mListFilterAction );

  mPrintAction = KStandardAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  mEditAction  = new KAction(KIcon("edit"), i18n("&Edit Message"), this);
  actionCollection()->addAction("edit", mEditAction );
  connect(mEditAction, SIGNAL(triggered(bool)), SLOT(slotEditMsg()));
  mEditAction->setShortcut(QKeySequence(Qt::Key_T));
#ifdef __GNUC__
#warning Port me!
#endif
//  mEditAction->plugAccel( actionCollection()->kaccel() );

  mUseAction = new KAction( KIcon( "document-new" ), i18n("New Message From &Template"),
                            actionCollection() );
  mUseAction->setShortcut( KShortcut( Qt::Key_N ) );
  connect( mUseAction, SIGNAL(triggered(bool)),
                            this, SLOT(slotUseTemplate()) );
#ifdef __GNUC__
#warning Port me!
#endif
//  mUseAction->plugAccel( actionCollection()->kaccel() );


  //----- "Mark Message" submenu
  mStatusMenu  = new KActionMenu(i18n("Mar&k Message"), this);
  actionCollection()->addAction("set_status", mStatusMenu );

  action  = new KAction(KIcon("kmmsgread"), i18n("Mark Message as &Read"), this);
  actionCollection()->addAction("status_read", action );
  action->setToolTip(i18n("Mark selected messages as read"));
  connect(action, SIGNAL(triggered(bool)), SLOT(slotSetMsgStatusRead()));
  mStatusMenu->addAction(action);

  action  = new KAction(KIcon("kmmsgnew"), i18n("Mark Message as &New"), this);
  actionCollection()->addAction("status_new", action );
  action->setToolTip(i18n("Mark selected messages as new"));
  connect(action, SIGNAL(triggered(bool)), SLOT(slotSetMsgStatusNew()));
  mStatusMenu->addAction(action);

  action  = new KAction(KIcon("kmmsgunseen"), i18n("Mark Message as &Unread"), this);
  actionCollection()->addAction("status_unread", action );
  action->setToolTip(i18n("Mark selected messages as unread"));
  connect(action, SIGNAL(triggered(bool)), SLOT(slotSetMsgStatusUnread()));
  mStatusMenu->addAction(action);

  mStatusMenu->addSeparator();

  // -------- Toggle Actions
  mToggleFlagAction  = new KToggleAction(KIcon("mail-flag-kmail"), i18n("Mark Message as &Important"), this);
  actionCollection()->addAction("status_flag", mToggleFlagAction );
  connect(mToggleFlagAction, SIGNAL(triggered(bool) ), SLOT(slotSetMsgStatusFlag()));
  mToggleFlagAction->setCheckedState( KGuiItem(i18n("Remove &Important Message Mark")) );
  mStatusMenu->addAction( mToggleFlagAction );

  mToggleTodoAction  = new KToggleAction(KIcon("mail_todo"), i18n("Mark Message as &To-do"), this);
  actionCollection()->addAction("status_todo", mToggleTodoAction );
  connect(mToggleTodoAction, SIGNAL(triggered(bool) ), SLOT(slotSetMsgStatusTodo()));
  mToggleTodoAction->setCheckedState( KGuiItem(i18n("Remove &To-do Message Mark")) );
  mStatusMenu->addAction( mToggleTodoAction );

  //----- "Mark Thread" submenu
  mThreadStatusMenu  = new KActionMenu(i18n("Mark &Thread"), this);
  actionCollection()->addAction("thread_status", mThreadStatusMenu );

  mMarkThreadAsReadAction  = new KAction(KIcon("kmmsgread"), i18n("Mark Thread as &Read"), this);
  actionCollection()->addAction("thread_read", mMarkThreadAsReadAction );
  connect(mMarkThreadAsReadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusRead()));
  mMarkThreadAsReadAction->setToolTip(i18n("Mark all messages in the selected thread as read"));
  mThreadStatusMenu->addAction( mMarkThreadAsReadAction );

  mMarkThreadAsNewAction  = new KAction(KIcon("kmmsgnew"), i18n("Mark Thread as &New"), this);
  actionCollection()->addAction("thread_new", mMarkThreadAsNewAction );
  connect(mMarkThreadAsNewAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusNew()));
  mMarkThreadAsNewAction->setToolTip( i18n("Mark all messages in the selected thread as new"));
  mThreadStatusMenu->addAction( mMarkThreadAsNewAction );

  mMarkThreadAsUnreadAction  = new KAction(KIcon("kmmsgunseen"), i18n("Mark Thread as &Unread"), this);
  actionCollection()->addAction("thread_unread", mMarkThreadAsUnreadAction );
  connect(mMarkThreadAsUnreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusUnread()));
  mMarkThreadAsUnreadAction->setToolTip(i18n("Mark all messages in the selected thread as unread"));
  mThreadStatusMenu->addAction( mMarkThreadAsUnreadAction );

  mThreadStatusMenu->addSeparator();

  //----- "Mark Thread" toggle actions
  mToggleThreadFlagAction  = new KToggleAction(KIcon("mail_flag"), i18n("Mark Thread as &Important"), this);
  actionCollection()->addAction("thread_flag", mToggleThreadFlagAction );
  connect(mToggleThreadFlagAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusFlag()));
  mToggleThreadFlagAction->setCheckedState( KGuiItem(i18n("Remove &Important Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadFlagAction );

  mToggleThreadTodoAction  = new KToggleAction(KIcon("mail_todo"), i18n("Mark Thread as &To-do"), this);
  actionCollection()->addAction("thread_todo", mToggleThreadTodoAction );
  connect(mToggleThreadTodoAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusTodo()));
  mToggleThreadTodoAction->setCheckedState( KGuiItem(i18n("Remove &To-do Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadTodoAction );

  //------- "Watch and ignore thread" actions
  mWatchThreadAction  = new KToggleAction(KIcon("kmmsgwatched"), i18n("&Watch Thread"), this);
  actionCollection()->addAction("thread_watched", mWatchThreadAction );
  connect(mWatchThreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusWatched()));

  mIgnoreThreadAction  = new KToggleAction(KIcon("mail_ignore"), i18n("&Ignore Thread"), this);
  actionCollection()->addAction("thread_ignored", mIgnoreThreadAction );
  connect(mIgnoreThreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusIgnored()));

  mThreadStatusMenu->addSeparator();
  mThreadStatusMenu->addAction( mWatchThreadAction );
  mThreadStatusMenu->addAction( mIgnoreThreadAction );

  mSaveAttachmentsAction  = new KAction(KIcon("attach"), i18n("Save A&ttachments..."), this);
  actionCollection()->addAction("file_save_attachments", mSaveAttachmentsAction );
  connect(mSaveAttachmentsAction, SIGNAL(triggered(bool) ), SLOT(slotSaveAttachments()));

  mMoveActionMenu  = new KActionMenu(i18n("&Move To"), this);
  actionCollection()->addAction("move_to", mMoveActionMenu );

  mCopyActionMenu  = new KActionMenu(i18n("&Copy To"), this);
  actionCollection()->addAction("copy_to", mCopyActionMenu );

  mApplyAllFiltersAction  = new KAction(KIcon("search-filter"), i18n("Appl&y All Filters"), this);
  actionCollection()->addAction("apply_filters", mApplyAllFiltersAction );
  connect(mApplyAllFiltersAction, SIGNAL(triggered(bool) ), SLOT(slotApplyFilters()));
  mApplyAllFiltersAction->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_J));

  mApplyFilterActionsMenu  = new KActionMenu(i18n("A&pply Filter"), this);
  actionCollection()->addAction("apply_filter_actions", mApplyFilterActionsMenu );

  //----- View Menu
  // Unread Submenu
  KActionMenu * unreadMenu  = new KActionMenu(i18nc("View->", "&Unread Count"), this);
  actionCollection()->addAction("view_unread", unreadMenu );
  unreadMenu->setToolTip( i18n("Choose how to display the count of unread messages") );
  QActionGroup *group = new QActionGroup( this );

  mUnreadColumnToggle  = new KToggleAction(i18nc("View->Unread Count", "View in &Separate Column"), this);
  actionCollection()->addAction("view_unread_column", mUnreadColumnToggle );
  connect(mUnreadColumnToggle, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)),
          SLOT(slotToggleUnread()));

  group->addAction( mUnreadColumnToggle );
  unreadMenu->addAction( mUnreadColumnToggle );

  mUnreadTextToggle  = new KToggleAction(i18nc("View->Unread Count", "View After &Folder Name"), this);
  actionCollection()->addAction("view_unread_text", mUnreadTextToggle );
  connect(mUnreadTextToggle, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)),
          SLOT(slotToggleUnread()));
  group->addAction( mUnreadTextToggle );
  unreadMenu->addAction( mUnreadTextToggle );

  // toggle for total column
  mTotalColumnToggle  = new KToggleAction(i18nc("View->", "&Total Column"), this);
  actionCollection()->addAction("view_columns_total", mTotalColumnToggle );
  connect(mTotalColumnToggle, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)),
          SLOT(slotToggleTotalColumn()));
  mTotalColumnToggle->setToolTip( i18n("Toggle display of column showing the "
                                      "total number of messages in folders.") );

  action  = new KAction(i18nc("View->","&Expand Thread"), this);
  actionCollection()->addAction("expand_thread", action );
  action->setShortcut(QKeySequence(Qt::Key_Period));
  action->setToolTip(i18n("Expand the current thread"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpandThread()));

  action  = new KAction(i18nc("View->","&Collapse Thread"), this);
  actionCollection()->addAction("collapse_thread", action );
  action->setShortcut(QKeySequence(Qt::Key_Comma));
  action->setToolTip( i18n("Collapse the current thread"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCollapseThread()));

  action  = new KAction(i18nc("View->","Ex&pand All Threads"), this);
  actionCollection()->addAction("expand_all_threads", action );
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Period));
  action->setToolTip( i18n("Expand all threads in the current folder"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpandAllThreads()));

  action  = new KAction(i18nc("View->","C&ollapse All Threads"), this);
  actionCollection()->addAction("collapse_all_threads", action );
  action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Comma));
  action->setToolTip( i18n("Collapse all threads in the current folder"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCollapseAllThreads()));

  mViewSourceAction  = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  QAction *dukeOfMonmoth  = new KAction(i18n("&Display Message"), this);
  actionCollection()->addAction("display_message", dukeOfMonmoth );
  connect(dukeOfMonmoth, SIGNAL(triggered(bool) ), SLOT( slotDisplayCurrentMessage() ));
  dukeOfMonmoth->setShortcut(QKeySequence(Qt::Key_Return));
#ifdef __GNUC__
#warning Port me!
#endif
//  dukeOfMonmoth->plugAccel( actionCollection()->kaccel() );

  //----- Go Menu
  action  = new KAction(i18n("&Next Message"), this);
  actionCollection()->addAction("go_next_message", action );
  action->setShortcuts(KShortcut( "N;Right" ));
  action->setToolTip(i18n("Go to the next message"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotNextMessage()));

  action  = new KAction(i18n("Next &Unread Message"), this);
  actionCollection()->addAction("go_next_unread_message", action );
  action->setShortcut(QKeySequence(Qt::Key_Plus));
  action->setIcon(KIcon(QApplication::isRightToLeft() ? "find-previous" : "find-next"));
  action->setToolTip(i18n("Go to the next unread message"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotNextUnreadMessage()));

  /* ### needs better support from folders:
  new KAction( KGuiItem( i18n("Next &Important Message"), QString(),
                         i18n("Go to the next important message") ),
                         0, this, SLOT(slotNextImportantMessage()),
                         actionCollection(), "go_next_important_message" );
  */

  action  = new KAction(i18n("&Previous Message"), this);
  actionCollection()->addAction("go_prev_message", action );
  action->setToolTip(i18n("Go to the previous message"));
  action->setShortcuts(KShortcut( "P;Left" ));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotPrevMessage()));

  action  = new KAction(i18n("Previous Unread &Message"), this);
  actionCollection()->addAction("go_prev_unread_message", action );
  action->setShortcut(QKeySequence(Qt::Key_Minus));
  action->setToolTip(i18n("Go to the previous unread message"));
  action->setIcon(KIcon(QApplication::isRightToLeft() ? "find-next" : "find-previous"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotPrevUnreadMessage()));

  /* needs better support from folders:
  new KAction( KGuiItem( i18n("Previous I&mportant Message"), QString(),
                         i18n("Go to the previous important message") ),
                         0, this, SLOT(slotPrevImportantMessage()),
                         actionCollection(), "go_prev_important_message" );
  */

  action  = new KAction(i18n("Next Unread &Folder"), this);
  actionCollection()->addAction("go_next_unread_folder", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotNextUnreadFolder()));
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Plus));
  action->setToolTip(i18n("Go to the next folder with unread messages"));
  KShortcut shortcut = KShortcut(action->shortcuts());
  shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Plus ) );
  action->setShortcuts( shortcut );

  action  = new KAction(i18n("Previous Unread F&older"), this);
  actionCollection()->addAction("go_prev_unread_folder", action );
  action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Minus));
  action->setToolTip(i18n("Go to the previous folder with unread messages"));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotPrevUnreadFolder()));
  shortcut = KShortcut(action->shortcuts());
  shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Minus ) );
  action->setShortcuts( shortcut );

  action  = new KAction(i18nc("Go->","Next Unread &Text"), this);
  actionCollection()->addAction("go_next_unread_text", action );
  action->setShortcut(QKeySequence(Qt::Key_Space));
  action->setToolTip(i18n("Go to the next unread text"));
  action->setWhatsThis( i18n("Scroll down current message. "
                              "If at end of current message, "
                              "go to next unread message."));
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotReadOn()));

  //----- Settings Menu
    mToggleShowQuickSearchAction  = new KToggleAction(i18n("Show Quick Search"), this);
    actionCollection()->addAction("show_quick_search", mToggleShowQuickSearchAction );
  connect(mToggleShowQuickSearchAction, SIGNAL(triggered(bool) ), SLOT(slotToggleShowQuickSearch()));
  mToggleShowQuickSearchAction->setChecked( GlobalSettings::self()->quickSearchActive() );
  mToggleShowQuickSearchAction->setWhatsThis(
        i18n( GlobalSettings::self()->quickSearchActiveItem()->whatsThis().toUtf8() ) );

  action  = new KAction(i18n("Configure &Filters..."), this);
  actionCollection()->addAction("filter", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotFilter()));
  action  = new KAction(i18n("Configure &POP Filters..."), this);
  actionCollection()->addAction("popFilter", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotPopFilter()));
  action  = new KAction(i18n("Manage &Sieve Scripts..."), this);
  actionCollection()->addAction("sieveFilters", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotManageSieveScripts()));

  action  = new KAction(i18n("KMail &Introduction"), this);
  actionCollection()->addAction("help_kmail_welcomepage", action );
  action->setToolTip( i18n("Display KMail's Welcome Page") );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotIntro()));


  // ----- Standard Actions
//  KStandardAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
    action  = new KAction(KIcon("knotify"), i18n("Configure &Notifications..."), this);
    actionCollection()->addAction("kmail_configure_notifications", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotEditNotifications()));
//  KStandardAction::preferences(this, SLOT(slotSettings()), actionCollection());
    action  = new KAction(KIcon("configure"), i18n("&Configure KMail..."), this);
    actionCollection()->addAction("kmail_configure_kmail", action );
  connect(action, SIGNAL(triggered(bool) ), kmkernel, SLOT(slotShowConfigurationDialog()));

  actionCollection()->addAction(KStandardAction::Undo,  "kmail_undo", this, SLOT(slotUndo()));

  KStandardAction::tipOfDay( this, SLOT( slotShowTip() ), actionCollection() );

  menutimer = new QTimer( this );
  menutimer->setObjectName( "menutimer" );
  menutimer->setSingleShot( true );
  connect( menutimer, SIGNAL( timeout() ), SLOT( updateMessageActions() ) );
  connect( kmkernel->undoStack(),
           SIGNAL( undoStackChanged() ), this, SLOT( slotUpdateUndo() ));

  initializeIMAPActions( false ); // don't set state, config not read yet
  updateMessageActions();
  updateCustomTemplateMenus();
  updateFolderMenu();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEditNotifications()
{
//  if(kmkernel->xmlGuiInstance())
//    KNotifyDialog::configure(this, 0, kmkernel->xmlGuiInstance()->aboutData());
//  else
    KNotifyConfigWidget::configure(this);
}

void KMMainWidget::slotEditKeys()
{
  KShortcutsDialog::configure( actionCollection(),
    KShortcutsEditor::LetterShortcutsAllowed );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReadOn()
{
    if ( !mMsgView )
        return;

    if ( !mMsgView->atBottom() ) {
        mMsgView->slotJumpDown();
        return;
    }
    slotNextUnreadMessage();
}

void KMMainWidget::slotNextUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->nextUnreadFolder();
}

void KMMainWidget::slotPrevUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->prevUnreadFolder();
}

void KMMainWidget::slotExpandThread()
{
  mHeaders->slotExpandOrCollapseThread( true ); // expand
}

void KMMainWidget::slotCollapseThread()
{
  mHeaders->slotExpandOrCollapseThread( false ); // collapse
}

void KMMainWidget::slotExpandAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( true ); // expand
}

void KMMainWidget::slotCollapseAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( false ); // collapse
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowMsgSrc()
{
  if ( mMsgView )
    mMsgView->setUpdateAttachment( false );

  KMMessage *msg = mHeaders->currentMsg();
  if ( !msg )
    return;
  KMCommand *command = new KMShowMsgSrcCommand( this, msg,
                                                mMsgView
                                                ? mMsgView->isFixedFont()
                                                : false );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::moveSelectedToFolder( QAction* act )
{
  if (mMenuToFolder[act])
    mHeaders->moveMsgToFolder( mMenuToFolder[act] );
}


//-----------------------------------------------------------------------------
void KMMainWidget::copySelectedToFolder( QAction* act )
{
  if (mMenuToFolder[act])
    mHeaders->copyMsgToFolder( mMenuToFolder[act] );
}


//-----------------------------------------------------------------------------
void KMMainWidget::updateMessageMenu()
{
  mMenuToFolder.clear();
  folderTree()->folderToPopupMenu( KMFolderTree::MoveMessage, this, &mMenuToFolder, mMoveActionMenu->menu() );
  folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage, this, &mMenuToFolder, mCopyActionMenu->menu() );
  updateMessageActions();
}

void KMMainWidget::startUpdateMessageActionsTimer()
{
    menutimer->stop();
    menutimer->start( 20 );
}

void KMMainWidget::updateMessageActions()
{
    int count = 0;
    QList<Q3ListViewItem*> selectedItems;

    if ( mFolder ) {
      for (Q3ListViewItem *item = mHeaders->firstChild(); item; item = item->itemBelow())
        if (item->isSelected() )
          selectedItems.append(item);
      if ( selectedItems.isEmpty() && mFolder->count() ) // there will always be one in mMsgView
        count = 1;
      else
        count = selectedItems.count();
    }

    updateListFilterAction();

    bool allSelectedInCommonThread = false;
    if ( mHeaders->isThreaded() && count > 1 ) {
      allSelectedInCommonThread = true;
      Q3ListViewItem * curItemParent = mHeaders->currentItem();
      while ( curItemParent->parent() )
        curItemParent = curItemParent->parent();
      QList<Q3ListViewItem*>::const_iterator it;
      for ( it = selectedItems.begin(); it != selectedItems.end(); ++ it ) {
        Q3ListViewItem * item = *it;
        while ( item->parent() )
          item = item->parent();
        if ( item != curItemParent ) {
          allSelectedInCommonThread = false;
          break;
        }
      }
    }
    else if ( mHeaders->isThreaded() && count == 1 ) {
      allSelectedInCommonThread = true;
    }

    bool mass_actions = count >= 1;
    bool thread_actions = mass_actions && allSelectedInCommonThread &&
                          mHeaders->isThreaded();
    mStatusMenu->setEnabled( mass_actions );
    mThreadStatusMenu->setEnabled( thread_actions );
    // these need to be handled individually, the user might have them
    // in the toolbar
    mWatchThreadAction->setEnabled( thread_actions );
    mIgnoreThreadAction->setEnabled( thread_actions );
    mMarkThreadAsNewAction->setEnabled( thread_actions );
    mMarkThreadAsReadAction->setEnabled( thread_actions );
    mMarkThreadAsUnreadAction->setEnabled( thread_actions );
    mToggleThreadTodoAction->setEnabled( thread_actions );
    mToggleThreadFlagAction->setEnabled( thread_actions );
    mTrashThreadAction->setEnabled( thread_actions && !mFolder->isReadOnly() );
    mDeleteThreadAction->setEnabled( thread_actions && !mFolder->isReadOnly() );

    if ( mFolder && mHeaders && mHeaders->currentMsg() ) {
      MessageStatus status = mHeaders->currentMsg()->status();
      mToggleTodoAction->setChecked( status.isTodo() );
      mToggleFlagAction->setChecked( status.isImportant() );
      if (thread_actions) {
        mToggleThreadTodoAction->setChecked( status.isTodo() );
        mToggleThreadFlagAction->setChecked( status.isImportant() );
        mWatchThreadAction->setChecked( status.isWatched() );
        mIgnoreThreadAction->setChecked( status.isIgnored() );
      }
    }

    mMoveActionMenu->setEnabled( mass_actions && !mFolder->isReadOnly() );
    mCopyActionMenu->setEnabled( mass_actions );
    mTrashAction->setEnabled( mass_actions && !mFolder->isReadOnly() );
    mDeleteAction->setEnabled( mass_actions && !mFolder->isReadOnly() );
    mFindInMessageAction->setEnabled( mass_actions );
    mForwardAction->setEnabled( mass_actions );
    mForwardAttachedAction->setEnabled( mass_actions );

    forwardMenu()->setEnabled( mass_actions );

    bool single_actions = count == 1;

    mEditAction->setEnabled( single_actions &&
                             ( kmkernel->folderIsDraftOrOutbox( mFolder ) ||
                               kmkernel->folderIsTemplates( mFolder ) ) );
    mUseAction->setEnabled( single_actions &&
                            kmkernel->folderIsTemplates( mFolder ) );

    replyMenu()->setEnabled( single_actions );
    filterMenu()->setEnabled( single_actions );
    replyAction()->setEnabled( single_actions );
    noQuoteReplyAction()->setEnabled( single_actions );
    replyAuthorAction()->setEnabled( single_actions );
    replyAllAction()->setEnabled( single_actions );
    replyListAction()->setEnabled( single_actions );
    redirectAction()->setEnabled( single_actions );
    printAction()->setEnabled( single_actions );
    viewSourceAction()->setEnabled( single_actions );

    mSendAgainAction->setEnabled( single_actions &&
             ( mHeaders->currentMsg() && mHeaders->currentMsg()->status().isSent() )
          || ( mFolder && mHeaders->currentMsg() &&
               ( kmkernel->folderIsDraftOrOutbox( mFolder )
          || kmkernel->folderIsSentMailFolder( mFolder ) ) ) );
    mSaveAsAction->setEnabled( mass_actions );
    bool mails = mFolder && mFolder->count();
    bool enable_goto_unread = mails
       || (GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders);
    actionCollection()->action( "go_next_message" )->setEnabled( mails );
    actionCollection()->action( "go_next_unread_message" )->setEnabled( enable_goto_unread );
    actionCollection()->action( "go_prev_message" )->setEnabled( mails );
    actionCollection()->action( "go_prev_unread_message" )->setEnabled( enable_goto_unread );
    actionCollection()->action( "send_queued" )->setEnabled( kmkernel->outboxFolder()->count() > 0 );
    actionCollection()->action( "send_queued_via" )->setEnabled( kmkernel->outboxFolder()->count() > 0 );
    slotUpdateOnlineStatus( static_cast<GlobalSettingsBase::EnumNetworkState::type>( GlobalSettings::self()->networkState() ) );
    if (action( "edit_undo" ))
      action( "edit_undo" )->setEnabled( mHeaders->canUndo() );

    if ( count == 1 ) {
      KMMessage *msg;
      int aIdx;
      if((aIdx = mHeaders->currentItemIndex()) <= -1)
        return;
      if(!(msg = mFolder->getMsg(aIdx)))
        return;

      if (mFolder == kmkernel->outboxFolder())
        mEditAction->setEnabled( !msg->transferInProgress() );
    }

    mApplyAllFiltersAction->setEnabled(count);
    mApplyFilterActionsMenu->setEnabled(count);
}

// This needs to be updated more often, so it is in its method.
void KMMainWidget::updateMarkAsReadAction()
{
  mMarkAllAsReadAction->setEnabled( mFolder && (mFolder->countUnread() > 0) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFolderMenu()
{
  bool folderWithContent = mFolder && !mFolder->noContent();
  bool multiFolder = folderTree()->selectedFolders().count() > 1;
  mModifyFolderAction->setEnabled( folderWithContent && !multiFolder );
  mFolderMailingListPropertiesAction->setEnabled( folderWithContent && !multiFolder );
  mCompactFolderAction->setEnabled( folderWithContent && !multiFolder );

  // This is the refresh-folder action in the menu. See kmfoldertree for the one in the RMB...
  bool imap = mFolder && mFolder->folderType() == KMFolderTypeImap;
  bool cachedImap = mFolder && mFolder->folderType() == KMFolderTypeCachedImap;
  // For dimap, check that the imap path is known before allowing "check mail in this folder".
  bool knownImapPath = cachedImap && !static_cast<KMFolderCachedImap*>( mFolder->storage() )->imapPath().isEmpty();
  mRefreshFolderAction->setEnabled( folderWithContent && ( imap
                                                           || ( cachedImap && knownImapPath ) ) && !multiFolder );
  if ( mTroubleshootFolderAction )
    mTroubleshootFolderAction->setEnabled( folderWithContent && ( cachedImap && knownImapPath ) && !multiFolder );
  mEmptyFolderAction->setEnabled( folderWithContent && ( mFolder->count() > 0 ) && !mFolder->isReadOnly() && !multiFolder );
  mEmptyFolderAction->setText( (mFolder && kmkernel->folderIsTrash(mFolder))
    ? i18n("E&mpty Trash") : i18n("&Move All Messages to Trash") );
  mRemoveFolderAction->setEnabled( mFolder && !mFolder->isSystemFolder() && !mFolder->isReadOnly() && !multiFolder);
  mRemoveFolderAction->setText( mFolder && mFolder->folderType() == KMFolderTypeSearch ? i18n("&Delete Search") : i18n("&Delete Folder") );
  mExpireFolderAction->setEnabled( mFolder && mFolder->isAutoExpire() && !multiFolder );
  updateMarkAsReadAction();
  // the visual ones only make sense if we are showing a message list
  mPreferHtmlAction->setEnabled( mHeaders->folder() ? true : false );
  mPreferHtmlLoadExtAction->setEnabled( mHeaders->folder() && (mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref) ? true : false );
  mThreadMessagesAction->setEnabled( mHeaders->folder() ? true : false );

  mPreferHtmlAction->setChecked( mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref );
  mPreferHtmlLoadExtAction->setChecked( mHtmlLoadExtPref ? !mFolderHtmlLoadExtPref : mFolderHtmlLoadExtPref );
  mThreadMessagesAction->setChecked(
      mThreadPref ? !mFolderThreadPref : mFolderThreadPref );
  mThreadBySubjectAction->setEnabled(
      mHeaders->folder() ? ( mThreadMessagesAction->isChecked()) : false );
  mThreadBySubjectAction->setChecked( mFolderThreadSubjPref );

  mNewFolderAction->setEnabled( !multiFolder );
  mRemoveDuplicatesAction->setEnabled( !multiFolder );
  mFolderShortCutCommandAction->setEnabled( !multiFolder );
}


#ifdef MALLOC_DEBUG
static QString fmt(long n) {
  char buf[32];

  if(n > 1024*1024*1024)
    sprintf(buf, "%0.2f GB", ((double)n)/1024.0/1024.0/1024.0);
  else if(n > 1024*1024)
    sprintf(buf, "%0.2f MB", ((double)n)/1024.0/1024.0);
  else if(n > 1024)
    sprintf(buf, "%0.2f KB", ((double)n)/1024.0);
  else
    sprintf(buf, "%ld Byte", n);
  return QString(buf);
}
#endif

void KMMainWidget::slotMemInfo() {
#ifdef MALLOC_DEBUG
  struct mallinfo mi;

  mi = mallinfo();
  QString s = QString("\nMALLOC - Info\n\n"
		      "Number of mmapped regions : %1\n"
		      "Memory allocated in use   : %2\n"
		      "Memory allocated, not used: %3\n"
		      "Memory total allocated    : %4\n"
		      "Max. freeable memory      : %5\n")
    .arg(mi.hblks).arg(fmt(mi.uordblks)).arg(fmt(mi.fordblks))
    .arg(fmt(mi.arena)).arg(fmt(mi.keepcost));
  KMessageBox::information(0, s, "Malloc information", s);
#endif
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotIntro()
{
  if ( !mMsgView ) return;

  mMsgView->clear( true );
  // hide widgets that are in the way:
  if ( mSearchAndHeaders && mHeaders && mLongFolderList )
    mSearchAndHeaders->hide();


  mMsgView->displayAboutPage();

  mFolder = 0;
}

void KMMainWidget::slotShowStartupFolder()
{
  if ( mFolderTree ) {
    mFolderTree->reload();
    mFolderTree->readConfig();
    // get rid of old-folders
    mFolderTree->cleanupConfigFile();
  }

  connect( kmkernel->filterMgr(), SIGNAL( filterListUpdated() ),
	   this, SLOT( initializeFilterActions() ));

  // plug shortcut filter actions now
  initializeFilterActions();

  // plug folder shortcut actions
  initializeFolderShortcutActions();

  QString newFeaturesMD5 = KMReaderWin::newFeaturesMD5();
  if ( kmkernel->firstStart() ||
       GlobalSettings::self()->previousNewFeaturesMD5() != newFeaturesMD5 ) {
    GlobalSettings::self()->setPreviousNewFeaturesMD5( newFeaturesMD5 );
    slotIntro();
    return;
  }

  KMFolder* startup = 0;
  if ( !mStartupFolder.isEmpty() ) {
    // find the startup-folder
    startup = kmkernel->findFolderById( mStartupFolder );
  }
  if ( !startup )
    startup = kmkernel->inboxFolder();

  if ( mFolderTree ) {
    mFolderTree->showFolder( startup );
  }
}

void KMMainWidget::slotShowTip()
{
  KTipDialog::showTip( this, QString(), true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotChangeCaption(Q3ListViewItem * i)
{
  if ( !i ) return;
  // set the caption to the current full path
  QStringList names;
  for ( Q3ListViewItem * item = i ; item ; item = item->parent() )
    names.prepend( item->text(0) );
  emit captionChangeRequest( names.join( "/" ) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::removeDuplicates()
{
  if ( !mFolder ) {
    return;
  }
  KMFolder *oFolder = mFolder;
  mHeaders->setFolder( 0 );
  QMap< QString, QList<int> > idMD5s;
  QList<int> redundantIds;
  QList<int>::Iterator kt;
  mFolder->open( "removedups" );
  for ( int i = mFolder->count() - 1; i >= 0; --i ) {
    QString id = (*mFolder)[i]->msgIdMD5();
    if ( !id.isEmpty() ) {
      QString subjMD5 = (*mFolder)[i]->strippedSubjectMD5();
      int other = -1;
      if ( idMD5s.contains(id) ) {
        other = idMD5s[id].first();
      } else {
        idMD5s[id].append( i );
      }
      if ( other != -1 ) {
        QString otherSubjMD5 = (*mFolder)[other]->strippedSubjectMD5();
        if ( otherSubjMD5 == subjMD5 ) {
          idMD5s[id].append( i );
        }
      }
    }
  }

  QMap< QString, QList<int> >::Iterator it;
  for ( it = idMD5s.begin(); it != idMD5s.end() ; ++it ) {
    QList<int>::Iterator jt;
    bool finished = false;
    for ( jt = (*it).begin(); jt != (*it).end() && !finished; ++jt )
      if (!((*mFolder)[*jt]->status().isUnread())) {
        (*it).erase( jt );
        (*it).prepend( *jt );
        finished = true;
      }
    for ( jt = (*it).begin(), ++jt; jt != (*it).end(); ++jt )
      redundantIds.append( *jt );
  }
  qSort( redundantIds );
  kt = redundantIds.end();
  int numDuplicates = 0;
  if (kt != redundantIds.begin()) do {
    mFolder->removeMsg( *(--kt) );
    ++numDuplicates;
  }
  while (kt != redundantIds.begin());

  mFolder->close( "removedups" );
  mHeaders->setFolder(oFolder);
  QString msg;
  if ( numDuplicates )
    msg = i18np("Removed %1 duplicate message.",
               "Removed %1 duplicate messages.", numDuplicates );
    else
      msg = i18n("No duplicate messages found.");
  BroadcastStatus::instance()->setStatusMsg( msg );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateUndo()
{
    if (actionCollection()->action( "edit_undo" ))
        actionCollection()->action( "edit_undo" )->setEnabled( mHeaders->canUndo() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::clearFilterActions()
{
  if ( !mFilterTBarActions.isEmpty() ) {
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "toolbar_filter_actions" );
    qDeleteAll( mFilterTBarActions );
    mFilterTBarActions.clear();
  }
  mApplyFilterActionsMenu->menu()->clear();
  if ( !mFilterMenuActions.isEmpty() ) {
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "menu_filter_actions" );
    qDeleteAll( mFilterMenuActions );
    mFilterMenuActions.clear();
  }
  mFilterCommands.clear();
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeFolderShortcutActions()
{

  // If we are loaded as a part, this will be set to fals, since the part
  // does xml loading. Temporarily set to true, in that case, so the
  // accels are added to the collection as expected.
#ifdef __GNUC__
#warning Port me: *AutoConnectShortcuts has beeen removed from KActionCollection
#endif
//  bool old = actionCollection()->isAutoConnectShortcuts();

//  actionCollection()->setAutoConnectShortcuts( true );
  QList< QPointer< KMFolder > > folders = kmkernel->allFolders();
  QList< QPointer< KMFolder > >::Iterator it = folders.begin();
  while ( it != folders.end() ) {
    KMFolder *folder = (*it);
    ++it;
    slotShortcutChanged( folder ); // load the initial accel
  }
//  actionCollection()->setAutoConnectShortcuts( old );
}


//-----------------------------------------------------------------------------
void KMMainWidget::initializeFilterActions()
{
  QString filterName, normalizedName;
  KMMetaFilterActionCommand *filterCommand;
  QAction *filterAction = 0;

  clearFilterActions();
  mApplyFilterActionsMenu->menu()->addAction( mApplyAllFiltersAction );
  bool addedSeparator = false;
  QList<KMFilter*>::const_iterator it = kmkernel->filterMgr()->filters().begin();
  for ( ;it != kmkernel->filterMgr()->filters().end(); ++it ) {
    if (!(*it)->isEmpty() && (*it)->configureShortcut()) {
      filterName = QString("Filter %1").arg((*it)->name());
      normalizedName = filterName.replace(" ", "_");
      if (action(normalizedName.toUtf8()))
        continue;
      filterCommand = new KMMetaFilterActionCommand(*it, mHeaders, this);
      mFilterCommands.append(filterCommand);
      QString as = i18n("Filter %1", (*it)->name());
      QString icon = (*it)->icon();
      if ( icon.isEmpty() )
        icon = "gear";
      filterAction = new KAction(KIcon(icon), as, actionCollection());
      actionCollection()->addAction(normalizedName.toLocal8Bit(), filterAction);
      connect(filterAction, SIGNAL(triggered(bool) ), filterCommand, SLOT(start()));
      filterAction->setShortcuts((*it)->shortcut());
      if(!addedSeparator) {
        mApplyFilterActionsMenu->menu()->addSeparator();
        addedSeparator = !addedSeparator;
        QAction *a = new QAction(this);
        a->setSeparator(true);
        mFilterMenuActions.append(a);
      }
      mApplyFilterActionsMenu->menu()->addAction( filterAction );
      mFilterMenuActions.append(filterAction);
      if ( (*it)->configureToolbar() )
        mFilterTBarActions.append(filterAction);
    }
  }
  if ( !mFilterMenuActions.isEmpty() && mGUIClient->factory() )
    mGUIClient->plugActionList( "menu_filter_actions", mFilterMenuActions );
  if ( !mFilterTBarActions.isEmpty() && mGUIClient->factory() ) {
    mFilterTBarActions.prepend( mToolbarActionSeparator );
    mGUIClient->plugActionList( "toolbar_filter_actions", mFilterTBarActions );
  }
}

void KMMainWidget::slotFolderRemoved( KMFolder *folder )
{
  mFolderShortcutCommands.remove( folder->idString() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeIMAPActions( bool setState /* false the first time, true later on */ )
{
  bool hasImapAccount = false;
  for( KMAccount *a = kmkernel->acctMgr()->first(); a;
       a = kmkernel->acctMgr()->next() ) {
    if ( a->type() == "cachedimap" ) {
      hasImapAccount = true;
      break;
    }
  }
  if ( hasImapAccount == ( mTroubleshootFolderAction != 0 ) )
    return; // nothing to do

  KXMLGUIFactory* factory = mGUIClient->factory();
  if ( factory )
    factory->removeClient( mGUIClient );

  if ( !mTroubleshootFolderAction ) {
    mTroubleshootFolderAction  = new KAction(KIcon("wizard"), i18n("&Troubleshoot IMAP Cache..."), this);
    actionCollection()->addAction("troubleshoot_folder", mTroubleshootFolderAction );
    connect(mTroubleshootFolderAction, SIGNAL(triggered(bool)), SLOT(slotTroubleshootFolder()));
    if ( setState )
      updateFolderMenu(); // set initial state of the action
  } else {
    delete mTroubleshootFolderAction ;
    mTroubleshootFolderAction = 0;
  }

  if ( factory )
    factory->addClient( mGUIClient );
}

bool KMMainWidget::shortcutIsValid( const QKeySequence &sc ) const
{
  QList<QAction*> actions = actionCollection()->actions();
  foreach ( QAction *a, actions ) {
    if ( a->shortcut() == sc ) return false;
  }
  return true;
}

void KMMainWidget::slotShortcutChanged( KMFolder *folder )
{
  // remove the old one, autodelete
  mFolderShortcutCommands.remove( folder->idString() );
  if ( folder->shortcut().isEmpty() )
    return;

  FolderShortcutCommand *c = new FolderShortcutCommand( this, folder );
  mFolderShortcutCommands.insert( folder->idString(), c );

  QString actionlabel = QString( "FolderShortcut %1").arg( folder->prettyUrl() );
  QString actionname = QString( "FolderShortcut %1").arg( folder->idString() );
  QString normalizedName = actionname.replace(" ", "_");
  QAction * action = actionCollection()->addAction( normalizedName);
  action->setText( actionlabel );
  connect(action, SIGNAL(triggered(bool) ), c, SLOT(start()));
  action->setShortcuts(folder->shortcut());
  action->setIcon( KIcon( folder->unreadIconPath() ) );
  c->setAction( action ); // will be deleted along with the command
}

//-----------------------------------------------------------------------------
QString KMMainWidget::findCurrentImapPath()
{
  QString startPath;
  if (!mFolder) return startPath;
  if (mFolder->folderType() == KMFolderTypeImap)
  {
    startPath = static_cast<KMFolderImap*>(mFolder->storage())->imapPath();
  } else if (mFolder->folderType() == KMFolderTypeCachedImap)
  {
    startPath = static_cast<KMFolderCachedImap*>(mFolder->storage())->imapPath();
  }
  return startPath;
}

//-----------------------------------------------------------------------------
ImapAccountBase* KMMainWidget::findCurrentImapAccountBase()
{
  ImapAccountBase* account = 0;
  if (!mFolder) return account;
  if (mFolder->folderType() == KMFolderTypeImap)
  {
    account = static_cast<KMFolderImap*>(mFolder->storage())->account();
  } else if (mFolder->folderType() == KMFolderTypeCachedImap)
  {
    account = static_cast<KMFolderCachedImap*>(mFolder->storage())->account();
  }
  return account;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubscriptionDialog()
{
  if ( !kmkernel->askToGoOnline() )
    return;
  ImapAccountBase* account = findCurrentImapAccountBase();
  if ( !account ) return;
  const QString startPath = findCurrentImapPath();

  // KSubscription sets "DestruciveClose"
  SubscriptionDialog * dialog =
      new SubscriptionDialog(this, i18n("Subscription"), account, startPath);
  if ( dialog->exec() ) {
    // start a new listing
    if (mFolder->folderType() == KMFolderTypeImap)
      static_cast<KMFolderImap*>(mFolder->storage())->account()->listDirectory();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotLocalSubscriptionDialog()
{
  ImapAccountBase* account = findCurrentImapAccountBase();
  if ( !account ) return;

  const QString startPath = findCurrentImapPath();
  // KSubscription sets "DestruciveClose"
  LocalSubscriptionDialog *dialog =
      new LocalSubscriptionDialog(this, i18n("Local Subscription"), account, startPath);
  if ( dialog->exec() ) {
    // start a new listing
    if (mFolder->folderType() == KMFolderTypeImap)
      static_cast<KMFolderImap*>(mFolder->storage())->account()->listDirectory();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderTreeColumnsChanged()
{
  mTotalColumnToggle->setChecked( mFolderTree->isTotalActive() );
  mUnreadColumnToggle->setChecked( mFolderTree->isUnreadActive() );
}

void KMMainWidget::toggleSystemTray()
{
  if ( !mSystemTray && GlobalSettings::self()->systemTrayEnabled() ) {
    mSystemTray = new KMSystemTray();
  }
  else if ( mSystemTray && !GlobalSettings::self()->systemTrayEnabled() ) {
    // Get rid of system tray on user's request
    kDebug(5006) << "deleting systray" << endl;
    delete mSystemTray;
    mSystemTray = 0;
  }

  // Set mode of systemtray. If mode has changed, tray will handle this.
  if ( mSystemTray )
    mSystemTray->setMode( GlobalSettings::self()->systemTrayPolicy() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiSpamWizard()
{
  AntiSpamWizard wiz( AntiSpamWizard::AntiSpam, this, folderTree() );
  wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiVirusWizard()
{
  AntiSpamWizard wiz( AntiSpamWizard::AntiVirus, this, folderTree() );
  wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFilterLogViewer()
{
  FilterLogDialog * dlg = new FilterLogDialog( 0 );
  dlg->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFileMenu()
{
  QStringList actList = kmkernel->acctMgr()->getAccounts();

  actionCollection()->action("check_mail")->setEnabled( actList.size() > 0 );
  actionCollection()->action("check_mail_in")->setEnabled( actList.size() > 0 );
}


//-----------------------------------------------------------------------------
void KMMainWidget::setAccelsEnabled( bool enabled )
{
  if ( mAccel )
    mAccel->setEnabled( enabled );
}


//-----------------------------------------------------------------------------
KMSystemTray *KMMainWidget::systray() const
{
  return mSystemTray;
}

//-----------------------------------------------------------------------------
QString KMMainWidget::overrideEncoding() const
{
  if ( mMsgView )
    return mMsgView->overrideEncoding();
  else
    return GlobalSettings::self()->overrideCharacterEncoding();
}
