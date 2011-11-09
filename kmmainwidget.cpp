/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2002 Don Sanders <sanders@kde.org>
  Copyright (c) 2009, 2010, 2011 Montel Laurent <montel@kde.org>

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

// KMail includes
#include "kmreadermainwin.h"
#include "foldershortcutdialog.h"
#include "composer.h"
#include "searchwindow.h"
#include "antispamwizard.h"
#include "mailinglistpropertiesdialog.h"
#include "statusbarlabel.h"
#include "expirypropertiesdialog.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "customtemplatesmenu.h"
#include "folderselectiondialog.h"
#include "foldertreewidget.h"
#include "util.h"
#include "mailutil.h"
#include "mailkernel.h"
#include "archivefolderdialog.h"
#include "globalsettings.h"
#include "foldertreeview.h"
#include "tagactionmanager.h"
#include "foldershortcutactionmanager.h"
#include "collectionpane.h"
#if !defined(NDEBUG)
    #include <ksieveui/sievedebugdialog.h>
    using KSieveUi::SieveDebugDialog;
#endif

#include "collectionaclpage.h"
#include "collectionmaintenancepage.h"
#include "collectionquotapage.h"
#include "collectiontemplatespage.h"
#include "collectionviewpage.h"
#include "tagselectdialog.h"


#include "mailcommon/collectiongeneralpage.h"
#include "mailcommon/filtermanager.h"
#include "mailcommon/mailfilter.h"

// Other PIM includes
#include "kdepim-version.h"

#include "messageviewer/autoqpointer.h"
#include "messageviewer/globalsettings.h"
#include "messageviewer/viewer.h"
#include "messageviewer/attachmentstrategy.h"
#include "messageviewer/headerstrategy.h"
#include "messageviewer/headerstyle.h"
#ifndef QT_NO_CURSOR
#include "messageviewer/kcursorsaver.h"
#endif

#include "messagecomposer/messagesender.h"
#include "messagecomposer/messagehelper.h"

#include "templateparser/templateparser.h"

#include "messagecore/globalsettings.h"
#include "messagecore/mailinglist.h"
#include "messagecore/messagehelpers.h"

#include "kmknotify.h"

// LIBKDEPIM includes
#include "progressmanager.h"
#include "broadcaststatus.h"

// KDEPIMLIBS includes
#include <Akonadi/AgentManager>
#include <Akonadi/AttributeFactory>
#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionattributessynchronizationjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/contact/contactsearchjob.h>
#include <akonadi/collectionpropertiesdialog.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entitylistview.h>
#include <akonadi/etmviewstatesaver.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agenttype.h>
#include <akonadi/changerecorder.h>
#include <akonadi/session.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/favoritecollectionsmodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/control.h>
#include <akonadi/collectiondialog.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsdelegate.h>
#include <akonadi/favoritecollectionsmodel.h>
#include <akonadi/statisticsproxymodel.h>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <akonadi/kmime/messageflags.h>
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <kpimutils/email.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>
#include <kmime/kmime_mdn.h>
#include <kmime/kmime_header_parsing.h>
#include <kmime/kmime_message.h>
#include <ksieveui/managesievescriptsdialog.h>
#include <ksieveui/util.h>
#include <ksieveui/vacation.h>

// KDELIBS includes
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
#include <ktoggleaction.h>
#include <ktoolinvocation.h>
#include <knotification.h>
#include <knotifyconfigwidget.h>
#include <kstringhandler.h>
#include <kconfiggroup.h>
#include <ktoolinvocation.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <ktreewidgetsearchline.h>
#include <Solid/Networking>
#include <nepomuk/resourcemanager.h>

// Qt includes
#include <QByteArray>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QShortcut>
#include <QProcess>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QTextDocument>
#include <QMenu>

// System includes
#include <assert.h>
#include <errno.h> // ugh
#include <akonadi/standardactionmanager.h>

#include "kmmainwidget.moc"

using namespace KMime;
using namespace Akonadi;
using namespace MailCommon;
using KPIM::ProgressManager;
using KPIM::BroadcastStatus;
using KMail::SearchWindow;
using KMail::AntiSpamWizard;
using KMime::Types::AddrSpecList;
using KSieveUi::Vacation;
using MessageViewer::AttachmentStrategy;


K_GLOBAL_STATIC( KMMainWidget::PtrList, theMainWidgetList )

//-----------------------------------------------------------------------------
  KMMainWidget::KMMainWidget( QWidget *parent, KXMLGUIClient *aGUIClient,
                              KActionCollection *actionCollection, KSharedConfig::Ptr config ) :
    QWidget( parent ),
    mCollectionProperties( 0 ),
    mFavoriteCollectionsView( 0 ),
    mMsgView( 0 ),
    mSplitter1( 0 ),
    mSplitter2( 0 ),
    mFolderViewSplitter( 0 ),
    mArchiveFolderAction( 0 ),
    mShowBusySplashTimer( 0 ),
    mShowingOfflineScreen( false ),
    mMsgActions( 0 ),
    mCurrentFolder( 0 ),
    mVacationIndicatorActive( false ),
    mGoToFirstUnreadMessageInSelectedFolder( false ),
    mCheckMailInProgress( false ),
    mMoveOrCopyToDialog( 0 ),
    mSelectFromAllFoldersDialog( 0 )
{
  // must be the first line of the constructor:
  mStartupDone = false;
  mWasEverShown = false;
  mReaderWindowActive = true;
  mReaderWindowBelow = true;
  mFolderHtmlPref = false;
  mFolderHtmlLoadExtPref = false;
  mSystemTray = 0;
  mDestructed = false;
  mActionCollection = actionCollection;
  mTopLayout = new QVBoxLayout( this );
  mTopLayout->setMargin( 0 );
  mConfig = config;
  mGUIClient = aGUIClient;
  mCustomTemplateMenus = 0;
  mFolderTreeWidget = 0;

  Akonadi::Control::widgetNeedsAkonadi( this );


  // FIXME This should become a line separator as soon as the API
  // is extended in kdelibs.
  mToolbarActionSeparator = new QAction( this );
  mToolbarActionSeparator->setSeparator( true );

  theMainWidgetList->append( this );

  readPreConfig();
  createWidgets();

  setupActions();

  readConfig();

  if ( !kmkernel->isOffline() ) { //kmail is set to online mode, make sure the agents are also online
    kmkernel->setAccountStatus(true);
  }


  QTimer::singleShot( 0, this, SLOT(slotShowStartupFolder()));

  connect( kmkernel, SIGNAL(startCheckMail()),
           this, SLOT(slotStartCheckMail()) );

  connect( kmkernel, SIGNAL(endCheckMail()),
           this, SLOT(slotEndCheckMail()) );

  connect( kmkernel, SIGNAL(configChanged()),
           this, SLOT(slotConfigChanged()) );

  connect( kmkernel, SIGNAL(onlineStatusChanged(GlobalSettings::EnumNetworkState::type)),
           this, SLOT(slotUpdateOnlineStatus(GlobalSettings::EnumNetworkState::type)) );

  connect( mTagActionManager, SIGNAL(tagActionTriggered(QString)),
           this, SLOT(slotUpdateMessageTagList(QString)) );

  connect( mTagActionManager, SIGNAL(tagMoreActionClicked()),
           this, SLOT(slotSelectMoreMessageTagList()) );

  connect ( Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
           this, SLOT(slotNetworkStatusChanged(Solid::Networking::Status)) );

  toggleSystemTray();

  { // make sure the pages are registered only once, since there can be multiple instances of KMMainWidget
    static bool pagesRegistered = false;

    if ( !pagesRegistered ) {
      Akonadi::CollectionPropertiesDialog::registerPage( new CollectionAclPageFactory );
      Akonadi::CollectionPropertiesDialog::registerPage( new MailCommon::CollectionGeneralPageFactory );
      Akonadi::CollectionPropertiesDialog::registerPage( new CollectionMaintenancePageFactory );
      Akonadi::CollectionPropertiesDialog::registerPage( new CollectionQuotaPageFactory );
      Akonadi::CollectionPropertiesDialog::registerPage( new CollectionTemplatesPageFactory );
      Akonadi::CollectionPropertiesDialog::registerPage( new CollectionViewPageFactory );

      pagesRegistered = true;
    }
  }

  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(window());
  KStatusBar *sb =  mainWin ? mainWin->statusBar() : 0;
  mVacationScriptIndicator = new KMail::StatusBarLabel( sb );
  mVacationScriptIndicator->hide();
  connect( mVacationScriptIndicator, SIGNAL(clicked()), SLOT(slotEditVacation()) );
  if ( KSieveUi::Util::checkOutOfOfficeOnStartup() )
    QTimer::singleShot( 0, this, SLOT(slotCheckVacation()) );

  restoreCollectionFolderViewConfig();

  if ( kmkernel->firstStart() ) {
    KMail::Util::launchAccountWizard( this );
  }
  // must be the last line of the constructor:
  mStartupDone = true;

  
  m_notificationTimer.setInterval( 10 * 1000 );
  m_notificationTimer.setSingleShot( true );
  connect( &m_notificationTimer, SIGNAL(timeout()), SLOT(slotShowNotification()) );

}

void KMMainWidget::restoreCollectionFolderViewConfig()
{
  ETMViewStateSaver *saver = new ETMViewStateSaver;
  saver->setView( mFolderTreeWidget->folderTreeView() );
  const KConfigGroup cfg( KMKernel::self()->config(), "CollectionFolderView" );
  mFolderTreeWidget->restoreHeaderState( cfg.readEntry( "HeaderState", QByteArray() ) );
  saver->restoreState( cfg );
  //Restore startup folder
  Akonadi::Collection::Id startupFolder = GlobalSettings::self()->startupFolder();
  if ( startupFolder > 0 )
    saver->restoreCurrentItem( QString::fromLatin1("c%1").arg(startupFolder) );
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
  mCurrentFolder.clear();
  delete mMoveOrCopyToDialog;
  delete mSelectFromAllFoldersDialog;
  mSystemTray = 0;
  mCustomTemplateMenus = 0;
  mDestructed = true;
}


void KMMainWidget::slotStartCheckMail()
{
  mCheckMailInProgress = true;
  if ( m_notificationTimer.isActive() )
    m_notificationTimer.stop();
}

void KMMainWidget::slotEndCheckMail()
{
  if ( !m_notificationTimer.isActive() )
    m_notificationTimer.start();
}

void KMMainWidget::slotShowNotification()
{
  if ( mCheckMailInProgress )
    mCheckMailInProgress = false;
  showNotifications();  
}

void KMMainWidget::showNotifications()
{
  if ( mCheckMailInProgress  )
    return;
  const bool sendOnAll =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnAllChecks;
  const bool sendOnManual =
    GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnManualChecks;
  if ( !kmkernel->isOffline() && ( sendOnAll || (sendOnManual /*&& sendOnCheck*/ ) ) ) {
    slotSendQueued();
  }
  if (  mCheckMail.isEmpty() )
    return;
  
  Akonadi::Collection::List collections;
  QMap<Akonadi::Collection::Id, int>::const_iterator it = mCheckMail.constBegin();
  while ( it != mCheckMail.constEnd() ) {
    Akonadi::Collection collection( it.key() );
    if ( it.value() != 0 )
      collections << collection;
    ++it;
  }
  if ( collections.isEmpty() )
    return;
  
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( collections, Akonadi::CollectionFetchJob::Base );
  connect( job, SIGNAL(finished(KJob*)), this, SLOT(slotEndCheckFetchCollectionsDone(KJob*)) );
}

void KMMainWidget::slotEndCheckFetchCollectionsDone(KJob* job)
{
  
  // build summary for new mail message
  bool showNotification = false;
  QString summary;

  if ( job && !job->error() ) {

    Akonadi::CollectionFetchJob *colJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List collections = colJob->collections();


    Q_FOREACH( const Akonadi::Collection& collection, collections ) {
      const QString folderPath( MailCommon::Util::fullCollectionPath( collection ) );
      const int numberOfMails = mCheckMail[ collection.id() ];

      const QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( collection,false );

      if ( fd && !fd->ignoreNewMail() ) {
        showNotification = true;
        if ( GlobalSettings::self()->verboseNewMailNotification() ) {
          summary += "<br />" + i18np( "1 new message in %2",
                                      "%1 new messages in %2",
                                      numberOfMails, folderPath );
        }
      }
    }
  }

  // update folder menus in case some mail got filtered to trash/current folder
  // and we can enable "empty trash/move all to trash" action etc.
  updateFolderMenu();

  if ( !showNotification ) {
    mCheckMail.clear();
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
                          window(),
                          KNotification::CloseOnTimeout,
                          kmkernel->xmlGuiInstance() );
  } else {
    KNotification::event( "new-mail-arrived",
                          summary,
                          QPixmap(),
                          window(),
                          KNotification::CloseOnTimeout );
  }

  if ( GlobalSettings::self()->beepOnMail() ) {
    KNotification::beep();
  }
  mCheckMail.clear();
}

void KMMainWidget::slotFolderChanged( const Akonadi::Collection& col )
{
  updateFolderMenu();
  folderSelected( col );
  emit captionChangeRequest( MailCommon::Util::fullCollectionPath( col ) );
}

void KMMainWidget::folderSelected( const Akonadi::Collection & col )
{
  // This is connected to the MainFolderView signal triggering when a folder is selected

  if ( mGoToFirstUnreadMessageInSelectedFolder )
  {
    // the default action has been overridden from outside
    mPreSelectionMode = MessageList::Core::PreSelectFirstUnreadCentered;
  } else {
    // use the default action
    switch ( GlobalSettings::self()->actionEnterFolder() )
    {
      case GlobalSettings::EnumActionEnterFolder::SelectFirstUnread:
        mPreSelectionMode = MessageList::Core::PreSelectFirstUnreadCentered;
      break;
      case GlobalSettings::EnumActionEnterFolder::SelectLastSelected:
        mPreSelectionMode = MessageList::Core::PreSelectLastSelected;
      break;
      case GlobalSettings::EnumActionEnterFolder::SelectNewest:
        mPreSelectionMode = MessageList::Core::PreSelectNewestCentered;
      break;
      case GlobalSettings::EnumActionEnterFolder::SelectOldest:
        mPreSelectionMode = MessageList::Core::PreSelectOldestCentered;
      break;
      default:
        mPreSelectionMode = MessageList::Core::PreSelectNone;
      break;
    }
  }

  mGoToFirstUnreadMessageInSelectedFolder = false;
#ifndef QT_NO_CURSOR
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif

  if (mMsgView)
    mMsgView->clear(true);
  const bool newFolder = mCurrentFolder && ( mCurrentFolder->collection() != col );
  // Re-enable the msg list and quicksearch if we're showing a splash
  // screen. This is true either if there's no active folder, or if we
  // have a timer that is no longer active (i.e. it has already fired)
  // To make the if() a bit more complicated, we suppress the hiding
  // when the new folder is also an IMAP folder, because that's an
  // async operation and we don't want flicker if it results in just
  // a new splash.
  const bool isNewImapFolder = col.isValid() && KMKernel::self()->isImapFolder( col ) && newFolder;
  if( ( !mCurrentFolder  )
      || ( !isNewImapFolder && mShowBusySplashTimer )
      || ( newFolder && mShowingOfflineScreen && !( isNewImapFolder && kmkernel->isOffline() ) ) ) {
    if ( mMsgView ) {
      mMsgView->viewer()->enableMessageDisplay();
      mMsgView->clear( true );
    }
    if ( mMessagePane )
      mMessagePane->show();
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

  mCurrentFolder = FolderCollection::forCollection( col );

  if ( col.isValid() && kmkernel->isImapFolder( col )
#if 0 //PORT TO AKONADI
       && ( !mMessageListView->isFolderOpen( mFolder ) )
#endif
       ) {
    if ( kmkernel->isOffline() )
      {
        //mMessageListView->setCurrentFolder( 0 ); <-- useless in the new view: just do nothing
        // FIXME: Use an "offline tab" ?
        showOfflinePage();
        return;
      }
  }

  readFolderConfig();
  if (mMsgView)
  {
    mMsgView->setHtmlOverride(mFolderHtmlPref);
    mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPref);
  }

  if ( !mCurrentFolder->isValid() && ( mMessagePane->count() < 2 ) )
    slotIntro();

  updateMessageActions();
  updateFolderMenu();

  // The message pane uses the selection model of the folder view to load the correct aggregation model and theme
  //  settings. At this point the selection model hasn't been updated yet to the user's new choice, so it would load
  //  the old folder settings instead.
  QTimer::singleShot( 0, this, SLOT(slotShowSelectedFolderInPane()) );
}

