/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2002 Don Sanders <sanders@kde.org>

  Based on the work of Stefan Taferner <taferner@kde.org>

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

//#define TEST_DOCKWIDGETS 1

#include <config-kmail.h>

#include <assert.h>

#include <QByteArray>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QSignalMapper>
#include <QSplitter>
#include <QToolBar>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QShortcut>
#include <QProcess>
#include <QDockWidget> // TEST_DOCKWIDGETS
#include <QMainWindow> // TEST_DOCKWIDGETS

#include <kaboutdata.h>
#include <kicon.h>
#include <kwindowsystem.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kacceleratormanager.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kstandardshortcut.h>
#include <kshortcutsdialog.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <ktip.h>
#include <kstandarddirs.h>
#include <kstandardaction.h>
#include <kaddrbookexternal.h>
#include <ktoggleaction.h>
#include <knotification.h>
#include <knotifyconfigwidget.h>
#include <kstringhandler.h>
#include <kconfiggroup.h>
#include <ktoolinvocation.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <kvbox.h>
#include <ktreewidgetsearchline.h>

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>

#include <kmime/kmime_mdn.h>
#include <kmime/kmime_header_parsing.h>
using namespace KMime;
using KMime::Types::AddrSpecList;

#include "progressmanager.h"
using KPIM::ProgressManager;
#include "globalsettings.h"
#include "kcursorsaver.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfoldermgr.h"
#include "kmfolderdialog.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmfilter.h"
#include "kmreadermainwin.h"
#include "kmfoldercachedimap.h"
#include "kmfolderimap.h"
#include "foldershortcutdialog.h"
#include "kmacctcachedimap.h"
#include "composer.h"
#include "folderselectiondialog.h"
#include "kmfiltermgr.h"
#include "messagesender.h"
#include "kmaddrbook.h"
#include "kmversion.h"
#include "searchwindow.h"
using KMail::SearchWindow;
#include "kmacctfolder.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmsgdict.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "kmmessagetag.h"
#include "imapaccountbase.h"
using KMail::ImapAccountBase;
#include "vacation.h"
using KMail::Vacation;
#include "favoritefolderview.h"

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
#include "mailinglistpropertiesdialog.h"
#include "templateparser.h"
using KMail::TemplateParser;
#include "statusbarlabel.h"
#include "actionscheduler.h"
#include "accountwizard.h"

#if !defined(NDEBUG)
    #include "sievedebugdialog.h"
    using KMail::SieveDebugDialog;
#endif

#include "messagecopyhelper.h"
#include "managesievescriptsdialog.h"
#include "customtemplatesmenu.h"
#include "mainfolderview.h"
#include "messagelistview/pane.h"
#include "messagelistview/messageset.h"
#include "messagetree.h"

#include <kabc/stdaddressbook.h>
#include <kpimutils/email.h>

#ifdef Nepomuk_FOUND
  #include <nepomuk/tag.h>
#endif

#include <errno.h> // ugh

#include "kmmainwidget.moc"

K_GLOBAL_STATIC( KMMainWidget::PtrList, theMainWidgetList )

//-----------------------------------------------------------------------------
KMMainWidget::KMMainWidget( QWidget *parent, KXMLGUIClient *aGUIClient,
                            KActionCollection *actionCollection, KConfig *config ) :
    QWidget( parent ),
    mFavoritesCheckMailAction( 0 ),
    mFavoriteFolderView( 0 ),
    mMsgView( 0 ),
    mSplitter1( 0 ),
    mSplitter2( 0 ),
    mFolderViewSplitter( 0 ),
    mShowBusySplashTimer( 0 ),
    mShowingOfflineScreen( false ),
    mMsgActions( 0 ),
    mVacationIndicatorActive( false ),
    mGoToFirstUnreadMessageInSelectedFolder( false )
{
  // must be the first line of the constructor:
  mStartupDone = false;
  mWasEverShown = false;
  mSearchWin = 0;
  mMainFolderView = 0;
  mMessageListView = 0;
  mIntegrated  = true;
  mFolder = 0;
  mTemplateFolder = 0;
  mReaderWindowActive = true;
  mReaderWindowBelow = true;
  mFolderHtmlPref = false;
  mFolderHtmlLoadExtPref = false;
  mSystemTray = 0;
  mDestructed = false;
  mMessageTagToggleMapper = 0;
  mActionCollection = actionCollection;
  mTopLayout = new QVBoxLayout( this );
  mTopLayout->setMargin( 0 );
  mJob = 0;
  mConfig = config;
  mGUIClient = aGUIClient;
  mOpenedImapFolder = false;
  mCustomTemplateMenus = 0;

  // Create the FolderViewManager that will handle the views for this widget.
  // We need it to be created before all the FolderView instances are created
  // and destroyed after they are destroyed.
  mFolderViewManager = new KMail::FolderViewManager( this, "FolderViewManager" );

  // FIXME This should become a line separator as soon as the API
  // is extended in kdelibs.
  mToolbarActionSeparator = new QAction( this );
  mToolbarActionSeparator->setSeparator( true );

  mMessageTagToolbarActionSeparator = new QAction( this );
  mMessageTagToolbarActionSeparator->setSeparator( true );

  theMainWidgetList->append( this );

  readPreConfig();
  createWidgets();

  setupActions();

  readConfig();

  QTimer::singleShot( 0, this, SLOT( slotShowStartupFolder() ));

  connect( mFolderViewManager, SIGNAL( folderActivated( KMFolder *, bool ) ),
           this, SLOT( slotFolderViewManagerFolderActivated( KMFolder *, bool ) ) );

  connect( kmkernel->acctMgr(), SIGNAL( checkedMail( bool, bool, const QMap<QString, int> & ) ),
           this, SLOT( slotMailChecked( bool, bool, const QMap<QString, int> & ) ) );

  connect( kmkernel->acctMgr(), SIGNAL( accountAdded( KMAccount* ) ),
           this, SLOT( initializeIMAPActions() ) );
  connect( kmkernel->acctMgr(), SIGNAL( accountRemoved( KMAccount* ) ),
           this, SLOT( initializeIMAPActions() ) );

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );

  connect( kmkernel->folderMgr(), SIGNAL( folderRemoved(KMFolder*) ),
           this, SLOT( slotFolderRemoved(KMFolder*) ) );

  connect( kmkernel->imapFolderMgr(), SIGNAL( folderRemoved(KMFolder*) ),
           this, SLOT( slotFolderRemoved(KMFolder*) ) );

  connect( kmkernel->dimapFolderMgr(), SIGNAL( folderRemoved(KMFolder*) ),
           this, SLOT( slotFolderRemoved(KMFolder*) ) );

  connect( kmkernel->searchFolderMgr(), SIGNAL( folderRemoved(KMFolder*) ),
           this, SLOT( slotFolderRemoved(KMFolder*) ) );

  connect( kmkernel, SIGNAL( onlineStatusChanged( GlobalSettings::EnumNetworkState::type ) ),
           this, SLOT( slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type ) ) );

  toggleSystemTray();

  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KStatusBar *sb =  mainWin ? mainWin->statusBar() : 0;
  mVacationScriptIndicator = new KMail::StatusBarLabel( sb );
  mVacationScriptIndicator->hide();
  connect( mVacationScriptIndicator, SIGNAL(clicked()), SLOT(slotEditVacation()) );
  if ( GlobalSettings::checkOutOfOfficeOnStartup() )
    QTimer::singleShot( 0, this, SLOT(slotCheckVacation()) );

  // must be the last line of the constructor:
  mStartupDone = true;
}


//-----------------------------------------------------------------------------
//The kernel may have already been deleted when this method is called,
//perform all cleanup that requires the kernel in destruct()
KMMainWidget::~KMMainWidget()
{
  theMainWidgetList->removeAll( this );
  qDeleteAll( mFilterCommands );
  destruct();
}


//-----------------------------------------------------------------------------
//This method performs all cleanup that requires the kernel to exist.
void KMMainWidget::destruct()
{
  if ( mDestructed )
    return;
  if ( mSearchWin )
    mSearchWin->close();
  writeConfig();
  writeFolderConfig();
  deleteWidgets();
  delete mSystemTray;
  delete mCustomTemplateMenus;
  mSystemTray = 0;
  mCustomTemplateMenus = 0;
  delete mFolderViewManager;
  mFolderViewManager = 0;
  mDestructed = true;
}


//-----------------------------------------------------------------------------
void KMMainWidget::readPreConfig()
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
  mEnableFavoriteFolderView = GlobalSettings::self()->enableFavoriteFolderView();
  mEnableFolderQuickSearch = GlobalSettings::self()->enableFolderQuickSearch();
}