void KMMainWidget::slotShowSelectedFolderInPane()
{
  if( mCurrentFolder ) {
    mMessagePane->setCurrentFolder( mCurrentFolder->collection(), false , mPreSelectionMode );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::readPreConfig()
{
  mLongFolderList = GlobalSettings::self()->folderList() == GlobalSettings::EnumFolderList::longlist;
  mReaderWindowActive = GlobalSettings::self()->readerWindowMode() != GlobalSettings::EnumReaderWindowMode::hide;
  mReaderWindowBelow = GlobalSettings::self()->readerWindowMode() == GlobalSettings::EnumReaderWindowMode::below;

  mHtmlPref = MessageViewer::GlobalSettings::self()->htmlMail();
  mHtmlLoadExtPref = MessageViewer::GlobalSettings::self()->htmlLoadExternal();
  mEnableFavoriteFolderView = ( GlobalSettings::self()->favoriteCollectionViewMode() != GlobalSettings::EnumFavoriteCollectionViewMode::HiddenMode );
  mEnableFolderQuickSearch = GlobalSettings::self()->enableFolderQuickSearch();
}


//-----------------------------------------------------------------------------
void KMMainWidget::readFolderConfig()
{
  if ( !mCurrentFolder || !mCurrentFolder->isValid() )
    return;

  KSharedConfig::Ptr config = KMKernel::self()->config();
  KConfigGroup group( config, mCurrentFolder->configGroupName() );
  mFolderHtmlPref =
      group.readEntry( "htmlMailOverride", false );
  mFolderHtmlLoadExtPref =
      group.readEntry( "htmlLoadExternalOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig()
{
  if ( mCurrentFolder && mCurrentFolder->isValid() ) {
    KSharedConfig::Ptr config = KMKernel::self()->config();
    KConfigGroup group( config, mCurrentFolder->configGroupName() );
    group.writeEntry( "htmlMailOverride", mFolderHtmlPref );
    group.writeEntry( "htmlLoadExternalOverride", mFolderHtmlLoadExtPref );
  }
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
    disconnect( mMsgView->copyAction(), SIGNAL(triggered(bool)),
                mMsgView, SLOT(slotCopySelectedText()) );

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

  mSplitter1 = new QSplitter( this );
  mSplitter2 = new QSplitter( mSplitter1 );

  QWidget * folderTreeWidget = mSearchAndTree;
  if ( mFavoriteCollectionsView ) {
    mFolderViewSplitter = new QSplitter( Qt::Vertical );
    mFolderViewSplitter->setOpaqueResize( opaqueResize );
    mFolderViewSplitter->setChildrenCollapsible( false );
    mFolderViewSplitter->addWidget( mFavoriteCollectionsView );
    mFavoriteCollectionsView->setParent( mFolderViewSplitter );
    mFolderViewSplitter->addWidget( mSearchAndTree );
    folderTreeWidget = mFolderViewSplitter;
  }

  if ( mLongFolderList ) {

    // add folder tree
    mSplitter1->setOrientation( Qt::Horizontal );
    mSplitter1->addWidget( folderTreeWidget );

    // and the rest to the right
    mSplitter1->addWidget( mSplitter2 );

    // add the message list to the right or below
    if ( readerWindowAtSide ) {
      mSplitter2->setOrientation( Qt::Horizontal );
    } else {
      mSplitter2->setOrientation( Qt::Vertical );
    }
    mSplitter2->addWidget( mMessagePane );

    // add the preview window, if there is one
    if ( mMsgView ) {
      mSplitter2->addWidget( mMsgView );
    }

  } else { // short folder list
    if ( mReaderWindowBelow ) {
      mSplitter1->setOrientation( Qt::Vertical );
      mSplitter2->setOrientation( Qt::Horizontal );
    } else { // at side or none
      mSplitter1->setOrientation( Qt::Horizontal );
      mSplitter2->setOrientation( Qt::Vertical );
    }

    mSplitter1->addWidget( mSplitter2 );

    // add folder tree
    mSplitter2->addWidget( folderTreeWidget );
    // add message list to splitter 2
    mSplitter2->addWidget( mMessagePane );

    // add the preview window, if there is one
    if ( mMsgView )
      mSplitter1->addWidget( mMsgView );
  }

  //
  // Set splitter properties
  //
  mSplitter1->setObjectName( "splitter1" );
  mSplitter1->setOpaqueResize( opaqueResize );
  mSplitter1->setChildrenCollapsible( false );
  mSplitter2->setObjectName( "splitter2" );
  mSplitter2->setOpaqueResize( opaqueResize );
  mSplitter2->setChildrenCollapsible( false );

  //
  // Set the stretch factors
  //
  mSplitter1->setStretchFactor( 0, 0 );
  mSplitter2->setStretchFactor( 0, 0 );
  mSplitter1->setStretchFactor( 1, 1 );
  mSplitter2->setStretchFactor( 1, 1 );

  if ( mFavoriteCollectionsView ) {
    mFolderViewSplitter->setStretchFactor( 0, 0 );
    mFolderViewSplitter->setStretchFactor( 1, 1 );
  }

  // Because the reader windows's width increases a tiny bit after each
  // restart in short folder list mode with message window at side, disable
  // the stretching as a workaround here
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
    int ffvHeight = GlobalSettings::self()->favoriteCollectionViewHeight();
    int ftHeight = GlobalSettings::self()->folderTreeHeight();
    splitterSizes << ffvHeight << ftHeight;
    mFolderViewSplitter->setSizes( splitterSizes );
  }

  //
  // Now add the splitters to the main layout
  //
  mTopLayout->addWidget( mSplitter1 );

  // Make sure the focus is on the view, and not on the quick search line edit, because otherwise
  // shortcuts like + or j go to the wrong place.
  // This would normally be done in the message list itself, but apparently something resets the focus
  // again, probably all the reparenting we do here.
  mMessagePane->focusView();

  // By default hide th unread and size columns on first run.
  if( kmkernel->firstStart() )
  {
    mFolderTreeWidget->folderTreeView()->hideColumn( 1 );
    mFolderTreeWidget->folderTreeView()->hideColumn( 3 );
    mFolderTreeWidget->folderTreeView()->header()->resizeSection( 0, folderViewWidth * 0.8 );
  }

  // Make the copy action work, see disconnect comment above
  if ( mMsgView )
    connect( mMsgView->copyAction(), SIGNAL(triggered(bool)),
             mMsgView, SLOT(slotCopySelectedText()) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::refreshFavoriteFoldersViewProperties()
{
  if( mFavoriteCollectionsView ) {
    if ( GlobalSettings::self()->favoriteCollectionViewMode() == GlobalSettings::EnumFavoriteCollectionViewMode::IconMode )
      mFavoriteCollectionsView->setViewMode( QListView::IconMode );
    else if ( GlobalSettings::self()->favoriteCollectionViewMode() == GlobalSettings::EnumFavoriteCollectionViewMode::ListMode )
      mFavoriteCollectionsView->setViewMode( QListView::ListMode );
    else
      Q_ASSERT(false); // we should never get here in hidden mode
    mFavoriteCollectionsView->setDropActionMenuEnabled( kmkernel->showPopupAfterDnD() );
    mFavoriteCollectionsView->setWordWrap( true );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::readConfig()
{
  KSharedConfig::Ptr config = KMKernel::self()->config();

  const bool oldLongFolderList = mLongFolderList;
  const bool oldReaderWindowActive = mReaderWindowActive;
  const bool oldReaderWindowBelow = mReaderWindowBelow;
  const bool oldFavoriteFolderView = mEnableFavoriteFolderView;
  const bool oldFolderQuickSearch = mEnableFolderQuickSearch;

  // on startup, the layout is always new and we need to relayout the widgets
  bool layoutChanged = !mStartupDone;

  if ( mStartupDone )
  {
    readPreConfig();

    layoutChanged = ( oldLongFolderList != mLongFolderList ) ||
                    ( oldReaderWindowActive != mReaderWindowActive ) ||
                    ( oldReaderWindowBelow != mReaderWindowBelow ) ||
                    ( oldFavoriteFolderView != mEnableFavoriteFolderView );

    if( layoutChanged ) {
      deleteWidgets();
      createWidgets();
      restoreCollectionFolderViewConfig();
    } else if ( oldFolderQuickSearch != mEnableFolderQuickSearch ) {
      if ( mEnableFolderQuickSearch )
        mFolderTreeWidget->filterFolderLineEdit()->show();
      else
        mFolderTreeWidget->filterFolderLineEdit()->hide();
    }
  }


  { // Read the config of the folder views and the header
    if ( mMsgView ) {
      mMsgView->readConfig();
    }
    mMessagePane->reloadGlobalConfiguration();
    mFolderTreeWidget->readConfig();
    refreshFavoriteFoldersViewProperties();
  }

  { // area for config group "General"
    if ( !mStartupDone )
    {
      // check mail on startup
      // do it after building the kmmainwin, so that the progressdialog is available
      QTimer::singleShot( 0, this, SLOT(slotCheckMailOnStartup()) );
    }
  }

  if ( layoutChanged ) {
    layoutSplitters();
  }

  updateMessageMenu();
  updateFileMenu();
  toggleSystemTray();

  connect( Akonadi::AgentManager::self(), SIGNAL(instanceAdded(Akonadi::AgentInstance)),
           this, SLOT(updateFileMenu()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
           this, SLOT(updateFileMenu()) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::writeConfig()
{
  // Don't save the sizes of all the widgets when we were never shown.
  // This can happen in Kontact, where the KMail plugin is automatically
  // loaded, but not necessarily shown.
  // This prevents invalid sizes from being saved
  if ( mWasEverShown ) {

    // The height of the header widget can be 0, this happens when the user
    // did not switch to the header widget onced and the "Welcome to KMail"
    // HTML widget was shown the whole time
    int headersHeight = mMessagePane->height();
    if ( headersHeight == 0 )
      headersHeight = height() / 2;

    GlobalSettings::self()->setSearchAndHeaderHeight( headersHeight );
    GlobalSettings::self()->setSearchAndHeaderWidth( mMessagePane->width() );
    if ( mFavoriteCollectionsView ) {
      GlobalSettings::self()->setFavoriteCollectionViewHeight( mFavoriteCollectionsView->height() );
      GlobalSettings::self()->setFolderTreeHeight( mFolderTreeWidget->height() );
      if ( !mLongFolderList )
        GlobalSettings::self()->setFolderViewHeight( mFolderViewSplitter->height() );
    }
    else if ( !mLongFolderList && mFolderTreeWidget )
      {
        GlobalSettings::self()->setFolderTreeHeight( mFolderTreeWidget->height() );
      }
    if ( mFolderTreeWidget )
    {
      GlobalSettings::self()->setFolderViewWidth( mFolderTreeWidget->width() );
      KSharedConfig::Ptr config = KMKernel::self()->config();
      KConfigGroup group(config, "CollectionFolderView");

      ETMViewStateSaver saver;
      saver.setView( mFolderTreeWidget->folderTreeView() );
      saver.saveState( group );

      group.writeEntry( "HeaderState", mFolderTreeWidget->folderTreeView()->header()->saveState() );
      //Work around from startup folder
      group.deleteEntry( "Selection" );
      group.sync();
    }

    if ( mMsgView ) {
      if ( !mReaderWindowBelow )
        GlobalSettings::self()->setReaderWindowWidth( mMsgView->width() );
      mMsgView->viewer()->writeConfig();
      GlobalSettings::self()->setReaderWindowHeight( mMsgView->height() );
    }
  }
}

void KMMainWidget::writeReaderConfig()
{
  if ( mWasEverShown ) {
    if ( mMsgView ) {
      mMsgView->viewer()->writeConfig();
    }
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::deleteWidgets()
{
  // Simply delete the top splitter, which always is mSplitter1, regardless
  // of the layout. This deletes all children.
  // akonadi action manager is created in createWidgets(), parented to this
  //  so not autocleaned up.
  delete mAkonadiStandardActionManager;
  delete mSplitter1;
  mMsgView = 0;
  mSearchAndTree = 0;
  mFolderViewSplitter = 0;
  mFavoriteCollectionsView = 0;
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
  FolderTreeWidget::TreeViewOptions opt = FolderTreeWidget::ShowUnreadCount;
  opt |= FolderTreeWidget::UseLineEditForFiltering;
  opt |= FolderTreeWidget::ShowCollectionStatisticAnimation;
  opt |= FolderTreeWidget::DontKeyFilter;
  mFolderTreeWidget = new FolderTreeWidget( this, mGUIClient, opt );

  connect( mFolderTreeWidget->folderTreeView(), SIGNAL(currentChanged(Akonadi::Collection)), this, SLOT(slotFolderChanged(Akonadi::Collection)) );

  connect( mFolderTreeWidget->folderTreeView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateFolderMenu()) );

  connect( mFolderTreeWidget->folderTreeView(), SIGNAL(prefereCreateNewTab(bool)), this, SLOT(slotCreateNewTab(bool)) );

  mFolderTreeWidget->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mMessagePane = new CollectionPane( KMKernel::self()->entityTreeModel(),
                                        mFolderTreeWidget->folderTreeView()->selectionModel(),
                                        this );

  mMessagePane->setXmlGuiClient( mGUIClient );
  connect( mMessagePane, SIGNAL(messageSelected(Akonadi::Item)),
           this, SLOT(slotMessageSelected(Akonadi::Item)) );
  connect( mMessagePane, SIGNAL(fullSearchRequest()), this,SLOT(slotRequestFullSearchFromQuickSearch()) );
  connect( mMessagePane, SIGNAL(selectionChanged()),
           SLOT(startUpdateMessageActionsTimer()) );
  connect( mMessagePane, SIGNAL(currentTabChanged()), this, SLOT(refreshMessageListSelection()) );
  connect( mMessagePane, SIGNAL(messageActivated(Akonadi::Item)),
           this, SLOT(slotMessageActivated(Akonadi::Item)) );
  connect( mMessagePane, SIGNAL(messageStatusChangeRequest(Akonadi::Item,Akonadi::MessageStatus,Akonadi::MessageStatus)),
           SLOT(slotMessageStatusChangeRequest(Akonadi::Item,Akonadi::MessageStatus,Akonadi::MessageStatus)) );

  connect( mMessagePane, SIGNAL(statusMessage(QString)),
           BroadcastStatus::instance(), SLOT(setStatusMsg(QString)) );

  {
    KAction *action = new KAction( i18n("Set Focus to Quick Search"), this );
    action->setShortcut( QKeySequence( Qt::ALT + Qt::Key_Q ) );
    actionCollection()->addAction( "focus_to_quickseach", action );
    connect( action, SIGNAL(triggered(bool)),
             SLOT(slotFocusQuickSearch()) );
  }

  mPreviousMessageAction = new KAction( i18n( "Extend Selection to Previous Message" ), this );
  mPreviousMessageAction->setShortcut( QKeySequence( Qt::SHIFT + Qt::Key_Left ) );
  actionCollection()->addAction( "previous_message", mPreviousMessageAction );
  connect( mPreviousMessageAction, SIGNAL(triggered(bool)),
           this, SLOT(slotExtendSelectionToPreviousMessage()) );

  mNextMessageAction = new KAction( i18n( "Extend Selection to Next Message" ), this );
  mNextMessageAction->setShortcut( QKeySequence( Qt::SHIFT + Qt::Key_Right ) );
  actionCollection()->addAction( "next_message", mNextMessageAction );
  connect( mNextMessageAction, SIGNAL(triggered(bool)),
           this, SLOT(slotExtendSelectionToNextMessage()) );

  //
  // Create the reader window
  //
  if ( mReaderWindowActive ) {
    mMsgView = new KMReaderWin( this, this, actionCollection(), 0 );
    if ( mMsgActions ) {
      mMsgActions->setMessageView( mMsgView );
    }
    connect( mMsgView->viewer(), SIGNAL(replaceMsgByUnencryptedVersion()),
             this, SLOT(slotReplaceMsgByUnencryptedVersion()) );
    connect( mMsgView->viewer(), SIGNAL(popupMenu(Akonadi::Item,KUrl,QPoint)),
             this, SLOT(slotMessagePopup(Akonadi::Item,KUrl,QPoint)) );
  }
  else {
    if ( mMsgActions ) {
      mMsgActions->setMessageView( 0 );
    }
  }

  //
  // Create the folder tree
  // the "folder tree" consists of a quicksearch input field and the tree itself
  //

  mSearchAndTree = new QWidget( this );
  QVBoxLayout *vboxlayout = new QVBoxLayout;
  vboxlayout->setMargin(0);
  mSearchAndTree->setLayout( vboxlayout );

  vboxlayout->addWidget( mFolderTreeWidget );

  if ( !GlobalSettings::self()->enableFolderQuickSearch() ) {
    mFolderTreeWidget->filterFolderLineEdit()->hide();
  }
  //
  // Create the favorite folder view
  //
  mAkonadiStandardActionManager = new Akonadi::StandardMailActionManager( mGUIClient->actionCollection(), this );
  connect( mAkonadiStandardActionManager, SIGNAL(actionStateUpdated()), this, SLOT(slotAkonadiStandardActionUpdated()) );

  mAkonadiStandardActionManager->setCollectionSelectionModel( mFolderTreeWidget->folderTreeView()->selectionModel() );
  mAkonadiStandardActionManager->setItemSelectionModel( mMessagePane->currentItemSelectionModel() );

  if ( mEnableFavoriteFolderView ) {

    mFavoriteCollectionsView = new Akonadi::EntityListView( mGUIClient, this );
    refreshFavoriteFoldersViewProperties();

    connect( mFavoriteCollectionsView, SIGNAL(currentChanged(Akonadi::Collection)), this, SLOT(slotFolderChanged(Akonadi::Collection)) );

    mFavoritesModel = new Akonadi::FavoriteCollectionsModel(
                                mFolderTreeWidget->folderTreeView()->model(),
                                KMKernel::self()->config()->group( "FavoriteCollections" ), this );

    mFavoriteCollectionsView->setModel( mFavoritesModel );

    Akonadi::CollectionStatisticsDelegate *delegate = new Akonadi::CollectionStatisticsDelegate( mFavoriteCollectionsView );
    delegate->setProgressAnimationEnabled( true );
    mFavoriteCollectionsView->setItemDelegate(delegate);
    delegate->setUnreadCountShown( true );

    mAkonadiStandardActionManager->setFavoriteCollectionsModel( mFavoritesModel );
    mAkonadiStandardActionManager->setFavoriteSelectionModel( mFavoriteCollectionsView->selectionModel() );
  }
  mAkonadiStandardActionManager->createAllActions();

  mAkonadiStandardActionManager->interceptAction( Akonadi::StandardActionManager::CollectionProperties );
  connect( mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CollectionProperties ), SIGNAL(triggered(bool)), this, SLOT(slotCollectionProperties()) );


  //
  // Create all kinds of actions
  //
  mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::RemoveDuplicates )->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Asterisk ) );
  {
    mCollectionProperties = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CollectionProperties );
  }
  {
    mMoveMsgToFolderAction = new KAction( i18n("Move Message to Folder"), this );
    mMoveMsgToFolderAction->setShortcut( QKeySequence( Qt::Key_M ) );
    actionCollection()->addAction( "move_message_to_folder", mMoveMsgToFolderAction );
    connect( mMoveMsgToFolderAction, SIGNAL(triggered(bool)),
             SLOT(slotMoveSelectedMessageToFolder()) );
  }
  {
    KAction *action = new KAction( i18n("Copy Message to Folder"), this );
    actionCollection()->addAction( "copy_message_to_folder", action );
    connect( action, SIGNAL(triggered(bool)),
             SLOT(slotCopySelectedMessagesToFolder()) );
    action->setShortcut( QKeySequence( Qt::Key_C ) );
  }
  {
    KAction *action = new KAction( i18n("Jump to Folder..."), this );
    actionCollection()->addAction( "jump_to_folder", action );
    connect( action, SIGNAL(triggered(bool)),
             SLOT(slotJumpToFolder()) );
    action->setShortcut( QKeySequence( Qt::Key_J ) );
  }
  {
    KAction *action = new KAction(i18n("Abort Current Operation"), this);
    actionCollection()->addAction("cancel", action );
    connect( action, SIGNAL(triggered(bool)),
             ProgressManager::instance(), SLOT(slotAbortAll()) );
    action->setShortcut( QKeySequence( Qt::Key_Escape ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Next Folder"), this);
    actionCollection()->addAction("inc_current_folder", action );
    connect( action, SIGNAL(triggered(bool)),
             mFolderTreeWidget->folderTreeView(), SLOT(slotFocusNextFolder()) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Right ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Previous Folder"), this);
    actionCollection()->addAction("dec_current_folder", action );
    connect( action, SIGNAL(triggered(bool)),
             mFolderTreeWidget->folderTreeView(), SLOT(slotFocusPrevFolder()) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Left ) );
  }
  {
    KAction *action = new KAction(i18n("Select Folder with Focus"), this);
    actionCollection()->addAction("select_current_folder", action );

    connect( action, SIGNAL(triggered(bool)),
             mFolderTreeWidget->folderTreeView(), SLOT(slotSelectFocusFolder()) );
    action->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_Space ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Next Message"), this);
    actionCollection()->addAction("inc_current_message", action );
    connect( action, SIGNAL(triggered(bool)),
             this, SLOT(slotFocusOnNextMessage()) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Right ) );
  }
  {
    KAction *action = new KAction(i18n("Focus on Previous Message"), this);
    actionCollection()->addAction("dec_current_message", action );
    connect( action, SIGNAL(triggered(bool)),
             this, SLOT(slotFocusOnPrevMessage()) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Left ) );
  }
  {
    KAction *action = new KAction(i18n("Select Message with Focus"), this);
    actionCollection()->addAction( "select_current_message", action );
    connect( action, SIGNAL(triggered(bool)),
             this, SLOT(slotSelectFocusedMessage()) );
    action->setShortcut( QKeySequence( Qt::ALT+Qt::Key_Space ) );
  }
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           SLOT(slotItemAdded(Akonadi::Item,Akonadi::Collection)) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(itemRemoved(Akonadi::Item)),
           SLOT(slotItemRemoved(Akonadi::Item)) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
           SLOT(slotItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), SLOT(slotCollectionChanged(Akonadi::Collection,QSet<QByteArray>)) );

  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), SLOT(slotCollectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );

}

void KMMainWidget::slotCollectionStatisticsChanged( const Akonadi::Collection::Id id, const Akonadi::CollectionStatistics& statistic )
{
  if ( id == CommonKernel->outboxCollectionFolder().id() ) {
    const qint64 nbMsgOutboxCollection = statistic.count();  
    actionCollection()->action( "send_queued" )->setEnabled( nbMsgOutboxCollection > 0 );
    actionCollection()->action( "send_queued_via" )->setEnabled( nbMsgOutboxCollection > 0 );
  }
}

void KMMainWidget::slotCreateNewTab( bool preferNewTab )
{
  mMessagePane->setPreferEmptyTab( preferNewTab );
}

void KMMainWidget::slotCollectionChanged( const Akonadi::Collection&collection, const QSet<QByteArray>&set )
{
  if ( mCurrentFolder
       && ( collection == mCurrentFolder->collection() )
       && set.contains( "MESSAGEFOLDER" ) ) {
    mMessagePane->resetModelStorage();
  } else if ( set.contains( "ENTITYDISPLAY" ) || set.contains( "NAME" ) ) {

    QIcon icon = KIcon( QLatin1String( "folder" ) );
    QString text;

    const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection( KMKernel::self()->collectionModel(), collection );
    if ( idx.isValid() ) {
      text = idx.data().toString();
      icon = idx.data( Qt::DecorationRole ).value<QIcon>();
      mMessagePane->updateTabIconText( collection, text,icon );
    }
  }
}


void KMMainWidget::slotItemAdded( const Akonadi::Item &msg, const Akonadi::Collection& col)
{
  if ( col.isValid() ) {
    if ( col == CommonKernel->outboxCollectionFolder() ) {
      startUpdateMessageActionsTimer();
    } else {
      addInfoInNotification( col );
    }
  }
}

void KMMainWidget::slotItemRemoved( const Akonadi::Item & item)
{
  if ( item.isValid() && item.parentCollection().isValid() && ( item.parentCollection() == CommonKernel->outboxCollectionFolder() ) ) {
    startUpdateMessageActionsTimer();
  }
}

void KMMainWidget::slotItemMoved( Akonadi::Item item, Akonadi::Collection from, Akonadi::Collection to )
{
  if( item.isValid() && ( ( from.id() == CommonKernel->outboxCollectionFolder().id() )
                          || to.id() == CommonKernel->outboxCollectionFolder().id() ) )
  {
    startUpdateMessageActionsTimer();
  }
  else{
    updateInfoInNotification( from, to );
  }
}

void KMMainWidget::updateInfoInNotification( const Akonadi::Collection& from, const Akonadi::Collection& to )
{
  if ( mCheckMail.contains( from.id() ) ) {
    mCheckMail[ from.id() ]--;
    if ( mCheckMail[from.id()] == 0 )
      mCheckMail.remove( from.id() );
  }

  if ( !excludeSpecialFolder( to ) ) {
    mCheckMail[ to.id() ]++;
  }
}

bool KMMainWidget::excludeSpecialFolder( const Akonadi::Collection &collection )
{
  if ( CommonKernel->outboxCollectionFolder() == collection ||
       CommonKernel->sentCollectionFolder() == collection ||
       CommonKernel->templatesCollectionFolder() == collection ||
       CommonKernel->trashCollectionFolder() == collection ||
       CommonKernel->draftsCollectionFolder() == collection )
    return true;
  return false;
}
  
void KMMainWidget::addInfoInNotification( const Akonadi::Collection &collection )
{
  if ( excludeSpecialFolder( collection ) )
    return;
  mCheckMail[ collection.id() ]++;
}

//-------------------------------------------------------------------------
void KMMainWidget::slotFocusQuickSearch()
{
  mMessagePane->focusQuickSearch();
}

//-------------------------------------------------------------------------
bool KMMainWidget::slotSearch()
{
  // check if we can search at all, ie. Nepomuk is running and email indexing is enabled
  if ( !Nepomuk::ResourceManager::instance()->initialized() ) {
    KMessageBox::information( this, i18n( "The Nepomuk semantic search service is not available. Searching is not possible without it. "
                                          "You can enable it in \"System Settings\"." ), i18n( "Search Not Available" ) );
    return false;
  }

  {
    KConfig config( "akonadi_nepomuk_feederrc" );
    KConfigGroup cfgGroup( &config, "akonadi_nepomuk_email_feeder" );
    if ( !cfgGroup.readEntry( "Enabled", true ) ) {
      KMessageBox::information( this, i18n( "You have disabled full text indexing of emails. Searching is not possible without that. "
                                            "You can enable it in \"System Settings\". Note that searching will only be possible after "
                                            "your emails have been fully indexed, which can take some time." ) );
      return false;
    }
  }

  
  if(!mSearchWin)
  {
    mSearchWin = new SearchWindow(this, mCurrentFolder ? mCurrentFolder->collection() : Akonadi::Collection());
    mSearchWin->setModal( false );
    mSearchWin->setObjectName( "Search" );
  }
  else
  {
    mSearchWin->activateFolder(mCurrentFolder ? mCurrentFolder->collection() : Akonadi::Collection());
  }

  mSearchWin->show();
  KWindowSystem::activateWindow( mSearchWin->winId() );
  return true;
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
  FilterIf->openFilterDialog( true );
}

void KMMainWidget::slotManageSieveScripts()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }
  KSieveUi::ManageSieveScriptsDialog * dlg = new KSieveUi::ManageSieveScriptsDialog( this );
  dlg->show();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotAddrBook()
{
  KRun::runCommand("kaddressbook", window());
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotImport()
{
  KRun::runCommand("kmailcvt", window());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckMail()
{
  kmkernel->checkMail();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckMailOnStartup()
{
  kmkernel->checkMailOnStartup();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckOneAccount( QAction* item )
{
  if ( !item ) {
    return;
  }

  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance( item->data().toString() );
  if ( agent.isValid() ) {
    if ( !agent.isOnline() ) {
      agent.setIsOnline( true );
    }
    agent.synchronize();
  } else {
    kDebug() << "account with identifier" << item->data().toString() << "not found";
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompose()
{
  KMail::Composer * win;
  KMime::Message::Ptr msg( new KMime::Message() );

  if ( mCurrentFolder ) {
      MessageHelper::initHeader( msg, KMKernel::self()->identityManager(), mCurrentFolder->identity() );
      if ( mCurrentFolder->collection().isValid() && mCurrentFolder->putRepliesInSameFolder() ) {
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Fcc", msg.get(), QString::number( mCurrentFolder->collection().id() ), "utf-8" );
        msg->setHeader( header );
      }
      TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
      parser.setIdentityManager( KMKernel::self()->identityManager() );
      parser.process( msg, mCurrentFolder->collection() );
      win = KMail::makeComposer( msg, KMail::Composer::New, mCurrentFolder->identity() );
  } else {
      MessageHelper::initHeader( msg, KMKernel::self()->identityManager() );
      TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
      parser.setIdentityManager( KMKernel::self()->identityManager() );
      parser.process( KMime::Message::Ptr(), Akonadi::Collection() );
      win = KMail::makeComposer( msg, KMail::Composer::New );
  }

  win->show();

}

//-----------------------------------------------------------------------------
// TODO: do we want the list sorted alphabetically?
void KMMainWidget::slotShowNewFromTemplate()
{
  if ( mCurrentFolder )
    {
      const KPIMIdentities::Identity & ident =
        kmkernel->identityManager()->identityForUoidOrDefault( mCurrentFolder->identity() );
      mTemplateFolder = CommonKernel->collectionFromId( ident.templates().toLongLong() );
    }

  if ( !mTemplateFolder.isValid() ) {
    mTemplateFolder = CommonKernel->templatesCollectionFolder();
  }
  if ( !mTemplateFolder.isValid() )
    return;

  mTemplateMenu->menu()->clear();

  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mTemplateFolder );
  job->fetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
  job->fetchScope().fetchFullPayload();
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedShowNewFromTemplate(KJob*)) );
}

void KMMainWidget::slotDelayedShowNewFromTemplate( KJob *job )
{
  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );

  const Akonadi::Item::List items = fetchJob->items();
  const int numberOfItems = items.count();
  for ( int idx = 0; idx < numberOfItems; ++idx ) {
    KMime::Message::Ptr msg = MessageCore::Util::message( items.at( idx ) );
    if ( msg ) {
      QString subj = msg->subject()->asUnicodeString();
      if ( subj.isEmpty() )
        subj = i18n("No Subject");

      QAction *templateAction = mTemplateMenu->menu()->addAction(KStringHandler::rsqueeze( subj.replace( '&', "&&" ) ) );
      QVariant var;
      var.setValue( items.at( idx ) );
      templateAction->setData( var );
    }
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

  if ( !mTemplateFolder.isValid() )
    return;
  const Akonadi::Item item = action->data().value<Akonadi::Item>();
  newFromTemplate( item );
}


//-----------------------------------------------------------------------------
void KMMainWidget::newFromTemplate( const Akonadi::Item &msg )
{
  if ( !msg.isValid() )
    return;
  KMCommand *command = new KMUseTemplateCommand( this, msg );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPostToML()
{
  if ( mCurrentFolder && mCurrentFolder->isMailingListEnabled() ) {
    KMail::Util::mailingListPost( mCurrentFolder );
  }
  else
    slotCompose();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderMailingListProperties()
{
  if ( !mFolderTreeWidget || !mCurrentFolder )
    return;

  ( new KMail::MailingListFolderPropertiesDialog( this, mCurrentFolder ) )->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowFolderShortcutDialog()
{
  if ( !mFolderTreeWidget || !mCurrentFolder )
    return;

  MessageViewer::AutoQPointer<KMail::FolderShortcutDialog> shorty( new KMail::FolderShortcutDialog( mCurrentFolder, kmkernel->getKMMainWidget(),
                                                                                                    mFolderTreeWidget ) );
  shorty->exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireFolder()
{
  if ( !mCurrentFolder )
    return;

  bool canBeExpired = true;
  if ( !mCurrentFolder->isAutoExpire() ) {
    canBeExpired = false;
  } else if ( mCurrentFolder->getUnreadExpireUnits() == FolderCollection::ExpireNever &&
              mCurrentFolder->getReadExpireUnits() == FolderCollection::ExpireNever ) {
    canBeExpired = false;
  }

  if ( !canBeExpired ) {
    const QString message = i18n( "This folder does not have any expiry options set" );
    KMessageBox::information( this, message );
    return;
  }

  if ( GlobalSettings::self()->warnBeforeExpire() ) {
    const QString message = i18n( "<qt>Are you sure you want to expire the folder <b>%1</b>?</qt>",
                                  Qt::escape( mCurrentFolder->name() ) );
    if ( KMessageBox::warningContinueCancel( this, message, i18n( "Expire Folder" ),
                                             KGuiItem( i18n( "&Expire" ) ) )
        != KMessageBox::Continue )
      return;
  }
  mCurrentFolder->expireOldMessages( true /*immediate*/ );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEmptyFolder()
{
  QString str;

  if (!mCurrentFolder) return;
  const bool isTrash = CommonKernel->folderIsTrash( mCurrentFolder->collection() );
  if ( GlobalSettings::self()->confirmBeforeEmpty() )
  {
    const QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
    const QString text = (isTrash) ?
      i18n("Are you sure you want to empty the trash folder?") :
      i18n("<qt>Are you sure you want to move all messages from "
           "folder <b>%1</b> to the trash?</qt>", Qt::escape( mCurrentFolder->name() ) );

    if (KMessageBox::warningContinueCancel(this, text, title, KGuiItem( title, "user-trash"))
      != KMessageBox::Continue) return;
  }
#ifndef QT_NO_CURSOR
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif  
  slotMarkAll();
  if (isTrash) {
    /* Don't ask for confirmation again when deleting, the user has already
       confirmed. */
    slotDeleteMsg( false );
  }
  else
    slotTrashSelectedMessages();

  if (mMsgView)
    mMsgView->clearCache();

  if ( !isTrash )
    BroadcastStatus::instance()->setStatusMsg(i18n("Moved all messages to the trash"));

  updateMessageActions();

  // Disable empty trash/move all to trash action - we've just deleted/moved
  // all folder contents.
  mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveAllToTrash )->setEnabled( false );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotArchiveFolder()
{
  if ( mCurrentFolder ) {
    KMail::ArchiveFolderDialog archiveDialog;
    archiveDialog.setFolder( mCurrentFolder->collection() );
    archiveDialog.exec();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotRemoveFolder()
{
  if ( !mCurrentFolder ) return;
  if ( mCurrentFolder->isSystemFolder() ) return;
  if ( mCurrentFolder->isReadOnly() ) return;

  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( mCurrentFolder->collection(), CollectionFetchJob::Recursive, this );
  job->fetchScope().setContentMimeTypes( QStringList() << KMime::Message::mimeType() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedRemoveFolder(KJob*)) );
}

void KMMainWidget::slotDelayedRemoveFolder( KJob *job )
{
  const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
  const bool hasNotSubDirectory = fetchJob->collections().isEmpty();

  QDir dir;
  QString str;
  QString title;
  QString buttonLabel;

  if ( mCurrentFolder->collection().resource() == QLatin1String( "akonadi_search_resource" ) ) {
    title = i18n("Delete Search");
    str = i18n("<qt>Are you sure you want to delete the search <b>%1</b>?<br />"
                "Any messages it shows will still be available in their original folder.</qt>",
             Qt::escape( mCurrentFolder->name() ) );
    buttonLabel = i18nc("@action:button Delete search", "&Delete");
  } else {
    title = i18n("Delete Folder");


    if ( mCurrentFolder->count() == 0 ) {
      if ( hasNotSubDirectory ) {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<b>%1</b>?</qt>",
                Qt::escape( mCurrentFolder->name() ) );
      } else {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<resource>%1</resource> and all its subfolders? Those subfolders might "
                   "not be empty and their contents will be discarded as well. "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
                Qt::escape( mCurrentFolder->name() ) );
      }
    } else {
      if ( hasNotSubDirectory ) {
        str = i18n("<qt>Are you sure you want to delete the folder "
                   "<resource>%1</resource>, discarding its contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
                Qt::escape( mCurrentFolder->name() ) );
      }else {
        str = i18n("<qt>Are you sure you want to delete the folder <resource>%1</resource> "
                   "and all its subfolders, discarding their contents? "
                   "<p><b>Beware</b> that discarded messages are not saved "
                   "into your Trash folder and are permanently deleted.</p></qt>",
              Qt::escape( mCurrentFolder->name() ) );
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
    const Akonadi::Collection::Id collectionId = mCurrentFolder->collection().id();
    kmkernel->checkFolderFromResources( collectionId );
    
    mCurrentFolder->removeCollection();
  }
  mCurrentFolder.clear();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireAll()
{
  int ret = 0;

  if ( GlobalSettings::self()->warnBeforeExpire() ) {
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
      "OverrideHtmlWarning", 0);
    if( result == KMessageBox::Cancel ) {
      mPreferHtmlAction->setChecked( false );
      return;
    }
  }
  mFolderHtmlPref = !mFolderHtmlPref;

  //Update mPrefererHtmlLoadExtAction
  mPreferHtmlLoadExtAction->setEnabled( mCurrentFolder && (mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref) ? true : false );

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
      "OverrideHtmlLoadExtWarning", 0);
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
  if (!CommonKernel->folderIsDraftOrOutbox(mCurrentFolder->collection()))
      return;
  if (mMsgView)
    mMsgView->update(true);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardInlineMsg()
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  KMForwardCommand * command = new KMForwardCommand(
      this, selectedMessages, mCurrentFolder->identity()
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMsg()
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  KMForwardAttachedCommand * command = new KMForwardAttachedCommand(
      this, selectedMessages, mCurrentFolder->identity()
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUseTemplate()
{
  newFromTemplate( mMessagePane->currentItem() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotResendMsg()
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;
  KMCommand *command = new KMResendMessageCommand( this, msg );

  command->start();
}

//-----------------------------------------------------------------------------
// Message moving and permanent deletion
//

void KMMainWidget::moveMessageSelected( MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion )
{
  QList<Akonadi::Item> selectMsg  = mMessagePane->itemListFromPersistentSet( ref );
  // If this is a deletion, ask for confirmation
  if ( !dest.isValid() && confirmOnDeletion )
  {
    int ret = KMessageBox::warningContinueCancel(
        this,
        i18np(
            "<qt>Do you really want to delete the selected message?<br />"
            "Once deleted, it cannot be restored.</qt>",
            "<qt>Do you really want to delete the %1 selected messages?<br />"
            "Once deleted, they cannot be restored.</qt>",
            selectMsg.count()
          ),
        selectMsg.count() > 1 ? i18n( "Delete Messages" ) : i18n( "Delete Message" ),
        KStandardGuiItem::del(),
        KStandardGuiItem::cancel(),
        "NoConfirmDelete"
      );
    if ( ret == KMessageBox::Cancel )
    {
      mMessagePane->deletePersistentSet( ref );
      return;  // user canceled the action
    }
  }
  mMessagePane->markMessageItemsAsAboutToBeRemoved( ref, true );
  // And stuff them into a KMMoveCommand :)
  KMMoveCommand *command = new KMMoveCommand( dest, selectMsg,ref );
  QObject::connect(
      command, SIGNAL(moveDone(KMMoveCommand*)),
      this, SLOT(slotMoveMessagesCompleted(KMMoveCommand*))
    );
  command->start();

  if ( dest.isValid() )
    BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages..." ) );
  else
    BroadcastStatus::instance()->setStatusMsg( i18n( "Deleting messages..." ) );
}

void KMMainWidget::slotMoveMessagesCompleted( KMMoveCommand *command )
{
  Q_ASSERT( command );
  mMessagePane->markMessageItemsAsAboutToBeRemoved( command->refSet(), false );
  mMessagePane->deletePersistentSet( command->refSet() );
  // Bleah :D
  const bool moveWasReallyADelete = !command->destFolder().isValid();

  if ( command->result() == KMCommand::OK )
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
  MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
  if ( ref != -1 )
    moveMessageSelected( ref, Akonadi::Collection(), confirmDelete );
}

void KMMainWidget::slotDeleteThread( bool confirmDelete )
{
  // Create a persistent set from the current thread.
  MessageList::Core::MessageItemSetReference ref = mMessagePane->currentThreadAsPersistentSet();
  if ( ref != -1 )
    moveMessageSelected( ref, Akonadi::Collection(), confirmDelete );
}

FolderSelectionDialog* KMMainWidget::moveOrCopyToDialog()
{
  if ( mMoveOrCopyToDialog == 0 ) {
    FolderSelectionDialog::SelectionFolderOption options = FolderSelectionDialog::HideVirtualFolder;
    mMoveOrCopyToDialog = new FolderSelectionDialog( this, options );
    mMoveOrCopyToDialog->setModal( true );
  }
  return mMoveOrCopyToDialog;
}

FolderSelectionDialog* KMMainWidget::selectFromAllFoldersDialog()
{
  if ( mSelectFromAllFoldersDialog == 0 ) {
    FolderSelectionDialog::SelectionFolderOptions options = FolderSelectionDialog::None;
    options |= FolderSelectionDialog::NotAllowToCreateNewFolder;

    mSelectFromAllFoldersDialog = new FolderSelectionDialog( this, options );
    mSelectFromAllFoldersDialog->setModal( true );
  }
  return mSelectFromAllFoldersDialog;
}

void KMMainWidget::slotMoveSelectedMessageToFolder()
{
  moveOrCopyToDialog()->setCaption(  i18n( "Move Messages to Folder" ) );
  if ( moveOrCopyToDialog()->exec() && moveOrCopyToDialog() ) {
    const Akonadi::Collection dest = moveOrCopyToDialog()->selectedCollection();
    if ( dest.isValid() ) {
      moveSelectedMessagesToFolder( dest );
    }
  }
}

void KMMainWidget::moveSelectedMessagesToFolder( const Akonadi::Collection & dest )
{
   MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
  if ( ref != -1 ) {
    //Need to verify if dest == src ??? akonadi do it for us.
    moveMessageSelected( ref, dest, false );
  }
}


void KMMainWidget::copyMessageSelected( const QList<Akonadi::Item> &selectMsg, const Akonadi::Collection &dest )
{
  if ( selectMsg.isEmpty() )
    return;
    // And stuff them into a KMCopyCommand :)
  KMCommand *command = new KMCopyCommand( dest, selectMsg );
  QObject::connect(
      command, SIGNAL(completed(KMCommand*)),
      this, SLOT(slotCopyMessagesCompleted(KMCommand*))
    );
  command->start();
  BroadcastStatus::instance()->setStatusMsg( i18n( "Copying messages..." ) );
}


void KMMainWidget::slotCopyMessagesCompleted( KMCommand *command )
{
  Q_ASSERT( command );
  if ( command->result() == KMCommand::OK )
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

void KMMainWidget::slotCopySelectedMessagesToFolder()
{
  moveOrCopyToDialog()->setCaption( i18n( "Copy Messages to Folder" ) );

  if ( moveOrCopyToDialog()->exec() && moveOrCopyToDialog() ) {
    const Akonadi::Collection dest = moveOrCopyToDialog()->selectedCollection();
    if ( dest.isValid() ) {
      copySelectedMessagesToFolder( dest );
    }
  }
}

void KMMainWidget::copySelectedMessagesToFolder( const Akonadi::Collection& dest )
{
  const QList<Akonadi::Item > lstMsg = mMessagePane->selectionAsMessageItemList();
  if ( !lstMsg.isEmpty() ) {
    copyMessageSelected( lstMsg, dest );
  }
}

//-----------------------------------------------------------------------------
// Message trashing
//
void KMMainWidget::trashMessageSelected( MessageList::Core::MessageItemSetReference ref )
{
  const QList<Akonadi::Item> select = mMessagePane->itemListFromPersistentSet( ref );
  mMessagePane->markMessageItemsAsAboutToBeRemoved( ref, true );

    // FIXME: Why we don't use KMMoveCommand( trashFolder(), selectedMessages ); ?
  // And stuff them into a KMTrashMsgCommand :)
  KMCommand *command = new KMTrashMsgCommand( mCurrentFolder->collection(), select,ref );

  QObject::connect(
      command, SIGNAL(moveDone(KMMoveCommand*)),
      this, SLOT(slotTrashMessagesCompleted(KMMoveCommand*))
    );
  command->start();
  BroadcastStatus::instance()->setStatusMsg( i18n( "Moving messages to trash..." ) );
}

void KMMainWidget::slotTrashMessagesCompleted( KMMoveCommand *command )
{
  Q_ASSERT( command );
  mMessagePane->markMessageItemsAsAboutToBeRemoved( command->refSet(), false );
  mMessagePane->deletePersistentSet( command->refSet() );
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

void KMMainWidget::slotTrashSelectedMessages()
{
  MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
  if ( ref != -1 ) {
    trashMessageSelected( ref );
  }
}

void KMMainWidget::slotTrashThread()
{
  MessageList::Core::MessageItemSetReference ref = mMessagePane->currentThreadAsPersistentSet();
  if ( ref != -1 )
    trashMessageSelected( ref );
}

//-----------------------------------------------------------------------------
// Message tag setting for messages
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....
void KMMainWidget::toggleMessageSetTag( const QList<Akonadi::Item> &select, const QString &taglabel )
{
  if ( select.isEmpty() )
    return;
  KMCommand *command = new KMSetTagCommand( QList<QString>()<<taglabel,select, KMSetTagCommand::Toggle );
  command->start();
}


void KMMainWidget::slotSelectMoreMessageTagList()
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  
  TagSelectDialog dlg( this, selectedMessages.count(), selectedMessages.first() );
  if ( dlg.exec() ) {
    const QList<QString> lst = dlg.selectedTag();
  
    KMCommand *command = new KMSetTagCommand( lst, selectedMessages, KMSetTagCommand::CleanExistingAndAddNew );
    command->start();
  }    
}


void KMMainWidget::slotUpdateMessageTagList( const QString &taglabel )
{
  // Create a persistent set from the current thread.
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  toggleMessageSetTag( selectedMessages, taglabel );
}

void KMMainWidget::refreshMessageListSelection()
{
  mAkonadiStandardActionManager->setItemSelectionModel( mMessagePane->currentItemSelectionModel() );
}

//-----------------------------------------------------------------------------
// Status setting for threads
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....
void KMMainWidget::setMessageSetStatus( const QList<Akonadi::Item> &select,
        const Akonadi::MessageStatus &status,
        bool toggle )
{
  KMCommand *command = new KMSetStatusCommand( status, select, toggle );
  command->start();
}

void KMMainWidget::setCurrentThreadStatus( const Akonadi::MessageStatus &status, bool toggle )
{
  const QList<Akonadi::Item> select = mMessagePane->currentThreadAsMessageList();
  if ( select.isEmpty() )
    return;
  setMessageSetStatus( select, status, toggle );
}

void KMMainWidget::slotSetThreadStatusUnread()
{
  setCurrentThreadStatus( MessageStatus::statusRead(), true );
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

//-----------------------------------------------------------------------------
void KMMainWidget::slotRedirectMsg()
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;
  KMCommand *command = new KMRedirectCommand( this, msg );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyToMsg( const QString &tmpl )
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;

  const QString text = mMsgView ? mMsgView->copyText() : "";

  kDebug() << "Reply with template:" << tmpl;

  KMCommand *command = new KMReplyCommand( this,
                                           msg,
                                           MessageComposer::ReplySmart, 
                                           text, false, 
                                           tmpl );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyAllToMsg( const QString &tmpl )
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;

  const QString text = mMsgView? mMsgView->copyText() : "";

  kDebug() << "Reply to All with template:" << tmpl;

  KMCommand *command = new KMReplyCommand(this,
                                          msg,
                                          MessageComposer::ReplyAll, 
                                          text,
                                          false, 
                                          tmpl
                                          );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomForwardMsg( const QString &tmpl )
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;

  kDebug() << "Forward with template:" << tmpl;
  KMForwardCommand * command = new KMForwardCommand(
      this, selectedMessages, mCurrentFolder->identity(), tmpl
    );

  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNoQuoteReplyToMsg()
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;

  KMCommand *command = new KMReplyCommand( this, msg, MessageComposer::ReplySmart, QString(), true );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubjectFilter()
{
  const KMime::Message::Ptr msg = mMessagePane->currentMessage();
  if ( !msg )
    return;

  KMCommand *command = new KMFilterCommand( "Subject", msg->subject()->asUnicodeString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailingListFilter()
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;

  KMCommand *command = new KMMailingListFilterCommand( this, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFromFilter()
{
  KMime::Message::Ptr msg = mMessagePane->currentMessage();
  if ( !msg )
    return;

  AddrSpecList al = MessageHelper::extractAddrSpecs( msg, "From" );
  KMCommand *command;
  if ( al.empty() )
    command = new KMFilterCommand( "From",  msg->from()->asUnicodeString() );
  else
    command = new KMFilterCommand( "From",  al.front().asString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToFilter()
{
  KMime::Message::Ptr msg = mMessagePane->currentMessage();
  if ( !msg )
    return;
  KMCommand *command = new KMFilterCommand( "To",  msg->to()->asUnicodeString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateListFilterAction()
{
  //Proxy the mListFilterAction to update the action text

  mListFilterAction->setText( i18n("Filter on Mailing-List...") );
  KMime::Message::Ptr msg = mMessagePane->currentMessage();
  if ( !msg ) {
    mListFilterAction->setEnabled( false );
    return;
  }

  QByteArray name;
  QString value;
  const QString lname = MailingList::name( msg, name, value );
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
  selectFromAllFoldersDialog()->setCaption( i18n( "Jump to Folder") );
  if ( selectFromAllFoldersDialog()->exec() && selectFromAllFoldersDialog() ) {
    Akonadi::Collection collection = selectFromAllFoldersDialog()->selectedCollection();
    if ( collection.isValid() ) {
      slotSelectCollectionFolder( collection );
    }
  }
}

void KMMainWidget::slotSelectCollectionFolder( const Akonadi::Collection & col )
{
  if ( mFolderTreeWidget ) {
    mFolderTreeWidget->selectCollectionFolder( col );
    slotFolderChanged( col );
  }
}

void KMMainWidget::slotApplyFilters()
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  applyFilters( selectedMessages );
}

void KMMainWidget::applyFilters( const QList<Akonadi::Item>& selectedMessages )
{
#ifndef QT_NO_CURSOR
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif  

  MailCommon::FilterManager::instance()->filter( selectedMessages );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckVacation()
{
  updateVacationScriptStatus( false );
  if ( !kmkernel->askToGoOnline() )
    return;

  Vacation *vac = new Vacation( this, true /* check only */ );
  connect( vac, SIGNAL(scriptActive(bool)), SLOT(updateVacationScriptStatus(bool)) );
  connect( vac, SIGNAL(requestEditVacation()), SLOT(slotEditVacation()) );
}

void KMMainWidget::slotEditVacation()
{
  if ( !kmkernel->askToGoOnline() ) {
    return;
  }

  if ( mVacation )
    return;

  mVacation = new Vacation( this );
  connect( mVacation, SIGNAL(scriptActive(bool)), SLOT(updateVacationScriptStatus(bool)) );
  connect( mVacation, SIGNAL(requestEditVacation()), SLOT(slotEditVacation()) );
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

  mSieveDebugDialog = new KSieveUi::SieveDebugDialog( this );
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
    kDebug() << "\nslotStartCertManager(): certificate manager started.";
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
void KMMainWidget::slotConfigChanged()
{
  readConfig();
  mMsgActions->setupForwardActions();
  mMsgActions->setupForwardingActionsList( mGUIClient );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveMsg()
{
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this, selectedMessages );
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
  const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
  if ( selectedMessages.isEmpty() )
    return;

  // Avoid re-downloading in the common case that only one message is selected, and the message
  // is also displayed in the viewer. For this, create a dummy item without a parent collection / item id,
  // so that KMCommand doesn't download it.
  KMSaveAttachmentsCommand *saveCommand = 0;
  if ( mMsgView && selectedMessages.size() == 1 &&
       mMsgView->message().hasPayload<KMime::Message::Ptr>() &&
       selectedMessages.first().id() == mMsgView->message().id() ) {
    Akonadi::Item dummyItem;
    dummyItem.setPayload<KMime::Message::Ptr>( mMsgView->message().payload<KMime::Message::Ptr>() );
    saveCommand = new KMSaveAttachmentsCommand( this, dummyItem );
  } else {
    saveCommand = new KMSaveAttachmentsCommand( this, selectedMessages );
  }

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
    slotCheckVacation();
  }
}

void KMMainWidget::slotUpdateOnlineStatus( GlobalSettings::EnumNetworkState::type )
{
  KAction * action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::ToggleWorkOffline );
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online ) {
    action->setText( i18n("Work Offline") );
    action->setIcon( KIcon("user-offline") );
  }
  else {
    action->setText( i18n("Work Online") );
    action->setIcon( KIcon("user-online") );
  }
}

void KMMainWidget::slotNetworkStatusChanged ( Solid::Networking::Status status)
{
  if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return;
    
  if ( status == Solid::Networking::Connected ) {
    BroadcastStatus::instance()->setStatusMsg(i18n("Network connection detected, all network jobs resumed"));
    kmkernel->setAccountStatus(true);
  }
  else {
    BroadcastStatus::instance()->setStatusMsg(i18n("No network connection detected, all network jobs are suspended"));
    kmkernel->setAccountStatus(false);
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

  const QStringList availTransports= MailTransport::TransportManager::self()->transportNames();
  if (availTransports.contains(item->text()))
    kmkernel->msgSender()->sendQueued( item->text() );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowBusySplash()
{
  if ( mReaderWindowActive ) {
    mMsgView->displayBusyPage();
  }
}

void KMMainWidget::showOfflinePage()
{
  if ( !mReaderWindowActive ) return;
  mShowingOfflineScreen = true;

  mMsgView->displayOfflinePage();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplaceMsgByUnencryptedVersion()
{
  kDebug();
  Akonadi::Item oldMsg = mMessagePane->currentItem();
  if( oldMsg.isValid() ) {
#if 0
    kDebug() << "Old message found";
    if( oldMsg->hasUnencryptedMsg() ) {
      kDebug() << "Extra unencrypted message found";
      KMime::Message* newMsg = oldMsg->unencryptedMsg();
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
      kDebug() << "Adding unencrypted message to folder";
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
        kDebug() << "Deleting encrypted message";
        mFolder->take( idx );
      }

      kDebug() << "Updating message actions";
      updateMessageActions();

      kDebug() << "Done.";
    } else
      kDebug() << "NO EXTRA UNENCRYPTED MESSAGE FOUND";
#else
   kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  } else
    kDebug() << "PANIC: NO OLD MESSAGE FOUND";
}

void KMMainWidget::slotFocusOnNextMessage()
{
  mMessagePane->focusNextMessageItem(MessageList::Core::MessageTypeAny, true,false );
}

void KMMainWidget::slotFocusOnPrevMessage()
{
  mMessagePane->focusPreviousMessageItem( MessageList::Core::MessageTypeAny, true, false );
}

void KMMainWidget::slotSelectFocusedMessage()
{
  mMessagePane->selectFocusedMessageItem(true );
}

void KMMainWidget::slotSelectNextMessage()
{
  mMessagePane->selectNextMessageItem( MessageList::Core::MessageTypeAny,
                                       MessageList::Core::ClearExistingSelection,
                                       true, true );
}

void KMMainWidget::slotExtendSelectionToNextMessage()
{
  mMessagePane->selectNextMessageItem(
                                      MessageList::Core::MessageTypeAny,
                                      MessageList::Core::GrowOrShrinkExistingSelection,
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
  if ( !mMessagePane->selectNextMessageItem(
      MessageList::Core::MessageTypeUnreadOnly,
      MessageList::Core::ClearExistingSelection,
      true,  // center item
      /*GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder*/
      GlobalSettings::self()->loopOnGotoUnread() != GlobalSettings::EnumLoopOnGotoUnread::DontLoop
    ) )
  {
    // no next unread message was found in the current folder
    if ( ( GlobalSettings::self()->loopOnGotoUnread() ==
           GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders ) ||
         ( GlobalSettings::self()->loopOnGotoUnread() ==
           GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders ) )
    {
      mGoToFirstUnreadMessageInSelectedFolder = true;
      mFolderTreeWidget->folderTreeView()->selectNextUnreadFolder( true );
      mGoToFirstUnreadMessageInSelectedFolder = false;
    }
  }
}

void KMMainWidget::slotSelectPreviousMessage()
{
  mMessagePane->selectPreviousMessageItem( MessageList::Core::MessageTypeAny,
                                           MessageList::Core::ClearExistingSelection,
                                           true, true );
}

void KMMainWidget::slotExtendSelectionToPreviousMessage()
{
  mMessagePane->selectPreviousMessageItem(
      MessageList::Core::MessageTypeAny,
      MessageList::Core::GrowOrShrinkExistingSelection,
      true,  // center item
      false  // don't loop in folder
    );
}

void KMMainWidget::slotSelectPreviousUnreadMessage()
{
  if ( !mMessagePane->selectPreviousMessageItem(
      MessageList::Core::MessageTypeUnreadOnly,
      MessageList::Core::ClearExistingSelection,
      true,  // center item
      GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder
    ) )
  {
    // no next unread message was found in the current folder
    if ( ( GlobalSettings::self()->loopOnGotoUnread() ==
           GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders ) ||
         ( GlobalSettings::self()->loopOnGotoUnread() ==
           GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders ) )
    {
      mGoToFirstUnreadMessageInSelectedFolder = true;
      mFolderTreeWidget->folderTreeView()->selectPrevUnreadFolder();
      mGoToFirstUnreadMessageInSelectedFolder = false;
    }
  }
}

void KMMainWidget::slotDisplayCurrentMessage()
{
  if ( mMessagePane->currentItem().isValid() )
    slotMessageActivated( mMessagePane->currentItem() );
}


void KMMainWidget::slotMessageActivated( const Akonadi::Item &msg )
{
  if ( !mCurrentFolder || !msg.isValid() )
    return;

  if ( CommonKernel->folderIsDraftOrOutbox( mCurrentFolder->collection() ) )
  {
    mMsgActions->setCurrentMessage( msg );
    mMsgActions->editCurrentMessage();
    return;
  }

  if ( CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) ) {
    slotUseTemplate();
    return;
  }

  ItemFetchJob *itemFetchJob = MessageViewer::Viewer::createFetchJob( msg );
  connect( itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
           SLOT(slotItemsFetchedForActivation(Akonadi::Item::List)) );
  connect( itemFetchJob, SIGNAL(result(KJob*)),
           SLOT(itemsFetchDone(KJob*)) );
}

void KMMainWidget::slotItemsFetchedForActivation( const Akonadi::Item::List &list )
{
  Q_ASSERT( list.size() == 1 );

  const Item msg = list.first();

  KMReaderMainWin *win = new KMReaderMainWin( mFolderHtmlPref, mFolderHtmlLoadExtPref );
  const bool useFixedFont = mMsgView ? mMsgView->isFixedFont() :
                            MessageViewer::GlobalSettings::self()->useFixedFont();
  win->setUseFixedFont( useFixedFont );


  const Akonadi::Collection parentCollection = MailCommon::Util::parentCollectionFromItem(msg);  
  win->showMessage( overrideEncoding(), msg, parentCollection );
  win->show();
}

void KMMainWidget::slotMessageStatusChangeRequest( const Akonadi::Item &item, const Akonadi::MessageStatus & set, const Akonadi::MessageStatus &clear )
{
  if ( !item.isValid() )
    return;

  if ( clear.toQInt32() != Akonadi::MessageStatus().toQInt32() )
  {
    KMCommand *command = new KMSetStatusCommand( clear, Akonadi::Item::List() << item, true );
    command->start();
  }

  if ( set.toQInt32() != Akonadi::MessageStatus().toQInt32() )
  {
    KMCommand *command = new KMSetStatusCommand( set, Akonadi::Item::List() << item, false );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAll()
{
  mMessagePane->selectAll();
  updateMessageActions();
}

void KMMainWidget::slotMessagePopup(const Akonadi::Item&msg ,const KUrl&aUrl,const QPoint& aPoint)
{
  updateMessageMenu();
  mUrlCurrent = aUrl;

  const QString email =  KPIMUtils::firstEmailAddress( aUrl.path() );
  Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
  job->setLimit( 1 );
  job->setQuery( Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch );
  job->setProperty( "msg", QVariant::fromValue( msg ) );
  job->setProperty( "point", aPoint );

  connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedMessagePopup(KJob*)) );
}

void KMMainWidget::slotDelayedMessagePopup( KJob *job )
{
  const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
  const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

#if 0 //TODO port it
  mMessageListView->activateMessage( &msg ); // make sure that this message is the active one

  // If this assertion fails then our current KMReaderWin is displaying a
  // message from a folder that is not the current in mMessageListView.
  // This should never happen as all the actions are messed up in this case.
  Q_ASSERT( &msg == mMessageListView->currentMessage() );

#endif

  const Akonadi::Item msg = job->property( "msg" ).value<Akonadi::Item>();
  const QPoint aPoint = job->property( "point" ).toPoint();

  KMenu *menu = new KMenu;

  bool urlMenuAdded = false;

  if ( !mUrlCurrent.isEmpty() ) {
    if ( mUrlCurrent.protocol() == QLatin1String( "mailto" ) ) {
      // popup on a mailto URL
      menu->addAction( mMsgView->mailToComposeAction() );
      menu->addAction( mMsgView->mailToReplyAction() );
      menu->addAction( mMsgView->mailToForwardAction() );

      menu->addSeparator();

      if ( contactAlreadyExists ) {
        menu->addAction( mMsgView->openAddrBookAction() );
      } else {
        menu->addAction( mMsgView->addAddrBookAction() );
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
    kDebug() << "URL is:" << mUrlCurrent;
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
    if (!mMessagePane->currentMessage()) {
      // no messages
      delete menu;
      return;
    }
    Akonadi::Collection parentCol = msg.parentCollection();
    if ( parentCol.isValid() && CommonKernel->folderIsTemplates( parentCol) ) {
      menu->addAction( mUseAction );
    } else {
      menu->addAction( mMsgActions->replyMenu() );
      menu->addAction( mMsgActions->forwardMenu() );
    }
    if( parentCol.isValid() && CommonKernel->folderIsSentMailFolder( parentCol ) ) {
      menu->addAction( sendAgainAction() );
    } else {
      menu->addAction( editAction() );
    }
    menu->addAction( mailingListActionMenu() );
    menu->addSeparator();

    menu->addAction( mCopyActionMenu );
    menu->addAction( mMoveActionMenu );

    menu->addSeparator();

    menu->addAction( mMsgActions->messageStatusMenu() );
    menu->addSeparator();

    menu->addAction( viewSourceAction() );
    if ( mMsgView ) {
      menu->addAction( mMsgView->toggleFixFontAction() );
      menu->addAction( mMsgView->toggleMimePartTreeAction() );
    }
    menu->addSeparator();
    menu->addAction( mMsgActions->printAction() );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAttachmentsAction );

    menu->addSeparator();
    if ( parentCol.isValid() && CommonKernel->folderIsTrash(parentCol) ) {
      menu->addAction( mDeleteAction );
    } else {
      menu->addAction( akonadiStandardAction( Akonadi::StandardMailActionManager::MoveToTrash ) );
    }
    menu->addSeparator();
    menu->addAction( mMsgActions->createTodoAction() );
    menu->addAction( mMsgActions->annotateAction() );
  }
  KAcceleratorManager::manage(menu);
  menu->exec( aPoint, 0 );
  delete menu;
}
//-----------------------------------------------------------------------------
void KMMainWidget::getAccountMenu()
{
  mActMenu->clear();
  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach ( const Akonadi::AgentInstance& type, lst )
  {
    // Explicitly make a copy, as we're not changing values of the list but only
    // the local copy which is passed to action.
    QAction* action = mActMenu->addAction( QString( type.name() ).replace('&', "&&") );
    action->setData( type.identifier() );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::getTransportMenu()
{

  mSendMenu->clear();
  QStringList availTransports = MailTransport::TransportManager::self()->transportNames();
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
    connect( mCustomTemplateMenus, SIGNAL(replyTemplateSelected(QString)),
             this, SLOT(slotCustomReplyToMsg(QString)) );
    connect( mCustomTemplateMenus, SIGNAL(replyAllTemplateSelected(QString)),
             this, SLOT(slotCustomReplyAllToMsg(QString)) );
    connect( mCustomTemplateMenus, SIGNAL(forwardTemplateSelected(QString)),
             this, SLOT(slotCustomForwardMsg(QString)) );
    connect( KMKernel::self(), SIGNAL(customTemplatesChanged()), mCustomTemplateMenus, SLOT(update()) );
  }

  mMsgActions->forwardMenu()->addSeparator();
  mMsgActions->forwardMenu()->addAction( mCustomTemplateMenus->forwardActionMenu() );

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
  connect(mSaveAsAction, SIGNAL(triggered(bool)), SLOT(slotSaveMsg()));
  mSaveAsAction->setShortcut(KStandardShortcut::save());

  mOpenAction = KStandardAction::open( this, SLOT(slotOpenMsg()),
                                  actionCollection() );

  {
    KAction *action = new KAction(i18n("&Expire All Folders"), this);
    actionCollection()->addAction("expire_all_folders", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotExpireAll()));
  }
  {
    KAction *action = new KAction(KIcon("mail-receive"), i18n("Check &Mail"), this);
    actionCollection()->addAction("check_mail", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCheckMail()));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_L));
  }

  KActionMenu *actActionMenu = new KActionMenu(KIcon("mail-receive"), i18n("Check Mail In"), this);
  actActionMenu->setIconText( i18n("Check Mail") );
  actActionMenu->setToolTip( i18n("Check Mail") );
  actionCollection()->addAction("check_mail_in", actActionMenu );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"
  connect(actActionMenu, SIGNAL(triggered(bool)), this, SLOT(slotCheckMail()));
  mActMenu = actActionMenu->menu();
  connect(mActMenu, SIGNAL(triggered(QAction*)),
          SLOT(slotCheckOneAccount(QAction*)));
  connect(mActMenu, SIGNAL(aboutToShow()), SLOT(getAccountMenu()));

  {
    mSendQueued = new KAction(KIcon("mail-send"), i18n("&Send Queued Messages"), this);
    actionCollection()->addAction("send_queued", mSendQueued );
    connect(mSendQueued, SIGNAL(triggered(bool)), SLOT(slotSendQueued()));
  }
  {

    KAction * action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::ToggleWorkOffline );
    mAkonadiStandardActionManager->interceptAction( Akonadi::StandardActionManager::ToggleWorkOffline );
    connect( action, SIGNAL(triggered(bool)), this, SLOT(slotOnlineStatus()) );
    action->setText( i18n("Online status (unknown)") );
  }

  KActionMenu *sendActionMenu = new KActionMenu(KIcon("mail-send-via"), i18n("Send Queued Messages Via"), this);
  actionCollection()->addAction("send_queued_via", sendActionMenu );
  sendActionMenu->setDelayed(true);

  mSendMenu = sendActionMenu->menu();
  connect(mSendMenu,SIGNAL(triggered(QAction*)), SLOT(slotSendQueuedVia(QAction*)));
  connect(mSendMenu,SIGNAL(aboutToShow()),SLOT(getTransportMenu()));

  //----- Tools menu
  if (parent()->inherits("KMMainWin")) {
    KAction *action = new KAction(KIcon("x-office-address-book"), i18n("&Address Book"), this);
    actionCollection()->addAction("addressbook", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotAddrBook()));
    if (KStandardDirs::findExe("kaddressbook").isEmpty())
      action->setEnabled(false);
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
    connect(action, SIGNAL(triggered(bool)), SLOT(slotDebugSieve()));
  }
#endif

  {
    KAction *action = new KAction(i18n("Filter &Log Viewer..."), this);
    actionCollection()->addAction("filter_log_viewer", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotFilterLogViewer()));
  }
  {
    KAction *action = new KAction(i18n("&Anti-Spam Wizard..."), this);
    actionCollection()->addAction("antiSpamWizard", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotAntiSpamWizard()));
  }
  {
    KAction *action = new KAction(i18n("&Anti-Virus Wizard..."), this);
    actionCollection()->addAction("antiVirusWizard", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotAntiVirusWizard()));
  }
  {
    KAction *action = new KAction( i18n("&Account Wizard..."), this );
    actionCollection()->addAction( "accountWizard", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotAccountWizard()) );
  }
  if ( KSieveUi::Util::allowOutOfOfficeSettings() )
  {
    KAction *action = new KAction( i18n("Edit \"Out of Office\" Replies..."), this );
    actionCollection()->addAction( "tools_edit_vacation", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotEditVacation()) );
  }

  // Disable the standard action delete key sortcut.
  KAction* const standardDelAction = akonadiStandardAction(  Akonadi::StandardActionManager::DeleteItems );
  standardDelAction->setShortcut( QKeySequence() );

  //----- Edit Menu

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
  mTrashThreadAction->setHelpText(i18n("Move thread to trashcan") );
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
  mFindInMessageAction->setShortcut(KStandardShortcut::find());

  {
    KAction *action = new KAction(i18n("Select &All Messages"), this);
    actionCollection()->addAction("mark_all_messages", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotMarkAll()));
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_A ) );
  }

  //----- Folder Menu

  mFolderMailingListPropertiesAction = new KAction(i18n("&Mailing List Management..."), this);
  actionCollection()->addAction("folder_mailinglist_properties", mFolderMailingListPropertiesAction );
  connect(mFolderMailingListPropertiesAction, SIGNAL(triggered(bool)), SLOT(slotFolderMailingListProperties()));
  // mFolderMailingListPropertiesAction->setIcon(KIcon("document-properties-mailing-list"));

  mShowFolderShortcutDialogAction = new KAction(KIcon("configure-shortcuts"), i18n("&Assign Shortcut..."), this);
  actionCollection()->addAction("folder_shortcut_command", mShowFolderShortcutDialogAction );
  connect( mShowFolderShortcutDialogAction, SIGNAL(triggered(bool)),
           SLOT(slotShowFolderShortcutDialog()) );

  // FIXME: this action is not currently enabled in the rc file, but even if
  // it were there is inconsistency between the action name and action.
  // "Expiration Settings" implies that this will lead to a settings dialogue
  // and it should be followed by a "...", but slotExpireFolder() performs
  // an immediate expiry.
  //
  // TODO: expire action should be disabled if there is no content or if
  // the folder can't delete messages.
  //
  // Leaving the action here for the moment, it and the "Expire" option in the
  // folder popup menu should be combined or at least made consistent.  Same for
  // slotExpireFolder() and FolderViewItem::slotShowExpiryProperties().
  mExpireFolderAction = new KAction(i18n("&Expiration Settings"), this);
  actionCollection()->addAction("expire", mExpireFolderAction );
  connect(mExpireFolderAction, SIGNAL(triggered(bool)), SLOT(slotExpireFolder()));


  mAkonadiStandardActionManager->interceptAction( Akonadi::StandardMailActionManager::MoveToTrash );
  connect( mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveToTrash ), SIGNAL(triggered(bool)), this, SLOT(slotTrashSelectedMessages()) );


  mAkonadiStandardActionManager->interceptAction( Akonadi::StandardMailActionManager::MoveAllToTrash );
  connect( mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveAllToTrash ), SIGNAL(triggered(bool)), this, SLOT(slotEmptyFolder()) );

  mAkonadiStandardActionManager->interceptAction( Akonadi::StandardActionManager::DeleteCollections );
  connect( mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::DeleteCollections ), SIGNAL(triggered(bool)), this, SLOT(slotRemoveFolder()) );

  // ### PORT ME: Add this to the context menu. Not possible right now because
  //              the context menu uses XMLGUI, and that would add the entry to
  //              all collection context menus
  mArchiveFolderAction = new KAction( i18n( "&Archive Folder..." ), this );
  actionCollection()->addAction( "archive_folder", mArchiveFolderAction );
  connect( mArchiveFolderAction, SIGNAL(triggered(bool)), SLOT(slotArchiveFolder()) );

  mPreferHtmlAction = new KToggleAction(i18n("Prefer &HTML to Plain Text"), this);
  actionCollection()->addAction("prefer_html", mPreferHtmlAction );
  connect(mPreferHtmlAction, SIGNAL(triggered(bool)), SLOT(slotOverrideHtml()));

  mPreferHtmlLoadExtAction = new KToggleAction(i18n("Load E&xternal References"), this);
  actionCollection()->addAction("prefer_html_external_refs", mPreferHtmlLoadExtAction );
  connect(mPreferHtmlLoadExtAction, SIGNAL(triggered(bool)), SLOT(slotOverrideHtmlLoadExt()));

  {
    KAction *action =  mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CopyCollections);
    action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_C));
  }
  {
    KAction *action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::Paste);
    action->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL+Qt::Key_V));
  }
  {
    KAction *action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CopyItems);
    action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_C));
  }
  {
    KAction *action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CutItems );
    action->setShortcut(QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_X));
  }

  {
    KAction *action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CopyItemToMenu );
    action->setText(i18n("Copy Message To...") );
    action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::MoveItemToMenu );
    action->setText(i18n("Move Message To...") );
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
  connect( mTemplateMenu->menu(), SIGNAL(aboutToShow()), this,
           SLOT(slotShowNewFromTemplate()) );
  connect( mTemplateMenu->menu(), SIGNAL(triggered(QAction*)), this,
           SLOT(slotNewFromTemplate(QAction*)) );

  mPostToMailinglistAction = new KAction( KIcon( "mail-message-new-list" ),
                                          i18n( "New Message t&o Mailing-List..." ),
                                          this );
  actionCollection()->addAction("post_message", mPostToMailinglistAction );
  connect( mPostToMailinglistAction, SIGNAL(triggered(bool)),
           SLOT(slotPostToML()) );
  mPostToMailinglistAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_N ) );

  mSendAgainAction = new KAction(i18n("Send A&gain..."), this);
  actionCollection()->addAction("send_again", mSendAgainAction );
  connect(mSendAgainAction, SIGNAL(triggered(bool)), SLOT(slotResendMsg()));

  //----- Create filter actions
  mFilterMenu = new KActionMenu(KIcon("view-filter"), i18n("&Create Filter"), this);
  actionCollection()->addAction("create_filter", mFilterMenu );
  connect( mFilterMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotFilter()) );
  mSubjectFilterAction = new KAction(i18n("Filter on &Subject..."), this);
  actionCollection()->addAction("subject_filter", mSubjectFilterAction );
  connect(mSubjectFilterAction, SIGNAL(triggered(bool)), SLOT(slotSubjectFilter()));
  mFilterMenu->addAction( mSubjectFilterAction );

  mFromFilterAction = new KAction(i18n("Filter on &From..."), this);
  actionCollection()->addAction("from_filter", mFromFilterAction );
  connect(mFromFilterAction, SIGNAL(triggered(bool)), SLOT(slotFromFilter()));
  mFilterMenu->addAction( mFromFilterAction );

  mToFilterAction = new KAction(i18n("Filter on &To..."), this);
  actionCollection()->addAction("to_filter", mToFilterAction );
  connect(mToFilterAction, SIGNAL(triggered(bool)), SLOT(slotToFilter()));
  mFilterMenu->addAction( mToFilterAction );

  mListFilterAction = new KAction(i18n("Filter on Mailing-&List..."), this);
  actionCollection()->addAction("mlist_filter", mListFilterAction );
  connect(mListFilterAction, SIGNAL(triggered(bool)), SLOT(slotMailingListFilter()));
  mFilterMenu->addAction( mListFilterAction );

  mUseAction = new KAction( KIcon("document-new"), i18n("New Message From &Template"), this );
  actionCollection()->addAction("use_template", mUseAction);
  connect(mUseAction, SIGNAL(triggered(bool)), SLOT(slotUseTemplate()));
  mUseAction->setShortcut(QKeySequence(Qt::Key_N));

  //----- "Mark Thread" submenu
  mThreadStatusMenu = new KActionMenu(i18n("Mark &Thread"), this);
  actionCollection()->addAction("thread_status", mThreadStatusMenu );

  mMarkThreadAsReadAction = new KAction(KIcon("mail-mark-read"), i18n("Mark Thread as &Read"), this);
  actionCollection()->addAction("thread_read", mMarkThreadAsReadAction );
  connect(mMarkThreadAsReadAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusRead()));
  mMarkThreadAsReadAction->setHelpText(i18n("Mark all messages in the selected thread as read"));
  mThreadStatusMenu->addAction( mMarkThreadAsReadAction );

  mMarkThreadAsUnreadAction = new KAction(KIcon("mail-mark-unread"), i18n("Mark Thread as &Unread"), this);
  actionCollection()->addAction("thread_unread", mMarkThreadAsUnreadAction );
  connect(mMarkThreadAsUnreadAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusUnread()));
  mMarkThreadAsUnreadAction->setHelpText(i18n("Mark all messages in the selected thread as unread"));
  mThreadStatusMenu->addAction( mMarkThreadAsUnreadAction );

  mThreadStatusMenu->addSeparator();

  //----- "Mark Thread" toggle actions
  mToggleThreadImportantAction = new KToggleAction(KIcon("mail-mark-important"), i18n("Mark Thread as &Important"), this);
  actionCollection()->addAction("thread_flag", mToggleThreadImportantAction );
  connect(mToggleThreadImportantAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusImportant()));
  mToggleThreadImportantAction->setCheckedState( KGuiItem(i18n("Remove &Important Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadImportantAction );

  mToggleThreadToActAction = new KToggleAction(KIcon("mail-mark-task"), i18n("Mark Thread as &Action Item"), this);
  actionCollection()->addAction("thread_toact", mToggleThreadToActAction );
  connect(mToggleThreadToActAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusToAct()));
  mToggleThreadToActAction->setCheckedState( KGuiItem(i18n("Remove &Action Item Thread Mark")) );
  mThreadStatusMenu->addAction( mToggleThreadToActAction );

  //------- "Watch and ignore thread" actions
  mWatchThreadAction = new KToggleAction(KIcon("mail-thread-watch"), i18n("&Watch Thread"), this);
  actionCollection()->addAction("thread_watched", mWatchThreadAction );
  connect(mWatchThreadAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusWatched()));

  mIgnoreThreadAction = new KToggleAction(KIcon("mail-thread-ignored"), i18n("&Ignore Thread"), this);
  actionCollection()->addAction("thread_ignored", mIgnoreThreadAction );
  connect(mIgnoreThreadAction, SIGNAL(triggered(bool)), SLOT(slotSetThreadStatusIgnored()));

  mThreadStatusMenu->addSeparator();
  mThreadStatusMenu->addAction( mWatchThreadAction );
  mThreadStatusMenu->addAction( mIgnoreThreadAction );

  mSaveAttachmentsAction = new KAction(KIcon("mail-attachment"), i18n("Save A&ttachments..."), this);
  actionCollection()->addAction("file_save_attachments", mSaveAttachmentsAction );
  connect(mSaveAttachmentsAction, SIGNAL(triggered(bool)), SLOT(slotSaveAttachments()));

  mMoveActionMenu = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::MoveItemToMenu);

  mCopyActionMenu = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CopyItemToMenu);

  mApplyAllFiltersAction =
    new KAction( KIcon( "view-filter" ), i18n( "Appl&y All Filters" ), this );
  actionCollection()->addAction( "apply_filters", mApplyAllFiltersAction );
  connect( mApplyAllFiltersAction, SIGNAL(triggered(bool)),
           SLOT(slotApplyFilters()) );
  mApplyAllFiltersAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_J ) );

  mApplyFilterActionsMenu = new KActionMenu( i18n( "A&pply Filter" ), this );
  actionCollection()->addAction( "apply_filter_actions", mApplyFilterActionsMenu );

  {
    KAction *action = new KAction(i18nc("View->","&Expand Thread / Group"), this);
    actionCollection()->addAction("expand_thread", action );
    action->setShortcut(QKeySequence(Qt::Key_Period));
    action->setHelpText(i18n("Expand the current thread or group"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotExpandThread()));
  }
  {
    KAction *action = new KAction(i18nc("View->","&Collapse Thread / Group"), this);
    actionCollection()->addAction("collapse_thread", action );
    action->setShortcut(QKeySequence(Qt::Key_Comma));
    action->setHelpText( i18n("Collapse the current thread or group"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCollapseThread()));
  }
  {
    KAction *action = new KAction(i18nc("View->","Ex&pand All Threads"), this);
    actionCollection()->addAction("expand_all_threads", action );
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Period));
    action->setHelpText( i18n("Expand all threads in the current folder"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotExpandAllThreads()));
  }
  {
    KAction *action = new KAction(i18nc("View->","C&ollapse All Threads"), this);
    actionCollection()->addAction("collapse_all_threads", action );
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Comma));
    action->setHelpText( i18n("Collapse all threads in the current folder"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotCollapseAllThreads()));
  }

  
  mViewSourceAction = new KAction(i18n("&View Source"), this);
  actionCollection()->addAction("view_source", mViewSourceAction );
  connect(mViewSourceAction, SIGNAL(triggered(bool)), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(QKeySequence(Qt::Key_V));

  KAction *dukeOfMonmoth = new KAction(i18n("&Display Message"), this);
  actionCollection()->addAction("display_message", dukeOfMonmoth );
  connect(dukeOfMonmoth, SIGNAL(triggered(bool)), SLOT(slotDisplayCurrentMessage()));
  KShortcut shortcut = KShortcut(QKeySequence( Qt::Key_Enter ));
  shortcut.setAlternate( QKeySequence( Qt::Key_Return ) );
  dukeOfMonmoth->setShortcuts( shortcut );

  //----- Go Menu
  {
    KAction *action = new KAction(i18n("&Next Message"), this);
    actionCollection()->addAction("go_next_message", action );
    action->setShortcuts(KShortcut( "N; Right" ));
    action->setHelpText(i18n("Go to the next message"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSelectNextMessage()));
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
    action->setHelpText(i18n("Go to the next unread message"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSelectNextUnreadMessage()));
  }
  {
    KAction *action = new KAction(i18n("&Previous Message"), this);
    actionCollection()->addAction("go_prev_message", action );
    action->setHelpText(i18n("Go to the previous message"));
    action->setShortcuts(KShortcut( "P; Left" ));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSelectPreviousMessage()));
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
    action->setHelpText(i18n("Go to the previous unread message"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSelectPreviousUnreadMessage()));
  }
  {
    KAction *action = new KAction(i18n("Next Unread &Folder"), this);
    actionCollection()->addAction("go_next_unread_folder", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotNextUnreadFolder()));
    action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Plus));
    action->setHelpText(i18n("Go to the next folder with unread messages"));
    KShortcut shortcut = KShortcut(action->shortcuts());
    shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Plus ) );
    action->setShortcuts( shortcut );
  }
  {
    KAction *action = new KAction(i18n("Previous Unread F&older"), this);
    actionCollection()->addAction("go_prev_unread_folder", action );
    action->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Minus));
    action->setHelpText(i18n("Go to the previous folder with unread messages"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotPrevUnreadFolder()));
    KShortcut shortcut = KShortcut(action->shortcuts());
    shortcut.setAlternate( QKeySequence( Qt::CTRL+Qt::Key_Minus ) );
    action->setShortcuts( shortcut );
  }
  {
    KAction *action = new KAction(i18nc("Go->","Next Unread &Text"), this);
    actionCollection()->addAction("go_next_unread_text", action );
    action->setShortcut(QKeySequence(Qt::Key_Space));
    action->setHelpText(i18n("Go to the next unread text"));
    action->setWhatsThis( i18n("Scroll down current message. "
                               "If at end of current message, "
                               "go to next unread message."));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotReadOn()));
  }

  //----- Settings Menu
  {
    KAction *action = new KAction(i18n("Configure &Filters..."), this);
    action->setMenuRole( QAction::NoRole ); // do not move to application menu on OS X
    actionCollection()->addAction("filter", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotFilter()));
  }
  {
    KAction *action = new KAction(i18n("Manage &Sieve Scripts..."), this);
    actionCollection()->addAction("sieveFilters", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotManageSieveScripts()));
  }
  {
    KAction *action = new KAction(KIcon("kmail"), i18n("KMail &Introduction"), this);
    actionCollection()->addAction("help_kmail_welcomepage", action );
    action->setHelpText( i18n("Display KMail's Welcome Page") );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotIntro()));
  }

  // ----- Standard Actions

//  KStandardAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
  {
    KAction *action = new KAction( KIcon("preferences-desktop-notification"),
                                   i18n("Configure &Notifications..."), this );
    action->setMenuRole( QAction::NoRole ); // do not move to application menu on OS X
    actionCollection()->addAction( "kmail_configure_notifications", action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotEditNotifications()));
  }

  {
    KAction *action = new KAction(KIcon("configure"), i18n("&Configure KMail..."), this);
    actionCollection()->addAction("kmail_configure_kmail", action );
    connect(action, SIGNAL(triggered(bool)), kmkernel, SLOT(slotShowConfigurationDialog()));
  }

  {
    mExpireConfigAction = new KAction( i18n( "Expire..." ), this );
    actionCollection()->addAction( "expire_settings",mExpireConfigAction );
    connect( mExpireConfigAction, SIGNAL(triggered(bool)), this, SLOT(slotShowExpiryProperties()) );
  }

  {
    mAddFavoriteFolder = new KAction( KIcon( "bookmark-new" ), i18n( "Add Favorite Folder..." ), this );
    actionCollection()->addAction( "add_favorite_folder", mAddFavoriteFolder );
    connect( mAddFavoriteFolder, SIGNAL(triggered(bool)), this, SLOT(slotAddFavoriteFolder()) );
  }



  actionCollection()->addAction(KStandardAction::Undo,  "kmail_undo", this, SLOT(slotUndo()));

  KStandardAction::tipOfDay( this, SLOT(slotShowTip()), actionCollection() );

  menutimer = new QTimer( this );
  menutimer->setObjectName( "menutimer" );
  menutimer->setSingleShot( true );
  connect( menutimer, SIGNAL(timeout()), SLOT(updateMessageActionsDelayed()) );
  connect( kmkernel->undoStack(),
           SIGNAL(undoStackChanged()), this, SLOT(slotUpdateUndo()));

  updateMessageActions();
  updateCustomTemplateMenus();
  updateFolderMenu();
  mTagActionManager = new KMail::TagActionManager( this, actionCollection(), mMsgActions,
                                                   mGUIClient );
  mFolderShortcutActionManager = new KMail::FolderShortcutActionManager( this, actionCollection() );

}