//-----------------------------------------------------------------------------
void KMMainWidget::readFolderConfig()
{
  if ( !mFolder )
    return;

  KConfig *config = KMKernel::config();
  KConfigGroup group( config, "Folder-" + mFolder->idString() );
  mFolderHtmlPref =
      group.readEntry( "htmlMailOverride", false );
  mFolderHtmlLoadExtPref =
      group.readEntry( "htmlLoadExternalOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig()
{
  if ( !mFolder )
    return;

  KConfig *config = KMKernel::config();
  KConfigGroup group( config, "Folder-" + mFolder->idString() );
  group.writeEntry( "htmlMailOverride", mFolderHtmlPref );
  group.writeEntry( "htmlLoadExternalOverride", mFolderHtmlLoadExtPref );
}

//-----------------------------------------------------------------------------
void KMMainWidget::layoutSplitters()
{
  // This function can only be called when the old splitters are already deleted
  assert( !mSplitter1 );
  assert( !mSplitter2 );

  // For some reason, this is necessary here so that the copy action still
  // works after changing the folder layout.
  if ( mMsgView )
    disconnect( mMsgView->copyAction(), SIGNAL(triggered(bool) ),
                mMsgView, SLOT( slotCopySelectedText() ) );

  // If long folder list is enabled, the splitters are:
  // Splitter 1: FolderView vs (HeaderAndSearch vs MessageViewer)
  // Splitter 2: HeaderAndSearch vs MessageViewer
  //
  // If long folder list is disabled, the splitters are:
  // Splitter 1: (FolderView vs HeaderAndSearch) vs MessageViewer
  // Splitter 2: FolderView vs HeaderAndSearch

  // The folder view is both the folder tree and the favorite folder view, if
  // enabled

  const bool opaqueResize = KGlobalSettings::opaqueResize();
  bool readerWindowAtSide = !mReaderWindowBelow && mReaderWindowActive;
  bool readerWindowBelow = mReaderWindowBelow && mReaderWindowActive;

  //
  // Create the splitters
  //
  mSplitter1 = new QSplitter( this );
  mSplitter1->setObjectName( "splitter1" );
  mSplitter1->setOpaqueResize( opaqueResize );
  mSplitter1->setChildrenCollapsible( false );
  mSplitter2 = new QSplitter( mSplitter1 );
  mSplitter2->setObjectName( "splitter2" );
  mSplitter2->setOpaqueResize( opaqueResize );
  mSplitter2->setChildrenCollapsible( false );
  mSplitter1->addWidget( mSplitter2 );

  //
  // Set the layout of the splitters and calculate the widget's parents
  //
  QSplitter *folderViewParent, *folderTreeParent, *messageViewerParent;
  if ( mLongFolderList ) {

    mSplitter1->setOrientation( Qt::Horizontal );
    Qt::Orientation splitter2orientation;
    if ( !readerWindowAtSide )
      splitter2orientation = Qt::Vertical;
    else
      splitter2orientation = Qt::Horizontal;
    mSplitter2->setOrientation( splitter2orientation );
    folderViewParent = mSplitter1;
    messageViewerParent = mSplitter2;

  } else {

    Qt::Orientation splitter1orientation;
    if ( !readerWindowAtSide )
      splitter1orientation = Qt::Vertical;
    else
      splitter1orientation = Qt::Horizontal;
    mSplitter1->setOrientation( splitter1orientation );
    mSplitter2->setOrientation( Qt::Horizontal );
    folderViewParent = mSplitter2;
    messageViewerParent = mSplitter1;
  }

  //
  // Add the widgets to the splitters and set the parents calculated above
  //

#ifdef TEST_DOCKWIDGETS
  bool bUseDockWidgets = parent()->inherits( "QMainWindow" );
#else
  bool bUseDockWidgets = false;
#endif

  int folderTreePosition = 0;

  if ( !bUseDockWidgets )
  {
    if ( mFavoriteFolderView ) {
      mFolderViewSplitter = new QSplitter( Qt::Vertical, folderViewParent );
      mFolderViewSplitter->setOpaqueResize( opaqueResize );
      mFolderViewSplitter->setChildrenCollapsible( false );
      folderTreeParent = mFolderViewSplitter;
      mFolderViewSplitter->addWidget( mFavoriteFolderView );
      mFavoriteFolderView->setParent( mFolderViewSplitter );
      folderViewParent->insertWidget( 0, mFolderViewSplitter );
      folderTreePosition = 1;
    } else
      folderTreeParent = folderViewParent;

    folderTreeParent->insertWidget( folderTreePosition, mSearchAndTree );
    mSplitter2->addWidget( mMessageListView );
  }

  if ( bUseDockWidgets )
    mMessageListView->setParent( mSplitter2 );

  if ( mMsgView ) {
    messageViewerParent->addWidget( mMsgView );
    mMsgView->setParent( messageViewerParent );
  }

  if ( !bUseDockWidgets )
  {
    mSearchAndTree->setParent( folderTreeParent );
    mMessageListView->setParent( mSplitter2 );
  }

  //
  // Set the stretch factors
  //
  mSplitter1->setStretchFactor( 0, 0 );
  mSplitter2->setStretchFactor( 0, 0 );
  mSplitter1->setStretchFactor( 1, 1 );
  mSplitter2->setStretchFactor( 1, 1 );

  if ( !bUseDockWidgets )
  {
    if ( mFavoriteFolderView ) {
      mFolderViewSplitter->setStretchFactor( 0, 0 );
      mFolderViewSplitter->setStretchFactor( 1, 1 );
    }
  }

  // Because the reader windows's width increases a tiny bit after each restart
  // in short folder list mode with mesage window at side, disable the stretching
  // as a workaround here
  if ( readerWindowAtSide && !mLongFolderList ) {
    mSplitter1->setStretchFactor( 0, 1 );
    mSplitter1->setStretchFactor( 1, 0 );
  }

  //
  // Set the sizes of the splitters to the values stored in the config
  //
  QList<int> splitter1Sizes;
  QList<int> splitter2Sizes;

  const int folderViewWidth = GlobalSettings::self()->folderViewWidth();
  int headerHeight = GlobalSettings::self()->searchAndHeaderHeight();
  const int messageViewerWidth = GlobalSettings::self()->readerWindowWidth();
  int headerWidth = GlobalSettings::self()->searchAndHeaderWidth();
  int messageViewerHeight = GlobalSettings::self()->readerWindowHeight();

  // If the message viewer was hidden before, make sure it is not zero height
  if ( messageViewerHeight < 10 && readerWindowBelow ) {
    headerHeight /= 2;
    messageViewerHeight = headerHeight;
  }

  if ( mLongFolderList ) {
    if ( !readerWindowAtSide ) {
      splitter1Sizes << folderViewWidth << headerWidth;
      splitter2Sizes << headerHeight << messageViewerHeight;
    } else {
      splitter1Sizes << folderViewWidth << ( headerWidth + messageViewerWidth );
      splitter2Sizes << headerWidth << messageViewerWidth;
    }
  } else {
    if ( !readerWindowAtSide ) {
      splitter1Sizes << headerHeight << messageViewerHeight;
      splitter2Sizes << folderViewWidth << headerWidth;
    } else {
      splitter1Sizes << headerWidth << messageViewerWidth;
      splitter2Sizes << folderViewWidth << ( headerWidth + messageViewerWidth );
    }
  }

  mSplitter1->setSizes( splitter1Sizes );
  mSplitter2->setSizes( splitter2Sizes );

  if ( mFolderViewSplitter ) {
    QList<int> splitterSizes;
    int ffvHeight = GlobalSettings::self()->favoriteFolderViewHeight();
    int ftHeight = GlobalSettings::self()->folderTreeHeight();
    splitterSizes << ffvHeight << ftHeight;
    mFolderViewSplitter->setSizes( splitterSizes );
  }

  //
  // Now add the splitters to the main layout
  //
  mTopLayout->addWidget( mSplitter1 );

  // Make the copy action work, see disconnect comment above
  if ( mMsgView )
    connect( mMsgView->copyAction(), SIGNAL( triggered(bool) ),
             mMsgView, SLOT( slotCopySelectedText() ) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::readConfig()
{
  KConfig *config = KMKernel::config();

  bool oldLongFolderList =  mLongFolderList;
  bool oldReaderWindowActive = mReaderWindowActive;
  bool oldReaderWindowBelow = mReaderWindowBelow;
  bool oldFavoriteFolderView = mEnableFavoriteFolderView;
  bool oldFolderQuickSearch = mEnableFolderQuickSearch;

  // on startup, the layout is always new and we need to relayout the widgets
  bool layoutChanged = !mStartupDone;

  if ( mStartupDone )
  {
    writeConfig();

    readPreConfig();

    layoutChanged = ( oldLongFolderList != mLongFolderList ) ||
                    ( oldReaderWindowActive != mReaderWindowActive ) ||
                    ( oldReaderWindowBelow != mReaderWindowBelow ) ||
                    ( oldFavoriteFolderView != mEnableFavoriteFolderView ) ||
                    ( oldFolderQuickSearch != mEnableFolderQuickSearch );

    if( layoutChanged ) {
      deleteWidgets();
      createWidgets();
    }
  }


  { // Read the config of the folder views and the header
    if ( mMsgView )
      mMsgView->readConfig();

    mMessageListView->reloadGlobalConfiguration();

    mMainFolderView->readConfig();
    mMainFolderView->reload();

    if ( mFavoriteFolderView )
    {
      mFavoriteFolderView->readConfig();
      mFavoriteFolderView->reload();
    }
    mFavoritesCheckMailAction->setEnabled( GlobalSettings::self()->enableFavoriteFolderView() );
  }

  { // area for config group "General"
    KConfigGroup group( config, "General" );
    mBeepOnNew = group.readEntry( "beep-on-mail", false );
    mConfirmEmpty = group.readEntry( "confirm-before-empty", true );
    // startup-Folder, defaults to system-inbox
    mStartupFolder = group.readEntry( "startupFolder", kmkernel->inboxFolder()->idString() );
    if ( !mStartupDone )
    {
      // check mail on startup
      if ( group.readEntry( "checkmail-startup", false ) )
        // do it after building the kmmainwin, so that the progressdialog is available
        QTimer::singleShot( 0, this, SLOT( slotCheckMail() ) );
    }
  }

  if ( layoutChanged ) {
    layoutSplitters();
  }

  updateMessageMenu();
  updateFileMenu();
  toggleSystemTray();
}

//-----------------------------------------------------------------------------
void KMMainWidget::writeConfig()
{
  KConfig *config = KMKernel::config();
  KConfigGroup geometry( config, "Geometry" );
  KConfigGroup general( config, "General" );

  // Don't save the sizes of all the widgets when we were never shown.
  // This can happen in Kontact, where the KMail plugin is automatically
  // loaded, but not necessarily shown.
  // This prevents invalid sizes from being saved
  if ( mWasEverShown ) {

    // The height of the header widget can be 0, this happens when the user
    // did not switch to the header widget onced and the "Welcome to KMail"
    // HTML widget was shown the whole time
    int headersHeight = mMessageListView->height();
    if ( headersHeight == 0 )
      headersHeight = height() / 2;

    GlobalSettings::self()->setSearchAndHeaderHeight( headersHeight );
    GlobalSettings::self()->setSearchAndHeaderWidth( mMessageListView->width() );

    if ( mFavoriteFolderView ) {
      GlobalSettings::self()->setFavoriteFolderViewHeight( mFavoriteFolderView->height() );
      GlobalSettings::self()->setFolderTreeHeight( mMainFolderView->height() );
      mFavoriteFolderView->writeConfig();
      if ( !mLongFolderList )
        GlobalSettings::self()->setFolderViewHeight( mFolderViewSplitter->height() );
    }
    else if ( !mLongFolderList && mMainFolderView )
      GlobalSettings::self()->setFolderTreeHeight( mMainFolderView->height() );

    if ( mMainFolderView )
    {
      GlobalSettings::self()->setFolderViewWidth( mMainFolderView->width() );
      mMainFolderView->writeConfig();
    }

    if ( mMsgView ) {
      if ( !mReaderWindowBelow )
        GlobalSettings::self()->setReaderWindowWidth( mMsgView->width() );
      mMsgView->writeConfig();
      GlobalSettings::self()->setReaderWindowHeight( mMsgView->width() );
    }
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::deleteWidgets()
{
  // Simply delete the top splitter, which always is mSplitter1, regardless
  // of the layout. This deletes all children.
  delete mSplitter1;
  mMsgView = 0;
  mSearchAndTree = 0;
  mFavoriteFolderView = 0;
  mFolderViewSplitter = 0;
  mSplitter1 = 0;
  mSplitter2 = 0;
}

//-----------------------------------------------------------------------------
void KMMainWidget::createWidgets()
{
  // Note that all widgets we create in this function have the parent 'this'.
  // They will be properly reparented in layoutSplitters()

  //
  // Create header view and search bar
  //
  mMessageListView = new KMail::MessageListView::Pane( this, this );
  mMessageListView->setObjectName( "messagelistview" );

  connect( mMessageListView, SIGNAL( currentFolderChanged( KMFolder * ) ),
           SLOT( slotMessageListViewCurrentFolderChanged( KMFolder * ) ) );
  connect( mMessageListView, SIGNAL( messageStatusChangeRequest( KMMsgBase *, const KPIM::MessageStatus &, const KPIM::MessageStatus &  ) ),
           SLOT( slotMessageStatusChangeRequest( KMMsgBase *, const KPIM::MessageStatus &, const KPIM::MessageStatus &  ) ) );
  {
    KAction *action = new KAction( i18n("Set Focus to Quick Search"), this );
    action->setShortcut( QKeySequence( Qt::Key_Alt + Qt::Key_Q ) );
    actionCollection()->addAction( "focus_to_quickseach", action );
    connect( action, SIGNAL( triggered( bool ) ),
             SLOT( slotFocusQuickSearch() ) );
  }

  connect( mMessageListView, SIGNAL( fullSearchRequest() ),
           this, SLOT( slotRequestFullSearchFromQuickSearch() ) );

#if 0
  // FIXME!
  if ( !GlobalSettings::self()->quickSearchActive() )
    mSearchToolBar->hide();
#endif

  connect( mMessageListView, SIGNAL( selectionChanged() ),
           SLOT( startUpdateMessageActionsTimer() ) );

  connect( mMessageListView, SIGNAL( messageActivated( KMMessage * ) ),
           this, SLOT( slotMsgActivated( KMMessage * ) ) );

  mPreviousMessageAction = new KAction( i18n( "Extend Selection to Previous Message" ), this );
  mPreviousMessageAction->setShortcut( QKeySequence( Qt::SHIFT + Qt::Key_Left ) );
  actionCollection()->addAction( "previous_message", mPreviousMessageAction );
  connect( mPreviousMessageAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotExtendSelectionToPreviousMessage() ) );

  mNextMessageAction = new KAction( i18n( "Extend Selection to Next Message" ), this );
  mNextMessageAction->setShortcut( QKeySequence( Qt::SHIFT + Qt::Key_Right ) );
  actionCollection()->addAction( "next_message", mNextMessageAction );
  connect( mNextMessageAction, SIGNAL( triggered( bool ) ),
           this, SLOT( slotExtendSelectionToNextMessage() ) );

  //
  // Create the reader window
  //
  if ( mReaderWindowActive ) {
    mMsgView = new KMReaderWin( this, this, actionCollection(), 0 );
    if ( mMsgActions ) {
      mMsgActions->setMessageView( mMsgView );
    }

    connect( mMsgView, SIGNAL( replaceMsgByUnencryptedVersion() ),
             this, SLOT( slotReplaceMsgByUnencryptedVersion() ) );
    connect( mMsgView, SIGNAL( popupMenu(KMMessage&,const KUrl&,const QPoint&) ),
             this, SLOT( slotMsgPopup(KMMessage&,const KUrl&,const QPoint&) ) );
    connect( mMsgView, SIGNAL( urlClicked(const KUrl&,int) ),
             mMsgView, SLOT( slotUrlClicked() ) );
#if 0
    // FIXME (Pragma)
    connect( mMsgView, SIGNAL( noDrag() ),
             mHeaders, SLOT( slotNoDrag() ) );
#endif

    connect( mMessageListView, SIGNAL( messageSelected( KMMessage * ) ),
             this, SLOT( slotMsgSelected( KMMessage * ) ) );

  }

  //
  // Create the folder tree
  // the "folder tree" consists of a quicksearch input field and the tree itself
  //

  // If we have a QMainWindow as parent then we can use QDockWidget, which is very cool :)
#ifdef TEST_DOCKWIDGETS
  bool bUseDockWidgets = parent()->inherits( "QMainWindow" );
#else
  bool bUseDockWidgets = false;
#endif

  QDockWidget *dw = 0;
  QMainWindow *mw = 0;
  if ( bUseDockWidgets )
  {
    mw = static_cast<QMainWindow *>( parent() );
    dw = new QDockWidget( i18n( "Old Folders" ), mw );
    dw->setObjectName( i18n( "Old Folders" ) );
    dw->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
//    dw->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  }

  mSearchAndTree = new QWidget( bUseDockWidgets ? static_cast<QWidget *>( dw ) : static_cast<QWidget *>( this ) );
  QVBoxLayout *vboxlayout = new QVBoxLayout;
  vboxlayout->setMargin(0);
  mFolderQuickSearch = new KTreeWidgetSearchLine( mSearchAndTree );
  mFolderQuickSearch->setClickMessage( i18n( "Search" ) );
  vboxlayout->addWidget( mFolderQuickSearch );
  mSearchAndTree->setLayout( vboxlayout );

  if ( bUseDockWidgets )
  {
    dw->setWidget( mSearchAndTree );
    mw->addDockWidget( Qt::LeftDockWidgetArea, dw );
    dw = new QDockWidget( i18n( "Folders" ), mw );
    dw->setObjectName( i18n( "Folders" ) );
    dw->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
//    dw->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  }

  mMainFolderView = new KMail::MainFolderView( this, mFolderViewManager, bUseDockWidgets ? static_cast<QWidget *>( dw ) : static_cast<QWidget *>( mSearchAndTree ), "folderTree" );
  mFolderQuickSearch->addTreeWidget( mMainFolderView );

  if ( bUseDockWidgets )
  {
    dw->setWidget( mMainFolderView );
    mw->addDockWidget( Qt::LeftDockWidgetArea, dw );
  } else {
    vboxlayout->addWidget( mMainFolderView );
  }

  if ( !GlobalSettings::enableFolderQuickSearch() ) {
    mFolderQuickSearch->hide();
  }

  connect( mMainFolderView, SIGNAL( folderDrop(KMFolder*) ),
           this, SLOT( slotMoveMsgToFolder(KMFolder*) ) );
  connect( mMainFolderView, SIGNAL( folderDropCopy(KMFolder*) ),
           this, SLOT( slotCopyMsgToFolder(KMFolder*) ) );

  //
  // Create the favorite folder view
  //
  if ( mEnableFavoriteFolderView ) {

    if ( bUseDockWidgets )
    {
      dw = new QDockWidget( i18n( "Favorite Folders" ), mw );
      dw->setObjectName( i18n( "Favorite Folders" ) );
      dw->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
//      dw->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    }

    mFavoriteFolderView = new KMail::FavoriteFolderView( this, mFolderViewManager, bUseDockWidgets ? static_cast<QWidget *>( dw ) : static_cast<QWidget *>( this ) );

    if ( bUseDockWidgets )
    {
      dw->setWidget( mFavoriteFolderView );
      mw->addDockWidget( Qt::LeftDockWidgetArea, dw );
    }


    if ( mFavoritesCheckMailAction )
      connect( mFavoritesCheckMailAction, SIGNAL(triggered(bool)),
               mFavoriteFolderView, SLOT( checkMail() ) );

     // FIXME: These signals should be emitted by the manager, probably
     connect( mFavoriteFolderView, SIGNAL( folderDrop( KMFolder * ) ),
             SLOT( slotMoveMsgToFolder( KMFolder * ) ) );
     connect( mFavoriteFolderView, SIGNAL( folderDropCopy( KMFolder * ) ),
             SLOT( slotCopyMsgToFolder( KMFolder * ) ) );
  }

  //
  // Create all kinds of actions
  //
  {
    mRemoveDuplicatesAction = new KAction( i18n("Remove Duplicate Messages"), this );
    actionCollection()->addAction( "remove_duplicate_messages", mRemoveDuplicatesAction );
    connect( mRemoveDuplicatesAction, SIGNAL( triggered( bool ) ),
             SLOT( removeDuplicates() ) );
    mRemoveDuplicatesAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Asterisk ) );
  }
  {
    mMoveMsgToFolderAction = new KAction( i18n("Move Message to Folder"), this );
    mMoveMsgToFolderAction->setShortcut( QKeySequence( Qt::Key_M ) );
    actionCollection()->addAction( "move_message_to_folder", mMoveMsgToFolderAction );
    connect( mMoveMsgToFolderAction, SIGNAL( triggered( bool ) ),
             SLOT( slotMoveMsg() ) );
  }
  {
    KAction *action = new KAction( i18n("Copy Message to Folder"), this );
    actionCollection()->addAction( "copy_message_to_folder", action );
    connect( action, SIGNAL( triggered( bool ) ),
             SLOT( slotCopyMsg() ) );
    action->setShortcut( QKeySequence( Qt::Key_C ) );
  }
  {
    KAction *action = new KAction( i18n("Jump to Folder"), this );
    actionCollection()->addAction( "jump_to_folder", action );
    connect( action, SIGNAL( triggered ( bool ) ),
             SLOT( slotJumpToFolder() ) );
    action->setShortcut( QKeySequence( Qt::Key_J ) );
  }
  {
    KAction *action = new KAction(i18n("Abort Current Operation"), this);
    actionCollection()->addAction("cancel", action );
    connect( action, SIGNAL( triggered( bool ) ),
             ProgressManager::instance(), SLOT( slotAbortAll() ) );
    action->setShortcut( QKeySequence( Qt::Key_Escape ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Next Folder"), this);
    actionCollection()->addAction("inc_current_folder", action );
    connect( action, SIGNAL( triggered( bool ) ),
             mMainFolderView, SLOT( slotFocusNextFolder() ) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Right ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Previous Folder"), this);
    actionCollection()->addAction("dec_current_folder", action );
    connect( action, SIGNAL( triggered( bool ) ),
             mMainFolderView, SLOT( slotFocusPrevFolder() ) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Left ) );
  }
  {
    KAction *action = new KAction(i18n("Select Folder with Focus"), this);
    actionCollection()->addAction("select_current_folder", action );
    connect( action, SIGNAL( triggered( bool ) ),
             mMainFolderView, SLOT( slotSelectFocusedFolder() ) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Space ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Next Message"), this);
    actionCollection()->addAction("inc_current_message", action );
    connect( action, SIGNAL( triggered( bool) ),
             this, SLOT( slotFocusOnNextMessage() ) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Right ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Previous Message"), this);
    actionCollection()->addAction("dec_current_message", action );
    connect( action, SIGNAL( triggered( bool ) ),
             this, SLOT( slotFocusOnPrevMessage() ) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Left ) );
  }
  {
    KAction *action = new KAction(i18n("Select Message with Focus"), this);
    actionCollection()->addAction( "select_current_message", action );
    connect( action, SIGNAL( triggered( bool ) ),
             this, SLOT( slotSelectFocusedMessage() ) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Space ) );
  }

  connect( kmkernel->outboxFolder(), SIGNAL( msgRemoved(int, const QString&) ),
           SLOT( startUpdateMessageActionsTimer() ) );
  connect( kmkernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( startUpdateMessageActionsTimer() ) );
}

//-------------------------------------------------------------------------
void KMMainWidget::slotFocusQuickSearch()
{
  mMessageListView->focusQuickSearch();
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
  KWindowSystem::activateWindow( mSearchWin->winId() );
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
  KPIM::KAddrBookExternal::openAddressBook(this);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotImport()
{
  KRun::runCommand("kmailcvt", topLevelWidget());
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
void KMMainWidget::slotCheckOneAccount( QAction* item )
{
  if ( ! item ) {
    return;
  }

  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  KMAccount* t = kmkernel->acctMgr()->findByName( item->data().toString() );

  if ( t ) {
    kmkernel->acctMgr()->singleCheckMail( t );
  }
  else {
    kDebug(5006) <<" - account with name '" << item->data().toString() <<"' not found";
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailChecked( bool newMail, bool sendOnCheck,
                                    const QMap<QString, int> & newInFolder )
{
  const bool sendOnAll =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnAllChecks;
  const bool sendOnManual =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnManualChecks;
  if ( !kmkernel->isOffline() && ( sendOnAll || (sendOnManual && sendOnCheck ) ) ) {
    slotSendQueued();
  }

  if ( !newMail || newInFolder.isEmpty() ) {
    return;
  }

  QDBusMessage message =
    QDBusMessage::createSignal( "/KMail", "org.kde.kmail.kmail", "unreadCountChanged" );
  QDBusConnection::sessionBus().send( message );

  // build summary for new mail message
  bool showNotification = false;
  QString summary;
  QStringList keys( newInFolder.keys() );
  keys.sort();
  for ( QStringList::const_iterator it=keys.constBegin(); it!=keys.constEnd(); ++it ) {
//    kDebug(5006) << newInFolder.find( *it ).value() << "new message(s) in" << *it;

    KMFolder *folder = kmkernel->findFolderById( *it );

    if ( folder && !folder->ignoreNewMail() ) {
      showNotification = true;
      if ( GlobalSettings::self()->verboseNewMailNotification() ) {
        summary += "<br>" + i18np( "1 new message in %2",
                                   "%1 new messages in %2",
                                   newInFolder.find( *it ).value(),
                                   folder->prettyUrl() );
      }
    }
  }

  // update folder menus in case some mail got filtered to trash/current folder
  // and we can enable "empty trash/move all to trash" action etc.
  updateFolderMenu();

  if ( !showNotification ) {
    return;
  }

  if ( GlobalSettings::self()->verboseNewMailNotification() ) {
    summary = i18nc( "%1 is a list of the number of new messages per folder",
                     "<b>New mail arrived</b><br />%1",
                     summary );
  } else {
    summary = i18n( "New mail arrived" );
  }

  if( kmkernel->xmlGuiInstance().isValid() ) {
    KNotification::event( "new-mail-arrived",
                          summary,
                          QPixmap(),
                          topLevelWidget(),
                          KNotification::CloseOnTimeout,
                          kmkernel->xmlGuiInstance() );
  } else {
    KNotification::event( "new-mail-arrived",
                          summary,
                          QPixmap(),
                          topLevelWidget(),
                          KNotification::CloseOnTimeout );
  }

  if ( mBeepOnNew ) {
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
                             QString(), false, false, false );
      parser.process( NULL, mFolder );
      win = KMail::makeComposer( msg, mFolder->identity() );
  } else {
      msg->initHeader();
      TemplateParser parser( msg, TemplateParser::NewMessage,
                             QString(), false, false, false );
      parser.process( NULL, NULL );
      win = KMail::makeComposer( msg );
  }

  win->show();

}

KMFolder * KMMainWidget::folder() const
{
  Q_ASSERT( mFolder == messageListView()->currentFolder() || mFolder == 0 );
  return mFolder;
}


//-----------------------------------------------------------------------------
// TODO: do we want the list sorted alphabetically?
void KMMainWidget::slotShowNewFromTemplate()
{
  if ( mFolder )
  {
    const KPIMIdentities::Identity & ident =
      kmkernel->identityManager()->identityForUoidOrDefault( mFolder->identity() );
    mTemplateFolder = kmkernel->folderMgr()->findIdString( ident.templates() );
  }

  if ( !mTemplateFolder ) mTemplateFolder = kmkernel->templatesFolder();
  if ( !mTemplateFolder ) return;

  mTemplateMenu->menu()->clear();
  for ( int idx = 0; idx<mTemplateFolder->count(); ++idx ) {
    KMMsgBase *mb = mTemplateFolder->getMsgBase( idx );

    QString subj = mb->subject();
    if ( subj.isEmpty() )
      subj = i18n("No Subject");

    QAction *templateAction = mTemplateMenu->menu()->addAction(
        KStringHandler::rsqueeze( subj.replace( '&', "&&" ) ) );
    templateAction->setData( idx );
  }

  // If there are no templates available, add a menu entry which informs
  // the user about this.
  if ( mTemplateMenu->menu()->actions().isEmpty() ) {
    QAction *noAction = mTemplateMenu->menu()->addAction(
                                            i18n( "(no templates)" ) );
    noAction->setEnabled( false );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNewFromTemplate( QAction *action )
{
  if ( !mTemplateFolder )
    return;
  newFromTemplate( mTemplateFolder->getMsg( action->data().toInt() ) );
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
  if ( !mMainFolderView )
    return;

  KMFolder * folder = mMainFolderView->currentFolder();
  if ( !folder )
    return;

  ( new KMail::MailingListFolderPropertiesDialog( this, folder ) )->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderShortcutCommand()
{
  if (!mMainFolderView)
    return;

  KMFolder * folder = mMainFolderView->currentFolder();
  if ( !folder )
    return;

  ( new KMail::FolderShortcutDialog( folder, kmkernel->getKMMainWidget(), mMainFolderView ) )->exec();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotModifyFolder()
{
  if (!mMainFolderView)
    return;

  KMFolder * folder = mMainFolderView->currentFolder();
  if ( !folder )
    return;


  KMFolderDialog props( folder, folder->parent(), mMainFolderView,
                        i18n("Properties of Folder %1", folder->label() ) );
  props.exec();

  updateFolderMenu();
  //Kolab issue 2152
  if ( mSystemTray )
    mSystemTray->foldersChanged();
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

    if (KMessageBox::warningContinueCancel(this, text, title, KGuiItem( title, "user-trash"))
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
  QString buttonLabel;
  if ( mFolder->folderType() == KMFolderTypeSearch ) {
    title = i18n("Delete Search");
    str = i18n("<qt>Are you sure you want to delete the search <b>%1</b>?<br />"
                "Any messages it shows will still be available in their original folder.</qt>",
             Qt::escape( mFolder->label() ) );
    buttonLabel = i18nc("@action:button Delete search", "&Delete");
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
                   "<resource>%1</resource> and all its subfolders? Those subfolders might "
                   "not be empty and their contents will be discarded as well. "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
                Qt::escape( mFolder->label() ) );
      }
    } else {
      if ( !mFolder->child() || mFolder->child()->isEmpty() ) {
        str = i18n("<qt>Are you sure you want to delete the folder "
                   "<resource>%1</resource>, discarding its contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
                Qt::escape( mFolder->label() ) );
      }
      else {
        str = i18n("<qt>Are you sure you want to delete the folder <resource>%1</resource> "
                   "and all its subfolders, discarding their contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
              Qt::escape( mFolder->label() ) );
      }
    }
    buttonLabel = i18nc("@action:button Delete folder", "&Delete");
  }

  if ( KMessageBox::warningContinueCancel( this, str, title,
                                           KGuiItem( buttonLabel, "edit-delete" ),
                                           KStandardGuiItem::cancel(), "",
                                           KMessageBox::Notify | KMessageBox::Dangerous )
      == KMessageBox::Continue )
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
  if (!mFolder)
    return;

  KCursorSaver busy(KBusyPtr::busy());
  mFolder->compact( KMFolder::CompactNow );
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

void KMMainWidget::slotTroubleshootMaildir()
{
  if ( !mFolder || !mFolder->folderType() == KMFolderTypeMaildir )
    return;
  KMFolderMaildir* f = static_cast<KMFolderMaildir*>( mFolder->storage() );
  if ( KMessageBox::warningContinueCancel( this,
             i18nc( "@info",
                    "You are about to recreate the index for folder <resource>%1</resource>.<nl/>"
                    "<warning>This will destroy all message status information.</warning><nl/>"
                    "Are you sure you want to continue?", mFolder->label() ),
             i18nc( "@title", "Really recreate index?" ),
             KGuiItem( i18nc( "@action:button", "Recreate Index" ) ),
             KStandardGuiItem::cancel(), QString(),
             KMessageBox::Notify | KMessageBox::Dangerous )
        == KMessageBox::Continue ) {
    f->createIndexFromContents();
    KMessageBox::information( this,
                              i18n( "The index of folder %1 has been recreated.",
                                    mFolder->label() ),
                              i18n( "Index recreated" ) );
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
  QList< KMMsgBase * > selectedMessages = mMessageListView->selectionAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
    return;

  KMForwardCommand * command = new KMForwardCommand(
      this, selectedMessages, mFolder->identity()
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMsg()
{
  QList< KMMsgBase * > selectedMessages = mMessageListView->selectionAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
    return;

  KMForwardAttachedCommand * command = new KMForwardAttachedCommand(
      this, selectedMessages, mFolder->identity()
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUseTemplate()
{
  newFromTemplate( mMessageListView->currentMessage() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotResendMsg()
{
  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
    return;

  KMCommand *command = new KMResendMessageCommand( this, msg );

  command->start();
}

//-----------------------------------------------------------------------------
// Message moving and permanent deletion
//

void KMMainWidget::moveMessageSet( KMail::MessageListView::MessageSet * set, KMFolder * destination, bool confirmOnDeletion )
{
  Q_ASSERT( set );

  if ( !set->isValid() )
  {
    delete set;
    return;
  }

  Q_ASSERT( set->folder() ); // must exist since the set is valid

  // Get the list of messages
  QList< KMMsgBase * > selectedMessages = set->contentsAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
  {
    delete set;
    return;
  }

  // If this is a deletion, ask for confirmation
  if ( !destination && confirmOnDeletion )
  {
    int ret = KMessageBox::warningContinueCancel(
        this,
        i18np(
            "<qt>Do you really want to delete the selected message?<br />"
            "Once deleted, it cannot be restored.</qt>",
            "<qt>Do you really want to delete the %1 selected messages?<br />"
            "Once deleted, they cannot be restored.</qt>",
            selectedMessages.count()
          ),
        selectedMessages.count() > 1 ? i18n( "Delete Messages" ) : i18n( "Delete Message" ),
        KStandardGuiItem::del(),
        KStandardGuiItem::cancel(),
        "NoConfirmDelete"
      );
    if ( ret == KMessageBox::Cancel )
    {
      delete set;
      return;  // user canceled the action
    }
  }

  // And stuff them into a KMMoveCommand :)
  KMCommand *command = new KMMoveCommand( destination, selectedMessages );

  // Reparent the set to the command so it's deleted even if the command
  // doesn't notify the completion for some reason.
  set->setParent( command ); // so it will be deleted when the command finishes

  // Set the name to something unique so we can find it back later in the children list
  set->setObjectName( QString( "moveMsgCommandMessageSet" ) );

  QObject::connect(
      command, SIGNAL( completed( KMCommand * ) ),
      this, SLOT( slotMoveMessagesCompleted( KMCommand * ) )
    );

  // Mark the messages as about to be removed (will remove them from the slelection,
  // make non selectable and make them appear dimmer).
  set->markAsAboutToBeRemoved( true );

  command->start();

  if ( destination )
    BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages..." ) );
  else
    BroadcastStatus::instance()->setStatusMsg( i18n( "Deleting messages..." ) );
}

void KMMainWidget::slotMoveMessagesCompleted( KMCommand *command )
{
  Q_ASSERT( command );

  // We have given our persistent set a nice name when it has been created.
  // We have also attacched it as the command child.
  KMail::MessageListView::MessageSet * set = \
      command->findChild< KMail::MessageListView::MessageSet * >( QString( "moveMsgCommandMessageSet" ) );

  Q_ASSERT( set );

  // Bleah :D
  bool moveWasReallyADelete = static_cast<KMMoveCommand *>( command )->destFolder() == 0;

  // clear the "about to be removed" state from anything that KMCommand failed to remove
  set->markAsAboutToBeRemoved( false );

  if ( command->result() != KMCommand::OK )
  {
    if ( moveWasReallyADelete )
      BroadcastStatus::instance()->setStatusMsg( i18n( "Messages deleted successfully." ) );
    else
      BroadcastStatus::instance()->setStatusMsg( i18n( "Messages moved successfully." ) );
  } else {
    if ( moveWasReallyADelete )
    {
      if ( command->result() == KMCommand::Failed )
        BroadcastStatus::instance()->setStatusMsg( i18n( "Deleting messages failed." ) );
      else
        BroadcastStatus::instance()->setStatusMsg( i18n( "Deleting messages canceled." ) );
    } else {
      if ( command->result() == KMCommand::Failed )
        BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages failed." ) );
      else
        BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages canceled." ) );
    }
  }

  // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotDeleteMsg( bool confirmDelete )
{
  // Create a persistent message set from the current selection
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromSelection();
  if ( !set ) // no selection
    return;

  moveMessageSet( set, 0, confirmDelete );
}

void KMMainWidget::slotDeleteThread( bool confirmDelete )
{
  // Create a persistent set from the current thread.
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromCurrentThread();
  if ( !set ) // no current thread
    return;

  moveMessageSet( set, 0, confirmDelete );
}

void KMMainWidget::slotMoveSelectedMessagesToFolder( QAction * act )
{
  KMFolder * folder = static_cast< KMFolder * >( act->data().value< void * >() );

  if ( !folder )
    return; // would be a deletion

  // FIXME: Test for folder validity in some quick and elegant way ?
  //        Actually we ASSUME that the folder is valid...

  slotMoveMsgToFolder( folder );
}

// FIXME: Use better name for this (slotMoveSelectedMessagesToFolder() ?)
//        When changing the name also change the slot name in the QObject::connect calls all around...
void KMMainWidget::slotMoveMsg()
{
  KMail::FolderSelectionDialog dlg( this, i18n( "Move Messages to Folder" ), true );

  if ( !dlg.exec() )
    return;

  KMFolder * dest = dlg.folder();

  if ( !dest )
    return; // would be a deletion!

  slotMoveMsgToFolder( dest );
}

// FIXME: Use better name for this (slotMoveSelectedMessagesToFolder() ?)
//        When changing the name also change the slot name in the QObject::connect calls all around...
void KMMainWidget::slotMoveMsgToFolder( KMFolder *dest )
{
  Q_ASSERT( dest );

  // Create a persistent message set from the current selection
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromSelection();
  if ( !set ) // no selection
    return;

  // Check for senseless move attempts
  if ( dest == set->folder() )
  {
    delete set;
    return;
  }

  moveMessageSet( set, dest, false );
}

//-----------------------------------------------------------------------------
// Message copying
// FIXME: Couldn't parts of it be merged with moving code ?
//

void KMMainWidget::copyMessageSet( KMail::MessageListView::MessageSet * set, KMFolder * destination )
{
  Q_ASSERT( set );
  Q_ASSERT( destination );

  if ( !set->isValid() )
  {
    delete set;
    return;
  }

  Q_ASSERT( set->folder() ); // must exist since the set is valid

  // Get the list of messages
  QList< KMMsgBase * > selectedMessages = set->contentsAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
  {
    delete set;
    return;
  }

  // And stuff them into a KMCopyCommand :)
  KMCommand *command = new KMCopyCommand( destination, selectedMessages );

  // Reparent the set to the command so it's deleted even if the command
  // doesn't notify the completion for some reason.
  set->setParent( command ); // so it will be deleted when the command finishes

  // Set the name to something unique so we can find it back later in the children list
  set->setObjectName( QString( "copyMsgCommandMessageSet" ) );

  QObject::connect(
      command, SIGNAL( completed( KMCommand * ) ),
      this, SLOT( slotCopyMessagesCompleted( KMCommand * ) )
    );

  command->start();

  BroadcastStatus::instance()->setStatusMsg( i18n( "Copying messages..." ) );
}

void KMMainWidget::slotCopyMessagesCompleted( KMCommand *command )
{
  Q_ASSERT( command );

  // We have given our persistent set a nice name when it has been created.
  // We have also attacched it as the command child.
  KMail::MessageListView::MessageSet * set = \
      command->findChild< KMail::MessageListView::MessageSet * >( QString( "copyMsgCommandMessageSet" ) );

  Q_ASSERT( set );

  if ( command->result() != KMCommand::OK )
  {
    BroadcastStatus::instance()->setStatusMsg( i18n( "Messages copied successfully." ) );
  } else {
    if ( command->result() == KMCommand::Failed )
      BroadcastStatus::instance()->setStatusMsg( i18n( "Copying messages failed." ) );
    else
      BroadcastStatus::instance()->setStatusMsg( i18n( "Copying messages canceled." ) );
  }

  // The command will autodelete itself and will also kill the set.
}


// FIXME: Use better name for this (slotCopySelectedMessagesToFolder() ?)
//        When changing the name also change the slot name in the QObject::connect calls all around...
void KMMainWidget::slotCopyMsg()
{
  KMail::FolderSelectionDialog dlg( this, i18n( "Copy Messages to Folder" ), true );

  if ( !dlg.exec() )
    return;

  KMFolder * dest = dlg.folder();

  if ( !dest )
    return; // would be a deletion!

  slotCopyMsgToFolder( dest );
}

// FIXME: Use better name for this (slotCopySelectedMessagesToFolder() ?)
//        When changing the name also change the slot name in the QObject::connect calls all around...
void KMMainWidget::slotCopyMsgToFolder( KMFolder *dest )
{
  Q_ASSERT( dest );

  // Create a persistent message set from the current selection
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromSelection();
  if ( !set ) // no selection
    return;

  // Check for senseless move attempts
  if ( dest == set->folder() )
  {
    delete set;
    return;
  }

  copyMessageSet( set, dest );
}

void KMMainWidget::slotCopySelectedMessagesToFolder( QAction * act )
{
  KMFolder * folder = static_cast< KMFolder * >( act->data().value< void * >() );

  if ( !folder )
    return; // would be a deletion

  // FIXME: Test for folder validity in some quick and elegant way ?
  //        Actually we ASSUME that the folder is valid...

  slotCopyMsgToFolder( folder );
}


//-----------------------------------------------------------------------------
// Message trashing
//

void KMMainWidget::trashMessageSet( KMail::MessageListView::MessageSet * set )
{
  Q_ASSERT( set );

  if ( !set->isValid() )
  {
    delete set;
    return;
  }

  Q_ASSERT( set->folder() ); // must exist since the set is valid

  // Get the list of messages
  QList< KMMsgBase * > selectedMessages = set->contentsAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
  {
    delete set;
    return;
  }

  // FIXME: Why KMDeleteMsgCommand is so misnamed ?
  // FIXME: Why we don't use KMMoveCommand( trashFolder(), selectedMessages ); ?

  // And stuff them into a KMDeleteMsgCommand :)
  KMCommand *command = new KMDeleteMsgCommand( set->folder(), selectedMessages );

  // Reparent the set to the command so it's deleted even if the command
  // doesn't notify the completion for some reason.
  set->setParent( command ); // so it will be deleted when the command finishes

  // Set the name to something unique so we can find it back later in the children list
  set->setObjectName( QString( "trashMsgCommandMessageSet" ) );

  QObject::connect(
      command, SIGNAL( completed( KMCommand * ) ),
      this, SLOT( slotTrashMessagesCompleted( KMCommand * ) )
    );

  // Mark the messages as about to be removed (will remove them from the slelection,
  // make non selectable and make them appear dimmer).
  set->markAsAboutToBeRemoved( true );

  command->start();

  BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages to trash..." ) );
}

void KMMainWidget::slotTrashMessagesCompleted( KMCommand *command )
{
  Q_ASSERT( command );

  // We have given our persistent set a nice name when it has been created.
  // We have also attacched it as the command child.
  KMail::MessageListView::MessageSet * set = \
      command->findChild< KMail::MessageListView::MessageSet * >( QString( "trashMsgCommandMessageSet" ) );

  Q_ASSERT( set );

  // clear the "about to be removed" state from anything that KMCommand failed to remove
  set->markAsAboutToBeRemoved( false );

  if ( command->result() != KMCommand::OK )
  {
    BroadcastStatus::instance()->setStatusMsg( i18n( "Messages moved to trash successfully." ) );
  } else {
    if ( command->result() == KMCommand::Failed )
      BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages to trash failed." ) );
    else
      BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages to trash canceled." ) );
  }

  // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotTrashMsg()
{
  // Create a persistent message set from the current selection
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromSelection();
  if ( !set ) // no selection
    return;

  trashMessageSet( set );
}

void KMMainWidget::slotTrashThread()
{
  // Create a persistent set from the current thread.
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromCurrentThread();
  if ( !set ) // no current thread
    return;

  trashMessageSet( set );
}

//-----------------------------------------------------------------------------
// Message tag setting for messages
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....

void KMMainWidget::toggleMessageSetTag(
    KMail::MessageListView::MessageSet * set,
    const QString &taglabel
  )
{
  // TODO: Use KMCommand class !!!

  Q_ASSERT( set );

  if ( !set->isValid() )
  {
    delete set;
    return;
  }

  Q_ASSERT( set->folder() ); // must exist since the set is valid

  // Get the list of messages
  QList< KMMsgBase * > selectedMessages = set->contentsAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
  {
    delete set;
    return;
  }

#ifdef Nepomuk_FOUND
  //Set the visible name for the tag
  const KMMessageTagDescription *tmp_desc = kmkernel->msgTagMgr()->find( taglabel );

  Nepomuk::Tag n_tag( taglabel );
  if ( tmp_desc )
    n_tag.setLabel( tmp_desc->name() );
#endif

  for( QList< KMMsgBase * >::Iterator it = selectedMessages.begin(); it != selectedMessages.end(); ++it )
  {
    KMMsgBase * msgBase = *it;
#ifdef Nepomuk_FOUND
    Nepomuk::Resource n_resource( QString("kmail-email-%1").arg( msgBase->getMsgSerNum() ) );
#endif

    if ( msgBase->tagList() )
    {
      KMMessageTagList tmp_list = *msgBase->tagList();
      int tagPosition = tmp_list.indexOf( taglabel );

      if ( tagPosition == -1 )
      {
        tmp_list.append( taglabel );
#ifdef Nepomuk_FOUND
        n_resource.addTag( n_tag );
#endif
      } else {
#ifdef Nepomuk_FOUND
        QList< Nepomuk::Tag > n_tag_list = n_resource.tags();
        for (int i = 0; i < n_tag_list.count(); ++i )
        {
          if ( n_tag_list[i].identifiers()[0] == taglabel )
          {
            n_tag_list.removeAt(i);
            break;
          }
        }
        n_resource.setTags( n_tag_list );
#endif
        tmp_list.removeAt( tagPosition );
      }
      msgBase->setTagList( tmp_list );
    }
  }
}

void KMMainWidget::slotUpdateMessageTagList( const QString &taglabel )
{
  // Create a persistent set from the current thread.
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromSelection();
  if ( !set ) // no current thread
    return;

  toggleMessageSetTag( set, taglabel );
}


//-----------------------------------------------------------------------------
// Status setting for threads
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....

void KMMainWidget::setMessageSetStatus(
    KMail::MessageListView::MessageSet * set,
    const KPIM::MessageStatus &status, bool toggle
  )
{
  Q_ASSERT( set );

  if ( !set->isValid() )
  {
    delete set;
    return;
  }

  Q_ASSERT( set->folder() ); // must exist since the set is valid

  // Get the list of messages
  QList< KMMsgBase * > selectedMessages = set->contentsAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
  {
    delete set;
    return;
  }

  // And stuff them into a KMSetStatusCommand :)

  // FIXME: Why we use SerNumList instead of QList< KMMsgBase * > here ?
  SerNumList serNums;

  for( QList< KMMsgBase * >::Iterator it = selectedMessages.begin(); it != selectedMessages.end(); ++it )
    serNums.append( ( *it )->getMsgSerNum() );

  Q_ASSERT( !serNums.empty() );

  KMCommand *command = new KMSetStatusCommand( status, serNums, toggle );

  // Reparent the set to the command so it's deleted even if the command
  // doesn't notify the completion for some reason.
  set->setParent( command ); // so it will be deleted when the command finishes

  // Set the name to something unique so we can find it back later in the children list
  set->setObjectName( QString( "setStatusMsgCommandMessageSet" ) );

  command->start();
}

void KMMainWidget::setCurrentThreadStatus( const KPIM::MessageStatus &status, bool toggle )
{
  // Create a persistent set from the current thread.
  KMail::MessageListView::MessageSet * set = mMessageListView->createMessageSetFromCurrentThread();
  if ( !set ) // no current thread
    return;

  setMessageSetStatus( set, status, toggle );
}

void KMMainWidget::slotSetThreadStatusNew()
{
  setCurrentThreadStatus( MessageStatus::statusNew(), false );
}

void KMMainWidget::slotSetThreadStatusUnread()
{
  setCurrentThreadStatus( MessageStatus::statusUnread(), false );
}

void KMMainWidget::slotSetThreadStatusImportant()
{
  setCurrentThreadStatus( MessageStatus::statusImportant(), true );
}

void KMMainWidget::slotSetThreadStatusRead()
{
  setCurrentThreadStatus( MessageStatus::statusRead(), false );
}

void KMMainWidget::slotSetThreadStatusToAct()
{
  setCurrentThreadStatus( MessageStatus::statusToAct(), true );
}

void KMMainWidget::slotSetThreadStatusWatched()
{
  setCurrentThreadStatus( MessageStatus::statusWatched(), true );
  if ( mWatchThreadAction->isChecked() )
    mIgnoreThreadAction->setChecked(false);
}

void KMMainWidget::slotSetThreadStatusIgnored()
{
  setCurrentThreadStatus( MessageStatus::statusIgnored(), true );
  if ( mIgnoreThreadAction->isChecked() )
    mWatchThreadAction->setChecked(false);
}

void KMMainWidget::slotMessageStatusChangeRequest( KMMsgBase *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus & clear )
{
  Q_ASSERT( msg );

  SerNumList serNums;
  serNums.append( msg->getMsgSerNum() );

  if ( clear.toQInt32() != KPIM::MessageStatus().toQInt32() )
  {
    KMCommand *command = new KMSetStatusCommand( clear, serNums, true );
    command->start();
  }

  if ( set.toQInt32() != KPIM::MessageStatus().toQInt32() )
  {
    KMCommand *command = new KMSetStatusCommand( set, serNums, false );
    command->start();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Message clipboard management

void KMMainWidget::fillMessageClipboard()
{
  QList< KMMsgBase * > selected = mMessageListView->selectionAsMsgBaseList();
  if ( selected.isEmpty() )
    return;

  mMessageClipboard.clear();

  for ( QList< KMMsgBase * >::Iterator it = selected.begin(); it != selected.end(); ++it )
    mMessageClipboard.append( ( *it )->getMsgSerNum() );
}

void KMMainWidget::setMessageClipboardContents( const QList< quint32 > &msgs, bool move )
{
  mMessageClipboard = msgs;
  mMessageClipboardInCutMode = move;
}

void KMMainWidget::slotCopyMessages()
{
  fillMessageClipboard();

  mMessageClipboardInCutMode = false;

  updateCutCopyPasteActions();
}

void KMMainWidget::slotCutMessages()
{
  fillMessageClipboard();

  mMessageClipboardInCutMode = true;

  updateCutCopyPasteActions();
}

void KMMainWidget::slotPasteMessages()
{
  if ( mMessageClipboard.isEmpty() )
    return; // nothing to do

  new KMail::MessageCopyHelper( mMessageClipboard, folder(), mMessageClipboardInCutMode );

  if ( mMessageClipboardInCutMode )
    mMessageClipboard.clear(); // moved messages can't be pasted again (FIXME: should re-copied!)

  updateCutCopyPasteActions();
}

void KMMainWidget::updateCutCopyPasteActions()
{
  QAction *copy = action( "copy_messages" );
  QAction *cut = action( "cut_messages" );
  QAction *paste = action( "paste_messages" );

  bool haveSelection = !mMessageListView->selectionEmpty();

  copy->setEnabled( haveSelection );
  cut->setEnabled( haveSelection && folder() && ( folder()->canDeleteMessages() ) );
  paste->setEnabled( ( !mMessageClipboard.isEmpty() ) && folder() && ( !folder()->isReadOnly() ) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotRedirectMsg()
{
  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
    return;

  KMCommand *command = new KMRedirectCommand( this, msg );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyToMsg( const QString &tmpl )
{
  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
    return;

  QString text = mMsgView ? mMsgView->copyText() : "";

  kDebug(5006) <<"Reply with template:" << tmpl;

  KMCommand *command = new KMCustomReplyToCommand(
      this, msg, text, tmpl
    );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyAllToMsg( const QString &tmpl )
{
  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
    return;

  QString text = mMsgView? mMsgView->copyText() : "";

  kDebug(5006) <<"Reply to All with template:" << tmpl;

  KMCommand *command = new KMCustomReplyAllToCommand(
      this, msg, text, tmpl
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomForwardMsg( const QString &tmpl )
{
  QList< KMMsgBase * > selectedMessages = mMessageListView->selectionAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
    return;

  kDebug(5006) <<"Forward with template:" << tmpl;

  KMCustomForwardCommand * command = new KMCustomForwardCommand(
      this, selectedMessages, mFolder->identity(), tmpl
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNoQuoteReplyToMsg()
{
  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
    return;

  KMCommand *command = new KMNoQuoteReplyToCommand( this, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubjectFilter()
{
  KMMessage *msg = mMessageListView->currentMessage();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailingListFilter()
{
  KMMessage *msg = mMessageListView->currentMessage();
  if (!msg)
    return;

  KMCommand *command = new KMMailingListFilterCommand( this, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFromFilter()
{
  KMMessage *msg = mMessageListView->currentMessage();
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
  KMMessage *msg = mMessageListView->currentMessage();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "To",  msg->to() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateListFilterAction()
{
  //Proxy the mListFilterAction to update the action text

  mListFilterAction->setText( i18n("Filter on Mailing-List...") );

  KMMessage * msg = mMessageListView->currentMessage();
  if ( !msg )
  {
    mListFilterAction->setEnabled( false );
    return;
  }

  QByteArray name;
  QString value;
  QString lname = MailingList::name( mMessageListView->currentMessage(), name, value );
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
  kmkernel->undoStack()->undo();
  updateMessageActions();
  updateFolderMenu();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotJumpToFolder()
{
  KMail::FolderSelectionDialog dlg( this, i18n("Jump to Folder"), false );
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  slotSelectFolder( dest );
}


void KMMainWidget::slotApplyFilters()
{
  QList< KMMsgBase * > msgList = mMessageListView->selectionAsMsgBaseList();

  if (KMail::ActionScheduler::isEnabled() || kmkernel->filterMgr()->atLeastOneOnlineImapFolderTarget())
  {
    // uses action scheduler
    KMFilterMgr::FilterSet set = KMFilterMgr::Explicit;
    QList<KMFilter*> filters = kmkernel->filterMgr()->filters();
    KMail::ActionScheduler *scheduler = new KMail::ActionScheduler( set, filters );
    scheduler->setAutoDestruct( true );

    foreach ( KMMsgBase *msg, msgList ) {
      scheduler->execFilters( msg );
    }

    return;
  }

  //prevent issues with stale message pointers by using serial numbers instead
  QList<unsigned long> serNums = KMMsgDict::serNumList( msgList );
  if ( serNums.isEmpty() )
    return;

  KCursorSaver busy( KBusyPtr::busy() );
  int msgCount = 0;
  int msgCountToFilter = serNums.count();

  ProgressItem* progressItem = ProgressManager::createProgressItem (
      "filter"+ProgressManager::getUniqueID(),
      i18n( "Filtering messages" )
    );

  progressItem->setTotalItems( msgCountToFilter );

  for ( QList<unsigned long>::ConstIterator it = serNums.constBegin(); it != serNums.constEnd(); ++it )
  {
    msgCount++;
    if ( msgCountToFilter - msgCount < 10 || !( msgCount % 10 ) || msgCount <= 10 )
    {
      progressItem->updateProgress();
      QString statusMsg = i18n( "Filtering message %1 of %2", msgCount, msgCountToFilter );
      KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );
      qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 50 );
    }

    KMFolder *folder = 0;
    int idx;
    KMMsgDict::instance()->getLocation( *it, &folder, &idx );
    KMMessage *msg = 0;
    if (folder)
      msg = folder->getMsg(idx);
    if (msg)
    {
      if (msg->transferInProgress())
        continue;
      msg->setTransferInProgress(true);
      if ( !msg->isComplete() )
      {
        FolderJob *job = mFolder->createJob(msg);
        connect(job, SIGNAL(messageRetrieved(KMMessage*)),
                this, SLOT(slotFilterMsg(KMMessage*)));
        job->start();
      } else {
        if (slotFilterMsg(msg) == 2)
          break;
      }
    } else {
      kDebug () << "A message went missing during filtering";
    }
    progressItem->incCompletedItems();
  }

  progressItem->setComplete();
  progressItem = 0;

}

int KMMainWidget::slotFilterMsg(KMMessage *msg)
{
  if ( !msg ) return 2; // messageRetrieve(0) is always possible
  msg->setTransferInProgress(false);
  int filterResult = kmkernel->filterMgr()->process(msg,KMFilterMgr::Explicit);
  if (filterResult == 2)
  {
    // something went horribly wrong (out of space?)
    kmkernel->emergencyExit( i18n("Unable to process messages: " ) + QString::fromLocal8Bit(strerror(errno)));
    return 2;
  }
  if (msg->parent())
  { // unGet this msg
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( msg, &p, &idx );
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
  }

  return filterResult;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckVacation()
{
  updateVactionScriptStatus( false );
  if ( !kmkernel->askToGoOnline() )
    return;

  Vacation *vac = new Vacation( this, true /* check only */ );
  connect( vac, SIGNAL(scriptActive(bool)), SLOT(updateVactionScriptStatus(bool)) );
}

void KMMainWidget::slotEditVacation()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  if ( mVacation )
    return;

  mVacation = new Vacation( this );
  connect( mVacation, SIGNAL(scriptActive(bool)), SLOT(updateVactionScriptStatus(bool)) );
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
  if( !QProcess::startDetached("kleopatra" ) )
    KMessageBox::error( this, i18n( "Could not start certificate manager; "
                                    "please check your installation." ),
                                    i18n( "KMail Error" ) );
  else
    kDebug(5006) <<"\nslotStartCertManager(): certificate manager started.";
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotStartWatchGnuPG()
{
  if( !QProcess::startDetached("kwatchgnupg") )
    KMessageBox::error( this, i18n( "Could not start GnuPG LogViewer (kwatchgnupg); "
                                    "please check your installation." ),
                                    i18n( "KMail Error" ) );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPrintMsg()
{
  KMMessage *msg = mMessageListView->currentMessage();
  if (!msg)
    return;

  bool htmlOverride = mMsgView ? mMsgView->htmlOverride() : false;
  bool htmlLoadExtOverride = mMsgView ? mMsgView->htmlLoadExtOverride() : false;
  KConfigGroup reader( KMKernel::config(), "Reader" );
  bool useFixedFont = mMsgView ? mMsgView->isFixedFont()
                               : reader.readEntry( "useFixedFont", false );

  KMCommand *command =
    new KMPrintCommand( this, msg,
                        mMsgView ? mMsgView->headerStyle() : 0,
                        mMsgView ? mMsgView->headerStrategy() : 0,
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
  QList< KMMsgBase * > selectedMessages = mMessageListView->selectionAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
    return;

  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this, selectedMessages );

  if ( saveCommand->url().isEmpty() )
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
  QList< KMMsgBase * > selectedMessages = mMessageListView->selectionAsMsgBaseList();
  if ( selectedMessages.isEmpty() )
    return;

  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( this, selectedMessages );

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
  QAction *action = actionCollection()->action( "online_status" );
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online ) {
    action->setText( i18n("Work Offline") );
    action->setIcon( KIcon("user-offline") );
  }
  else {
    action->setText( i18n("Work Online") );
    action->setIcon( KIcon("user-online") );
  }
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

  QStringList availTransports= MailTransport::TransportManager::self()->transportNames();
  if (availTransports.contains(item->text()))
    kmkernel->msgSender()->sendQueued( item->text() );
}


void KMMainWidget::openFolder()
{
  if ( !mFolder || mFolder->folderType() != KMFolderTypeImap ) {
    return;
  }
  KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
  assert( !mOpenedImapFolder );
  imap->open("mainwidget"); // will be closed in the folderSelected slot
  mOpenedImapFolder = true;
  // first get new headers before we select the folder
  imap->setSelected( true );

  // If the folder gets closed because of renaming, re-open it again to prevent
  // an assert in closeFolder() later.
  disconnect( imap, SIGNAL( closed( KMFolder* ) ),
              this, SLOT( folderClosed( KMFolder*) ) );
  connect( imap, SIGNAL( closed( KMFolder* ) ),
           this, SLOT( folderClosed( KMFolder*) ) );
}

void KMMainWidget::closeFolder()
{
  if ( !mFolder || mFolder->folderType() != KMFolderTypeImap ) {
    return;
  }
  assert( mOpenedImapFolder );
  KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
  imap->setSelected( false );
  mFolder->close( "mainwidget" );
  mOpenedImapFolder = false;
}

KMFolder * KMMainWidget::activeFolder() const
{
  return mFolder;
}

//-----------------------------------------------------------------------------
void KMMainWidget::folderClosed( KMFolder *folder )
{
  Q_UNUSED( folder );
  if ( !mFolder || mFolder->folderType() != KMFolderTypeImap ) {
    return;
  }

  mOpenedImapFolder = false;
  openFolder();
}

//-----------------------------------------------------------------------------
// Folder selection management
//
// FIXE: This is quite a messy (it does A LOT of things): needs deeper review
//

void KMMainWidget::folderSelected()
{
  // This is also triggered when the currenlty selected folder changes some internal state

  folderSelected( mFolder );
  updateFolderMenu();

  // update the caption (useful if the name changed)
  emit captionChangeRequest( mMainFolderView->currentItemFullPath() );
}

void KMMainWidget::slotMessageListViewCurrentFolderChanged( KMFolder * fld )
{
  if ( fld == mFolder )
    return;

  mMainFolderView->setCurrentFolder( fld );
}

void KMMainWidget::slotFolderViewManagerFolderActivated( KMFolder * fld, bool middleClick )
{
  // This is triggered specifically when a folder is activated in the one of the FolderView
  folderSelected( fld, false, middleClick );
}

//-----------------------------------------------------------------------------
void KMMainWidget::folderSelected( KMFolder* aFolder, bool forceJumpToUnread, bool preferNewTabForOpening )
{
  // This is connected to the MainFolderView signal triggering when a folder is selected

  if ( !forceJumpToUnread )
    forceJumpToUnread = mGoToFirstUnreadMessageInSelectedFolder;

  mGoToFirstUnreadMessageInSelectedFolder = false;

  KMail::MessageListView::Core::PreSelectionMode preSelectionMode;
  if ( forceJumpToUnread )
  {
    // the default action has been overridden from outside
    preSelectionMode = KMail::MessageListView::Core::PreSelectFirstNewOrUnreadCentered;
  } else {
    // use the default action
    switch ( GlobalSettings::self()->actionEnterFolder() )
    {
      case GlobalSettings::EnumActionEnterFolder::SelectFirstNew:
        preSelectionMode = KMail::MessageListView::Core::PreSelectFirstNewCentered;
      break;
      case GlobalSettings::EnumActionEnterFolder::SelectFirstUnreadNew:
        preSelectionMode = KMail::MessageListView::Core::PreSelectFirstNewOrUnreadCentered;
      break;
      case GlobalSettings::EnumActionEnterFolder::SelectLastSelected:
        preSelectionMode = KMail::MessageListView::Core::PreSelectLastSelected;
      break;
      default:
        preSelectionMode = KMail::MessageListView::Core::PreSelectNone;
      break;
    }
  }

  KCursorSaver busy(KBusyPtr::busy());

  if (mMsgView)
    mMsgView->clear(true);

  bool newFolder = ( (KMFolder*)mFolder != aFolder );

  if ( mFolder && newFolder && ( mFolder->folderType() == KMFolderTypeImap ) && !mFolder->noContent() )
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
  bool isNewImapFolder = aFolder && ( aFolder->folderType() == KMFolderTypeImap ) && newFolder;
  if( !mFolder
      || ( !isNewImapFolder && mShowBusySplashTimer )
      || ( newFolder && mShowingOfflineScreen && !( isNewImapFolder && kmkernel->isOffline() ) ) ) {
    if ( mMsgView ) {
      mMsgView->enableMsgDisplay();
      mMsgView->clear( true );
    }
    if ( mMessageListView )
      mMessageListView->show();
    mShowingOfflineScreen = false;
  }

  // Delete any pending timer, if needed it will be recreated below
  delete mShowBusySplashTimer;
  mShowBusySplashTimer = 0;

  if ( newFolder )
  {
    // We're changing folder: write configuration for the old one
    writeFolderConfig();
  }

  if ( mFolder )
  {
    // Have an old folder: disconnect it first.
    disconnect( mFolder, SIGNAL( changed() ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
    disconnect( mFolder, SIGNAL( msgRemoved( KMFolder * ) ),
           this, SLOT( updateMarkAsReadAction() ) );
  }

  if ( newFolder )
  {
    // Close the old folder, if any
    closeFolder();
  }

  mFolder = aFolder;

  if ( newFolder )
  {
    // Open the new folder
    openFolder();
  }

  // FIXME: re-fetch the contents also if the folder is already open ?
  if ( aFolder && ( aFolder->folderType() == KMFolderTypeImap )  && ( !mMessageListView->isFolderOpen( mFolder ) ) )
  {
    // IMAP folders require extra care.

    if ( kmkernel->isOffline() )
    {
      //mMessageListView->setCurrentFolder( 0 ); <-- useless in the new view: just do nothing
      // FIXME: Use an "offline tab" ?
      showOfflinePage();
      return;
    }

    KMFolderImap *imap = static_cast<KMFolderImap*>( aFolder->storage() );

    if ( newFolder && ( !mFolder->noContent() ) )
    {
      // Folder is not offline, but the contents might need to be fetched
      connect( imap, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
               this, SLOT( folderSelected() ) );

      imap->getAndCheckFolder();

      mFolder = 0;          // will make us ignore the "folderChanged" signal from KMail::MessageListView (prevent circular loops)
      mMessageListView->setCurrentFolder(
          0,
          preferNewTabForOpening,
          KMail::MessageListView::Core::PreSelectNone,
          i18nc( "tab title when loading an IMAP folder", "Loading..." )
        );

      mFolder = aFolder;    // re-enable the signals from KMail::MessageListView

      updateFolderMenu();
      mForceJumpToUnread = forceJumpToUnread;

      // Set a timer to show a splash screen if fetching folder contents
      // takes more than the amount of seconds configured in the kmailrc (default 1000 msec)

      // FIXME: Use a "loading" tab instead ?
      mShowBusySplashTimer = new QTimer( this );
      mShowBusySplashTimer->setSingleShot( true );
      connect( mShowBusySplashTimer, SIGNAL( timeout() ), this, SLOT( slotShowBusySplash() ) );
      mShowBusySplashTimer->start( GlobalSettings::self()->folderLoadingTimeout() );
      return;

    }

    // the folder is complete now - so go ahead
    disconnect( imap, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
                this, SLOT( folderSelected() ) );

    forceJumpToUnread = mForceJumpToUnread;
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

  mMessageListView->setCurrentFolder(
      mFolder,
      preferNewTabForOpening,
      preSelectionMode
    );

  updateMessageActions();
  updateFolderMenu();
  if ( !mFolder && ( mMessageListView->count() < 2 ) )
    slotIntro();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowBusySplash()
{
  if ( mReaderWindowActive )
  {
    mMsgView->displayBusyPage();
    // hide widgets that are in the way:
    if ( mMessageListView && mLongFolderList )
      mMessageListView->hide();
  }
}

void KMMainWidget::showOfflinePage()
{
  if ( !mReaderWindowActive ) return;
  mShowingOfflineScreen = true;

  mMsgView->displayOfflinePage();
  // hide widgets that are in the way:
  if ( mMessageListView && mLongFolderList )
    mMessageListView->hide();
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
    mJob = msg->parent()->createJob( msg, FolderJob::tGetMessage, msg->parent(),
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
  mMsgView->setDecryptMessageOverwrite( false );
  mMsgView->setShowSignatureDetails( false );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectFolder(KMFolder* folder)
{
  mMainFolderView->setCurrentFolder( folder );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectMessage(KMMessage* msg)
{
  int idx = mFolder->find(msg);
  if (idx != -1) {
#if 0
    // FIXME (Pragma)
    mHeaders->setCurrentMsg(idx);
#endif
    if (mMsgView)
      mMsgView->setMsg(msg);
    else
      slotMsgActivated(msg);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplaceMsgByUnencryptedVersion()
{
  kDebug(5006);
  KMMessage* oldMsg = mMessageListView->currentMessage();
  if( oldMsg ) {
    kDebug(5006) << "Old message found";
    if( oldMsg->hasUnencryptedMsg() ) {
      kDebug(5006) << "Extra unencrypted message found";
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
      kDebug(5006) << "Adding unencrypted message to folder";
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
#if 0
      // FIXME (Pragma)
      mHeaders->setCurrentItemByIndex( newMsgIdx );
#endif
      // remove the old one
      if ( idx != -1 ) {
        kDebug(5006) << "Deleting encrypted message";
        mFolder->take( idx );
      }

      kDebug(5006) << "Updating message actions";
      updateMessageActions();

      kDebug(5006) << "Done.";
    } else
      kDebug(5006) << "NO EXTRA UNENCRYPTED MESSAGE FOUND";
  } else
    kDebug(5006) << "PANIC: NO OLD MESSAGE FOUND";
}

void KMMainWidget::slotFocusOnNextMessage()
{
  mMessageListView->focusNextMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      true,  // center item
      false  // don't loop
    );
}

void KMMainWidget::slotFocusOnPrevMessage()
{
  mMessageListView->focusPreviousMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      true,  // center item
      false  // don't loop
    );
}

void KMMainWidget::slotSelectFocusedMessage()
{
  mMessageListView->selectFocusedMessageItem(
      true   // center item
    );
}

void KMMainWidget::slotSelectNextMessage()
{
  mMessageListView->selectNextMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      KMail::MessageListView::Core::ClearExistingSelection,
      true,  // center item
      false  // don't loop in folder
    );
}

void KMMainWidget::slotExtendSelectionToNextMessage()
{
  mMessageListView->selectNextMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      KMail::MessageListView::Core::GrowOrShrinkExistingSelection,
      true,  // center item
      false  // don't loop in folder
    );
}

void KMMainWidget::slotSelectNextUnreadMessage()
{
  // The looping logic is: "Don't loop" just never loops, "Loop in current folder"
  // loops just in current folder, "Loop in all folders" loops in the current folder
  // first and then after confirmation jumps to the next folder.
  // A bad point here is that if you answer "No, and don't ask me again" to the confirmation
  // dialog then you have "Loop in current folder" and "Loop in all folders" that do
  // the same thing and no way to get the old behaviour. However, after a consultation on #kontact,
  // for bug-to-bug backward compatibility, the masters decided to keep it b0rken :D
  // If nobody complains, it stays like it is: if you complain enough maybe the masters will
  // decide to reconsider :)
  if ( !mMessageListView->selectNextMessageItem(
      KMail::MessageListView::Core::MessageTypeNewOrUnreadOnly,
      KMail::MessageListView::Core::ClearExistingSelection,
      true,  // center item
      /*GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder*/
      GlobalSettings::self()->loopOnGotoUnread() != GlobalSettings::EnumLoopOnGotoUnread::DontLoop
    ) )
  {
    // no next unread message was found in the current folder
    if ( GlobalSettings::self()->loopOnGotoUnread() ==
         GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders )
    {
      mGoToFirstUnreadMessageInSelectedFolder = true;
      mMainFolderView->selectNextUnreadFolder(
          true // confirm
        );
      mGoToFirstUnreadMessageInSelectedFolder = false;
    }
  }
}

void KMMainWidget::slotSelectPreviousMessage()
{
  mMessageListView->selectPreviousMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      KMail::MessageListView::Core::ClearExistingSelection,
      true,  // center item
      false  // don't loop in folder
    );
}

void KMMainWidget::slotExtendSelectionToPreviousMessage()
{
  mMessageListView->selectPreviousMessageItem(
      KMail::MessageListView::Core::MessageTypeAny,
      KMail::MessageListView::Core::GrowOrShrinkExistingSelection,
      true,  // center item
      false  // don't loop in folder
    );
}

void KMMainWidget::slotSelectPreviousUnreadMessage()
{
  if ( !mMessageListView->selectPreviousMessageItem(
      KMail::MessageListView::Core::MessageTypeNewOrUnreadOnly,
      KMail::MessageListView::Core::ClearExistingSelection,
      true,  // center item
      GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder
    ) )
  {
    // no next unread message was found in the current folder
    if ( GlobalSettings::self()->loopOnGotoUnread() ==
         GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders )
    {
      mGoToFirstUnreadMessageInSelectedFolder = true;
      mMainFolderView->selectPrevUnreadFolder();
      mGoToFirstUnreadMessageInSelectedFolder = false;
    }
  }
}

void KMMainWidget::slotDisplayCurrentMessage()
{
  if ( mMessageListView->currentMessage() )
    slotMsgActivated( mMessageListView->currentMessage() );
}

//-----------------------------------------------------------------------------
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
    mMsgActions->editCurrentMessage();
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
  mMessageListView->selectAll();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgPopup( KMMessage &msg, const KUrl &aUrl, const QPoint &aPoint )
{
  mMessageListView->activateMessage( &msg ); // make sure that this message is the active one

  // If this assertion fails then our current KMReaderWin is displaying a
  // message from a folder that is not the current in mMessageListView.
  // This should never happen as all the actions are messed up in this case.
  Q_ASSERT( &msg == mMessageListView->currentMessage() );

  updateMessageMenu();

  KMenu *menu = new KMenu;
  mUrlCurrent = aUrl;

  bool urlMenuAdded = false;

  if ( !aUrl.isEmpty() ) {
    if ( aUrl.protocol() == "mailto" ) {
      // popup on a mailto URL
      menu->addAction( mMsgView->mailToComposeAction() );
      menu->addAction( mMsgView->mailToReplyAction() );
      menu->addAction( mMsgView->mailToForwardAction() );

      menu->addSeparator();

      QString email =  KPIMUtils::firstEmailAddress( aUrl.path() );
      KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
      KABC::Addressee::List addresseeList = addressBook->findByEmail( email );

      if ( addresseeList.count() == 0 ) {
        menu->addAction( mMsgView->addAddrBookAction() );
      } else {
        menu->addAction( mMsgView->openAddrBookAction() );
      }
      menu->addAction( mMsgView->copyURLAction() );
    } else {
      // popup on a not-mailto URL
      menu->addAction( mMsgView->urlOpenAction() );
      menu->addAction( mMsgView->addBookmarksAction() );
      menu->addAction( mMsgView->urlSaveAsAction() );
      menu->addAction( mMsgView->copyURLAction() );
    }

    urlMenuAdded = true;
    kDebug() <<" URL is:" << aUrl;
  }

  if ( mMsgView && !mMsgView->copyText().isEmpty() ) {
    if ( urlMenuAdded ) {
      menu->addSeparator();
    }
    menu->addAction( mMsgActions->replyMenu() );
    menu->addSeparator();

    menu->addAction( mMsgView->copyAction() );
    menu->addAction( mMsgView->selectAllAction() );
  } else if ( !urlMenuAdded ) {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMessageListView->currentMessage()) {
      // no messages
      delete menu;
      return;
    }

    if ( mFolder->isTemplates() ) {
      menu->addAction( mUseAction );
    } else {
      menu->addAction( mMsgActions->replyMenu() );
      menu->addAction( mForwardActionMenu );
    }
    menu->addAction(editAction());
    menu->addSeparator();

    menu->addAction( mCopyActionMenu );
    menu->addAction( mMoveActionMenu );

    menu->addSeparator();

    menu->addAction( mMsgActions->messageStatusMenu() );
    menu->addSeparator();

    menu->addAction( viewSourceAction() );
    if ( mMsgView ) {
      menu->addAction( mMsgView->toggleFixFontAction() );
    }
    menu->addSeparator();
    menu->addAction( mPrintAction );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAttachmentsAction );

    menu->addSeparator();
    if ( mFolder->isTrash() ) {
      menu->addAction( mDeleteAction );
    } else {
      menu->addAction( mTrashAction );
    }

    menu->addSeparator();
    menu->addAction( mMsgActions->createTodoAction() );
  }
  KAcceleratorManager::manage(menu);
  menu->exec( aPoint, 0 );
  delete menu;
}

//-----------------------------------------------------------------------------
void KMMainWidget::getAccountMenu()
{
  QStringList actList;

  mActMenu->clear();
  actList = kmkernel->acctMgr()->getAccounts();
  QStringList::Iterator it;
  foreach ( const QString& accountName, actList )
  {
    // Explicitly make a copy, as we're not changing values of the list but only
    // the local copy which is passed to action.
    QAction* action = mActMenu->addAction( QString( accountName ).replace('&', "&&") );
    action->setData( accountName );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::getTransportMenu()
{
  QStringList availTransports;

  mSendMenu->clear();
  availTransports = MailTransport::TransportManager::self()->transportNames();
  QStringList::Iterator it;
  for(it = availTransports.begin(); it != availTransports.end() ; ++it)
    mSendMenu->addAction((*it).replace('&', "&&"));
}


//-----------------------------------------------------------------------------
void KMMainWidget::updateCustomTemplateMenus()
{
  if ( !mCustomTemplateMenus )
  {
    mCustomTemplateMenus = new CustomTemplatesMenu( this, actionCollection() );
    connect( mCustomTemplateMenus, SIGNAL(replyTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyToMsg( const QString& )) );
    connect( mCustomTemplateMenus, SIGNAL(replyAllTemplateSelected( const QString& )),
             this, SLOT(slotCustomReplyAllToMsg( const QString& )) );
    connect( mCustomTemplateMenus, SIGNAL(forwardTemplateSelected( const QString& )),
             this, SLOT(slotCustomForwardMsg( const QString& )) );
  }

  mForwardActionMenu->addSeparator();
  mForwardActionMenu->addAction( mCustomTemplateMenus->forwardActionMenu() );

  mMsgActions->replyMenu()->addSeparator();
  mMsgActions->replyMenu()->addAction( mCustomTemplateMenus->replyActionMenu() );
  mMsgActions->replyMenu()->addAction( mCustomTemplateMenus->replyAllActionMenu() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::setupActions()
{
  mMsgActions = new KMail::MessageActions( actionCollection(), this );
  mMsgActions->setMessageView( mMsgView );

  //----- File Menu
  mSaveAsAction = new KAction(KIcon("document-save"), i18n("Save &As..."), this);
  actionCollection()->addAction("file_save_as", mSaveAsAction );
  connect(mSaveAsAction, SIGNAL(triggered(bool) ), SLOT(slotSaveMsg()));
  mSaveAsAction->setShortcut(KStandardShortcut::save());

  mOpenAction = KStandardAction::open( this, SLOT( slotOpenMsg() ),
                                  actionCollection() );

  {
    KAction *action = new KAction(i18n("&Compact All Folders"), this);
    actionCollection()->addAction("compact_all_folders", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCompactAll()));
  }
  {
    KAction *action = new KAction(i18n("&Expire All Folders"), this);
    actionCollection()->addAction("expire_all_folders", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpireAll()));
  }
  {
    KAction *action = new KAction(KIcon("view-refresh"), i18n("&Refresh Local IMAP Cache"), this);
    actionCollection()->addAction("file_invalidate_imap_cache", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotInvalidateIMAPFolders()));
  }
  {
    KAction *action = new KAction(i18n("Empty All &Trash Folders"), this);
    actionCollection()->addAction("empty_trash", action );
    connect(action, SIGNAL(triggered(bool) ), KMKernel::self(), SLOT(slotEmptyTrash()));
  }
  {
    KAction *action = new KAction(KIcon("mail-receive"), i18n("Check &Mail"), this);
    actionCollection()->addAction("check_mail", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCheckMail()));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));
  }

  mFavoritesCheckMailAction = new KAction( KIcon( "mail-receive"),
                                           i18n( "Check Mail in Favorite Folders" ), this );
  actionCollection()->addAction( "favorite_check_mail", mFavoritesCheckMailAction );
  mFavoritesCheckMailAction->setShortcut( QKeySequence( Qt::CTRL+Qt::SHIFT+Qt::Key_L ) );
  if ( mFavoriteFolderView ) {
    connect( mFavoritesCheckMailAction, SIGNAL(triggered(bool)),
             mFavoriteFolderView, SLOT(checkMail()) );
  }

  KActionMenu *actActionMenu = new KActionMenu(KIcon("mail-receive"), i18n("Check Ma&il"), this);
  actionCollection()->addAction("check_mail_in", actActionMenu );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"
  connect(actActionMenu, SIGNAL(triggered(bool)), this, SLOT(slotCheckMail()));
  mActMenu = actActionMenu->menu();
  connect(mActMenu, SIGNAL(triggered(QAction*)),
          SLOT(slotCheckOneAccount(QAction*)));
  connect(mActMenu, SIGNAL(aboutToShow()), SLOT(getAccountMenu()));

  {
    KAction *action = new KAction(KIcon("mail-send"), i18n("&Send Queued Messages"), this);
    actionCollection()->addAction("send_queued", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSendQueued()));
  }
  {
    KAction *action = new KAction( i18n("Onlinestatus (unknown)"), this );
    actionCollection()->addAction( "online_status", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotOnlineStatus()) );
  }

  KActionMenu *sendActionMenu = new KActionMenu(KIcon("mail-send-via"), i18n("Send Queued Messages Via"), this);
  actionCollection()->addAction("send_queued_via", sendActionMenu );
  sendActionMenu->setDelayed(true);

  mSendMenu = sendActionMenu->menu();
  connect(mSendMenu,SIGNAL(triggered(QAction*)), SLOT(slotSendQueuedVia(QAction*)));
  connect(mSendMenu,SIGNAL(aboutToShow()),SLOT(getTransportMenu()));

  //----- Tools menu
  if (parent()->inherits("KMMainWin")) {
    KAction *action = new KAction(KIcon("help-contents"), i18n("&Address Book"), this);
    actionCollection()->addAction("addressbook", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotAddrBook()));
    if (KStandardDirs::findExe("kaddressbook").isEmpty()) action->setEnabled(false);
  }

  {
    KAction *action = new KAction(KIcon("pgp-keys"), i18n("Certificate Manager"), this);
    actionCollection()->addAction("tools_start_certman", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotStartCertManager()));
    // disable action if no certman binary is around
    if (KStandardDirs::findExe("kleopatra").isEmpty()) action->setEnabled(false);
  }
  {
    KAction *action = new KAction(KIcon("pgp-keys"), i18n("GnuPG Log Viewer"), this);
    actionCollection()->addAction("tools_start_kwatchgnupg", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotStartWatchGnuPG()));
    // disable action if no kwatchgnupg binary is around
    bool usableKWatchGnupg = !KStandardDirs::findExe("kwatchgnupg").isEmpty();
#ifdef Q_OS_WIN32
    // not ported yet, underlying infrastructure missing on Windows
    usableKWatchGnupg = false;
#endif
    action->setEnabled(usableKWatchGnupg);
  }
  {
    KAction *action = new KAction(KIcon("document-import"), i18n("&Import Messages"), this);
    actionCollection()->addAction("import", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotImport()));
    if (KStandardDirs::findExe("kmailcvt").isEmpty()) action->setEnabled(false);
  }

#if !defined(NDEBUG)
  {
    KAction *action = new KAction(i18n("&Debug Sieve..."), this);
    actionCollection()->addAction("tools_debug_sieve", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotDebugSieve()));
  }
#endif

  {
    KAction *action = new KAction(i18n("Filter &Log Viewer..."), this);
    actionCollection()->addAction("filter_log_viewer", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotFilterLogViewer()));
  }
  {
    KAction *action = new KAction(i18n("&Anti-Spam Wizard..."), this);
    actionCollection()->addAction("antiSpamWizard", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotAntiSpamWizard()));
  }
  {
    KAction *action = new KAction(i18n("&Anti-Virus Wizard..."), this);
    actionCollection()->addAction("antiVirusWizard", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotAntiVirusWizard()));
  }
  {
    KAction *action = new KAction( i18n("&Account Wizard..."), this );
    actionCollection()->addAction( "accountWizard", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotAccountWizard()) );
  }
  if ( GlobalSettings::allowOutOfOfficeSettings() )
  {
    KAction *action = new KAction( i18n("Edit \"Out of Office\" Replies..."), this );
    actionCollection()->addAction( "tools_edit_vacation", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotEditVacation()) );
  }

  //----- Edit Menu
  mTrashAction = new KAction(i18n("&Move to Trash"), this);
  actionCollection()->addAction("move_to_trash", mTrashAction );
  mTrashAction->setIcon(KIcon("user-trash"));
  mTrashAction->setIconText( i18nc( "@action:intoolbar Move to Trash", "Trash" ) );
  mTrashAction->setShortcut(QKeySequence(Qt::Key_Delete));
  mTrashAction->setToolTip(i18n("Move message to trashcan"));
  connect(mTrashAction, SIGNAL(triggered(bool)), SLOT(slotTrashMsg()));

  /* The delete action is nowhere in the gui, by default, so we need to make
   * sure it is plugged into the KAccel now, since that won't happen on
   * XMLGui construction or manual ->plug(). This is only a problem when run
   * as a part, though. */
  mDeleteAction = new KAction(KIcon("edit-delete"), i18nc("@action Hard delete, bypassing trash", "&Delete"), this);
  actionCollection()->addAction("delete", mDeleteAction );
  connect(mDeleteAction, SIGNAL(triggered(bool)), SLOT(slotDeleteMsg()));
  mDeleteAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_Delete));

  mTrashThreadAction = new KAction(i18n("M&ove Thread to Trash"), this);
  actionCollection()->addAction("move_thread_to_trash", mTrashThreadAction );
  mTrashThreadAction->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Delete));
  mTrashThreadAction->setIcon(KIcon("user-trash"));
  mTrashThreadAction->setToolTip(i18n("Move thread to trashcan") );
  connect(mTrashThreadAction, SIGNAL(triggered(bool)), SLOT(slotTrashThread()));

  mDeleteThreadAction = new KAction(KIcon("edit-delete"), i18n("Delete T&hread"), this);
  actionCollection()->addAction("delete_thread", mDeleteThreadAction );
  connect(mDeleteThreadAction, SIGNAL(triggered(bool)), SLOT(slotDeleteThread()));
  mDeleteThreadAction->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Delete));

  {
    KAction *action = new KAction(KIcon("edit-find-mail"), i18n("&Find Messages..."), this);
    actionCollection()->addAction("search_messages", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotRequestFullSearchFromQuickSearch()));
    action->setShortcut(QKeySequence(Qt::Key_S));
  }

  mFindInMessageAction = new KAction(KIcon("edit-find"), i18n("&Find in Message..."), this);
  actionCollection()->addAction("find_in_messages", mFindInMessageAction );
  connect(mFindInMessageAction, SIGNAL(triggered(bool)), SLOT(slotFind()));
  //FIXME: this causes ambiguous Ctrl-F problems and the shortcut fails
  //unfortunately, this has the side-effect of having a blank Ctrl-F shortcut
  //string on the Find menu.
  //mFindInMessageAction->setShortcut(KStandardShortcut::find());

  {
    KAction *action = new KAction(i18n("Select &All Messages"), this);
    actionCollection()->addAction("mark_all_messages", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotMarkAll()));
    action->setShortcut(KStandardShortcut::selectAll());
  }

  //----- Folder Menu
  mNewFolderAction = new KAction( KIcon("folder-new"), i18n("&New Folder..."), this );
  actionCollection()->addAction( "new_folder", mNewFolderAction );
  connect( mNewFolderAction, SIGNAL(triggered(bool)), mMainFolderView, SLOT(slotAddChildFolder()) );

  mModifyFolderAction = new KAction(KIcon("document-properties"), i18n("&Properties"), this);
  actionCollection()->addAction("modify", mModifyFolderAction );
  connect(mModifyFolderAction, SIGNAL(triggered(bool)), SLOT(slotModifyFolder()));

  mFolderMailingListPropertiesAction = new KAction(i18n("&Mailing List Management..."), this);
  actionCollection()->addAction("folder_mailinglist_properties", mFolderMailingListPropertiesAction );
  connect(mFolderMailingListPropertiesAction, SIGNAL(triggered(bool)), SLOT( slotFolderMailingListProperties()));
  // mFolderMailingListPropertiesAction->setIcon(KIcon("document-properties-mailing-list"));

  // FIXME: A very similar action is set up also by the folder views... (???)
  mFolderShortCutCommandAction = new KAction(KIcon("configure-shortcuts"), i18n("&Assign Shortcut..."), this);
  actionCollection()->addAction("folder_shortcut_command", mFolderShortCutCommandAction );
  connect(mFolderShortCutCommandAction, SIGNAL(triggered(bool) ), SLOT( slotFolderShortcutCommand() ));


  mMarkAllAsReadAction = new KAction(KIcon("mail-mark-read"), i18n("Mark All Messages as &Read"), this);
  actionCollection()->addAction("mark_all_as_read", mMarkAllAsReadAction );
  connect(mMarkAllAsReadAction, SIGNAL(triggered(bool)), SLOT(slotMarkAllAsRead()));

  mExpireFolderAction = new KAction(i18n("&Expiration Settings"), this);
  actionCollection()->addAction("expire", mExpireFolderAction );
  connect(mExpireFolderAction, SIGNAL(triggered(bool) ), SLOT(slotExpireFolder()));

  mCompactFolderAction = new KAction(i18n("&Compact Folder"), this);
  actionCollection()->addAction("compact", mCompactFolderAction );
  connect(mCompactFolderAction, SIGNAL(triggered(bool) ), SLOT(slotCompactFolder()));

  mRefreshFolderAction = new KAction(KIcon("view-refresh"), i18n("Check Mail &in This Folder"), this);
  actionCollection()->addAction("refresh_folder", mRefreshFolderAction );
  connect(mRefreshFolderAction, SIGNAL(triggered(bool) ), SLOT(slotRefreshFolder()));
  mRefreshFolderAction->setShortcut(KStandardShortcut::reload());
  mTroubleshootFolderAction = 0; // set in initializeIMAPActions

  mTroubleshootMaildirAction = new KAction( KIcon("tools-wizard"), i18n("Rebuild Index..."), this );
  actionCollection()->addAction( "troubleshoot_maildir", mTroubleshootMaildirAction );
  connect( mTroubleshootMaildirAction, SIGNAL(triggered()), SLOT(slotTroubleshootMaildir()) );

  mEmptyFolderAction = new KAction(KIcon("user-trash"),
                                    "foo" /*set in updateFolderMenu*/, this);
  actionCollection()->addAction("empty", mEmptyFolderAction );
  connect(mEmptyFolderAction, SIGNAL(triggered(bool)), SLOT(slotEmptyFolder()));

  mRemoveFolderAction = new KAction(KIcon("edit-delete"),
                                     "foo" /*set in updateFolderMenu*/, this);
  actionCollection()->addAction("delete_folder", mRemoveFolderAction );
  connect(mRemoveFolderAction, SIGNAL(triggered(bool)), SLOT(slotRemoveFolder()));

  mPreferHtmlAction = new KToggleAction(i18n("Prefer &HTML to Plain Text"), this);
  actionCollection()->addAction("prefer_html", mPreferHtmlAction );
  connect(mPreferHtmlAction, SIGNAL(triggered(bool) ), SLOT(slotOverrideHtml()));

  mPreferHtmlLoadExtAction = new KToggleAction(i18n("Load E&xternal References"), this);
  actionCollection()->addAction("prefer_html_external_refs", mPreferHtmlLoadExtAction );
  connect(mPreferHtmlLoadExtAction, SIGNAL(triggered(bool) ), SLOT(slotOverrideHtmlLoadExt()));

  {
    KAction *action = new KAction(KIcon("edit-copy"), i18n("Copy Folder"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_C));
    actionCollection()->addAction("copy_folder", action);
    connect(action, SIGNAL(triggered(bool)), mMainFolderView, SLOT(slotCopyFolder()));
  }
  {
    KAction *action = new KAction(KIcon("edit-cut"), i18n("Cut Folder"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_X));
    actionCollection()->addAction("cut_folder", action);
    connect(action, SIGNAL(triggered(bool)), mMainFolderView, SLOT(slotCutFolder()));
  }
  {
    KAction *action = new KAction(KIcon("edit-paste"), i18n("Paste Folder"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_V));
    actionCollection()->addAction("paste_folder", action);
    connect(action, SIGNAL(triggered(bool)), mMainFolderView, SLOT(slotPasteFolder()));
  }
  {
    KAction *action = new KAction(KIcon("edit-copy"), i18n("Copy Messages"), this);
    action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_C));
    actionCollection()->addAction("copy_messages", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotCopyMessages()));
  }
  {
    KAction *action = new KAction(KIcon("edit-cut"), i18n("Cut Messages"), this);
    action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_X));
    actionCollection()->addAction("cut_messages", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotCutMessages()));
  }
  {
    KAction *action = new KAction(KIcon("edit-paste"), i18n("Paste Messages"), this);
    action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_V));
    actionCollection()->addAction("paste_messages", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotPasteMessages()));
  }

  //----- Message Menu
  {
    KAction *action = new KAction(KIcon("mail-message-new"), i18n("&New Message..."), this);
    actionCollection()->addAction("new_message", action );
    action->setIconText( i18nc("@action:intoolbar New Empty Message", "New" ) );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCompose()));
    // do not set a New shortcut if kmail is a component
    if ( !kmkernel->xmlGuiInstance().isValid() ) {
      action->setShortcut(KStandardShortcut::openNew());
    }
  }

  mTemplateMenu = new KActionMenu( KIcon( "document-new" ), i18n("Message From &Template"),
                                   actionCollection() );
  mTemplateMenu->setDelayed( true );
  actionCollection()->addAction("new_from_template", mTemplateMenu );
  connect( mTemplateMenu->menu(), SIGNAL( aboutToShow() ), this,
           SLOT( slotShowNewFromTemplate() ) );
  connect( mTemplateMenu->menu(), SIGNAL( triggered(QAction*) ), this,
           SLOT( slotNewFromTemplate(QAction*) ) );

  mPostToMailinglistAction = new KAction( KIcon( "mail-message-new-list" ),
                                          i18n( "New Message t&o Mailing-List..." ),
                                          this );
  actionCollection()->addAction("post_message", mPostToMailinglistAction );
  connect( mPostToMailinglistAction, SIGNAL( triggered(bool) ),
           SLOT( slotPostToML() ) );
  mPostToMailinglistAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_N ) );

  mForwardActionMenu = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotForwardMsg()) );

  mForwardAttachedAction = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","As &Attachment..."), this);
  actionCollection()->addAction("message_forward_as_attachment", mForwardAttachedAction );
  mForwardAttachedAction->setShortcut(QKeySequence(Qt::Key_F));
  connect(mForwardAttachedAction, SIGNAL(triggered(bool) ), SLOT(slotForwardAttachedMsg()));
  mForwardActionMenu->addAction( forwardAttachedAction() );
  mForwardAction = new KAction(KIcon("mail-forward"),
    i18nc("@action:inmenu Message->Forward->", "&Inline..."), this);
  actionCollection()->addAction("message_forward_inline", mForwardAction );
  connect(mForwardAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));
  mForwardAction->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F));

  mForwardActionMenu->addAction( forwardAction() );

  mSendAgainAction = new KAction(i18n("Send A&gain..."), this);
  actionCollection()->addAction("send_again", mSendAgainAction );
  connect(mSendAgainAction, SIGNAL(triggered(bool) ), SLOT(slotResendMsg()));

  mRedirectAction = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","&Redirect..."), this);
  actionCollection()->addAction("message_forward_redirect", mRedirectAction );
  mRedirectAction->setShortcut(QKeySequence(Qt::Key_E));
  connect(mRedirectAction, SIGNAL(triggered(bool) ), SLOT(slotRedirectMsg()));
  mForwardActionMenu->addAction( redirectAction() );

  //----- Create filter actions
  mFilterMenu = new KActionMenu(KIcon("view-filter"), i18n("&Create Filter"), this);
  actionCollection()->addAction("create_filter", mFilterMenu );
  connect( mFilterMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotFilter()) );
  mSubjectFilterAction = new KAction(i18n("Filter on &Subject..."), this);
  actionCollection()->addAction("subject_filter", mSubjectFilterAction );
  connect(mSubjectFilterAction, SIGNAL(triggered(bool) ), SLOT(slotSubjectFilter()));
  mFilterMenu->addAction( mSubjectFilterAction );

  mFromFilterAction = new KAction(i18n("Filter on &From..."), this);
  actionCollection()->addAction("from_filter", mFromFilterAction );
  connect(mFromFilterAction, SIGNAL(triggered(bool) ), SLOT(slotFromFilter()));
  mFilterMenu->addAction( mFromFilterAction );

  mToFilterAction = new KAction(i18n("Filter on &To..."), this);
  actionCollection()->addAction("to_filter", mToFilterAction );
  connect(mToFilterAction, SIGNAL(triggered(bool) ), SLOT(slotToFilter()));
  mFilterMenu->addAction( mToFilterAction );

  mListFilterAction = new KAction(i18n("Filter on Mailing-&List..."), this);
  actionCollection()->addAction("mlist_filter", mListFilterAction );
  connect(mListFilterAction, SIGNAL(triggered(bool) ), SLOT(slotMailingListFilter()));
  mFilterMenu->addAction( mListFilterAction );

  mPrintAction = KStandardAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  mUseAction = new KAction( KIcon("file-new"), i18n("New Message From &Template"), this );
  actionCollection()->addAction("use_templace", mUseAction);
  connect(mUseAction, SIGNAL(triggered(bool) ), SLOT(slotUseTemplate()));
  mUseAction->setShortcut(QKeySequence(Qt::Key_N));

  //----- "Mark Thread" submenu
  mThreadStatusMenu = new KActionMenu(i18n("Mark &Thread"), this);
  actionCollection()->addAction("thread_status", mThreadStatusMenu );

  mMarkThreadAsReadAction = new KAction(KIcon("mail-mark-read"), i18n("Mark Thread as &Read"), this);
  actionCollection()->addAction("thread_read", mMarkThreadAsReadAction );
  connect(mMarkThreadAsReadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusRead()));
  mMarkThreadAsReadAction->setToolTip(i18n("Mark all messages in the selected thread as read"));
  mThreadStatusMenu->addAction( mMarkThreadAsReadAction );

  mMarkThreadAsNewAction = new KAction(KIcon("mail-mark-unread-new"), i18n("Mark Thread as &New"), this);
  actionCollection()->addAction("thread_new", mMarkThreadAsNewAction );
  connect(mMarkThreadAsNewAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusNew()));
  mMarkThreadAsNewAction->setToolTip( i18n("Mark all messages in the selected thread as new"));
  mThreadStatusMenu->addAction( mMarkThreadAsNewAction );

  mMarkThreadAsUnreadAction = new KAction(KIcon("mail-mark-unread"), i18n("Mark Thread as &Unread"), this);
  actionCollection()->addAction("thread_unread", mMarkThreadAsUnreadAction );
  connect(mMarkThreadAsUnreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusUnread()));
  mMarkThreadAsUnreadAction->setToolTip(i18n("Mark all messages in the selected thread as unread"));
  mThreadStatusMenu->addAction( mMarkThreadAsUnreadAction );

  mThreadStatusMenu->addSeparator();

  //----- "Mark Thread" toggle actions
  mToggleThreadImportantAction = new KToggleAction(KIcon("mail-mark-important"), i18n("Mark Thread as &Important"), this);
  actionCollection()->addAction("thread_flag", mToggleThreadImportantAction );
  connect(mToggleThreadImportantAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusImportant()));
  mToggleThreadImportantAction->setCheckedState( KGuiItem(i18n("Remove &Important Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadImportantAction );

  mToggleThreadToActAction = new KToggleAction(KIcon("mail-mark-task"), i18n("Mark Thread as &Action Item"), this);
  actionCollection()->addAction("thread_toact", mToggleThreadToActAction );
  connect(mToggleThreadToActAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusToAct()));
  mToggleThreadToActAction->setCheckedState( KGuiItem(i18n("Remove &Action Item Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadToActAction );

  //------- "Watch and ignore thread" actions
  mWatchThreadAction = new KToggleAction(KIcon("mail-thread-watch"), i18n("&Watch Thread"), this);
  actionCollection()->addAction("thread_watched", mWatchThreadAction );
  connect(mWatchThreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusWatched()));

  mIgnoreThreadAction = new KToggleAction(KIcon("mail-thread-ignored"), i18n("&Ignore Thread"), this);
  actionCollection()->addAction("thread_ignored", mIgnoreThreadAction );
  connect(mIgnoreThreadAction, SIGNAL(triggered(bool) ), SLOT(slotSetThreadStatusIgnored()));

  mThreadStatusMenu->addSeparator();
  mThreadStatusMenu->addAction( mWatchThreadAction );
  mThreadStatusMenu->addAction( mIgnoreThreadAction );

  mSaveAttachmentsAction = new KAction(KIcon("mail-attachment"), i18n("Save A&ttachments..."), this);
  actionCollection()->addAction("file_save_attachments", mSaveAttachmentsAction );
  connect(mSaveAttachmentsAction, SIGNAL(triggered(bool) ), SLOT(slotSaveAttachments()));

  mMoveActionMenu =
    new KActionMenu( KIcon( "go-jump" ), i18n( "&Move To" ), this );
  actionCollection()->addAction( "move_to", mMoveActionMenu );

  mCopyActionMenu =
    new KActionMenu( KIcon( "edit-copy" ), i18n( "&Copy To" ), this );
  actionCollection()->addAction( "copy_to", mCopyActionMenu );

  mApplyAllFiltersAction =
    new KAction( KIcon( "view-filter" ), i18n( "Appl&y All Filters" ), this );
  actionCollection()->addAction( "apply_filters", mApplyAllFiltersAction );
  connect( mApplyAllFiltersAction, SIGNAL(triggered(bool)),
           SLOT(slotApplyFilters()) );
  mApplyAllFiltersAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_J ) );

  mApplyFilterActionsMenu = new KActionMenu( i18n( "A&pply Filter" ), this );
  actionCollection()->addAction( "apply_filter_actions", mApplyFilterActionsMenu );

  {
    KAction *action = new KAction(i18nc("View->","&Expand Thread"), this);
    actionCollection()->addAction("expand_thread", action );
    action->setShortcut(QKeySequence(Qt::Key_Period));
    action->setToolTip(i18n("Expand the current thread"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpandThread()));
  }
  {
    KAction *action = new KAction(i18nc("View->","&Collapse Thread"), this);
    actionCollection()->addAction("collapse_thread", action );
    action->setShortcut(QKeySequence(Qt::Key_Comma));
    action->setToolTip( i18n("Collapse the current thread"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCollapseThread()));
  }
  {
    KAction *action = new KAction(i18nc("View->","Ex&pand All Threads"), this);
    actionCollection()->addAction("expand_all_threads", action );
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Period));
    action->setToolTip( i18n("Expand all threads in the current folder"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotExpandAllThreads()));
  }
  {
    KAction *action = new KAction(i18nc("View->","C&ollapse All Threads"), this);
    actionCollection()->addAction("collapse_all_threads", action );
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Comma));
    action->setToolTip( i18n("Collapse all threads in the current folder"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotCollapseAllThreads()));
  }

  mViewSourceAction = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  KAction *dukeOfMonmoth = new KAction(i18n("&Display Message"), this);
  actionCollection()->addAction("display_message", dukeOfMonmoth );
  connect(dukeOfMonmoth, SIGNAL(triggered(bool) ), SLOT( slotDisplayCurrentMessage() ));
  dukeOfMonmoth->setShortcut(QKeySequence(Qt::Key_Return));

  //----- Go Menu
  {
    KAction *action = new KAction(i18n("&Next Message"), this);
    actionCollection()->addAction("go_next_message", action );
    action->setShortcuts(KShortcut( "N; Right" ));
    action->setToolTip(i18n("Go to the next message"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotSelectNextMessage()));
  }
  {
    KAction *action = new KAction(i18n("Next &Unread Message"), this);
    actionCollection()->addAction("go_next_unread_message", action );
    action->setShortcut(QKeySequence(Qt::Key_Plus));
    if ( QApplication::isRightToLeft() ) {
      action->setIcon( KIcon( "go-previous" ) );
    } else {
      action->setIcon( KIcon( "go-next" ) );
    }
    action->setIconText( i18nc( "@action:inmenu Goto next unread message", "Next" ) );
    action->setToolTip(i18n("Go to the next unread message"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotSelectNextUnreadMessage()));
  }
  {
    KAction *action = new KAction(i18n("&Previous Message"), this);
    actionCollection()->addAction("go_prev_message", action );
    action->setToolTip(i18n("Go to the previous message"));
    action->setShortcuts(KShortcut( "P; Left" ));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotSelectPreviousMessage()));
  }
  {
    KAction *action = new KAction(i18n("Previous Unread &Message"), this);
    actionCollection()->addAction("go_prev_unread_message", action );
    action->setShortcut(QKeySequence(Qt::Key_Minus));
    if ( QApplication::isRightToLeft() ) {
      action->setIcon( KIcon( "go-next" ) );
    } else {
      action->setIcon( KIcon( "go-previous" ) );
    }
    action->setIconText( i18nc( "@action:inmenu Goto previous unread message.","Previous" ) );
    action->setToolTip(i18n("Go to the previous unread message"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotSelectPreviousUnreadMessage()));
  }
  {
    KAction *action = new KAction(i18n("Next Unread &Folder"), this);
    actionCollection()->addAction("go_next_unread_folder", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotNextUnreadFolder()));
    action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Plus));
    action->setToolTip(i18n("Go to the next folder with unread messages"));
    KShortcut shortcut = KShortcut(action->shortcuts());
    shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Plus ) );
    action->setShortcuts( shortcut );
  }
  {
    KAction *action = new KAction(i18n("Previous Unread F&older"), this);
    actionCollection()->addAction("go_prev_unread_folder", action );
    action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Minus));
    action->setToolTip(i18n("Go to the previous folder with unread messages"));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotPrevUnreadFolder()));
    KShortcut shortcut = KShortcut(action->shortcuts());
    shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Minus ) );
    action->setShortcuts( shortcut );
  }
  {
    KAction *action = new KAction(i18nc("Go->","Next Unread &Text"), this);
    actionCollection()->addAction("go_next_unread_text", action );
    action->setShortcut(QKeySequence(Qt::Key_Space));
    action->setToolTip(i18n("Go to the next unread text"));
    action->setWhatsThis( i18n("Scroll down current message. "
                               "If at end of current message, "
                               "go to next unread message."));
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotReadOn()));
  }

  //----- Settings Menu
  {
    KAction *action = new KAction(i18n("Configure &Filters..."), this);
    action->setMenuRole( QAction::NoRole ); // do not move to application menu on OS X
    actionCollection()->addAction("filter", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotFilter()));
  }
  {
    KAction *action = new KAction(i18n("Configure &POP Filters..."), this);
    actionCollection()->addAction("popFilter", action );
    action->setMenuRole( QAction::NoRole ); // do not move to application menu on OS X
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotPopFilter()));
  }
  {
    KAction *action = new KAction(i18n("Manage &Sieve Scripts..."), this);
    actionCollection()->addAction("sieveFilters", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotManageSieveScripts()));
  }
  {
    KAction *action = new KAction(KIcon("kmail"), i18n("KMail &Introduction"), this);
    actionCollection()->addAction("help_kmail_welcomepage", action );
    action->setToolTip( i18n("Display KMail's Welcome Page") );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotIntro()));
  }

  // ----- Standard Actions

//  KStandardAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
  {
    KAction *action = new KAction( KIcon("preferences-desktop-notification"),
                                   i18n("Configure &Notifications..."), this );
    action->setMenuRole( QAction::NoRole ); // do not move to application menu on OS X
    actionCollection()->addAction( "kmail_configure_notifications", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT(slotEditNotifications()));
  }

//  KStandardAction::preferences(this, SLOT(slotSettings()), actionCollection());
  {
    KAction *action = new KAction(KIcon("configure"), i18n("&Configure KMail..."), this);
    actionCollection()->addAction("kmail_configure_kmail", action );
    connect(action, SIGNAL(triggered(bool) ), kmkernel, SLOT(slotShowConfigurationDialog()));
  }

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
  KComponentData d = kmkernel->xmlGuiInstance();
  if ( d.isValid() ) {
    const KAboutData *a = d.aboutData();
    KNotifyConfigWidget::configure( this, a->appName() );
  } else {
    KNotifyConfigWidget::configure( this );
  }
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
    slotSelectNextUnreadMessage();
}

void KMMainWidget::slotNextUnreadFolder()
{
  if ( !mMainFolderView )
    return;
  mGoToFirstUnreadMessageInSelectedFolder = true;
  mMainFolderView->selectNextUnreadFolder();
  mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotPrevUnreadFolder()
{
  if ( !mMainFolderView )
    return;
  mGoToFirstUnreadMessageInSelectedFolder = true;
  mMainFolderView->selectPrevUnreadFolder();
  mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotExpandThread()
{
  mMessageListView->setCurrentThreadExpanded( true );
}

void KMMainWidget::slotCollapseThread()
{
  mMessageListView->setCurrentThreadExpanded( false );
}

void KMMainWidget::slotExpandAllThreads()
{
  // TODO: Make this asynchronous ? (if there is enough demand)
  KCursorSaver busy( KBusyPtr::busy() );
  mMessageListView->setAllThreadsExpanded( true );
}

void KMMainWidget::slotCollapseAllThreads()
{
  // TODO: Make this asynchronous ? (if there is enough demand)
  KCursorSaver busy( KBusyPtr::busy() );
  mMessageListView->setAllThreadsExpanded( false );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowMsgSrc()
{
  if ( mMsgView )
    mMsgView->setUpdateAttachment( false );

  KMMessage *msg = mMessageListView->currentMessage();
  if ( !msg )
    return;
  KMCommand *command = new KMShowMsgSrcCommand( this, msg,
                                                mMsgView
                                                ? mMsgView->isFixedFont()
                                                : false );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::updateMessageMenu()
{
  mMainFolderView->folderToPopupMenu( KMail::MainFolderView::MoveMessage, this, mMoveActionMenu->menu() );
  mMainFolderView->folderToPopupMenu( KMail::MainFolderView::CopyMessage, this, mCopyActionMenu->menu() );
  updateMessageActions();
}

void KMMainWidget::startUpdateMessageActionsTimer()
{
  // FIXME: This delay effectively CAN make the actions to be in an incoherent state
  //        Maybe we should mark actions as "dirty" here and check it in every action handler...
  menutimer->stop();
  menutimer->start( 20 );
}

void KMMainWidget::updateMessageActions()
{
  int count;

  updateCutCopyPasteActions();

  QList< Q_UINT32 > selectedSernums;
  QList< Q_UINT32 > selectedVisibleSernums;
  bool allSelectedBelongToSameThread = false;

  KMMessage * currentMessage;

  if (
       mFolder &&
       mMessageListView->getSelectionStats( selectedSernums, selectedVisibleSernums, &allSelectedBelongToSameThread )
     )
  {
    count = selectedSernums.count();

    currentMessage = mMessageListView->currentMessage();

    mMsgActions->setCurrentMessage( currentMessage );
    mMsgActions->setSelectedSernums( selectedSernums );
    mMsgActions->setSelectedVisibleSernums( selectedVisibleSernums );

  } else {
    count = 0;
    currentMessage = 0;
    mMsgActions->setCurrentMessage( 0 );
  }

  updateListFilterAction();

  bool mass_actions = count >= 1;
  bool thread_actions = mass_actions && allSelectedBelongToSameThread && mMessageListView->isThreaded();
  bool flags_available = GlobalSettings::self()->allowLocalFlags() || !(mFolder ? mFolder->isReadOnly() : true);

  mThreadStatusMenu->setEnabled( thread_actions );
  // these need to be handled individually, the user might have them
  // in the toolbar
  mWatchThreadAction->setEnabled( thread_actions && flags_available );
  mIgnoreThreadAction->setEnabled( thread_actions && flags_available );
  mMarkThreadAsNewAction->setEnabled( thread_actions );
  mMarkThreadAsReadAction->setEnabled( thread_actions );
  mMarkThreadAsUnreadAction->setEnabled( thread_actions );
  mToggleThreadToActAction->setEnabled( thread_actions && flags_available );
  mToggleThreadImportantAction->setEnabled( thread_actions && flags_available );
  mTrashThreadAction->setEnabled( thread_actions && !mFolder->isReadOnly() );
  mDeleteThreadAction->setEnabled( thread_actions && mFolder->canDeleteMessages() );

  if ( currentMessage )
  {
    MessageStatus status = currentMessage->status();
    updateMessageTagActions( count );
    if (thread_actions)
    {
      mToggleThreadToActAction->setChecked( status.isToAct() );
      mToggleThreadImportantAction->setChecked( status.isImportant() );
      mWatchThreadAction->setChecked( status.isWatched() );
      mIgnoreThreadAction->setChecked( status.isIgnored() );
    }
  }

  mMoveActionMenu->setEnabled( mass_actions && mFolder->canDeleteMessages() );
  mMoveMsgToFolderAction->setEnabled( mass_actions && mFolder->canDeleteMessages() );
  mCopyActionMenu->setEnabled( mass_actions );
  mTrashAction->setEnabled( mass_actions && !mFolder->isReadOnly() );
  mDeleteAction->setEnabled( mass_actions && !mFolder->isReadOnly() );
  mFindInMessageAction->setEnabled( mass_actions );
  mForwardAction->setEnabled( mass_actions );
  mForwardActionMenu->setEnabled( mass_actions );
  mForwardAttachedAction->setEnabled( mass_actions );

  forwardMenu()->setEnabled( mass_actions );

  bool single_actions = count == 1;
  mMsgActions->editAction()->setEnabled( single_actions );
  mUseAction->setEnabled( single_actions && kmkernel->folderIsTemplates( mFolder ) );
  filterMenu()->setEnabled( single_actions );
  redirectAction()->setEnabled( single_actions );

  if ( mCustomTemplateMenus )
  {
    mCustomTemplateMenus->forwardActionMenu()->setEnabled( mass_actions );
    mCustomTemplateMenus->replyActionMenu()->setEnabled( single_actions );
    mCustomTemplateMenus->replyAllActionMenu()->setEnabled( single_actions );
  }

  printAction()->setEnabled( single_actions );
  viewSourceAction()->setEnabled( single_actions );

  mSendAgainAction->setEnabled(
      single_actions &&
      ( currentMessage && currentMessage->status().isSent() ) ||
      ( currentMessage && kmkernel->folderIsSentMailFolder( mFolder ) )
    );

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
    action( "edit_undo" )->setEnabled( kmkernel->undoStack()->size() > 0 );

  if ( ( count == 1 ) && currentMessage )
  {
    if ((KMFolder*)mFolder == kmkernel->outboxFolder())
      editAction()->setEnabled( !currentMessage->transferInProgress() );
  }

  // Enable / disable all filters.
  foreach ( QAction *filterAction, mFilterMenuActions ) {
    filterAction->setEnabled( count > 0 );
  }

  mApplyAllFiltersAction->setEnabled( count);
  mApplyFilterActionsMenu->setEnabled( count );
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
  bool multiFolder = mMainFolderView->selectedFolders().count() > 1;
  bool mailingList = mFolder && mFolder->isMailingListEnabled() && !multiFolder;

  mPostToMailinglistAction->setEnabled( mailingList );
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
  mTroubleshootMaildirAction->setVisible( mFolder && mFolder->folderType() == KMFolderTypeMaildir );
  mEmptyFolderAction->setEnabled( folderWithContent && ( mFolder->count() > 0 ) && mFolder->canDeleteMessages() && !multiFolder );
  mEmptyFolderAction->setText( (mFolder && kmkernel->folderIsTrash(mFolder))
    ? i18n("E&mpty Trash") : i18n("&Move All Messages to Trash") );
  mRemoveFolderAction->setEnabled( mFolder && !mFolder->isSystemFolder() && mFolder->canDeleteMessages() && !multiFolder);
  mRemoveFolderAction->setText( mFolder && mFolder->folderType() == KMFolderTypeSearch ? i18n("&Delete Search") : i18n("&Delete Folder") );
  mExpireFolderAction->setEnabled( mFolder && mFolder->isAutoExpire() && !multiFolder && mFolder->canDeleteMessages() );
  updateMarkAsReadAction();
  // the visual ones only make sense if we are showing a message list
  mPreferHtmlAction->setEnabled( mMessageListView->currentFolder() ? true : false );
  mPreferHtmlLoadExtAction->setEnabled( mMessageListView->currentFolder() && (mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref) ? true : false );

  mPreferHtmlAction->setChecked( mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref );
  mPreferHtmlLoadExtAction->setChecked( mHtmlLoadExtPref ? !mFolderHtmlLoadExtPref : mFolderHtmlLoadExtPref );

  mNewFolderAction->setEnabled( !multiFolder && ( mFolder && mFolder->folderType() != KMFolderTypeSearch) );
  mRemoveDuplicatesAction->setEnabled( !multiFolder && mFolder && mFolder->canDeleteMessages() );
  mFolderShortCutCommandAction->setEnabled( !multiFolder );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotIntro()
{
  if ( !mMsgView )
    return;

  mMsgView->clear( true );

  // hide widgets that are in the way:
  if ( mMessageListView && mLongFolderList )
    mMessageListView->hide();

  mMsgView->displayAboutPage();

  closeFolder();
  mFolder = 0;
}

void KMMainWidget::slotShowStartupFolder()
{
  if ( mMainFolderView )
  {
    mMainFolderView->readConfig();
    mMainFolderView->reload();
    // get rid of old-folders
    mMainFolderView->cleanupConfigFile();
  }

  connect( kmkernel->filterMgr(), SIGNAL( filterListUpdated() ),
           this, SLOT( initializeFilterActions() ) );

  connect( kmkernel->msgTagMgr(), SIGNAL( msgTagListChanged() ),
           this, SLOT( initializeMessageTagActions() ) );

  // plug shortcut filter actions now
  initializeFilterActions();

  // plug folder shortcut actions
  initializeFolderShortcutActions();

  // plug tag actions now
  initializeMessageTagActions();

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

  if ( mMainFolderView )
    mMainFolderView->setCurrentFolder( startup );
}

void KMMainWidget::slotShowTip()
{
  KTipDialog::showTip( this, QString(), true );
}

void KMMainWidget::updateMessageTagActions( const int count )
{
  //TODO: Behaves differently according to number of messages selected

  KToggleAction *aToggler = 0;
  if ( 1 == count )
  {
    KMMessage * currentMessage = mMessageListView->currentMessage();
    Q_ASSERT( currentMessage );
    KMMessageTagList *aTagList = currentMessage->tagList();
    for ( QList<MessageTagPtrPair>::ConstIterator it =
          mMessageTagMenuActions.constBegin();
          it != mMessageTagMenuActions.constEnd(); ++it )
    {
      bool list_present = false;
      if ( aTagList )
        list_present =
           ( aTagList->indexOf( QString((*it).first->label() ) ) != -1 );
      aToggler = static_cast<KToggleAction*>( (*it).second );
      aToggler->setChecked( list_present );
    }
  } else if ( count > 1 )
  {
    for ( QList<MessageTagPtrPair>::ConstIterator it =
          mMessageTagMenuActions.constBegin();
          it != mMessageTagMenuActions.constEnd(); ++it )
    {
      aToggler = static_cast<KToggleAction*>( (*it).second );
      aToggler->setChecked( false );
      aToggler->setText( i18n("Toggle Message Tag %1", (*it).first->name() ) );
    }
  } else {
    for ( QList<MessageTagPtrPair>::ConstIterator it =
          mMessageTagMenuActions.constBegin();
          it != mMessageTagMenuActions.constEnd(); ++it )
    {
      aToggler = static_cast<KToggleAction*>( (*it).second );
      aToggler->setEnabled( false );
    }
  }
}

QList<KActionCollection*> KMMainWidget::actionCollections() const {
  return QList<KActionCollection*>() << actionCollection();
}


void KMMainWidget::clearMessageTagActions()
{
  //Remove the tag actions from the toolbar
  if ( !mMessageTagTBarActions.isEmpty() ) {
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "toolbar_messagetag_actions" );
    mMessageTagTBarActions.clear();
  }

  //Remove the tag actions from the status menu and the action collection,
  //then delete them.
  for ( QList<MessageTagPtrPair>::ConstIterator it =
        mMessageTagMenuActions.constBegin();
        it != mMessageTagMenuActions.constEnd(); ++it ) {
    mMsgActions->messageStatusMenu()->removeAction( (*it).second );

    // This removes and deletes the action at the same time
    actionCollection()->removeAction( (*it).second );
  }

  mMessageTagMenuActions.clear();
  delete mMessageTagToggleMapper;
  mMessageTagToggleMapper = 0;
}

void KMMainWidget::initializeMessageTagActions()
{
  clearMessageTagActions();
  const QHash<QString,KMMessageTagDescription*> *tagDict = kmkernel->msgTagMgr()->msgTagDict();
  if ( !tagDict )
    return;
  //Use a mapper to understand which tag button is triggered
  mMessageTagToggleMapper = new QSignalMapper( this );
  connect( mMessageTagToggleMapper, SIGNAL( mapped( const QString& ) ),
    this, SLOT( slotUpdateMessageTagList( const QString& ) ) );

  //TODO: No need to do this anymore, just use the ordered list
  const int numTags = tagDict->count();
  if ( !numTags ) return;
  for ( int i = 0; i < numTags; ++i ) {
    mMessageTagMenuActions.append( MessageTagPtrPair( 0, 0 ) );
  }
  KAction *tagAction = 0;

  QHashIterator<QString,KMMessageTagDescription*> it( *tagDict );
  while( it.hasNext() ) {
    it.next();
    if ( ! it.value() || it.value()->isEmpty() )
      continue;
    QString cleanName = i18n("Message Tag %1", it.value()->name() );
    QString iconText = it.value()->name();
    cleanName.replace('&',"&&");
    tagAction = new KToggleAction( KIcon(it.value()->toolbarIconName()),
      cleanName, this );
    tagAction->setShortcut( it.value()->shortcut() );
    tagAction->setIconText( iconText );
    actionCollection()->addAction(it.value()->label().toLocal8Bit(), tagAction);
    connect(tagAction, SIGNAL(triggered(bool)), mMessageTagToggleMapper, SLOT(map()));
    // The shortcut configuration is done in the config dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the tag descriptions correctly.
    tagAction->setShortcutConfigurable( false );
    mMessageTagToggleMapper->setMapping( tagAction, it.value()->label() );
    MessageTagPtrPair ptr_pair( it.value(), tagAction );
    //Relies on the fact that filters are always numbered from 0
    mMessageTagMenuActions[it.value()->priority()] = ptr_pair;
  }
  for ( int i=0; i < numTags; ++i ) {
    mMsgActions->messageStatusMenu()->menu()->addAction( mMessageTagMenuActions[i].second );
    if ( ( mMessageTagMenuActions[i].first )->inToolbar() )
      mMessageTagTBarActions.append( mMessageTagMenuActions[i].second );
  }

  if ( !mMessageTagTBarActions.isEmpty() && mGUIClient->factory() ) {
    //Separator doesn't work
    //mMessageTagTBarActions.prepend( mMessageTagToolbarActionSeparator );
    mGUIClient->plugActionList( "toolbar_messagetag_actions",
                                mMessageTagTBarActions );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::removeDuplicates()
{
  if ( !mFolder ) {
    return;
  }
  KMFolder *oFolder = mFolder;
  mMessageListView->setCurrentFolder( 0 );
  QMap< QString, QList<int> > idMD5s;
  QList<int> redundantIds;
  QList<int>::Iterator kt;
  oFolder->open( "removedups" );
  for ( int i = oFolder->count() - 1; i >= 0; --i ) {
    QString id = (*oFolder)[i]->msgIdMD5();
    if ( !id.isEmpty() ) {
      QString subjMD5 = (*oFolder)[i]->strippedSubjectMD5();
      int other = -1;
      if ( idMD5s.contains(id) ) {
        other = idMD5s[id].first();
      } else {
        idMD5s[id].append( i );
      }
      if ( other != -1 ) {
        QString otherSubjMD5 = (*oFolder)[other]->strippedSubjectMD5();
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
      if (!((*oFolder)[*jt]->status().isUnread())) {
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
    oFolder->removeMsg( *(--kt) );
    ++numDuplicates;
  }
  while (kt != redundantIds.begin());

  oFolder->close( "removedups" );
  mMessageListView->setCurrentFolder( oFolder );
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
  if ( actionCollection()->action( "edit_undo" ) ) {
    actionCollection()->action( "edit_undo" )->setEnabled( kmkernel->undoStack()->size() > 0 );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::clearFilterActions()
{
  if ( !mFilterTBarActions.isEmpty() )
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "toolbar_filter_actions" );

  if ( !mFilterMenuActions.isEmpty() )
    if ( mGUIClient->factory() )
      mGUIClient->unplugActionList( "menu_filter_actions" );

  foreach ( QAction *a, mFilterMenuActions )
    actionCollection()->removeAction( a );

  mApplyFilterActionsMenu->menu()->clear();
  mFilterTBarActions.clear();
  mFilterMenuActions.clear();

  qDeleteAll( mFilterCommands );
  mFilterCommands.clear();
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeFolderShortcutActions()
{
  QList< QPointer< KMFolder > > folders = kmkernel->allFolders();
  QList< QPointer< KMFolder > >::Iterator it = folders.begin();
  while ( it != folders.end() ) {
    KMFolder *folder = (*it);
    ++it;
    slotShortcutChanged( folder ); // load the initial accel
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeFilterActions()
{
  clearFilterActions();
  mApplyFilterActionsMenu->menu()->addAction( mApplyAllFiltersAction );
  bool addedSeparator = false;

  QList<KMFilter*>::const_iterator it = kmkernel->filterMgr()->filters().begin();
  for ( ;it != kmkernel->filterMgr()->filters().end(); ++it ) {
    if ( !(*it)->isEmpty() && (*it)->configureShortcut() ) {
      QString filterName = QString( "Filter %1").arg( (*it)->name() );
      QString normalizedName = filterName.replace(' ', '_');
      if ( action( normalizedName.toUtf8() ) ) {
        continue;
      }
      KMMetaFilterActionCommand *filterCommand = new KMMetaFilterActionCommand( *it, this );
      mFilterCommands.append( filterCommand );
      QString displayText = i18n( "Filter %1", (*it)->name() );
      QString icon = (*it)->icon();
      if ( icon.isEmpty() ) {
        icon = "system-run";
      }
      KAction *filterAction = new KAction( KIcon( icon ), displayText, actionCollection() );
      filterAction->setIconText( (*it)->toolbarName() );

      // The shortcut configuration is done in the filter dialog.
      // The shortcut set in the shortcut dialog would not be saved back to
      // the filter settings correctly.
      filterAction->setShortcutConfigurable( false );

      actionCollection()->addAction( normalizedName.toLocal8Bit(),
                                     filterAction );
      connect( filterAction, SIGNAL(triggered(bool) ),
               filterCommand, SLOT(start()) );
      filterAction->setShortcuts( (*it)->shortcut() );
      if ( !addedSeparator ) {
        QAction *a = mApplyFilterActionsMenu->menu()->addSeparator();
        mFilterMenuActions.append( a );
        addedSeparator = true;
      }
      mApplyFilterActionsMenu->menu()->addAction( filterAction );
      mFilterMenuActions.append( filterAction );
      if ( (*it)->configureToolbar() ) {
        mFilterTBarActions.append( filterAction );
      }
    }
  }
  if ( !mFilterMenuActions.isEmpty() && mGUIClient->factory() )
    mGUIClient->plugActionList( "menu_filter_actions", mFilterMenuActions );
  if ( !mFilterTBarActions.isEmpty() && mGUIClient->factory() ) {
    mFilterTBarActions.prepend( mToolbarActionSeparator );
    mGUIClient->plugActionList( "toolbar_filter_actions", mFilterTBarActions );
  }

  // Our filters have changed, now enable/disable them
  updateMessageActions();
}

void KMMainWidget::slotFolderRemoved( KMFolder *folder )
{
  delete mFolderShortcutCommands.take( folder->idString() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeIMAPActions( bool setState /* false the first time, true later on */ )
{
  bool hasImapAccount = false;
  QList<KMAccount*>::iterator accountIt = kmkernel->acctMgr()->begin();
  while ( accountIt != kmkernel->acctMgr()->end() ) {
    KMAccount *a = *accountIt;
    ++accountIt;
    if ( a->type() == KAccount::DImap ) {
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
    mTroubleshootFolderAction = new KAction(KIcon("tools-wizard"), i18n("&Troubleshoot IMAP Cache..."), this);
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

QList<QAction*> KMMainWidget::actionList()
{
  return actionCollection()->actions();
}

void KMMainWidget::slotShortcutChanged( KMFolder *folder )
{
  // remove the old one, no autodelete in Qt4
  slotFolderRemoved( folder );

  if ( folder->shortcut().isEmpty() )
    return;

  FolderShortcutCommand *c = new FolderShortcutCommand( this, folder );
  mFolderShortcutCommands.insert( folder->idString(), c );

  QString actionlabel = i18n( "Folder Shortcut %1", folder->prettyUrl() );
  QString actionname = i18n( "Folder Shortcut %1", folder->idString() );
  QString normalizedName = actionname.replace(' ', '_');
  KAction *action = actionCollection()->addAction( normalizedName );
  // The folder shortcut is set in the folder shortcut dialog.
  // The shortcut set in the shortcut dialog would not be saved back to
  // the folder settings correctly.
  action->setShortcutConfigurable( false );

  mMainFolderView->addAction( action ); // <-- FIXME: why this is added to the folder view ?
  action->setText( actionlabel );
  connect( action, SIGNAL( triggered(bool) ), c, SLOT( start() ) );
  action->setShortcuts( folder->shortcut() );
  action->setIcon( folder->useCustomIcons() ?
                   KIcon( folder->unreadIconPath() ) :
                   KIcon( "folder" ) );
  c->setAction( action ); // will be deleted along with the command
}

//-----------------------------------------------------------------------------
QString KMMainWidget::findCurrentImapPath()
{
  QString startPath;
  if ( !mFolder ) {
    return startPath;
  }
  if ( mFolder->folderType() == KMFolderTypeImap ) {
    startPath = static_cast<KMFolderImap*>( mFolder->storage() )->imapPath();
  } else if ( mFolder->folderType() == KMFolderTypeCachedImap ) {
    startPath = static_cast<KMFolderCachedImap*>( mFolder->storage() )->imapPath();
  }
  return startPath;
}

//-----------------------------------------------------------------------------
ImapAccountBase *KMMainWidget::findCurrentImapAccountBase()
{
  ImapAccountBase *account = 0;
  if ( !mFolder ) {
    return account;
  }
  if ( mFolder->folderType() == KMFolderTypeImap ) {
    account = static_cast<KMFolderImap*>( mFolder->storage() )->account();
  } else if ( mFolder->folderType() == KMFolderTypeCachedImap ) {
    account = static_cast<KMFolderCachedImap*>( mFolder->storage() )->account();
  }
  return account;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubscriptionDialog()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  ImapAccountBase *account = findCurrentImapAccountBase();
  if ( !account ) {
    return;
  }

  const QString startPath = findCurrentImapPath();

  // KSubscription sets "DestructiveClose"
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
  ImapAccountBase *account = findCurrentImapAccountBase();
  if ( !account ) {
    return;
  }

  const QString startPath = findCurrentImapPath();
  // KSubscription sets "DestructiveClose"
  LocalSubscriptionDialog *dialog =
      new LocalSubscriptionDialog(this, i18n("Local Subscription"), account, startPath);
  if ( dialog->exec() ) {
    // start a new listing
    if (mFolder->folderType() == KMFolderTypeImap)
      static_cast<KMFolderImap*>(mFolder->storage())->account()->listDirectory();
  }
}

void KMMainWidget::toggleSystemTray()
{
  if ( !mSystemTray && GlobalSettings::self()->systemTrayEnabled() ) {
    mSystemTray = new KMSystemTray();
  }
  else if ( mSystemTray && !GlobalSettings::self()->systemTrayEnabled() ) {
    // Get rid of system tray on user's request
    kDebug(5006) <<"deleting systray";
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
  AntiSpamWizard wiz( AntiSpamWizard::AntiSpam, this, mainFolderView() );
  wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiVirusWizard()
{
  AntiSpamWizard wiz( AntiSpamWizard::AntiVirus, this, mainFolderView() );
  wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAccountWizard()
{
  AccountWizard::start( kmkernel, this );
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
  actionCollection()->action("favorite_check_mail")->setEnabled( actList.size() > 0 );
}

//-----------------------------------------------------------------------------
const KMMainWidget::PtrList * KMMainWidget::mainWidgetList()
{
  // better safe than sorry; check whether the global static has already been destroyed
  if ( theMainWidgetList.isDestroyed() )
  {
    return 0;
  }
  return theMainWidgetList;
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

void KMMainWidget::slotCreateTodo()
{
  KMMessage *msg = mMessageListView->currentMessage();
  if ( !msg )
    return;
  KMCommand *command = new CreateTodoCommand( this, msg );
  command->start();
}

void KMMainWidget::showEvent( QShowEvent *event )
{
  QWidget::showEvent( event );
  mWasEverShown = true;
}

void KMMainWidget::slotRequestFullSearchFromQuickSearch()
{
  slotSearch();
  assert( mSearchWin );
  KMSearchPattern pattern;
  pattern.append( KMSearchRule::createInstance( "<message>", KMSearchRule::FuncContains, mMessageListView->currentFilterSearchString() ) );
  MessageStatus status = mMessageListView->currentFilterStatus();
  if ( status.hasAttachment() )
  {
    pattern.append( KMSearchRule::createInstance( "<message>", KMSearchRule::FuncHasAttachment ) );
    status.setHasAttachment( false );
  }

  if ( !status.isOfUnknownStatus() ) {
    pattern.append( new KMSearchRuleStatus( status ) );
  }
  mSearchWin->setSearchPattern( pattern );
}

void KMMainWidget::updateVactionScriptStatus( bool active )
{
  mVacationIndicatorActive = active;
  if ( active ) {
    mVacationScriptIndicator->setText( i18n("Out of office reply active") );
    mVacationScriptIndicator->setBackgroundColor( Qt::yellow );
    mVacationScriptIndicator->setCursor( QCursor( Qt::PointingHandCursor ) );
    mVacationScriptIndicator->show();
  } else {
    mVacationScriptIndicator->hide();
  }
}

QLabel * KMMainWidget::vacationScriptIndicator() const
{
  return mVacationScriptIndicator;
}