void KMMainWidget::slotAddFavoriteFolder()
{
  selectFromAllFoldersDialog()->setCaption( i18n("Add Favorite Folder") );
  if ( selectFromAllFoldersDialog()->exec() && selectFromAllFoldersDialog() ) {
    const Akonadi::Collection collection = selectFromAllFoldersDialog()->selectedCollection();
    if ( collection.isValid() ) {
      mFavoritesModel->addCollection( collection );
    }
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEditNotifications()
{
  KMKnotify notifyDlg( this );
  notifyDlg.exec();
}

void KMMainWidget::slotShowExpiryProperties()
{
  if ( mCurrentFolder ) {
     ExpiryPropertiesDialog *dlg = new ExpiryPropertiesDialog( this, mCurrentFolder );
     dlg->show();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReadOn()
{
    if ( !mMsgView )
        return;
    if ( !mMsgView->viewer()->atBottom() ) {
      mMsgView->viewer()->slotJumpDown();
      return;
    }
    slotSelectNextUnreadMessage();
}

void KMMainWidget::slotNextUnreadFolder()
{
  if ( !mFolderTreeWidget )
    return;
  mGoToFirstUnreadMessageInSelectedFolder = true;
  mFolderTreeWidget->folderTreeView()->selectNextUnreadFolder();
  mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotPrevUnreadFolder()
{
  if ( !mFolderTreeWidget )
    return;
  mGoToFirstUnreadMessageInSelectedFolder = true;
  mFolderTreeWidget->folderTreeView()->selectPrevUnreadFolder();
  mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotExpandThread()
{
  mMessagePane->setCurrentThreadExpanded( true );
}

void KMMainWidget::slotCollapseThread()
{
  mMessagePane->setCurrentThreadExpanded( false );
}

void KMMainWidget::slotExpandAllThreads()
{
  // TODO: Make this asynchronous ? (if there is enough demand)
#ifndef QT_NO_CURSOR	
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif  
  mMessagePane->setAllThreadsExpanded( true );
}

void KMMainWidget::slotCollapseAllThreads()
{
  // TODO: Make this asynchronous ? (if there is enough demand)
#ifndef QT_NO_CURSOR
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif  
  mMessagePane->setAllThreadsExpanded( false );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowMsgSrc()
{
  if ( mMsgView ) {
    mMsgView->viewer()->slotShowMessageSource();
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateMessageMenu()
{
  updateMessageActions();
}

void KMMainWidget::startUpdateMessageActionsTimer()
{
  // FIXME: This delay effectively CAN make the actions to be in an incoherent state
  //        Maybe we should mark actions as "dirty" here and check it in every action handler...
  updateMessageActions( true );
  
  menutimer->stop();
  menutimer->start( 500 );
}

void KMMainWidget::updateMessageActions( bool fast )
{
  Akonadi::Item::List selectedItems;
  Akonadi::Item::List selectedVisibleItems;
  bool allSelectedBelongToSameThread = false;
  Akonadi::Item currentMessage;
  if (mCurrentFolder && mCurrentFolder->isValid() &&
       mMessagePane->getSelectionStats( selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread )
     )
  {
    mMsgActions->setCurrentMessage( mMessagePane->currentItem() );
    mMsgActions->setSelectedItem( selectedItems );
    mMsgActions->setSelectedVisibleItems( selectedVisibleItems );

  } else {
    mMsgActions->setCurrentMessage( Akonadi::Item() );
  }

  if( !fast )
    updateMessageActionsDelayed();

}


void KMMainWidget::updateMessageActionsDelayed()
{
  int count;
  Akonadi::Item::List selectedItems;
  Akonadi::Item::List selectedVisibleItems;
  bool allSelectedBelongToSameThread = false;
  Akonadi::Item currentMessage;
  if (mCurrentFolder && mCurrentFolder->isValid() &&
       mMessagePane->getSelectionStats( selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread )
     )
  {
    count = selectedItems.count();

    currentMessage = mMessagePane->currentItem();

  } else {
    count = 0;
    currentMessage = Akonadi::Item();
  }

  //
  // Here we have:
  //
  // - A list of selected messages stored in selectedSernums.
  //   The selected messages might contain some invisible ones as a selected
  //   collapsed node "includes" all the children in the selection.
  // - A list of selected AND visible messages stored in selectedVisibleSernums.
  //   This list does not contain children of selected and collapsed nodes.
  //
  // Now, actions can operate on:
  // - Any set of messages
  //     These are called "mass actions" and are enabled whenever we have a message selected.
  //     In fact we should differentiate between actions that operate on visible selection
  //     and those that operate on the selection as a whole (without considering visibility)...
  // - A single thread
  //     These are called "thread actions" and are enabled whenever all the selected messages belong
  //     to the same thread. If the selection doesn't cover the whole thread then the action
  //     will act on the whole thread anyway (thus will silently extend the selection)
  // - A single message
  //     And we have two sub-cases:
  //     - The selection must contain exactly one message
  //       These actions can't ignore the hidden messages and thus must be disabled if
  //       the selection contains any.
  //     - The selection must contain exactly one visible message
  //       These actions will ignore the hidden message and thus can be enabled if
  //       the selection contains any.
  //

  updateListFilterAction();
  bool readOnly = mCurrentFolder && mCurrentFolder->isValid() && ( mCurrentFolder->rights() & Akonadi::Collection::ReadOnly );
  // can we apply strictly single message actions ? (this is false if the whole selection contains more than one message)
  bool single_actions = count == 1;
  // can we apply loosely single message actions ? (this is false if the VISIBLE selection contains more than one message)
  bool singleVisibleMessageSelected = selectedVisibleItems.count() == 1;
  // can we apply "mass" actions to the selection ? (this is actually always true if the selection is non-empty)
  bool mass_actions = count >= 1;
  // does the selection identify a single thread ?
  bool thread_actions = mass_actions && allSelectedBelongToSameThread && mMessagePane->isThreaded();
  // can we apply flags to the selected messages ?
  bool flags_available = GlobalSettings::self()->allowLocalFlags() || !(mCurrentFolder &&  mCurrentFolder->isValid() ? readOnly : true);

  mThreadStatusMenu->setEnabled( thread_actions );
  // these need to be handled individually, the user might have them
  // in the toolbar
  mWatchThreadAction->setEnabled( thread_actions && flags_available );
  mIgnoreThreadAction->setEnabled( thread_actions && flags_available );
  mMarkThreadAsReadAction->setEnabled( thread_actions );
  mMarkThreadAsUnreadAction->setEnabled( thread_actions );
  mToggleThreadToActAction->setEnabled( thread_actions && flags_available );
  mToggleThreadImportantAction->setEnabled( thread_actions && flags_available );
  bool canDeleteMessages = mCurrentFolder && mCurrentFolder->isValid() && ( mCurrentFolder->rights() & Akonadi::Collection::CanDeleteItem );

  mTrashThreadAction->setEnabled( thread_actions && !readOnly );
  mDeleteThreadAction->setEnabled( thread_actions && canDeleteMessages );

  if ( currentMessage.isValid() )
  {
    MessageStatus status;
    status.setStatusFromFlags( currentMessage.flags() );
    mTagActionManager->updateActionStates ( count, mMessagePane->currentItem() );
    if (thread_actions)
    {
      mToggleThreadToActAction->setChecked( status.isToAct() );
      mToggleThreadImportantAction->setChecked( status.isImportant() );
      mWatchThreadAction->setChecked( status.isWatched() );
      mIgnoreThreadAction->setChecked( status.isIgnored() );
    }
  }

  mMoveActionMenu->setEnabled( mass_actions && canDeleteMessages );
  mMoveMsgToFolderAction->setEnabled( mass_actions && canDeleteMessages );
  //mCopyActionMenu->setEnabled( mass_actions );

  mDeleteAction->setEnabled( mass_actions && !readOnly );
  mExpireConfigAction->setEnabled( canDeleteMessages );

  mFindInMessageAction->setEnabled( mass_actions && !CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );
  mMsgActions->forwardInlineAction()->setEnabled( mass_actions && !CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );
  mMsgActions->forwardAttachedAction()->setEnabled( mass_actions && !CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );
  mMsgActions->forwardMenu()->setEnabled( mass_actions && !CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );

  mMsgActions->editAction()->setEnabled( single_actions );
  mUseAction->setEnabled( single_actions && CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );
  filterMenu()->setEnabled( single_actions );
  mMsgActions->redirectAction()->setEnabled( single_actions && !CommonKernel->folderIsTemplates( mCurrentFolder->collection() ) );

  if ( mCustomTemplateMenus )
  {
    mCustomTemplateMenus->forwardActionMenu()->setEnabled( mass_actions );
    mCustomTemplateMenus->replyActionMenu()->setEnabled( single_actions );
    mCustomTemplateMenus->replyAllActionMenu()->setEnabled( single_actions );
  }

  // "Print" will act on the current message: it will ignore any hidden selection
  mMsgActions->printAction()->setEnabled( singleVisibleMessageSelected );

  // "View Source" will act on the current message: it will ignore any hidden selection
  viewSourceAction()->setEnabled( singleVisibleMessageSelected );

  MessageStatus status;
  status.setStatusFromFlags( currentMessage.flags() );

  QList< QAction *> actionList;
  bool statusSendAgain = single_actions && ( ( currentMessage.isValid() && status.isSent() ) || ( currentMessage.isValid() && CommonKernel->folderIsSentMailFolder( mCurrentFolder->collection() ) ) );
  if ( statusSendAgain ) {
    actionList << mSendAgainAction;
  } else if( single_actions ) {
    actionList << messageActions()->editAction();
  }
  mGUIClient->unplugActionList( QLatin1String( "messagelist_actionlist" ) );
  mGUIClient->plugActionList( QLatin1String( "messagelist_actionlist" ), actionList );
  mSendAgainAction->setEnabled( statusSendAgain );

  mSaveAsAction->setEnabled( mass_actions );

  const bool mails = mCurrentFolder&& mCurrentFolder->isValid() && mCurrentFolder->statistics().count() > 0;
  const bool enable_goto_unread = mails
       || (GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders)
       || (GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders);
  actionCollection()->action( "go_next_message" )->setEnabled( mails );
  actionCollection()->action( "go_next_unread_message" )->setEnabled( enable_goto_unread );
  actionCollection()->action( "go_prev_message" )->setEnabled( mails );
  actionCollection()->action( "go_prev_unread_message" )->setEnabled( enable_goto_unread );
  const qint64 nbMsgOutboxCollection = CommonKernel->outboxCollectionFolder().statistics().count();

  actionCollection()->action( "send_queued" )->setEnabled( nbMsgOutboxCollection > 0 );
  actionCollection()->action( "send_queued_via" )->setEnabled( nbMsgOutboxCollection > 0 );

  slotUpdateOnlineStatus( static_cast<GlobalSettingsBase::EnumNetworkState::type>( GlobalSettings::self()->networkState() ) );
  if (action( "kmail_undo" ))
    action( "kmail_undo" )->setEnabled( kmkernel->undoStack()->size() > 0 );

  // Enable / disable all filters.
  foreach ( QAction *filterAction, mFilterMenuActions ) {
    filterAction->setEnabled( count > 0 );
  }

  mApplyAllFiltersAction->setEnabled( count);
  mApplyFilterActionsMenu->setEnabled( count );
}


void KMMainWidget::slotAkonadiStandardActionUpdated()
{
  const bool multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
  if ( mCollectionProperties ) {
    mCollectionProperties->setEnabled( mCurrentFolder && !multiFolder && 
                                       !mCurrentFolder->isStructural() &&
                                       !MailCommon::Util::isVirtualCollection( mCurrentFolder->collection() ) );
    QList< QAction* > collectionProperties;
    if ( mCollectionProperties->isEnabled() )
      collectionProperties << mCollectionProperties;
    mGUIClient->unplugActionList( "akonadi_collection_collectionproperties_actionlist" );
    mGUIClient->plugActionList( "akonadi_collection_collectionproperties_actionlist", collectionProperties );

  }

  const bool folderWithContent = mCurrentFolder && !mCurrentFolder->isStructural();

  if ( mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::DeleteCollections ) ) {

    mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::DeleteCollections )->setEnabled( mCurrentFolder
                                                                                                            && !multiFolder
                                                                                                            && ( mCurrentFolder->collection().rights() & Collection::CanDeleteCollection )
                                                                                                            && !mCurrentFolder->isSystemFolder()
                                                                                                            && folderWithContent
                                                                                                            && mCurrentFolder->collection().resource() != "akonadi_nepomuktag_resource" );
  }

  if ( mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveAllToTrash ) ) {
    mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveAllToTrash )->setEnabled( folderWithContent
                                                                                                             && ( mCurrentFolder->count() > 0 )
                                                                                                             && mCurrentFolder->canDeleteMessages()
                                                                                                             && !multiFolder );
    mAkonadiStandardActionManager->action( Akonadi::StandardMailActionManager::MoveAllToTrash )->setText( (mCurrentFolder && CommonKernel->folderIsTrash(mCurrentFolder->collection())) ? i18n("E&mpty Trash") : i18n("&Move All Messages to Trash") );
  }


  QList< QAction* > addToFavorite;
  QAction *actionAddToFavoriteCollections = akonadiStandardAction( Akonadi::StandardActionManager::AddToFavoriteCollections );
  if ( actionAddToFavoriteCollections ) {
    if( mEnableFavoriteFolderView && actionAddToFavoriteCollections->isEnabled() )
      addToFavorite << actionAddToFavoriteCollections;
    mGUIClient->unplugActionList( "akonadi_collection_add_to_favorites_actionlist" );
    mGUIClient->plugActionList( "akonadi_collection_add_to_favorites_actionlist", addToFavorite );
  }

  QList< QAction* > syncActionList;
  QAction *actionSync = akonadiStandardAction( Akonadi::StandardActionManager::SynchronizeCollections );
  if ( actionSync && actionSync->isEnabled() ) {
    syncActionList << actionSync;
  }
  actionSync = akonadiStandardAction( Akonadi::StandardActionManager::SynchronizeCollectionsRecursive );
  if ( actionSync && actionSync->isEnabled() ) {
    syncActionList << actionSync;
  }
  mGUIClient->unplugActionList( "akonadi_collection_sync_actionlist" );
  mGUIClient->plugActionList( "akonadi_collection_sync_actionlist", syncActionList );



  QList< QAction* > actionList;

  QAction *action = mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CreateCollection );
  if ( action && action->isEnabled() ) {
    actionList << action;
  }

  action =  mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::MoveCollectionToMenu );
  if ( action && action->isEnabled() ) {
    actionList <<action;
  }

  action =  mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::CopyCollectionToMenu);
  if ( action && action->isEnabled() ) {
    actionList <<action;
  }
  mGUIClient->unplugActionList( "akonadi_collection_move_copy_menu_actionlist" );
  mGUIClient->plugActionList( "akonadi_collection_move_copy_menu_actionlist", actionList );


}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFolderMenu()
{
  const bool folderWithContent = mCurrentFolder && !mCurrentFolder->isStructural();
  const bool multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
  mFolderMailingListPropertiesAction->setEnabled( folderWithContent &&
                                                  !multiFolder &&
                                                  !mCurrentFolder->isSystemFolder() );


  QList< QAction* > actionlist;
  if ( mCurrentFolder && mCurrentFolder->collection().id() == CommonKernel->outboxCollectionFolder().id() &&
      CommonKernel->outboxCollectionFolder().statistics().count() > 0 ) {
    kDebug() << "Enabling send queued";
    actionlist << mSendQueued;
  }
//   if( mCurrentFolder && mCurrentFolder->collection().id() != CommonKernel->trashCollectionFolder().id() ) {
//     actionlist << mTrashAction;
//   }
  mGUIClient->unplugActionList( QLatin1String( "outbox_folder_actionlist" ) );
  mGUIClient->plugActionList( QLatin1String( "outbox_folder_actionlist" ), actionlist );
  actionlist.clear();

  const bool isASearchFolder = mCurrentFolder && mCurrentFolder->collection().resource() == QLatin1String( "akonadi_search_resource" );
   mAkonadiStandardActionManager->action( Akonadi::StandardActionManager::DeleteCollections )->setText( isASearchFolder ? i18n("&Delete Search") : i18n("&Delete Folder") );

  mArchiveFolderAction->setEnabled( mCurrentFolder && !multiFolder && folderWithContent );

  bool isInTrashFolder = (mCurrentFolder && CommonKernel->folderIsTrash(mCurrentFolder->collection()));
  akonadiStandardAction( Akonadi::StandardMailActionManager::MoveToTrash )->setText( isInTrashFolder ? i18nc("@action Hard delete, bypassing trash", "&Delete"): i18n("&Move to Trash") );

  mTrashThreadAction->setText(isInTrashFolder ?i18n("Delete T&hread"): i18n("M&ove Thread to Trash"));



  mExpireConfigAction->setEnabled( mCurrentFolder &&
                                   !mCurrentFolder->isStructural() &&
                                   !multiFolder &&
                                   mCurrentFolder->canDeleteMessages() &&
                                   folderWithContent &&
                                   !MailCommon::Util::isVirtualCollection( mCurrentFolder->collection() ) );

  // the visual ones only make sense if we are showing a message list
  mPreferHtmlAction->setEnabled( mFolderTreeWidget->folderTreeView()->currentFolder().isValid() && !multiFolder );

  mPreferHtmlLoadExtAction->setEnabled( mFolderTreeWidget->folderTreeView()->currentFolder().isValid() && !multiFolder && (mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref) ? true : false );
  mPreferHtmlAction->setChecked( !multiFolder &&  ( mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref ) );
  mPreferHtmlLoadExtAction->setChecked( !multiFolder &&  ( mHtmlLoadExtPref ? !mFolderHtmlLoadExtPref : mFolderHtmlLoadExtPref ) );
  mShowFolderShortcutDialogAction->setEnabled( !multiFolder && folderWithContent );

  actionlist << akonadiStandardAction( Akonadi::StandardActionManager::ManageLocalSubscriptions );

  mGUIClient->unplugActionList( QLatin1String( "collectionview_actionlist" ) );
  mGUIClient->plugActionList( QLatin1String( "collectionview_actionlist" ), actionlist );

}

//-----------------------------------------------------------------------------
void KMMainWidget::slotIntro()
{
  if ( !mMsgView )
    return;

  mMsgView->clear( true );

  // hide widgets that are in the way:
  if ( mMessagePane && mLongFolderList )
    mMessagePane->hide();
  mMsgView->displayAboutPage();

  mCurrentFolder.clear();
}

void KMMainWidget::slotShowStartupFolder()
{
  connect( MailCommon::FilterManager::instance(), SIGNAL(filtersChanged()),
           this, SLOT(initializeFilterActions()) );

  // Plug various action lists. This can't be done in the constructor, as that is called before
  // the main window or Kontact calls createGUI().
  // This function however is called with a single shot timer.
  initializeFilterActions();
  mFolderShortcutActionManager->createActions();
  mTagActionManager->createActions();
  messageActions()->setupForwardingActionsList( mGUIClient );

  QString newFeaturesMD5 = KMReaderWin::newFeaturesMD5();
  if ( kmkernel->firstStart() ||
       GlobalSettings::self()->previousNewFeaturesMD5() != newFeaturesMD5 ) {
    GlobalSettings::self()->setPreviousNewFeaturesMD5( newFeaturesMD5 );
    slotIntro();
    return;
  }
}

void KMMainWidget::slotShowTip()
{
  KTipDialog::showTip( this, QString(), true );
}

QList<KActionCollection*> KMMainWidget::actionCollections() const {
  return QList<KActionCollection*>() << actionCollection();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateUndo()
{
  if ( actionCollection()->action( "kmail_undo" ) ) {
    actionCollection()->action( "kmail_undo" )->setEnabled( kmkernel->undoStack()->size()>0 );
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
void KMMainWidget::initializeFilterActions()
{
  clearFilterActions();
  mApplyFilterActionsMenu->menu()->addAction( mApplyAllFiltersAction );
  bool addedSeparator = false;

  foreach ( MailFilter *filter, MailCommon::FilterManager::instance()->filters() ) {
    if ( !filter->isEmpty() && filter->configureShortcut() && filter->isEnabled() ) {
      QString filterName = QString::fromLatin1( "Filter %1").arg( filter->name() );
      QString normalizedName = filterName.replace(' ', '_');
      if ( action( normalizedName.toUtf8() ) ) {
        continue;
      }
      KMMetaFilterActionCommand *filterCommand = new KMMetaFilterActionCommand( filter->identifier(), this );
      mFilterCommands.append( filterCommand );
      QString displayText = i18n( "Filter %1", filter->name() );
      QString icon = filter->icon();
      if ( icon.isEmpty() ) {
        icon = "system-run";
      }
      KAction *filterAction = new KAction( KIcon( icon ), displayText, actionCollection() );
      filterAction->setIconText( filter->toolbarName() );

      // The shortcut configuration is done in the filter dialog.
      // The shortcut set in the shortcut dialog would not be saved back to
      // the filter settings correctly.
      filterAction->setShortcutConfigurable( false );
      actionCollection()->addAction( normalizedName.toLocal8Bit(),
                                     filterAction );
      connect( filterAction, SIGNAL(triggered(bool)),
               filterCommand, SLOT(start()) );
      filterAction->setShortcuts( filter->shortcut() );
      if ( !addedSeparator ) {
        QAction *a = mApplyFilterActionsMenu->menu()->addSeparator();
        mFilterMenuActions.append( a );
        addedSeparator = true;
      }
      mApplyFilterActionsMenu->menu()->addAction( filterAction );
      mFilterMenuActions.append( filterAction );
      if ( filter->configureToolbar() ) {
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

//-----------------------------------------------------------------------------

QList<QAction*> KMMainWidget::actionList()
{
  return actionCollection()->actions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::toggleSystemTray()
{
  if ( !mSystemTray && GlobalSettings::self()->systemTrayEnabled() ) {
    mSystemTray = new KMSystemTray(this);
  }
  else if ( mSystemTray && !GlobalSettings::self()->systemTrayEnabled() ) {
    // Get rid of system tray on user's request
    kDebug() << "deleting systray";
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
  AntiSpamWizard wiz( AntiSpamWizard::AntiSpam, this );
  wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiVirusWizard()
{
  AntiSpamWizard wiz( AntiSpamWizard::AntiVirus, this);
  wiz.exec();
}
//-----------------------------------------------------------------------------
void KMMainWidget::slotAccountWizard()
{
  KMail::Util::launchAccountWizard( this );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFilterLogViewer()
{
  MailCommon::FilterManager::instance()->showFilterLogDialog();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFileMenu()
{
  const bool isEmpty = MailCommon::Util::agentInstances().isEmpty();
  actionCollection()->action("check_mail")->setEnabled( !isEmpty );
  actionCollection()->action("check_mail_in")->setEnabled( !isEmpty );
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

QSharedPointer<FolderCollection> KMMainWidget::currentFolder() const
{
  return mCurrentFolder;
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
    return MessageCore::GlobalSettings::self()->overrideCharacterEncoding();
}

void KMMainWidget::slotCreateTodo()
{
  const Akonadi::Item msg = mMessagePane->currentItem();
  if ( !msg.isValid() )
    return;

  MailCommon::Util::createTodoFromMail( msg );
}

void KMMainWidget::showEvent( QShowEvent *event )
{
  QWidget::showEvent( event );
  mWasEverShown = true;
}

void KMMainWidget::slotRequestFullSearchFromQuickSearch()
{
  // First, open the search window. If we are currently on a search folder,
  // the search associated with that will be loaded.
  if ( !slotSearch() )
    return;

  assert( mSearchWin );

  // Now we look at the current state of the quick search, and if there's
  // something in there, we add the criteria to the existing search for
  // the search folder, if applicable, or make a new one from it.
  SearchPattern pattern;
  const QString searchString = mMessagePane->currentFilterSearchString();
  if ( !searchString.isEmpty() )
    pattern.append( SearchRule::createInstance( "<message>", SearchRule::FuncContains, searchString ) );
  MessageStatus status = mMessagePane->currentFilterStatus();
  if ( status.hasAttachment() )
  {
    pattern.append( SearchRule::createInstance( "<message>", SearchRule::FuncHasAttachment ) );
    status.setHasAttachment( false );
  }

  if ( !status.isOfUnknownStatus() ) {
    pattern.append( SearchRule::Ptr( new SearchRuleStatus( status ) ) );
  }

  if ( pattern.size() > 0 )
    mSearchWin->addRulesToSearchPattern( pattern );
}

void KMMainWidget::updateVacationScriptStatus( bool active )
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

void KMMainWidget::slotMessageSelected(const Akonadi::Item &item)
{
  delete mShowBusySplashTimer;
  mShowBusySplashTimer = 0;
  if ( mMsgView ) {
    // The current selection was cleared, so we'll remove the previously
    // selected message from the preview pane
    if ( !item.isValid() ) {
      mMsgView->clear();
    } else {
      mShowBusySplashTimer = new QTimer( this );
      mShowBusySplashTimer->setSingleShot( true );
      connect( mShowBusySplashTimer, SIGNAL(timeout()), this, SLOT(slotShowBusySplash()) );
      mShowBusySplashTimer->start( GlobalSettings::self()->folderLoadingTimeout() ); //TODO: check if we need a different timeout setting for this

      Akonadi::ItemFetchJob *itemFetchJob = MessageViewer::Viewer::createFetchJob( item );
      connect( itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
              SLOT(itemsReceived(Akonadi::Item::List)) );
      connect( itemFetchJob, SIGNAL(result(KJob*)), SLOT(itemsFetchDone(KJob*)) );
    }
  }
}

void KMMainWidget::itemsReceived(const Akonadi::Item::List &list )
{
  Q_ASSERT( list.size() == 1 );
  delete mShowBusySplashTimer;
  mShowBusySplashTimer = 0;

  if ( !mMsgView )
    return;

  Item item = list.first();

  if ( mMessagePane ) {
    mMessagePane->show();

    if ( mMessagePane->currentItem() != item ) {
      // The user has selected another email already, so don't render this one.
      // Mark it as read, though, if the user settings say so.
      if ( MessageViewer::GlobalSettings::self()->delayedMarkAsRead() &&
           MessageViewer::GlobalSettings::self()->delayedMarkTime() == 0 ) {
        item.setFlag( Akonadi::MessageFlags::Seen );
        Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item, this );
        modifyJob->setIgnorePayload( true );
      }
      return;
    }
  }

  mMsgView->setMessage( item );
  // reset HTML override to the folder setting
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPref);
  mMsgView->setDecryptMessageOverwrite( false );
}

void KMMainWidget::itemsFetchDone( KJob *job )
{
  delete mShowBusySplashTimer;
  mShowBusySplashTimer = 0;
  if ( job->error() ) {
    kDebug() << job->errorString();
  }
}

KAction *KMMainWidget::akonadiStandardAction( Akonadi::StandardActionManager::Type type )
{
  return mAkonadiStandardActionManager->action( type );
}

KAction *KMMainWidget::akonadiStandardAction( Akonadi::StandardMailActionManager::Type type )
{
  return mAkonadiStandardActionManager->action( type );
}


void KMMainWidget::slotCollectionProperties()
{
  if ( !mCurrentFolder )
    return;

  if ( Solid::Networking::status() == Solid::Networking::Unconnected ) {
    KMessageBox::information( this, i18n( "Network is unconnected, some infos from folder could not be updated." ) );
    slotCollectionPropertiesContinued( 0 );
  } else {
    Akonadi::CollectionAttributesSynchronizationJob *sync
        = new Akonadi::CollectionAttributesSynchronizationJob( mCurrentFolder->collection() );
    sync->setProperty( "collectionId", mCurrentFolder->collection().id() );
    connect( sync, SIGNAL(result(KJob*)),
             this, SLOT(slotCollectionPropertiesContinued(KJob*)) );
    sync->start();
  }
}

void KMMainWidget::slotCollectionPropertiesContinued( KJob* job )
{
  if ( job ) {
    Akonadi::CollectionAttributesSynchronizationJob *sync
        = dynamic_cast<Akonadi::CollectionAttributesSynchronizationJob *>( job );
    Q_ASSERT( sync );
    if ( sync->property( "collectionId" ) != mCurrentFolder->collection().id() )
      return;
  }
  Akonadi::CollectionFetchJob fetch( mCurrentFolder->collection(), Akonadi::CollectionFetchJob::Base );
  fetch.fetchScope().setIncludeStatistics( true );
  fetch.exec();

  const Akonadi::Collection collection = fetch.collections().first();

  const QStringList pages = QStringList() << QLatin1String( "MailCommon::CollectionGeneralPage" )
                                          << QLatin1String( "KMail::CollectionViewPage" )
                                          << QLatin1String( "Akonadi::CachePolicyPage" )
                                          << QLatin1String( "KMail::CollectionTemplatesPage" )
                                          << QLatin1String( "KMail::CollectionAclPage" )
                                          << QLatin1String( "KMail::CollectionQuotaPage" )
                                          << QLatin1String( "KMail::CollectionMaintenancePage" );

  Akonadi::CollectionPropertiesDialog *dlg = new Akonadi::CollectionPropertiesDialog( collection, pages, this );
  dlg->setCaption( i18nc( "@title:window", "Properties of Folder %1", collection.name() ) );
  dlg->resize( 500, 400 );
  dlg->show();
}

