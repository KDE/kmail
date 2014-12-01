/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2002 Don Sanders <sanders@kde.org>
  Copyright (c) 2009, 2010, 2011, 2012 Montel Laurent <montel@kde.org>

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
#include "editor/composer.h"
#include "searchdialog/searchwindow.h"
#include "antispam-virus/antispamwizard.h"
#include "widgets/vacationscriptindicatorwidget.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "customtemplatesmenu.h"
#include "folderselectiondialog.h"
#include "foldertreewidget.h"
#include "util.h"
#include "util/mailutil.h"
#include "kernel/mailkernel.h"
#include "dialog/archivefolderdialog.h"
#include "settings/globalsettings.h"
#include "foldertreeview.h"
#include "tag/tagactionmanager.h"
#include "foldershortcutactionmanager.h"
#include "widgets/collectionpane.h"
#include "manageshowcollectionproperties.h"
#if !defined(NDEBUG)
#include <ksieveui/debug/sievedebugdialog.h>
using KSieveUi::SieveDebugDialog;
#endif

#include "collectionpage/collectionmaintenancepage.h"
#include "collectionpage/collectionquotapage.h"
#include "collectionpage/collectiontemplatespage.h"
#include "collectionpage/collectionshortcutpage.h"
#include "collectionpage/collectionviewpage.h"
#include "collectionpage/collectionmailinglistpage.h"
#include "tag/tagselectdialog.h"
#include "job/createnewcontactjob.h"
#include "folderarchive/folderarchiveutil.h"
#include "folderarchive/folderarchivemanager.h"

#include "pimcommon/acl/collectionaclpage.h"
#include "mailcommon/collectionpage/collectiongeneralpage.h"
#include "mailcommon/collectionpage/collectionexpirypage.h"
#include "mailcommon/collectionpage/expirecollectionattribute.h"
#include "mailcommon/filter/filtermanager.h"
#include "mailcommon/filter/mailfilter.h"
#include "mailcommon/widgets/favoritecollectionwidget.h"
#include "mailcommon/folder/foldertreewidget.h"
#include "mailcommon/folder/foldertreeview.h"
#include "mailcommon/mailcommonsettings_base.h"
#include "kmmainwidget.h"

// Other PIM includes
#include "kdepim-version.h"

#include "messageviewer/utils/autoqpointer.h"
#include "messageviewer/settings/globalsettings.h"
#include "messageviewer/viewer/viewer.h"
#include "messageviewer/viewer/attachmentstrategy.h"
#include "messageviewer/header/headerstrategy.h"
#include "messageviewer/header/headerstyle.h"
#ifndef QT_NO_CURSOR
#include "messageviewer/utils/kcursorsaver.h"
#endif

#include "messagecomposer/sender/messagesender.h"
#include "messagecomposer/helper/messagehelper.h"

#include "templateparser/templateparser.h"

#include "messagecore/settings/globalsettings.h"
#include "messagecore/misc/mailinglist.h"
#include "messagecore/helpers/messagehelpers.h"

#include "dialog/kmknotify.h"
#include "widgets/displaymessageformatactionmenu.h"

#include "ksieveui/vacation/vacationmanager.h"
#include "kmconfigureagent.h"

// LIBKDEPIM includes
#include "progresswidget/progressmanager.h"
#include "misc/broadcaststatus.h"

// KDEPIMLIBS includes
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/collectionattributessynchronizationjob.h>
#include <AkonadiCore/collectionfetchjob.h>
#include <AkonadiCore/collectionfetchscope.h>
#include <Akonadi/Contact/ContactSearchJob>
#include <AkonadiWidgets/collectionpropertiesdialog.h>
#include <AkonadiCore/entitydisplayattribute.h>
#include <AkonadiWidgets/entitylistview.h>
#include <AkonadiWidgets/etmviewstatesaver.h>
#include <AkonadiCore/agentinstance.h>
#include <AkonadiCore/agenttype.h>
#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/session.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/favoritecollectionsmodel.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/itemmodifyjob.h>
#include <AkonadiCore/control.h>
#include <AkonadiWidgets/collectiondialog.h>
#include <AkonadiCore/collectionstatistics.h>
#include <AkonadiWidgets/collectionstatisticsdelegate.h>
#include <AkonadiCore/EntityMimeTypeFilterModel>
#include <Akonadi/KMime/MessageFlags>
#include <Akonadi/KMime/RemoveDuplicatesJob>
#include <AkonadiCore/collectiondeletejob.h>
#include <kdbusconnectionpool.h>
#include <AkonadiCore/CachePolicy>

#include <kidentitymanagement/identity.h>
#include <kidentitymanagement/identitymanager.h>
#include <kpimutils/email.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>
#include <kmime/kmime_mdn.h>
#include <kmime/kmime_header_parsing.h>
#include <kmime/kmime_message.h>
#include <ksieveui/managesievescriptsdialog.h>
#include <ksieveui/util/util.h>

// KDELIBS includes
#include <kwindowsystem.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <QMenu>
#include <kacceleratormanager.h>
#include <kglobalsettings.h>
#include <kstandardshortcut.h>
#include <kshortcutsdialog.h>
#include <kcharsets.h>
#include <qdebug.h>
#include <ktip.h>

#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <knotification.h>
#include <knotifyconfigwidget.h>
#include <kstringhandler.h>
#include <kconfiggroup.h>
#include <ktoolinvocation.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>
#include <QStatusBar>
#include <QAction>
#include <ktreewidgetsearchline.h>
#include <Solid/Networking>
#include <KRecentFilesAction>

// Qt includes
#include <QByteArray>
#include <QHeaderView>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QShortcut>
#include <QProcess>
#include <QDBusConnection>
#include <QTextDocument>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QDir>
// System includes
#include <errno.h> // ugh
#include <AkonadiWidgets/standardactionmanager.h>
#include <KHelpClient>
#include <QStandardPaths>
#include <job/manageserversidesubscriptionjob.h>
#include <job/removeduplicatemailjob.h>

using namespace KMime;
using namespace Akonadi;
using namespace MailCommon;
using KPIM::ProgressManager;
using KPIM::BroadcastStatus;
using KMail::SearchWindow;
using KMail::AntiSpamWizard;
using KMime::Types::AddrSpecList;
using MessageViewer::AttachmentStrategy;

Q_DECLARE_METATYPE(KPIM::ProgressItem *)
Q_DECLARE_METATYPE(Akonadi::Job *)
Q_DECLARE_METATYPE(QPointer<KPIM::ProgressItem>)
Q_GLOBAL_STATIC(KMMainWidget::PtrList, theMainWidgetList)

//-----------------------------------------------------------------------------
KMMainWidget::KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient,
                           KActionCollection *actionCollection, KSharedConfig::Ptr config) :
    QWidget(parent),
    mMoveMsgToFolderAction(0),
    mCollectionProperties(0),
    mFavoriteCollectionsView(0),
    mMsgView(0),
    mSplitter1(0),
    mSplitter2(0),
    mFolderViewSplitter(0),
    mArchiveFolderAction(0),
    mShowBusySplashTimer(0),
    mMsgActions(0),
    mCurrentFolder(0),
    mVacationIndicatorActive(false),
    mGoToFirstUnreadMessageInSelectedFolder(false),
    mDisplayMessageFormatMenu(0),
    mFolderDisplayFormatPreference(MessageViewer::Viewer::UseGlobalSetting),
    mSearchMessages(0),
    mManageShowCollectionProperties(new ManageShowCollectionProperties(this, this))
{
    mConfigAgent = new KMConfigureAgent(this, this);
    // must be the first line of the constructor:
    mStartupDone = false;
    mWasEverShown = false;
    mReaderWindowActive = true;
    mReaderWindowBelow = true;
    mFolderHtmlLoadExtPreference = false;
    mDestructed = false;
    mActionCollection = actionCollection;
    mTopLayout = new QVBoxLayout(this);
    mTopLayout->setMargin(0);
    mConfig = config;
    mGUIClient = aGUIClient;
    mFolderTreeWidget = 0;
    mPreferHtmlLoadExtAction = 0;
    Akonadi::Control::widgetNeedsAkonadi(this);
    mFavoritesModel = 0;
    mVacationManager = new KSieveUi::VacationManager(this);

    // FIXME This should become a line separator as soon as the API
    // is extended in kdelibs.
    mToolbarActionSeparator = new QAction(this);
    mToolbarActionSeparator->setSeparator(true);

    theMainWidgetList->append(this);

    readPreConfig();
    createWidgets();
    setupActions();

    readConfig();

    if (!kmkernel->isOffline()) {   //kmail is set to online mode, make sure the agents are also online
        kmkernel->setAccountStatus(true);
    }

    QTimer::singleShot(0, this, SLOT(slotShowStartupFolder()));

    connect(kmkernel, SIGNAL(startCheckMail()),
            this, SLOT(slotStartCheckMail()));

    connect(kmkernel, SIGNAL(endCheckMail()),
            this, SLOT(slotEndCheckMail()));

    connect(kmkernel, SIGNAL(configChanged()),
            this, SLOT(slotConfigChanged()));

    connect(kmkernel, SIGNAL(onlineStatusChanged(GlobalSettings::EnumNetworkState::type)),
            this, SLOT(slotUpdateOnlineStatus(GlobalSettings::EnumNetworkState::type)));

    connect(mTagActionManager, &KMail::TagActionManager::tagActionTriggered,
            this, &KMMainWidget::slotUpdateMessageTagList);

    connect(mTagActionManager, &KMail::TagActionManager::tagMoreActionClicked,
            this, &KMMainWidget::slotSelectMoreMessageTagList);

    kmkernel->toggleSystemTray();

    {
        // make sure the pages are registered only once, since there can be multiple instances of KMMainWidget
        static bool pagesRegistered = false;

        if (!pagesRegistered) {
            Akonadi::CollectionPropertiesDialog::registerPage(new PimCommon::CollectionAclPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new MailCommon::CollectionGeneralPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionMaintenancePageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionQuotaPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionTemplatesPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new MailCommon::CollectionExpiryPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionViewPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionMailingListPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new CollectionShortcutPageFactory);

            pagesRegistered = true;
        }
    }

    KMainWindow *mainWin = dynamic_cast<KMainWindow *>(window());
    QStatusBar *sb =  mainWin ? mainWin->statusBar() : 0;
    mVacationScriptIndicator = new KMail::VacationScriptIndicatorWidget(sb);
    mVacationScriptIndicator->hide();
    connect(mVacationScriptIndicator, SIGNAL(clicked(QString)), SLOT(slotEditVacation(QString)));
    if (KSieveUi::Util::checkOutOfOfficeOnStartup()) {
        QTimer::singleShot(0, this, SLOT(slotCheckVacation()));
    }

    connect(mFolderTreeWidget->folderTreeView()->model(), SIGNAL(modelReset()),
            this, SLOT(restoreCollectionFolderViewConfig()));
    restoreCollectionFolderViewConfig();

    if (kmkernel->firstStart()) {
        if (MailCommon::Util::foundMailer()) {
            if (KMessageBox::questionYesNo(this, i18n("Another mailer was found on system. Do you want to import data from it?")) == KMessageBox::Yes) {
                const QString path = QStandardPaths::findExecutable(QLatin1String("importwizard"));
                if (!QProcess::startDetached(path)) {
                    KMessageBox::error(this, i18n("Could not start the import wizard. "
                                                  "Please check your installation."),
                                       i18n("Unable to start import wizard"));
                }
            } else {
                KMail::Util::launchAccountWizard(this);
            }
        } else {
            KMail::Util::launchAccountWizard(this);
        }
    }
    // must be the last line of the constructor:
    mStartupDone = true;

    mCheckMailTimer.setInterval(3 * 1000);
    mCheckMailTimer.setSingleShot(true);
    connect(&mCheckMailTimer, &QTimer::timeout, this, &KMMainWidget::slotUpdateActionsAfterMailChecking);

}

void KMMainWidget::restoreCollectionFolderViewConfig()
{
    ETMViewStateSaver *saver = new ETMViewStateSaver;
    saver->setView(mFolderTreeWidget->folderTreeView());
    const KConfigGroup cfg(KMKernel::self()->config(), "CollectionFolderView");
    mFolderTreeWidget->restoreHeaderState(cfg.readEntry("HeaderState", QByteArray()));
    saver->restoreState(cfg);
    //Restore startup folder

    Akonadi::Collection::Id id = -1;
    if (mCurrentFolder && mCurrentFolder->collection().isValid()) {
        id = mCurrentFolder->collection().id();
    }

    if (id == -1) {
        if (GlobalSettings::self()->startSpecificFolderAtStartup()) {
            Akonadi::Collection::Id startupFolder = GlobalSettings::self()->startupFolder();
            if (startupFolder > 0) {
                saver->restoreCurrentItem(QStringLiteral("c%1").arg(startupFolder));
            }
        }
    } else {
        saver->restoreCurrentItem(QStringLiteral("c%1").arg(id));
    }
}

//-----------------------------------------------------------------------------
//The kernel may have already been deleted when this method is called,
//perform all cleanup that requires the kernel in destruct()
KMMainWidget::~KMMainWidget()
{
    theMainWidgetList->removeAll(this);
    qDeleteAll(mFilterCommands);
    destruct();
}

//-----------------------------------------------------------------------------
//This method performs all cleanup that requires the kernel to exist.
void KMMainWidget::destruct()
{
    if (mDestructed) {
        return;
    }
    if (mSearchWin) {
        mSearchWin->close();
    }
    writeConfig(false); /* don't force kmkernel sync when close BUG: 289287 */
    writeFolderConfig();
    deleteWidgets();
    mCurrentFolder.clear();
    delete mMoveOrCopyToDialog;
    delete mSelectFromAllFoldersDialog;

    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), this, 0);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemRemoved(Akonadi::Item)), this, 0);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)), this, 0);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), this, 0);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), this, 0);

    mDestructed = true;
}

void KMMainWidget::slotStartCheckMail()
{
    if (mCheckMailTimer.isActive()) {
        mCheckMailTimer.stop();
    }
}

void KMMainWidget::slotEndCheckMail()
{
    if (!mCheckMailTimer.isActive()) {
        mCheckMailTimer.start();
    }
}

void KMMainWidget::slotUpdateActionsAfterMailChecking()
{
    const bool sendOnAll =
        GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnAllChecks;
    const bool sendOnManual =
        GlobalSettings::self()->sendOnCheck() == GlobalSettings::EnumSendOnCheck::SendOnManualChecks;
    if (!kmkernel->isOffline() && (sendOnAll || (sendOnManual /*&& sendOnCheck*/))) {
        slotSendQueued();
    }
    // update folder menus in case some mail got filtered to trash/current folder
    // and we can enable "empty trash/move all to trash" action etc.
    updateFolderMenu();
}

void KMMainWidget::slotCollectionFetched(int collectionId)
{
    // Called when a collection is fetched for the first time by the ETM.
    // This is the right time to update the caption (which still says "Loading...")
    // and to update the actions that depend on the number of mails in the folder.
    if (mCurrentFolder && collectionId == mCurrentFolder->collection().id()) {
        mCurrentFolder->setCollection(MailCommon::Util::updatedCollection(mCurrentFolder->collection()));
        updateMessageActions();
        updateFolderMenu();
    }
    // We call this for any collection, it could be one of our parents...
    if (mCurrentFolder) {
        emit captionChangeRequest(MailCommon::Util::fullCollectionPath(mCurrentFolder->collection()));
    }
}

void KMMainWidget::slotFolderChanged(const Akonadi::Collection &collection)
{
    folderSelected(collection);
    if (collection.cachePolicy().syncOnDemand()) {
        AgentManager::self()->synchronizeCollection(collection, false);
    }
    mMsgActions->setCurrentMessage(Akonadi::Item());
    emit captionChangeRequest(MailCommon::Util::fullCollectionPath(collection));
}

void KMMainWidget::folderSelected(const Akonadi::Collection &col)
{
    // This is connected to the MainFolderView signal triggering when a folder is selected

    if (mGoToFirstUnreadMessageInSelectedFolder) {
        // the default action has been overridden from outside
        mPreSelectionMode = MessageList::Core::PreSelectFirstUnreadCentered;
    } else {
        // use the default action
        switch (GlobalSettings::self()->actionEnterFolder()) {
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
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif

    if (mMsgView) {
        mMsgView->clear(true);
    }
    const bool newFolder = mCurrentFolder && (mCurrentFolder->collection() != col);

    // Delete any pending timer, if needed it will be recreated below
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = 0;
    if (newFolder) {
        // We're changing folder: write configuration for the old one
        writeFolderConfig();
    }

    mCurrentFolder = FolderCollection::forCollection(col);

    readFolderConfig();
    if (mMsgView) {
        mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
        mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPreference);
    }

    if (!mCurrentFolder->isValid() && (mMessagePane->count() < 2)) {
        slotIntro();
    }

    updateMessageActions();
    updateFolderMenu();

    // The message pane uses the selection model of the folder view to load the correct aggregation model and theme
    //  settings. At this point the selection model hasn't been updated yet to the user's new choice, so it would load
    //  the old folder settings instead.
    QTimer::singleShot(0, this, SLOT(slotShowSelectedFolderInPane()));
}

void KMMainWidget::slotShowSelectedFolderInPane()
{
    if (mCurrentFolder && mCurrentFolder->collection().isValid()) {
        mMessagePane->setCurrentFolder(mCurrentFolder->collection(), false , mPreSelectionMode);
    }
}

void KMMainWidget::clearViewer()
{
    if (mMsgView) {
        mMsgView->clear(true);
        mMsgView->displayAboutPage();
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::readPreConfig()
{
    mLongFolderList = GlobalSettings::self()->folderList() == GlobalSettings::EnumFolderList::longlist;
    mReaderWindowActive = GlobalSettings::self()->readerWindowMode() != GlobalSettings::EnumReaderWindowMode::hide;
    mReaderWindowBelow = GlobalSettings::self()->readerWindowMode() == GlobalSettings::EnumReaderWindowMode::below;

    mHtmlGlobalSetting = MessageViewer::GlobalSettings::self()->htmlMail();
    mHtmlLoadExtGlobalSetting = MessageViewer::GlobalSettings::self()->htmlLoadExternal();

    mEnableFavoriteFolderView = (MailCommon::MailCommonSettings::self()->favoriteCollectionViewMode() != MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode);
    mEnableFolderQuickSearch = GlobalSettings::self()->enableFolderQuickSearch();
    readFolderConfig();
    updateHtmlMenuEntry();
    if (mMsgView) {
        mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
        mMsgView->update(true);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::readFolderConfig()
{
    if (!mCurrentFolder || !mCurrentFolder->isValid()) {
        return;
    }
    KSharedConfig::Ptr config = KMKernel::self()->config();
    KConfigGroup group(config, MailCommon::FolderCollection::configGroupName(mCurrentFolder->collection()));
    if (group.hasKey("htmlMailOverride")) {
        const bool useHtml = group.readEntry("htmlMailOverride", false);
        mFolderDisplayFormatPreference = useHtml ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
        group.deleteEntry("htmlMailOverride");
        group.sync();
    } else {
        mFolderDisplayFormatPreference = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(group.readEntry("displayFormatOverride", static_cast<int>(MessageViewer::Viewer::UseGlobalSetting)));
    }
    mFolderHtmlLoadExtPreference =
        group.readEntry("htmlLoadExternalOverride", false);
}

//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig()
{
    if (mCurrentFolder && mCurrentFolder->isValid()) {
        KSharedConfig::Ptr config = KMKernel::self()->config();
        KConfigGroup group(config, MailCommon::FolderCollection::configGroupName(mCurrentFolder->collection()));
        group.writeEntry("htmlLoadExternalOverride", mFolderHtmlLoadExtPreference);
        if (mFolderDisplayFormatPreference == MessageViewer::Viewer::UseGlobalSetting) {
            group.deleteEntry("displayFormatOverride");
        } else {
            group.writeEntry("displayFormatOverride", static_cast<int>(mFolderDisplayFormatPreference));
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::layoutSplitters()
{
    // This function can only be called when the old splitters are already deleted
    Q_ASSERT(!mSplitter1);
    Q_ASSERT(!mSplitter2);

    // For some reason, this is necessary here so that the copy action still
    // works after changing the folder layout.
    if (mMsgView)
        disconnect(mMsgView->copyAction(), &QAction::triggered,
                   mMsgView, &KMReaderWin::slotCopySelectedText);

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
    const bool readerWindowAtSide = !mReaderWindowBelow && mReaderWindowActive;
    const bool readerWindowBelow = mReaderWindowBelow && mReaderWindowActive;

    mSplitter1 = new QSplitter(this);
    mSplitter2 = new QSplitter(mSplitter1);

    QWidget *folderTreeWidget = mSearchAndTree;
    if (mFavoriteCollectionsView) {
        mFolderViewSplitter = new QSplitter(Qt::Vertical);
        mFolderViewSplitter->setOpaqueResize(opaqueResize);
        //mFolderViewSplitter->setChildrenCollapsible( false );
        mFolderViewSplitter->addWidget(mFavoriteCollectionsView);
        mFavoriteCollectionsView->setParent(mFolderViewSplitter);
        mFolderViewSplitter->addWidget(mSearchAndTree);
        folderTreeWidget = mFolderViewSplitter;
    }

    if (mLongFolderList) {

        // add folder tree
        mSplitter1->setOrientation(Qt::Horizontal);
        mSplitter1->addWidget(folderTreeWidget);

        // and the rest to the right
        mSplitter1->addWidget(mSplitter2);

        // add the message list to the right or below
        if (readerWindowAtSide) {
            mSplitter2->setOrientation(Qt::Horizontal);
        } else {
            mSplitter2->setOrientation(Qt::Vertical);
        }
        mSplitter2->addWidget(mMessagePane);

        // add the preview window, if there is one
        if (mMsgView) {
            mSplitter2->addWidget(mMsgView);
        }

    } else { // short folder list
        if (mReaderWindowBelow) {
            mSplitter1->setOrientation(Qt::Vertical);
            mSplitter2->setOrientation(Qt::Horizontal);
        } else { // at side or none
            mSplitter1->setOrientation(Qt::Horizontal);
            mSplitter2->setOrientation(Qt::Vertical);
        }

        mSplitter1->addWidget(mSplitter2);

        // add folder tree
        mSplitter2->addWidget(folderTreeWidget);
        // add message list to splitter 2
        mSplitter2->addWidget(mMessagePane);

        // add the preview window, if there is one
        if (mMsgView) {
            mSplitter1->addWidget(mMsgView);
        }
    }

    //
    // Set splitter properties
    //
    mSplitter1->setObjectName(QLatin1String("splitter1"));
    mSplitter1->setOpaqueResize(opaqueResize);
    //mSplitter1->setChildrenCollapsible( false );
    mSplitter2->setObjectName(QLatin1String("splitter2"));
    mSplitter2->setOpaqueResize(opaqueResize);
    //mSplitter2->setChildrenCollapsible( false );

    //
    // Set the stretch factors
    //
    mSplitter1->setStretchFactor(0, 0);
    mSplitter2->setStretchFactor(0, 0);
    mSplitter1->setStretchFactor(1, 1);
    mSplitter2->setStretchFactor(1, 1);

    if (mFavoriteCollectionsView) {
        mFolderViewSplitter->setStretchFactor(0, 0);
        mFolderViewSplitter->setStretchFactor(1, 1);
    }

    // Because the reader windows's width increases a tiny bit after each
    // restart in short folder list mode with message window at side, disable
    // the stretching as a workaround here
    if (readerWindowAtSide && !mLongFolderList) {
        mSplitter1->setStretchFactor(0, 1);
        mSplitter1->setStretchFactor(1, 0);
    }

    //
    // Set the sizes of the splitters to the values stored in the config
    //
    QList<int> splitter1Sizes;
    QList<int> splitter2Sizes;

    const int folderViewWidth = GlobalSettings::self()->folderViewWidth();
    int ftHeight = GlobalSettings::self()->folderTreeHeight();
    int headerHeight = GlobalSettings::self()->searchAndHeaderHeight();
    const int messageViewerWidth = GlobalSettings::self()->readerWindowWidth();
    int headerWidth = GlobalSettings::self()->searchAndHeaderWidth();
    int messageViewerHeight = GlobalSettings::self()->readerWindowHeight();

    int ffvHeight = mFolderViewSplitter ? MailCommon::MailCommonSettings::self()->favoriteCollectionViewHeight() : 0;

    // If the message viewer was hidden before, make sure it is not zero height
    if (messageViewerHeight < 10 && readerWindowBelow) {
        headerHeight /= 2;
        messageViewerHeight = headerHeight;
    }

    if (mLongFolderList) {
        if (!readerWindowAtSide) {
            splitter1Sizes << folderViewWidth << headerWidth;
            splitter2Sizes << headerHeight << messageViewerHeight;
        } else {
            splitter1Sizes << folderViewWidth << (headerWidth + messageViewerWidth);
            splitter2Sizes << headerWidth << messageViewerWidth;
        }
    } else {
        if (!readerWindowAtSide) {
            splitter1Sizes << headerHeight << messageViewerHeight;
            splitter2Sizes << folderViewWidth << headerWidth;
        } else {
            splitter1Sizes << headerWidth << messageViewerWidth;
            splitter2Sizes << ftHeight + ffvHeight << messageViewerHeight;
        }
    }

    mSplitter1->setSizes(splitter1Sizes);
    mSplitter2->setSizes(splitter2Sizes);

    if (mFolderViewSplitter) {
        QList<int> splitterSizes;
        splitterSizes << ffvHeight << ftHeight;
        mFolderViewSplitter->setSizes(splitterSizes);
    }

    //
    // Now add the splitters to the main layout
    //
    mTopLayout->addWidget(mSplitter1);

    // Make sure the focus is on the view, and not on the quick search line edit, because otherwise
    // shortcuts like + or j go to the wrong place.
    // This would normally be done in the message list itself, but apparently something resets the focus
    // again, probably all the reparenting we do here.
    mMessagePane->focusView();

    // By default hide th unread and size columns on first run.
    if (kmkernel->firstStart()) {
        mFolderTreeWidget->folderTreeView()->hideColumn(1);
        mFolderTreeWidget->folderTreeView()->hideColumn(3);
        mFolderTreeWidget->folderTreeView()->header()->resizeSection(0, folderViewWidth * 0.8);
    }

    // Make the copy action work, see disconnect comment above
    if (mMsgView)
        connect(mMsgView->copyAction(), &QAction::triggered,
                mMsgView, &KMReaderWin::slotCopySelectedText);
}

//-----------------------------------------------------------------------------
void KMMainWidget::refreshFavoriteFoldersViewProperties()
{
    if (mFavoriteCollectionsView) {
        if (MailCommon::MailCommonSettings::self()->favoriteCollectionViewMode() == MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode) {
            mFavoriteCollectionsView->changeViewMode(QListView::IconMode);
        } else if (MailCommon::MailCommonSettings::self()->favoriteCollectionViewMode() == MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode) {
            mFavoriteCollectionsView->changeViewMode(QListView::ListMode);
        } else {
            Q_ASSERT(false);    // we should never get here in hidden mode
        }
        mFavoriteCollectionsView->setDropActionMenuEnabled(kmkernel->showPopupAfterDnD());
        mFavoriteCollectionsView->setWordWrap(true);
        mFavoriteCollectionsView->updateMode();
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::readConfig()
{
    const bool oldLongFolderList = mLongFolderList;
    const bool oldReaderWindowActive = mReaderWindowActive;
    const bool oldReaderWindowBelow = mReaderWindowBelow;
    const bool oldFavoriteFolderView = mEnableFavoriteFolderView;
    const bool oldFolderQuickSearch = mEnableFolderQuickSearch;

    // on startup, the layout is always new and we need to relayout the widgets
    bool layoutChanged = !mStartupDone;

    if (mStartupDone) {
        readPreConfig();

        layoutChanged = (oldLongFolderList != mLongFolderList) ||
                        (oldReaderWindowActive != mReaderWindowActive) ||
                        (oldReaderWindowBelow != mReaderWindowBelow) ||
                        (oldFavoriteFolderView != mEnableFavoriteFolderView);

        if (layoutChanged) {
            deleteWidgets();
            createWidgets();
            restoreCollectionFolderViewConfig();
            emit recreateGui();
        } else if (oldFolderQuickSearch != mEnableFolderQuickSearch) {
            if (mEnableFolderQuickSearch) {
                mFolderTreeWidget->filterFolderLineEdit()->show();
            } else {
                mFolderTreeWidget->filterFolderLineEdit()->hide();
            }
        }
    }

    {
        // Read the config of the folder views and the header
        if (mMsgView) {
            mMsgView->readConfig();
        }
        mMessagePane->reloadGlobalConfiguration();
        mFolderTreeWidget->readConfig();
        if (mFavoriteCollectionsView) {
            mFavoriteCollectionsView->readConfig();
        }
        refreshFavoriteFoldersViewProperties();
    }

    {
        // area for config group "General"
        if (!mStartupDone) {
            // check mail on startup
            // do it after building the kmmainwin, so that the progressdialog is available
            QTimer::singleShot(0, this, SLOT(slotCheckMailOnStartup()));
        }
    }

    if (layoutChanged) {
        layoutSplitters();
    }

    updateMessageMenu();
    updateFileMenu();
    kmkernel->toggleSystemTray();

    connect(Akonadi::AgentManager::self(), &AgentManager::instanceAdded,
            this, &KMMainWidget::updateFileMenu);
    connect(Akonadi::AgentManager::self(), &AgentManager::instanceRemoved,
            this, &KMMainWidget::updateFileMenu);
}

//-----------------------------------------------------------------------------
void KMMainWidget::writeConfig(bool force)
{
    // Don't save the sizes of all the widgets when we were never shown.
    // This can happen in Kontact, where the KMail plugin is automatically
    // loaded, but not necessarily shown.
    // This prevents invalid sizes from being saved
    if (mWasEverShown) {
        // The height of the header widget can be 0, this happens when the user
        // did not switch to the header widget onced and the "Welcome to KMail"
        // HTML widget was shown the whole time
        int headersHeight = mMessagePane->height();
        if (headersHeight == 0) {
            headersHeight = height() / 2;
        }

        GlobalSettings::self()->setSearchAndHeaderHeight(headersHeight);
        GlobalSettings::self()->setSearchAndHeaderWidth(mMessagePane->width());
        if (mFavoriteCollectionsView) {
            MailCommon::MailCommonSettings::self()->setFavoriteCollectionViewHeight(mFavoriteCollectionsView->height());
            GlobalSettings::self()->setFolderTreeHeight(mFolderTreeWidget->height());
            if (!mLongFolderList) {
                GlobalSettings::self()->setFolderViewHeight(mFolderViewSplitter->height());
            }
        } else if (!mLongFolderList && mFolderTreeWidget) {
            GlobalSettings::self()->setFolderTreeHeight(mFolderTreeWidget->height());
        }
        if (mFolderTreeWidget) {
            GlobalSettings::self()->setFolderViewWidth(mFolderTreeWidget->width());
            KSharedConfig::Ptr config = KMKernel::self()->config();
            KConfigGroup group(config, "CollectionFolderView");

            ETMViewStateSaver saver;
            saver.setView(mFolderTreeWidget->folderTreeView());
            saver.saveState(group);

            group.writeEntry("HeaderState", mFolderTreeWidget->folderTreeView()->header()->saveState());
            //Work around from startup folder
            group.deleteEntry("Selection");
#if 0
            if (!GlobalSettings::self()->startSpecificFolderAtStartup()) {
                group.deleteEntry("Current");
            }
#endif
            group.sync();
        }

        if (mMsgView) {
            if (!mReaderWindowBelow) {
                GlobalSettings::self()->setReaderWindowWidth(mMsgView->width());
            }
            mMsgView->viewer()->writeConfig(force);
            GlobalSettings::self()->setReaderWindowHeight(mMsgView->height());
        }
    }
}

void KMMainWidget::writeReaderConfig()
{
    if (mWasEverShown) {
        if (mMsgView) {
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
    mAkonadiStandardActionManager = 0;
    delete mSplitter1;
    mMsgView = 0;
    mSearchAndTree = 0;
    mFolderViewSplitter = 0;
    mFavoriteCollectionsView = 0;
    mSplitter1 = 0;
    mSplitter2 = 0;
    mFavoritesModel = 0;
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
    mFolderTreeWidget = new FolderTreeWidget(this, mGUIClient, opt);

    connect(mFolderTreeWidget->folderTreeView(), SIGNAL(currentChanged(Akonadi::Collection)), this, SLOT(slotFolderChanged(Akonadi::Collection)));

    connect(mFolderTreeWidget->folderTreeView()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &KMMainWidget::updateFolderMenu);

    connect(mFolderTreeWidget->folderTreeView(), &FolderTreeView::prefereCreateNewTab, this, &KMMainWidget::slotCreateNewTab);

    mFolderTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mMessagePane = new CollectionPane(!GlobalSettings::self()->startSpecificFolderAtStartup(), KMKernel::self()->entityTreeModel(),
                                      mFolderTreeWidget->folderTreeView()->selectionModel(),
                                      this);
    connect(KMKernel::self()->entityTreeModel(), &Akonadi::EntityTreeModel::collectionFetched, this, &KMMainWidget::slotCollectionFetched);

    mMessagePane->setXmlGuiClient(mGUIClient);
    connect(mMessagePane, &MessageList::Pane::messageSelected,
            this, &KMMainWidget::slotMessageSelected);
    connect(mMessagePane, &MessageList::Pane::selectionChanged,
            this, &KMMainWidget::startUpdateMessageActionsTimer);
    connect(mMessagePane, &CollectionPane::currentTabChanged, this, &KMMainWidget::refreshMessageListSelection);
    connect(mMessagePane, &MessageList::Pane::messageActivated,
            this, &KMMainWidget::slotMessageActivated);
    connect(mMessagePane, &MessageList::Pane::messageStatusChangeRequest,
            this, &KMMainWidget::slotMessageStatusChangeRequest);

    connect(mMessagePane, SIGNAL(statusMessage(QString)),
            BroadcastStatus::instance(), SLOT(setStatusMsg(QString)));

    //
    // Create the reader window
    //
    if (mReaderWindowActive) {
        mMsgView = new KMReaderWin(this, this, actionCollection(), 0);
        if (mMsgActions) {
            mMsgActions->setMessageView(mMsgView);
        }
        connect(mMsgView->viewer(), &MessageViewer::Viewer::replaceMsgByUnencryptedVersion,
                this, &KMMainWidget::slotReplaceMsgByUnencryptedVersion);
        connect(mMsgView->viewer(), &MessageViewer::Viewer::popupMenu,
                this, &KMMainWidget::slotMessagePopup);
        connect(mMsgView->viewer(), &MessageViewer::Viewer::moveMessageToTrash,
                this, &KMMainWidget::slotMoveMessageToTrash);
    } else {
        if (mMsgActions) {
            mMsgActions->setMessageView(0);
        }
    }

    //
    // Create the folder tree
    // the "folder tree" consists of a quicksearch input field and the tree itself
    //

    mSearchAndTree = new QWidget(this);
    QVBoxLayout *vboxlayout = new QVBoxLayout;
    vboxlayout->setMargin(0);
    mSearchAndTree->setLayout(vboxlayout);

    vboxlayout->addWidget(mFolderTreeWidget);

    if (!GlobalSettings::self()->enableFolderQuickSearch()) {
        mFolderTreeWidget->filterFolderLineEdit()->hide();
    }
    //
    // Create the favorite folder view
    //
    mAkonadiStandardActionManager = new Akonadi::StandardMailActionManager(mGUIClient->actionCollection(), this);
    connect(mAkonadiStandardActionManager, &Akonadi::StandardMailActionManager::actionStateUpdated, this, &KMMainWidget::slotAkonadiStandardActionUpdated);

    mAkonadiStandardActionManager->setCollectionSelectionModel(mFolderTreeWidget->folderTreeView()->selectionModel());
    mAkonadiStandardActionManager->setItemSelectionModel(mMessagePane->currentItemSelectionModel());

    if (mEnableFavoriteFolderView) {

        mFavoriteCollectionsView = new FavoriteCollectionWidget(mGUIClient, this);
        refreshFavoriteFoldersViewProperties();
        connect(mFavoriteCollectionsView, SIGNAL(currentChanged(Akonadi::Collection)), this, SLOT(slotFolderChanged(Akonadi::Collection)));

        mFavoritesModel = new Akonadi::FavoriteCollectionsModel(
            mFolderTreeWidget->folderTreeView()->model(),
            KMKernel::self()->config()->group("FavoriteCollections"), this);

        mFavoriteCollectionsView->setModel(mFavoritesModel);

        Akonadi::CollectionStatisticsDelegate *delegate = new Akonadi::CollectionStatisticsDelegate(mFavoriteCollectionsView);
        delegate->setProgressAnimationEnabled(true);
        mFavoriteCollectionsView->setItemDelegate(delegate);
        delegate->setUnreadCountShown(true);

        mAkonadiStandardActionManager->setFavoriteCollectionsModel(mFavoritesModel);
        mAkonadiStandardActionManager->setFavoriteSelectionModel(mFavoriteCollectionsView->selectionModel());
    }

    //Don't use mMailActionManager->createAllActions() to save memory by not
    //creating actions that doesn't make sense.
    QList<StandardActionManager::Type> standardActions;
    standardActions << StandardActionManager::CreateCollection
                    << StandardActionManager::CopyCollections
                    << StandardActionManager::DeleteCollections
                    << StandardActionManager::SynchronizeCollections
                    << StandardActionManager::CollectionProperties
                    << StandardActionManager::CopyItems
                    << StandardActionManager::Paste
                    << StandardActionManager::DeleteItems
                    << StandardActionManager::ManageLocalSubscriptions
                    << StandardActionManager::CopyCollectionToMenu
                    << StandardActionManager::CopyItemToMenu
                    << StandardActionManager::MoveItemToMenu
                    << StandardActionManager::MoveCollectionToMenu
                    << StandardActionManager::CutItems
                    << StandardActionManager::CutCollections
                    << StandardActionManager::CreateResource
                    << StandardActionManager::DeleteResources
                    << StandardActionManager::ResourceProperties
                    << StandardActionManager::SynchronizeResources
                    << StandardActionManager::ToggleWorkOffline
                    << StandardActionManager::SynchronizeCollectionsRecursive;

    Q_FOREACH (StandardActionManager::Type standardAction, standardActions) {
        mAkonadiStandardActionManager->createAction(standardAction);
    }

    if (mEnableFavoriteFolderView) {
        QList<StandardActionManager::Type> favoriteActions;
        favoriteActions << StandardActionManager::AddToFavoriteCollections
                        << StandardActionManager::RemoveFromFavoriteCollections
                        << StandardActionManager::RenameFavoriteCollection
                        << StandardActionManager::SynchronizeFavoriteCollections;
        Q_FOREACH (StandardActionManager::Type favoriteAction, favoriteActions) {
            mAkonadiStandardActionManager->createAction(favoriteAction);
        }
    }

    QList<StandardMailActionManager::Type> mailActions;
    mailActions << StandardMailActionManager::MarkAllMailAsRead
                << StandardMailActionManager::MoveToTrash
                << StandardMailActionManager::MoveAllToTrash
                << StandardMailActionManager::RemoveDuplicates
                << StandardMailActionManager::EmptyAllTrash
                << StandardMailActionManager::MarkMailAsRead
                << StandardMailActionManager::MarkMailAsUnread
                << StandardMailActionManager::MarkMailAsImportant
                << StandardMailActionManager::MarkMailAsActionItem;

    Q_FOREACH (StandardMailActionManager::Type mailAction, mailActions) {
        mAkonadiStandardActionManager->createAction(mailAction);
    }

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::CollectionProperties);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CollectionProperties), SIGNAL(triggered(bool)), mManageShowCollectionProperties, SLOT(slotCollectionProperties()));

    //
    // Create all kinds of actions
    //
    mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::RemoveDuplicates)->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Asterisk));
    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::RemoveDuplicates);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::RemoveDuplicates), SIGNAL(triggered(bool)), this, SLOT(slotRemoveDuplicates()));

    {
        mCollectionProperties = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CollectionProperties);
    }
    connect(kmkernel->folderCollectionMonitor(), SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
            SLOT(slotItemAdded(Akonadi::Item,Akonadi::Collection)));
    connect(kmkernel->folderCollectionMonitor(), SIGNAL(itemRemoved(Akonadi::Item)),
            SLOT(slotItemRemoved(Akonadi::Item)));
    connect(kmkernel->folderCollectionMonitor(), SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
            SLOT(slotItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)));
    connect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), SLOT(slotCollectionChanged(Akonadi::Collection,QSet<QByteArray>)));

    connect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), SLOT(slotCollectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)));

}

void KMMainWidget::updateMoveAction(const Akonadi::CollectionStatistics &statistic)
{
    const bool hasUnreadMails = (statistic.unreadCount() > 0);
    const bool hasMails = (statistic.count() > 0);
    updateMoveAction(hasUnreadMails, hasMails);
}

void KMMainWidget::updateMoveAction(bool hasUnreadMails, bool hasMails)
{
    const bool enable_goto_unread = hasUnreadMails
                                    || (GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders)
                                    || (GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders);
    actionCollection()->action(QLatin1String("go_next_message"))->setEnabled(hasMails);
    actionCollection()->action(QLatin1String("go_next_unread_message"))->setEnabled(enable_goto_unread);
    actionCollection()->action(QLatin1String("go_prev_message"))->setEnabled(hasMails);
    actionCollection()->action(QLatin1String("go_prev_unread_message"))->setEnabled(enable_goto_unread);
    if (mAkonadiStandardActionManager && mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MarkAllMailAsRead)) {
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MarkAllMailAsRead)->setEnabled(hasUnreadMails);
    }
}

void KMMainWidget::updateAllToTrashAction(int statistics)
{
    bool multiFolder = false;
    if (mFolderTreeWidget) {
        multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
    }
    if (mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)) {
        const bool folderWithContent = mCurrentFolder && !mCurrentFolder->isStructural();
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setEnabled(folderWithContent
                && (statistics > 0)
                && mCurrentFolder->canDeleteMessages()
                && !multiFolder);
    }
}

void KMMainWidget::slotCollectionStatisticsChanged(const Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistic)
{
    if (id == CommonKernel->outboxCollectionFolder().id()) {
        const qint64 nbMsgOutboxCollection = statistic.count();
        mSendQueued->setEnabled(nbMsgOutboxCollection > 0);
        mSendActionMenu->setEnabled(nbMsgOutboxCollection > 0);
    } else if (mCurrentFolder && (id == mCurrentFolder->collection().id())) {
        updateMoveAction(statistic);
        updateAllToTrashAction(statistic.count());
        mCurrentFolder->setCollection(MailCommon::Util::updatedCollection(mCurrentFolder->collection()));
    }
}

void KMMainWidget::slotCreateNewTab(bool preferNewTab)
{
    mMessagePane->setPreferEmptyTab(preferNewTab);
}

void KMMainWidget::slotCollectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &set)
{
    if (mCurrentFolder
            && (collection == mCurrentFolder->collection())
            && (set.contains("MESSAGEFOLDER") || set.contains("expirationcollectionattribute"))) {
        if (set.contains("MESSAGEFOLDER")) {
            mMessagePane->resetModelStorage();
        } else {
            mCurrentFolder->setCollection(collection);
        }
    } else if (set.contains("ENTITYDISPLAY") || set.contains("NAME")) {
        const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(KMKernel::self()->collectionModel(), collection);
        if (idx.isValid()) {
            const QString text = idx.data().toString();
            const QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
            mMessagePane->updateTabIconText(collection, text, icon);
        }
    }
}

void KMMainWidget::slotItemAdded(const Akonadi::Item &msg, const Akonadi::Collection &col)
{
    Q_UNUSED(msg);
    if (col.isValid()) {
        if (col == CommonKernel->outboxCollectionFolder()) {
            startUpdateMessageActionsTimer();
        }
    }
}

void KMMainWidget::slotItemRemoved(const Akonadi::Item &item)
{
    if (item.isValid() && item.parentCollection().isValid() && (item.parentCollection() == CommonKernel->outboxCollectionFolder())) {
        startUpdateMessageActionsTimer();
    }
}

void KMMainWidget::slotItemMoved(const Akonadi::Item &item, const Akonadi::Collection &from, const Akonadi::Collection &to)
{
    if (item.isValid() && ((from.id() == CommonKernel->outboxCollectionFolder().id())
                           || to.id() == CommonKernel->outboxCollectionFolder().id())) {
        startUpdateMessageActionsTimer();
    }
}

//-------------------------------------------------------------------------
void KMMainWidget::slotFocusQuickSearch()
{
    const QString text = mMsgView ? mMsgView->copyText() : QString();
    mMessagePane->focusQuickSearch(text);
}

//-------------------------------------------------------------------------
bool KMMainWidget::slotSearch()
{
    if (!mSearchWin) {
        mSearchWin = new SearchWindow(this, mCurrentFolder ? mCurrentFolder->collection() : Akonadi::Collection());
        mSearchWin->setModal(false);
        mSearchWin->setObjectName(QLatin1String("Search"));
    } else {
        mSearchWin->activateFolder(mCurrentFolder ? mCurrentFolder->collection() : Akonadi::Collection());
    }

    mSearchWin->show();
    KWindowSystem::activateWindow(mSearchWin->winId());
    return true;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotHelp()
{
    KHelpClient::invokeHelp();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFilter()
{
    FilterIf->openFilterDialog(true);
}

void KMMainWidget::slotManageSieveScripts()
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }
    if (mManageSieveDialog) {
        return;
    }

    mManageSieveDialog = new KSieveUi::ManageSieveScriptsDialog(this);
    mManageSieveDialog->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAddrBook()
{
    KRun::runCommand(QLatin1String("kaddressbook"), window());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotImport()
{
    KRun::runCommand(QLatin1String("kmailcvt"), window());
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
void KMMainWidget::slotCheckOneAccount(QAction *item)
{
    if (!item) {
        return;
    }

    Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(item->data().toString());
    if (agent.isValid()) {
        if (!agent.isOnline()) {
            agent.setIsOnline(true);
        }
        agent.synchronize();
    } else {
        qDebug() << "account with identifier" << item->data().toString() << "not found";
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompose()
{
    KMail::Composer *win;
    KMime::Message::Ptr msg(new KMime::Message());

    bool forceCursorPosition = false;
    if (mCurrentFolder) {
        MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), mCurrentFolder->identity());
        //Laurent: bug 289905
        /*
        if ( mCurrentFolder->collection().isValid() && mCurrentFolder->putRepliesInSameFolder() ) {
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Fcc", msg.get(), QString::number( mCurrentFolder->collection().id() ), "utf-8" );
        msg->setHeader( header );
        }
        */
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(msg, mCurrentFolder->collection());
        win = KMail::makeComposer(msg, false, false, KMail::Composer::New, mCurrentFolder->identity());
        win->setCollectionForNewMessage(mCurrentFolder->collection());
        forceCursorPosition = parser.cursorPositionWasSet();
    } else {
        MessageHelper::initHeader(msg, KMKernel::self()->identityManager());
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(KMime::Message::Ptr(), Akonadi::Collection());
        win = KMail::makeComposer(msg, false, false, KMail::Composer::New);
        forceCursorPosition = parser.cursorPositionWasSet();
    }
    if (forceCursorPosition) {
        win->setFocusToEditor();
    }
    win->show();

}

//-----------------------------------------------------------------------------
// TODO: do we want the list sorted alphabetically?
void KMMainWidget::slotShowNewFromTemplate()
{
    if (mCurrentFolder) {
        const KIdentityManagement::Identity &ident =
            kmkernel->identityManager()->identityForUoidOrDefault(mCurrentFolder->identity());
        mTemplateFolder = CommonKernel->collectionFromId(ident.templates().toLongLong());
    }

    if (!mTemplateFolder.isValid()) {
        mTemplateFolder = CommonKernel->templatesCollectionFolder();
    }
    if (!mTemplateFolder.isValid()) {
        return;
    }

    mTemplateMenu->menu()->clear();

    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mTemplateFolder);
    job->fetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
    job->fetchScope().fetchFullPayload();
    connect(job, &Akonadi::ItemFetchJob::result, this, &KMMainWidget::slotDelayedShowNewFromTemplate);
}

void KMMainWidget::slotDelayedShowNewFromTemplate(KJob *job)
{
    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);

    const Akonadi::Item::List items = fetchJob->items();
    const int numberOfItems = items.count();
    for (int idx = 0; idx < numberOfItems; ++idx) {
        KMime::Message::Ptr msg = MessageCore::Util::message(items.at(idx));
        if (msg) {
            QString subj = msg->subject()->asUnicodeString();
            if (subj.isEmpty()) {
                subj = i18n("No Subject");
            }

            QAction *templateAction = mTemplateMenu->menu()->addAction(KStringHandler::rsqueeze(subj.replace(QLatin1Char('&'), QLatin1String("&&"))));
            QVariant var;
            var.setValue(items.at(idx));
            templateAction->setData(var);
        }
    }

    // If there are no templates available, add a menu entry which informs
    // the user about this.
    if (mTemplateMenu->menu()->actions().isEmpty()) {
        QAction *noAction = mTemplateMenu->menu()->addAction(
                                i18n("(no templates)"));
        noAction->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotNewFromTemplate(QAction *action)
{

    if (!mTemplateFolder.isValid()) {
        return;
    }
    const Akonadi::Item item = action->data().value<Akonadi::Item>();
    newFromTemplate(item);
}

//-----------------------------------------------------------------------------
void KMMainWidget::newFromTemplate(const Akonadi::Item &msg)
{
    if (!msg.isValid()) {
        return;
    }
    KMCommand *command = new KMUseTemplateCommand(this, msg);
    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotPostToML()
{
    if (mCurrentFolder && mCurrentFolder->isMailingListEnabled()) {
        if (KMail::Util::mailingListPost(mCurrentFolder)) {
            return;
        }
    }
    slotCompose();
}

void KMMainWidget::slotExpireFolder()
{
    if (!mCurrentFolder) {
        return;
    }
    bool mustDeleteExpirationAttribute = false;
    MailCommon::ExpireCollectionAttribute *attr = MailCommon::ExpireCollectionAttribute::expirationCollectionAttribute(mCurrentFolder->collection(), mustDeleteExpirationAttribute);
    ;
    bool canBeExpired = true;
    if (!attr->isAutoExpire()) {
        canBeExpired = false;
    } else if (attr->unreadExpireUnits() == MailCommon::ExpireCollectionAttribute::ExpireNever &&
               attr->readExpireUnits() == MailCommon::ExpireCollectionAttribute::ExpireNever) {
        canBeExpired = false;
    }

    if (!canBeExpired) {
        const QString message = i18n("This folder does not have any expiry options set");
        KMessageBox::information(this, message);
        if (mustDeleteExpirationAttribute) {
            delete attr;
        }
        return;
    }

    if (GlobalSettings::self()->warnBeforeExpire()) {
        const QString message = i18n("<qt>Are you sure you want to expire the folder <b>%1</b>?</qt>",
                                     mCurrentFolder->name().toHtmlEscaped());
        if (KMessageBox::warningContinueCancel(this, message, i18n("Expire Folder"),
                                               KGuiItem(i18n("&Expire")))
                != KMessageBox::Continue) {
            if (mustDeleteExpirationAttribute) {
                delete attr;
            }
            return;
        }
    }

    MailCommon::Util::expireOldMessages(mCurrentFolder->collection(), true /*immediate*/);
    if (mustDeleteExpirationAttribute) {
        delete attr;
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEmptyFolder()
{
    if (!mCurrentFolder) {
        return;
    }
    const bool isTrash = CommonKernel->folderIsTrash(mCurrentFolder->collection());
    if (GlobalSettings::self()->confirmBeforeEmpty()) {
        const QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
        const QString text = (isTrash) ?
                             i18n("Are you sure you want to empty the trash folder?") :
                             i18n("<qt>Are you sure you want to move all messages from "
                                  "folder <b>%1</b> to the trash?</qt>", mCurrentFolder->name().toHtmlEscaped());

        if (KMessageBox::warningContinueCancel(this, text, title, KGuiItem(title, QLatin1String("user-trash")))
                != KMessageBox::Continue) {
            return;
        }
    }
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif
    slotMarkAll();
    if (isTrash) {
        /* Don't ask for confirmation again when deleting, the user has already
        confirmed. */
        slotDeleteMsg(false);
    } else {
        slotTrashSelectedMessages();
    }

    if (mMsgView) {
        mMsgView->clearCache();
    }

    if (!isTrash) {
        BroadcastStatus::instance()->setStatusMsg(i18n("Moved all messages to the trash"));
    }

    updateMessageActions();

    // Disable empty trash/move all to trash action - we've just deleted/moved
    // all folder contents.
    mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setEnabled(false);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotArchiveFolder()
{
    if (mCurrentFolder && mCurrentFolder->collection().isValid()) {
        KMail::ArchiveFolderDialog archiveDialog;
        archiveDialog.setFolder(mCurrentFolder->collection());
        archiveDialog.exec();
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotRemoveFolder()
{
    if (!mCurrentFolder) {
        return;
    }
    if (!mCurrentFolder->collection().isValid()) {
        return;
    }
    if (mCurrentFolder->isSystemFolder()) {
        return;
    }
    if (mCurrentFolder->isReadOnly()) {
        return;
    }

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(mCurrentFolder->collection(), CollectionFetchJob::FirstLevel, this);
    job->fetchScope().setContentMimeTypes(QStringList() << KMime::Message::mimeType());
    job->setProperty("collectionId", mCurrentFolder->collection().id());
    connect(job, &Akonadi::CollectionFetchJob::result, this, &KMMainWidget::slotDelayedRemoveFolder);
}

void KMMainWidget::slotDelayedRemoveFolder(KJob *job)
{
    const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List listOfCollection = fetchJob->collections();
    const bool hasNotSubDirectory = listOfCollection.isEmpty();

    const Akonadi::Collection::Id id = fetchJob->property("collectionId").toLongLong();
    Akonadi::Collection col = MailCommon::Util::updatedCollection(CommonKernel->collectionFromId(id));
    QDir dir;
    QString str;
    QString title;
    QString buttonLabel;
    if (col.resource() == QLatin1String("akonadi_search_resource")) {
        title = i18n("Delete Search");
        str = i18n("<qt>Are you sure you want to delete the search <b>%1</b>?<br />"
                   "Any messages it shows will still be available in their original folder.</qt>",
                   col.name().toHtmlEscaped());
        buttonLabel = i18nc("@action:button Delete search", "&Delete");
    } else {
        title = i18n("Delete Folder");

        if (col.statistics().count() == 0) {
            if (hasNotSubDirectory) {
                str = i18n("<qt>Are you sure you want to delete the empty folder "
                           "<b>%1</b>?</qt>",
                           col.name().toHtmlEscaped());
            } else {
                str = xi18n("<qt>Are you sure you want to delete the empty folder "
                            "<resource>%1</resource> and all its subfolders? Those subfolders might "
                            "not be empty and their contents will be discarded as well. "
                            "<p><b>Beware</b> that discarded messages are not saved "
                            "into your Trash folder and are permanently deleted.</p></qt>",
                            col.name().toHtmlEscaped());
            }
        } else {
            if (hasNotSubDirectory) {
                str = xi18n("<qt>Are you sure you want to delete the folder "
                            "<resource>%1</resource>, discarding its contents? "
                            "<p><b>Beware</b> that discarded messages are not saved "
                            "into your Trash folder and are permanently deleted.</p></qt>",
                            col.name().toHtmlEscaped());
            } else {
                str = xi18n("<qt>Are you sure you want to delete the folder <resource>%1</resource> "
                            "and all its subfolders, discarding their contents? "
                            "<p><b>Beware</b> that discarded messages are not saved "
                            "into your Trash folder and are permanently deleted.</p></qt>",
                            col.name().toHtmlEscaped());
            }
        }
        buttonLabel = i18nc("@action:button Delete folder", "&Delete");
    }

    if (KMessageBox::warningContinueCancel(this, str, title,
                                           KGuiItem(buttonLabel, QLatin1String("edit-delete")),
                                           KStandardGuiItem::cancel(), QString(),
                                           KMessageBox::Notify | KMessageBox::Dangerous)
            == KMessageBox::Continue) {
        kmkernel->checkFolderFromResources(listOfCollection << col);

        if (col.id() == mCurrentFolder->collection().id()) {
            mCurrentFolder.clear();
        }

        Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob(col);
        connect(job, &Akonadi::CollectionDeleteJob::result, this, &KMMainWidget::slotDeletionCollectionResult);
    }
}

void KMMainWidget::slotDeletionCollectionResult(KJob *job)
{
    if (job) {
        if (Util::showJobErrorMessage(job)) {
            return;
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireAll()
{
    if (GlobalSettings::self()->warnBeforeExpire()) {
        const int ret = KMessageBox::warningContinueCancel(KMainWindow::memberList().first(),
                        i18n("Are you sure you want to expire all old messages?"),
                        i18n("Expire Old Messages?"), KGuiItem(i18n("Expire")));
        if (ret != KMessageBox::Continue) {
            return;
        }
    }

    kmkernel->expireAllFoldersNow();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideHtmlLoadExt()
{
    if (mHtmlLoadExtGlobalSetting == mFolderHtmlLoadExtPreference) {
        int result = KMessageBox::warningContinueCancel(this,
                     // the warning text is taken from configuredialog.cpp:
                     i18n("Loading external references in html mail will make you more vulnerable to "
                          "\"spam\" and may increase the likelihood that your system will be "
                          "compromised by other present and anticipated security exploits."),
                     i18n("Security Warning"),
                     KGuiItem(i18n("Load External References")),
                     KStandardGuiItem::cancel(),
                     QLatin1String("OverrideHtmlLoadExtWarning"), 0);
        if (result == KMessageBox::Cancel) {
            mPreferHtmlLoadExtAction->setChecked(false);
            return;
        }
    }
    mFolderHtmlLoadExtPreference = !mFolderHtmlLoadExtPreference;

    if (mMsgView) {
        mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPreference);
        mMsgView->update(true);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMessageQueuedOrDrafted()
{
    if (!CommonKernel->folderIsDraftOrOutbox(mCurrentFolder->collection())) {
        return;
    }
    if (mMsgView) {
        mMsgView->update(true);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardInlineMsg()
{
    if (!mCurrentFolder) {
        return;
    }

    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    KMForwardCommand *command = new KMForwardCommand(
        this, selectedMessages, mCurrentFolder->identity()
    );

    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMsg()
{
    if (!mCurrentFolder) {
        return;
    }

    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    KMForwardAttachedCommand *command = new KMForwardAttachedCommand(
        this, selectedMessages, mCurrentFolder->identity()
    );

    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotUseTemplate()
{
    newFromTemplate(mMessagePane->currentItem());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotResendMsg()
{
    const Akonadi::Item msg = mMessagePane->currentItem();
    if (!msg.isValid()) {
        return;
    }
    KMCommand *command = new KMResendMessageCommand(this, msg);

    command->start();
}

//-----------------------------------------------------------------------------
// Message moving and permanent deletion
//

void KMMainWidget::moveMessageSelected(MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion)
{
    QList<Akonadi::Item> selectMsg  = mMessagePane->itemListFromPersistentSet(ref);
    // If this is a deletion, ask for confirmation
    if (!dest.isValid() && confirmOnDeletion) {
        int ret = KMessageBox::warningContinueCancel(
                      this,
                      i18np(
                          "<qt>Do you really want to delete the selected message?<br />"
                          "Once deleted, it cannot be restored.</qt>",
                          "<qt>Do you really want to delete the %1 selected messages?<br />"
                          "Once deleted, they cannot be restored.</qt>",
                          selectMsg.count()
                      ),
                      selectMsg.count() > 1 ? i18n("Delete Messages") : i18n("Delete Message"),
                      KStandardGuiItem::del(),
                      KStandardGuiItem::cancel(),
                      QLatin1String("NoConfirmDelete")
                  );
        if (ret == KMessageBox::Cancel) {
            mMessagePane->deletePersistentSet(ref);
            return;  // user canceled the action
        }
    }
    mMessagePane->markMessageItemsAsAboutToBeRemoved(ref, true);
    // And stuff them into a KMMoveCommand :)
    KMMoveCommand *command = new KMMoveCommand(dest, selectMsg, ref);
    QObject::connect(
        command, SIGNAL(moveDone(KMMoveCommand*)),
        this, SLOT(slotMoveMessagesCompleted(KMMoveCommand*))
    );
    command->start();

    if (dest.isValid()) {
        BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages..."));
    } else {
        BroadcastStatus::instance()->setStatusMsg(i18n("Deleting messages..."));
    }
}

void KMMainWidget::slotMoveMessagesCompleted(KMMoveCommand *command)
{
    Q_ASSERT(command);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(command->refSet(), false);
    mMessagePane->deletePersistentSet(command->refSet());
    // Bleah :D
    const bool moveWasReallyADelete = !command->destFolder().isValid();

    if (command->result() == KMCommand::OK) {
        if (moveWasReallyADelete) {
            BroadcastStatus::instance()->setStatusMsg(i18n("Messages deleted successfully."));
        } else {
            BroadcastStatus::instance()->setStatusMsg(i18n("Messages moved successfully."));
        }
    } else {
        if (moveWasReallyADelete) {
            if (command->result() == KMCommand::Failed) {
                BroadcastStatus::instance()->setStatusMsg(i18n("Deleting messages failed."));
            } else {
                BroadcastStatus::instance()->setStatusMsg(i18n("Deleting messages canceled."));
            }
        } else {
            if (command->result() == KMCommand::Failed) {
                BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages failed."));
            } else {
                BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages canceled."));
            }
        }
    }
    // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotDeleteMsg(bool confirmDelete)
{
    // Create a persistent message set from the current selection
    MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
    if (ref != -1) {
        moveMessageSelected(ref, Akonadi::Collection(), confirmDelete);
    }
}

void KMMainWidget::slotDeleteThread(bool confirmDelete)
{
    // Create a persistent set from the current thread.
    MessageList::Core::MessageItemSetReference ref = mMessagePane->currentThreadAsPersistentSet();
    if (ref != -1) {
        moveMessageSelected(ref, Akonadi::Collection(), confirmDelete);
    }
}

FolderSelectionDialog *KMMainWidget::moveOrCopyToDialog()
{
    if (!mMoveOrCopyToDialog) {
        FolderSelectionDialog::SelectionFolderOption options = FolderSelectionDialog::HideVirtualFolder;
        mMoveOrCopyToDialog = new FolderSelectionDialog(this, options);
        mMoveOrCopyToDialog->setModal(true);
    }
    return mMoveOrCopyToDialog;
}

FolderSelectionDialog *KMMainWidget::selectFromAllFoldersDialog()
{
    if (!mSelectFromAllFoldersDialog) {
        FolderSelectionDialog::SelectionFolderOptions options = FolderSelectionDialog::None;
        options |= FolderSelectionDialog::NotAllowToCreateNewFolder;

        mSelectFromAllFoldersDialog = new FolderSelectionDialog(this, options);
        mSelectFromAllFoldersDialog->setModal(true);
    }
    return mSelectFromAllFoldersDialog;
}

void KMMainWidget::slotMoveSelectedMessageToFolder()
{
    QPointer<MailCommon::FolderSelectionDialog> dialog(moveOrCopyToDialog());
    dialog->setWindowTitle(i18n("Move Messages to Folder"));
    if (dialog->exec() && dialog) {
        const Akonadi::Collection dest = dialog->selectedCollection();
        if (dest.isValid()) {
            moveSelectedMessagesToFolder(dest);
        }
    }
}

void KMMainWidget::moveSelectedMessagesToFolder(const Akonadi::Collection &dest)
{
    MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
    if (ref != -1) {
        //Need to verify if dest == src ??? akonadi do it for us.
        moveMessageSelected(ref, dest, false);
    }
}

void KMMainWidget::copyMessageSelected(const QList<Akonadi::Item> &selectMsg, const Akonadi::Collection &dest)
{
    if (selectMsg.isEmpty()) {
        return;
    }
    // And stuff them into a KMCopyCommand :)
    KMCommand *command = new KMCopyCommand(dest, selectMsg);
    QObject::connect(
        command, SIGNAL(completed(KMCommand*)),
        this, SLOT(slotCopyMessagesCompleted(KMCommand*))
    );
    command->start();
    BroadcastStatus::instance()->setStatusMsg(i18n("Copying messages..."));
}

void KMMainWidget::slotCopyMessagesCompleted(KMCommand *command)
{
    Q_ASSERT(command);
    if (command->result() == KMCommand::OK) {
        BroadcastStatus::instance()->setStatusMsg(i18n("Messages copied successfully."));
    } else {
        if (command->result() == KMCommand::Failed) {
            BroadcastStatus::instance()->setStatusMsg(i18n("Copying messages failed."));
        } else {
            BroadcastStatus::instance()->setStatusMsg(i18n("Copying messages canceled."));
        }
    }
    // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotCopySelectedMessagesToFolder()
{
    QPointer<MailCommon::FolderSelectionDialog> dialog(moveOrCopyToDialog());
    dialog->setWindowTitle(i18n("Copy Messages to Folder"));

    if (dialog->exec() && dialog) {
        const Akonadi::Collection dest = dialog->selectedCollection();
        if (dest.isValid()) {
            copySelectedMessagesToFolder(dest);
        }
    }
}

void KMMainWidget::copySelectedMessagesToFolder(const Akonadi::Collection &dest)
{
    const QList<Akonadi::Item > lstMsg = mMessagePane->selectionAsMessageItemList();
    if (!lstMsg.isEmpty()) {
        copyMessageSelected(lstMsg, dest);
    }
}

//-----------------------------------------------------------------------------
// Message trashing
//
void KMMainWidget::trashMessageSelected(MessageList::Core::MessageItemSetReference ref)
{
    if (!mCurrentFolder) {
        return;
    }

    const QList<Akonadi::Item> select = mMessagePane->itemListFromPersistentSet(ref);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(ref, true);

    // FIXME: Why we don't use KMMoveCommand( trashFolder(), selectedMessages ); ?
    // And stuff them into a KMTrashMsgCommand :)
    KMCommand *command = new KMTrashMsgCommand(mCurrentFolder->collection(), select, ref);

    QObject::connect(
        command, SIGNAL(moveDone(KMMoveCommand*)),
        this, SLOT(slotTrashMessagesCompleted(KMMoveCommand*))
    );
    command->start();
    BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages to trash..."));
}

void KMMainWidget::slotTrashMessagesCompleted(KMMoveCommand *command)
{
    Q_ASSERT(command);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(command->refSet(), false);
    mMessagePane->deletePersistentSet(command->refSet());
    if (command->result() == KMCommand::OK) {
        BroadcastStatus::instance()->setStatusMsg(i18n("Messages moved to trash successfully."));
    } else {
        if (command->result() == KMCommand::Failed) {
            BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages to trash failed."));
        } else {
            BroadcastStatus::instance()->setStatusMsg(i18n("Moving messages to trash canceled."));
        }
    }

    // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotTrashSelectedMessages()
{
    MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
    if (ref != -1) {
        trashMessageSelected(ref);
    }
}

void KMMainWidget::slotTrashThread()
{
    MessageList::Core::MessageItemSetReference ref = mMessagePane->currentThreadAsPersistentSet();
    if (ref != -1) {
        trashMessageSelected(ref);
    }
}

//-----------------------------------------------------------------------------
// Message tag setting for messages
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....
void KMMainWidget::toggleMessageSetTag(const QList<Akonadi::Item> &select, const Akonadi::Tag &tag)
{
    if (select.isEmpty()) {
        return;
    }
    KMCommand *command = new KMSetTagCommand(Akonadi::Tag::List() << tag, select, KMSetTagCommand::Toggle);
    command->start();
}

void KMMainWidget::slotSelectMoreMessageTagList()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }

    TagSelectDialog dlg(this, selectedMessages.count(), selectedMessages.first());
    if (dlg.exec()) {
        const Akonadi::Tag::List lst = dlg.selectedTag();

        KMCommand *command = new KMSetTagCommand(lst, selectedMessages, KMSetTagCommand::CleanExistingAndAddNew);
        command->start();
    }
}

void KMMainWidget::slotUpdateMessageTagList(const Akonadi::Tag &tag)
{
    // Create a persistent set from the current thread.
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    toggleMessageSetTag(selectedMessages, tag);
}

void KMMainWidget::refreshMessageListSelection()
{
    mAkonadiStandardActionManager->setItemSelectionModel(mMessagePane->currentItemSelectionModel());
    slotMessageSelected(mMessagePane->currentItem());
}

//-----------------------------------------------------------------------------
// Status setting for threads
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....
void KMMainWidget::setMessageSetStatus(const QList<Akonadi::Item> &select,
                                       const Akonadi::MessageStatus &status,
                                       bool toggle)
{
    KMCommand *command = new KMSetStatusCommand(status, select, toggle);
    command->start();
}

void KMMainWidget::setCurrentThreadStatus(const Akonadi::MessageStatus &status, bool toggle)
{
    const QList<Akonadi::Item> select = mMessagePane->currentThreadAsMessageList();
    if (select.isEmpty()) {
        return;
    }
    setMessageSetStatus(select, status, toggle);
}

void KMMainWidget::slotSetThreadStatusUnread()
{
    setCurrentThreadStatus(MessageStatus::statusRead(), true);
}

void KMMainWidget::slotSetThreadStatusImportant()
{
    setCurrentThreadStatus(MessageStatus::statusImportant(), true);
}

void KMMainWidget::slotSetThreadStatusRead()
{
    setCurrentThreadStatus(MessageStatus::statusRead(), false);
}

void KMMainWidget::slotSetThreadStatusToAct()
{
    setCurrentThreadStatus(MessageStatus::statusToAct(), true);
}

void KMMainWidget::slotSetThreadStatusWatched()
{
    setCurrentThreadStatus(MessageStatus::statusWatched(), true);
    if (mWatchThreadAction->isChecked()) {
        mIgnoreThreadAction->setChecked(false);
    }
}

void KMMainWidget::slotSetThreadStatusIgnored()
{
    setCurrentThreadStatus(MessageStatus::statusIgnored(), true);
    if (mIgnoreThreadAction->isChecked()) {
        mWatchThreadAction->setChecked(false);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotRedirectMsg()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }

    KMCommand *command = new KMRedirectCommand(this, selectedMessages);
    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyToMsg(const QString &tmpl)
{
    const Akonadi::Item msg = mMessagePane->currentItem();
    if (!msg.isValid()) {
        return;
    }

    const QString text = mMsgView ? mMsgView->copyText() : QString();

    qDebug() << "Reply with template:" << tmpl;

    KMCommand *command = new KMReplyCommand(this,
                                            msg,
                                            MessageComposer::ReplySmart,
                                            text, false,
                                            tmpl);
    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCustomReplyAllToMsg(const QString &tmpl)
{
    const Akonadi::Item msg = mMessagePane->currentItem();
    if (!msg.isValid()) {
        return;
    }

    const QString text = mMsgView ? mMsgView->copyText() : QString();

    qDebug() << "Reply to All with template:" << tmpl;

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
void KMMainWidget::slotCustomForwardMsg(const QString &tmpl)
{
    if (!mCurrentFolder) {
        return;
    }

    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }

    qDebug() << "Forward with template:" << tmpl;
    KMForwardCommand *command = new KMForwardCommand(
        this, selectedMessages, mCurrentFolder->identity(), tmpl
    );

    command->start();
}

void KMMainWidget::openFilterDialog(const QByteArray &field, const QString &value)
{
    FilterIf->openFilterDialog(false);
    FilterIf->createFilter(field, value);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubjectFilter()
{
    const KMime::Message::Ptr msg = mMessagePane->currentMessage();
    if (!msg) {
        return;
    }

    openFilterDialog("Subject", msg->subject()->asUnicodeString());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFromFilter()
{
    KMime::Message::Ptr msg = mMessagePane->currentMessage();
    if (!msg) {
        return;
    }

    AddrSpecList al = MessageHelper::extractAddrSpecs(msg, "From");
    if (al.empty()) {
        openFilterDialog("From",  msg->from()->asUnicodeString());
    } else {
        openFilterDialog("From",  al.front().asString());
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToFilter()
{
    KMime::Message::Ptr msg = mMessagePane->currentMessage();
    if (!msg) {
        return;
    }
    openFilterDialog("To",  msg->to()->asUnicodeString());
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
    QPointer<MailCommon::FolderSelectionDialog> dialog(selectFromAllFoldersDialog());
    dialog->setWindowTitle(i18n("Jump to Folder"));
    if (dialog->exec() && dialog) {
        Akonadi::Collection collection = dialog->selectedCollection();
        if (collection.isValid()) {
            slotSelectCollectionFolder(collection);
        }
    }
}

void KMMainWidget::slotSelectCollectionFolder(const Akonadi::Collection &col)
{
    if (mFolderTreeWidget) {
        mFolderTreeWidget->selectCollectionFolder(col);
        slotFolderChanged(col);
    }
}

void KMMainWidget::slotApplyFilters()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    applyFilters(selectedMessages);
}

void KMMainWidget::slotApplyFiltersOnFolder()
{
    if (mCurrentFolder && mCurrentFolder->collection().isValid()) {
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mCurrentFolder->collection(), this);
        connect(job, &Akonadi::ItemFetchJob::result, this, &KMMainWidget::slotFetchItemsForFolderDone);
    }
}

void KMMainWidget::slotFetchItemsForFolderDone(KJob *job)
{
    Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob *>(job);
    Q_ASSERT(fjob);
    Akonadi::Item::List items = fjob->items();
    applyFilters(items);
}

void KMMainWidget::applyFilters(const QList<Akonadi::Item> &selectedMessages)
{
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif

    MailCommon::FilterManager::instance()->filter(selectedMessages);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckVacation()
{
    updateVacationScriptStatus(false);
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    mVacationManager->checkVacation();
    connect(mVacationManager, SIGNAL(updateVacationScriptStatus(bool,QString)), SLOT(updateVacationScriptStatus(bool,QString)));
    connect(mVacationManager, SIGNAL(editVacation()), SLOT(slotEditVacation()));
}

void KMMainWidget::slotEditVacation(const QString &serverName)
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    mVacationManager->slotEditVacation(serverName);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotDebugSieve()
{
#if !defined(NDEBUG)
    if (mSieveDebugDialog) {
        return;
    }

    mSieveDebugDialog = new KSieveUi::SieveDebugDialog(this);
    mSieveDebugDialog->exec();
    delete mSieveDebugDialog;
#endif
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotStartCertManager()
{
    if (!QProcess::startDetached(QLatin1String("kleopatra")))
        KMessageBox::error(this, i18n("Could not start certificate manager; "
                                      "please check your installation."),
                           i18n("KMail Error"));
    else {
        qDebug() << "\nslotStartCertManager(): certificate manager started.";
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotStartWatchGnuPG()
{
    if (!QProcess::startDetached(QLatin1String("kwatchgnupg")))
        KMessageBox::error(this, i18n("Could not start GnuPG LogViewer (kwatchgnupg); "
                                      "please check your installation."),
                           i18n("KMail Error"));
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotConfigChanged()
{
    readConfig();
    mMsgActions->setupForwardActions(actionCollection());
    mMsgActions->setupForwardingActionsList(mGUIClient);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveMsg()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand(this, selectedMessages);
    saveCommand->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOpenMsg()
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(this, QUrl(), overrideEncoding(), this);

    openCommand->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveAttachments()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    // Avoid re-downloading in the common case that only one message is selected, and the message
    // is also displayed in the viewer. For this, create a dummy item without a parent collection / item id,
    // so that KMCommand doesn't download it.
    KMSaveAttachmentsCommand *saveCommand = 0;
    if (mMsgView && selectedMessages.size() == 1 &&
            mMsgView->message().hasPayload<KMime::Message::Ptr>() &&
            selectedMessages.first().id() == mMsgView->message().id()) {
        Akonadi::Item dummyItem;
        dummyItem.setPayload<KMime::Message::Ptr>(mMsgView->message().payload<KMime::Message::Ptr>());
        saveCommand = new KMSaveAttachmentsCommand(this, dummyItem, mMsgView->viewer());
    } else {
        saveCommand = new KMSaveAttachmentsCommand(this, selectedMessages);
    }

    saveCommand->start();
}

void KMMainWidget::slotOnlineStatus()
{
    // KMKernel will emit a signal when we toggle the network state that is caught by
    // KMMainWidget::slotUpdateOnlineStatus to update our GUI
    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online) {
        // if online; then toggle and set it offline.
        kmkernel->stopNetworkJobs();
    } else {
        kmkernel->resumeNetworkJobs();
        slotCheckVacation();
    }
}

void KMMainWidget::slotUpdateOnlineStatus(GlobalSettings::EnumNetworkState::type)
{
    if (!mAkonadiStandardActionManager) {
        return;
    }
    QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::ToggleWorkOffline);
    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online) {
        action->setText(i18n("Work Offline"));
        action->setIcon(QIcon::fromTheme(QLatin1String("user-offline")));
    } else {
        action->setText(i18n("Work Online"));
        action->setIcon(QIcon::fromTheme(QLatin1String("user-online")));
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueued()
{
    if (kmkernel->msgSender()) {
        kmkernel->msgSender()->sendQueued();
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueuedVia(QAction *item)
{
    const QStringList availTransports = MailTransport::TransportManager::self()->transportNames();
    if (!availTransports.isEmpty() && availTransports.contains(item->text())) {
        if (kmkernel->msgSender()) {
            kmkernel->msgSender()->sendQueued(item->text());
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotShowBusySplash()
{
    if (mReaderWindowActive) {
        mMsgView->displayBusyPage();
    }
}

void KMMainWidget::showOfflinePage()
{
    if (!mReaderWindowActive) {
        return;
    }

    mMsgView->displayOfflinePage();
}

void KMMainWidget::showResourceOfflinePage()
{
    if (!mReaderWindowActive) {
        return;
    }

    mMsgView->displayResourceOfflinePage();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplaceMsgByUnencryptedVersion()
{
    qDebug();
    Akonadi::Item oldMsg = mMessagePane->currentItem();
    if (oldMsg.isValid()) {
#if 0
        qDebug() << "Old message found";
        if (oldMsg->hasUnencryptedMsg()) {
            qDebug() << "Extra unencrypted message found";
            KMime::Message *newMsg = oldMsg->unencryptedMsg();
            // adjust the message id
            {
                QString msgId(oldMsg->msgId());
                QString prefix("DecryptedMsg.");
                int oldIdx = msgId.indexOf(prefix, 0, Qt::CaseInsensitive);
                if (-1 == oldIdx) {
                    int leftAngle = msgId.lastIndexOf('<');
                    msgId = msgId.insert((-1 == leftAngle) ? 0 : ++leftAngle, prefix);
                } else {
                    // toggle between "DecryptedMsg." and "DeCryptedMsg."
                    // to avoid same message id
                    QCharRef c = msgId[ oldIdx + 2 ];
                    if ('C' == c) {
                        c = 'c';
                    } else {
                        c = 'C';
                    }
                }
                newMsg->setMsgId(msgId);
                mMsgView->setIdOfLastViewedMessage(msgId);
            }
            // insert the unencrypted message
            qDebug() << "Adding unencrypted message to folder";
            mFolder->addMsg(newMsg);
            /* Figure out its index in the folder for selecting. This must be count()-1,
            * since we append. Be safe and do find, though, just in case. */
            int newMsgIdx = mFolder->find(newMsg);
            Q_ASSERT(newMsgIdx != -1);
            /* we need this unget, to have the message displayed correctly initially */
            mFolder->unGetMsg(newMsgIdx);
            int idx = mFolder->find(oldMsg);
            Q_ASSERT(idx != -1);
            /* only select here, so the old one is not un-Gotten before, which would
            * render the pointer we hold invalid so that find would fail */
#if 0
            // FIXME (Pragma)
            mHeaders->setCurrentItemByIndex(newMsgIdx);
#endif
            // remove the old one
            if (idx != -1) {
                qDebug() << "Deleting encrypted message";
                mFolder->take(idx);
            }

            qDebug() << "Updating message actions";
            updateMessageActions();

            qDebug() << "Done.";
        } else {
            qDebug() << "NO EXTRA UNENCRYPTED MESSAGE FOUND";
        }
#else
        qDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    } else {
        qDebug() << "PANIC: NO OLD MESSAGE FOUND";
    }
}

void KMMainWidget::slotFocusOnNextMessage()
{
    mMessagePane->focusNextMessageItem(MessageList::Core::MessageTypeAny, true, false);
}

void KMMainWidget::slotFocusOnPrevMessage()
{
    mMessagePane->focusPreviousMessageItem(MessageList::Core::MessageTypeAny, true, false);
}

void KMMainWidget::slotSelectFirstMessage()
{
    mMessagePane->selectFirstMessageItem(MessageList::Core::MessageTypeAny, true);
}

void KMMainWidget::slotSelectLastMessage()
{
    mMessagePane->selectLastMessageItem(MessageList::Core::MessageTypeAny, true);
}

void KMMainWidget::slotSelectFocusedMessage()
{
    mMessagePane->selectFocusedMessageItem(true);
}

void KMMainWidget::slotSelectNextMessage()
{
    mMessagePane->selectNextMessageItem(MessageList::Core::MessageTypeAny,
                                        MessageList::Core::ClearExistingSelection,
                                        true, false);
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
    if (!mMessagePane->selectNextMessageItem(
                MessageList::Core::MessageTypeUnreadOnly,
                MessageList::Core::ClearExistingSelection,
                true,  // center item
                /*GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder*/
                GlobalSettings::self()->loopOnGotoUnread() != GlobalSettings::EnumLoopOnGotoUnread::DontLoop
            )) {
        // no next unread message was found in the current folder
        if ((GlobalSettings::self()->loopOnGotoUnread() ==
                GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders) ||
                (GlobalSettings::self()->loopOnGotoUnread() ==
                 GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders)) {
            mGoToFirstUnreadMessageInSelectedFolder = true;
            mFolderTreeWidget->folderTreeView()->selectNextUnreadFolder(true);
            mGoToFirstUnreadMessageInSelectedFolder = false;
        }
    }
}

void KMMainWidget::slotSelectPreviousMessage()
{
    mMessagePane->selectPreviousMessageItem(MessageList::Core::MessageTypeAny,
                                            MessageList::Core::ClearExistingSelection,
                                            true, false);
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
    if (!mMessagePane->selectPreviousMessageItem(
                MessageList::Core::MessageTypeUnreadOnly,
                MessageList::Core::ClearExistingSelection,
                true,  // center item
                GlobalSettings::self()->loopOnGotoUnread() == GlobalSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder
            )) {
        // no next unread message was found in the current folder
        if ((GlobalSettings::self()->loopOnGotoUnread() ==
                GlobalSettings::EnumLoopOnGotoUnread::LoopInAllFolders) ||
                (GlobalSettings::self()->loopOnGotoUnread() ==
                 GlobalSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders)) {
            mGoToFirstUnreadMessageInSelectedFolder = true;
            mFolderTreeWidget->folderTreeView()->selectPrevUnreadFolder();
            mGoToFirstUnreadMessageInSelectedFolder = false;
        }
    }
}

void KMMainWidget::slotDisplayCurrentMessage()
{
    if (mMessagePane->currentItem().isValid() && !mMessagePane->searchEditHasFocus()) {
        slotMessageActivated(mMessagePane->currentItem());
    }
}

// Called by double-clicked or 'Enter' in the messagelist -> pop up reader window
void KMMainWidget::slotMessageActivated(const Akonadi::Item &msg)
{
    if (!mCurrentFolder || !msg.isValid()) {
        return;
    }

    if (CommonKernel->folderIsDraftOrOutbox(mCurrentFolder->collection())) {
        mMsgActions->setCurrentMessage(msg);
        mMsgActions->editCurrentMessage();
        return;
    }

    if (CommonKernel->folderIsTemplates(mCurrentFolder->collection())) {
        slotUseTemplate();
        return;
    }

    // Try to fetch the mail, even in offline mode, it might be cached
    KMFetchMessageCommand *cmd = new KMFetchMessageCommand(this, msg);
    connect(cmd, SIGNAL(completed(KMCommand*)),
            this, SLOT(slotItemsFetchedForActivation(KMCommand*)));
    cmd->start();
}

void KMMainWidget::slotItemsFetchedForActivation(KMCommand *command)
{
    KMCommand::Result result = command->result();
    if (result != KMCommand::OK) {
        qDebug() << "Result:" << result;
        return;
    }

    KMFetchMessageCommand *fetchCmd = qobject_cast<KMFetchMessageCommand *>(command);
    const Item msg = fetchCmd->item();

    KMReaderMainWin *win = new KMReaderMainWin(mFolderDisplayFormatPreference, mFolderHtmlLoadExtPreference);
    const bool useFixedFont = mMsgView ? mMsgView->isFixedFont() :
                              MessageViewer::GlobalSettings::self()->useFixedFont();
    win->setUseFixedFont(useFixedFont);

    const Akonadi::Collection parentCollection = MailCommon::Util::parentCollectionFromItem(msg);
    win->showMessage(overrideEncoding(), msg, parentCollection);
    win->show();
}

void KMMainWidget::slotMessageStatusChangeRequest(const Akonadi::Item &item, const Akonadi::MessageStatus &set, const Akonadi::MessageStatus &clear)
{
    if (!item.isValid()) {
        return;
    }

    if (clear.toQInt32() != Akonadi::MessageStatus().toQInt32()) {
        KMCommand *command = new KMSetStatusCommand(clear, Akonadi::Item::List() << item, true);
        command->start();
    }

    if (set.toQInt32() != Akonadi::MessageStatus().toQInt32()) {
        KMCommand *command = new KMSetStatusCommand(set, Akonadi::Item::List() << item, false);
        command->start();
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAll()
{
    mMessagePane->selectAll();
    updateMessageActions();
}

void KMMainWidget::slotMessagePopup(const Akonadi::Item &msg , const KUrl &aUrl, const KUrl &imageUrl, const QPoint &aPoint)
{
    updateMessageMenu();

    const QString email =  KPIMUtils::firstEmailAddress(aUrl.path()).toLower();
    if (aUrl.scheme() == QLatin1String("mailto") && !email.isEmpty()) {
        Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob(this);
        job->setLimit(1);
        job->setQuery(Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch);
        job->setProperty("msg", QVariant::fromValue(msg));
        job->setProperty("point", aPoint);
        job->setProperty("imageUrl", imageUrl);
        job->setProperty("url", aUrl);
        connect(job, &Akonadi::ContactSearchJob::result, this, &KMMainWidget::slotContactSearchJobForMessagePopupDone);
    } else {
        showMessagePopup(msg, aUrl, imageUrl, aPoint, false, false);
    }
}

void KMMainWidget::slotContactSearchJobForMessagePopupDone(KJob *job)
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);
    const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

    const QList<Akonadi::Item> listContact = searchJob->items();
    const bool uniqueContactFound = (listContact.count() == 1);
    if (uniqueContactFound) {
        mMsgView->setContactItem(listContact.first(), searchJob->contacts().at(0));
    } else {
        mMsgView->clearContactItem();
    }
    const Akonadi::Item msg = job->property("msg").value<Akonadi::Item>();
    const QPoint aPoint = job->property("point").toPoint();
    const KUrl imageUrl = job->property("imageUrl").value<KUrl>();
    const KUrl url = job->property("url").value<KUrl>();

    showMessagePopup(msg, url, imageUrl, aPoint, contactAlreadyExists, uniqueContactFound);
}

void KMMainWidget::showMessagePopup(const Akonadi::Item &msg , const KUrl &url, const KUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound)
{
    QMenu *menu = new QMenu;

    bool urlMenuAdded = false;

    if (!url.isEmpty()) {
        if (url.scheme() == QLatin1String("mailto")) {
            // popup on a mailto URL
            menu->addAction(mMsgView->mailToComposeAction());
            menu->addAction(mMsgView->mailToReplyAction());
            menu->addAction(mMsgView->mailToForwardAction());

            menu->addSeparator();

            if (contactAlreadyExists) {
                if (uniqueContactFound) {
                    menu->addAction(mMsgView->editContactAction());
                } else {
                    menu->addAction(mMsgView->openAddrBookAction());
                }
            } else {
                menu->addAction(mMsgView->addAddrBookAction());
                menu->addAction(mMsgView->addToExistingContactAction());
            }
            menu->addSeparator();
            menu->addMenu(mMsgView->viewHtmlOption());
            menu->addSeparator();
            menu->addAction(mMsgView->copyURLAction());
            urlMenuAdded = true;
        } else if (url.scheme() != QLatin1String("attachment")) {
            // popup on a not-mailto URL
            menu->addAction(mMsgView->urlOpenAction());
            menu->addAction(mMsgView->addBookmarksAction());
            menu->addAction(mMsgView->urlSaveAsAction());
            menu->addAction(mMsgView->copyURLAction());
            if (mMsgView->isAShortUrl(url)) {
                menu->addSeparator();
                menu->addAction(mMsgView->expandShortUrlAction());
            }
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                menu->addAction(mMsgView->copyImageLocation());
                menu->addAction(mMsgView->downloadImageToDiskAction());
                menu->addAction(mMsgView->shareImage());
                if (mMsgView->adblockEnabled()) {
                    menu->addSeparator();
                    menu->addAction(mMsgView->blockImage());
                }
            }
            urlMenuAdded = true;
        }
        qDebug() << "URL is:" << url;
    }
    const QString selectedText = mMsgView ? mMsgView->copyText() : QString();
    if (mMsgView && !selectedText.isEmpty()) {
        if (urlMenuAdded) {
            menu->addSeparator();
        }
        menu->addAction(mMsgActions->replyMenu());
        menu->addSeparator();

        menu->addAction(mMsgView->copyAction());
        menu->addAction(mMsgView->selectAllAction());
        menu->addSeparator();
        mMsgActions->addWebShortcutsMenu(menu, selectedText);
        menu->addSeparator();
        menu->addAction(mMsgView->translateAction());
        menu->addSeparator();
        menu->addAction(mMsgView->speakTextAction());
    } else if (!urlMenuAdded) {
        // popup somewhere else (i.e., not a URL) on the message
        if (!mMessagePane->currentMessage()) {
            // no messages
            delete menu;
            return;
        }
        Akonadi::Collection parentCol = msg.parentCollection();
        if (parentCol.isValid() && CommonKernel->folderIsTemplates(parentCol)) {
            menu->addAction(mUseAction);
        } else {
            menu->addAction(mMsgActions->replyMenu());
            menu->addAction(mMsgActions->forwardMenu());
        }
        if (parentCol.isValid() && CommonKernel->folderIsSentMailFolder(parentCol)) {
            menu->addAction(sendAgainAction());
        } else {
            menu->addAction(editAction());
        }
        menu->addAction(mailingListActionMenu());
        menu->addSeparator();

        menu->addAction(mCopyActionMenu);
        menu->addAction(mMoveActionMenu);

        menu->addSeparator();

        menu->addAction(mMsgActions->messageStatusMenu());
        menu->addSeparator();
        if (mMsgView) {
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                menu->addAction(mMsgView->copyImageLocation());
                menu->addAction(mMsgView->downloadImageToDiskAction());
                menu->addAction(mMsgView->shareImage());
                menu->addSeparator();
                if (mMsgView->adblockEnabled()) {
                    menu->addAction(mMsgView->blockImage());
                    menu->addSeparator();
                }
            }
            menu->addAction(mMsgView->viewSourceAction());
            menu->addAction(mMsgView->toggleFixFontAction());
            menu->addAction(mMsgView->toggleMimePartTreeAction());
        }
        menu->addSeparator();
        if (mMsgActions->printPreviewAction()) {
            menu->addAction(mMsgActions->printPreviewAction());
        }
        menu->addAction(mMsgActions->printAction());
        menu->addAction(mSaveAsAction);
        menu->addAction(mSaveAttachmentsAction);
        menu->addSeparator();
        if (parentCol.isValid() && CommonKernel->folderIsTrash(parentCol)) {
            menu->addAction(mDeleteAction);
        } else {
            menu->addAction(akonadiStandardAction(Akonadi::StandardMailActionManager::MoveToTrash));
        }
        menu->addSeparator();
        menu->addAction(mMsgView->createTodoAction());
        menu->addAction(mMsgView->createEventAction());
        menu->addSeparator();
        if (mMsgView) {
            menu->addAction(mMsgView->saveMessageDisplayFormatAction());
            menu->addAction(mMsgView->resetMessageDisplayFormatAction());
            menu->addSeparator();
        }
        menu->addAction(mMsgActions->annotateAction());
        if (mMsgView && mMsgView->adblockEnabled()) {
            menu->addSeparator();
            menu->addAction(mMsgView->openBlockableItems());
        }
        menu->addAction(mMsgActions->addFollowupReminderAction());
        if (kmkernel->allowToDebugBalooSupport()) {
            menu->addSeparator();
            menu->addAction(mMsgActions->debugBalooAction());
        }
    }
    KAcceleratorManager::manage(menu);
    menu->exec(aPoint, 0);
    delete menu;
}
//-----------------------------------------------------------------------------
void KMMainWidget::getAccountMenu()
{
    mActMenu->clear();
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    foreach (const Akonadi::AgentInstance &type, lst) {
        // Explicitly make a copy, as we're not changing values of the list but only
        // the local copy which is passed to action.
        QAction *action = mActMenu->addAction(QString(type.name()).replace(QLatin1Char('&'), QLatin1String("&&")));
        action->setData(type.identifier());
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::getTransportMenu()
{

    mSendMenu->clear();
    QStringList availTransports = MailTransport::TransportManager::self()->transportNames();
    QStringList::Iterator it;
    QStringList::Iterator end(availTransports.end());

    for (it = availTransports.begin(); it != end ; ++it) {
        mSendMenu->addAction((*it).replace(QLatin1Char('&'), QLatin1String("&&")));
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::setupActions()
{
    mMsgActions = new KMail::MessageActions(actionCollection(), this);
    mMsgActions->setMessageView(mMsgView);

    //----- File Menu
    mSaveAsAction = new QAction(QIcon::fromTheme(QLatin1String("document-save")), i18n("Save &As..."), this);
    actionCollection()->addAction(QLatin1String("file_save_as"), mSaveAsAction);
    connect(mSaveAsAction, &QAction::triggered, this, &KMMainWidget::slotSaveMsg);
    actionCollection()->setDefaultShortcut(mSaveAsAction, KStandardShortcut::save().first());

    mOpenAction = KStandardAction::open(this, SLOT(slotOpenMsg()),
                                        actionCollection());

    mOpenRecentAction = KStandardAction::openRecent(this, SLOT(slotOpenRecentMsg(QUrl)),
                        actionCollection());
    KConfigGroup grp = mConfig->group(QLatin1String("Recent Files"));
    mOpenRecentAction->loadEntries(grp);

    {
        QAction *action = new QAction(i18n("&Expire All Folders"), this);
        actionCollection()->addAction(QLatin1String("expire_all_folders"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpireAll);
    }
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("mail-receive")), i18n("Check &Mail"), this);
        actionCollection()->addAction(QLatin1String("check_mail"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCheckMail);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_L));
    }

    KActionMenu *actActionMenu = new KActionMenu(QIcon::fromTheme(QLatin1String("mail-receive")), i18n("Check Mail In"), this);
    actActionMenu->setIconText(i18n("Check Mail"));
    actActionMenu->setToolTip(i18n("Check Mail"));
    actionCollection()->addAction(QLatin1String("check_mail_in"), actActionMenu);
    actActionMenu->setDelayed(true); //needed for checking "all accounts"
    connect(actActionMenu, &KActionMenu::triggered, this, &KMMainWidget::slotCheckMail);
    mActMenu = actActionMenu->menu();
    connect(mActMenu, SIGNAL(triggered(QAction*)),
            SLOT(slotCheckOneAccount(QAction*)));
    connect(mActMenu, &QMenu::aboutToShow, this, &KMMainWidget::getAccountMenu);

    mSendQueued = new QAction(QIcon::fromTheme(QLatin1String("mail-send")), i18n("&Send Queued Messages"), this);
    actionCollection()->addAction(QLatin1String("send_queued"), mSendQueued);
    connect(mSendQueued, &QAction::triggered, this, &KMMainWidget::slotSendQueued);
    {

        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::ToggleWorkOffline);
        mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::ToggleWorkOffline);
        action->setCheckable(false);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotOnlineStatus);
        action->setText(i18n("Online status (unknown)"));
    }

    mSendActionMenu = new KActionMenu(QIcon::fromTheme(QLatin1String("mail-send-via")), i18n("Send Queued Messages Via"), this);
    actionCollection()->addAction(QLatin1String("send_queued_via"), mSendActionMenu);
    mSendActionMenu->setDelayed(true);

    mSendMenu = mSendActionMenu->menu();
    connect(mSendMenu, &QMenu::triggered, this, &KMMainWidget::slotSendQueuedVia);
    connect(mSendMenu, &QMenu::aboutToShow, this, &KMMainWidget::getTransportMenu);

    //----- Tools menu
    if (parent()->inherits("KMMainWin")) {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("x-office-address-book")), i18n("&Address Book"), this);
        actionCollection()->addAction(QLatin1String("addressbook"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAddrBook);
        if (QStandardPaths::findExecutable(QLatin1String("kaddressbook")).isEmpty()) {
            action->setEnabled(false);
        }
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("pgp-keys")), i18n("Certificate Manager"), this);
        actionCollection()->addAction(QLatin1String("tools_start_certman"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotStartCertManager);
        // disable action if no certman binary is around
        if (QStandardPaths::findExecutable(QLatin1String("kleopatra")).isEmpty()) {
            action->setEnabled(false);
        }
    }
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("pgp-keys")), i18n("GnuPG Log Viewer"), this);
        actionCollection()->addAction(QLatin1String("tools_start_kwatchgnupg"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotStartWatchGnuPG);
#ifdef Q_OS_WIN32
        // not ported yet, underlying infrastructure missing on Windows
        const bool usableKWatchGnupg = false;
#else
        // disable action if no kwatchgnupg binary is around
        bool usableKWatchGnupg = !QStandardPaths::findExecutable(QLatin1String("kwatchgnupg")).isEmpty();
#endif
        action->setEnabled(usableKWatchGnupg);
    }
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("document-import")), i18n("&Import Messages..."), this);
        actionCollection()->addAction(QLatin1String("import"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotImport);
        if (QStandardPaths::findExecutable(QLatin1String("kmailcvt")).isEmpty()) {
            action->setEnabled(false);
        }
    }

#if !defined(NDEBUG)
    {
        QAction *action = new QAction(i18n("&Debug Sieve..."), this);
        actionCollection()->addAction(QLatin1String("tools_debug_sieve"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotDebugSieve);
    }
#endif

    {
        QAction *action = new QAction(i18n("Filter &Log Viewer..."), this);
        actionCollection()->addAction(QLatin1String("filter_log_viewer"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotFilterLogViewer);
    }
    {
        QAction *action = new QAction(i18n("&Anti-Spam Wizard..."), this);
        actionCollection()->addAction(QLatin1String("antiSpamWizard"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAntiSpamWizard);
    }
    {
        QAction *action = new QAction(i18n("&Anti-Virus Wizard..."), this);
        actionCollection()->addAction(QLatin1String("antiVirusWizard"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAntiVirusWizard);
    }
    {
        QAction *action = new QAction(i18n("&Account Wizard..."), this);
        actionCollection()->addAction(QLatin1String("accountWizard"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAccountWizard);
    }
    {
        QAction *action = new QAction(i18n("&Import Wizard..."), this);
        actionCollection()->addAction(QLatin1String("importWizard"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotImportWizard);
    }
    if (KSieveUi::Util::allowOutOfOfficeSettings()) {
        QAction *action = new QAction(i18n("Edit \"Out of Office\" Replies..."), this);
        actionCollection()->addAction(QLatin1String("tools_edit_vacation"), action);
        connect(action, SIGNAL(triggered(bool)), SLOT(slotEditVacation()));
    }

    {
        QAction *action = new QAction(i18n("&Configure Automatic Archiving..."), this);
        actionCollection()->addAction(QLatin1String("tools_automatic_archiving"), action);
        connect(action, &QAction::triggered, mConfigAgent, &KMConfigureAgent::slotConfigureAutomaticArchiving);
    }

    {
        QAction *action = new QAction(i18n("Delayed Messages..."), this);
        actionCollection()->addAction(QLatin1String("message_delayed"), action);
        connect(action, &QAction::triggered, mConfigAgent, &KMConfigureAgent::slotConfigureSendLater);
    }

    {
        QAction *action = new QAction(i18n("Followup Reminder Messages..."), this);
        actionCollection()->addAction(QLatin1String("followup_reminder_messages"), action);
        connect(action, &QAction::triggered, mConfigAgent, &KMConfigureAgent::slotConfigureFollowupReminder);
    }

    // Disable the standard action delete key sortcut.
    QAction *const standardDelAction = akonadiStandardAction(Akonadi::StandardActionManager::DeleteItems);
    standardDelAction->setShortcut(QKeySequence());

    //----- Edit Menu

    /* The delete action is nowhere in the gui, by default, so we need to make
    * sure it is plugged into the KAccel now, since that won't happen on
    * XMLGui construction or manual ->plug(). This is only a problem when run
    * as a part, though. */
    mDeleteAction = new QAction(QIcon::fromTheme(QLatin1String("edit-delete")), i18nc("@action Hard delete, bypassing trash", "&Delete"), this);
    actionCollection()->addAction(QLatin1String("delete"), mDeleteAction);
    connect(mDeleteAction, SIGNAL(triggered(bool)), SLOT(slotDeleteMsg()));
    actionCollection()->setDefaultShortcut(mDeleteAction, QKeySequence(Qt::SHIFT + Qt::Key_Delete));

    mTrashThreadAction = new QAction(i18n("M&ove Thread to Trash"), this);
    actionCollection()->addAction(QLatin1String("move_thread_to_trash"), mTrashThreadAction);
    actionCollection()->setDefaultShortcut(mTrashThreadAction, QKeySequence(Qt::CTRL + Qt::Key_Delete));
    mTrashThreadAction->setIcon(QIcon::fromTheme(QLatin1String("user-trash")));
    KMail::Util::addQActionHelpText(mTrashThreadAction, i18n("Move thread to trashcan"));
    connect(mTrashThreadAction, &QAction::triggered, this, &KMMainWidget::slotTrashThread);

    mDeleteThreadAction = new QAction(QIcon::fromTheme(QLatin1String("edit-delete")), i18n("Delete T&hread"), this);
    actionCollection()->addAction(QLatin1String("delete_thread"), mDeleteThreadAction);
    connect(mDeleteThreadAction, SIGNAL(triggered(bool)), SLOT(slotDeleteThread()));
    actionCollection()->setDefaultShortcut(mDeleteThreadAction, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Delete));

    mSearchMessages = new QAction(QIcon::fromTheme(QLatin1String("edit-find-mail")), i18n("&Find Messages..."), this);
    actionCollection()->addAction(QLatin1String("search_messages"), mSearchMessages);
    connect(mSearchMessages, &QAction::triggered, this, &KMMainWidget::slotRequestFullSearchFromQuickSearch);
    actionCollection()->setDefaultShortcut(mSearchMessages, QKeySequence(Qt::Key_S));

    {
        QAction *action = new QAction(i18n("Select &All Messages"), this);
        actionCollection()->addAction(QLatin1String("mark_all_messages"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotMarkAll);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_A));
    }

    //----- Folder Menu

    mFolderMailingListPropertiesAction = new QAction(i18n("&Mailing List Management..."), this);
    actionCollection()->addAction(QLatin1String("folder_mailinglist_properties"), mFolderMailingListPropertiesAction);
    connect(mFolderMailingListPropertiesAction, &QAction::triggered, mManageShowCollectionProperties, &ManageShowCollectionProperties::slotFolderMailingListProperties);
    // mFolderMailingListPropertiesAction->setIcon(QIcon::fromTheme("document-properties-mailing-list"));

    mShowFolderShortcutDialogAction = new QAction(QIcon::fromTheme(QLatin1String("configure-shortcuts")), i18n("&Assign Shortcut..."), this);
    actionCollection()->addAction(QLatin1String("folder_shortcut_command"), mShowFolderShortcutDialogAction);
    connect(mShowFolderShortcutDialogAction, SIGNAL(triggered(bool)), mManageShowCollectionProperties,
            SLOT(slotShowFolderShortcutDialog()));
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
    mExpireFolderAction = new QAction(i18n("&Expiration Settings"), this);
    actionCollection()->addAction(QLatin1String("expire"), mExpireFolderAction);
    connect(mExpireFolderAction, SIGNAL(triggered(bool)), this, SLOT(slotExpireFolder()));

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::MoveToTrash);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveToTrash), SIGNAL(triggered(bool)), this, SLOT(slotTrashSelectedMessages()));

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::MoveAllToTrash);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash), SIGNAL(triggered(bool)), this, SLOT(slotEmptyFolder()));

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::DeleteCollections);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections), SIGNAL(triggered(bool)), this, SLOT(slotRemoveFolder()));

    // ### PORT ME: Add this to the context menu. Not possible right now because
    //              the context menu uses XMLGUI, and that would add the entry to
    //              all collection context menus
    mArchiveFolderAction = new QAction(i18n("&Archive Folder..."), this);
    actionCollection()->addAction(QLatin1String("archive_folder"), mArchiveFolderAction);
    connect(mArchiveFolderAction, &QAction::triggered, this, &KMMainWidget::slotArchiveFolder);

    mDisplayMessageFormatMenu = new DisplayMessageFormatActionMenu(this);
    connect(mDisplayMessageFormatMenu, &DisplayMessageFormatActionMenu::changeDisplayMessageFormat, this, &KMMainWidget::slotChangeDisplayMessageFormat);
    actionCollection()->addAction(QLatin1String("display_format_message"), mDisplayMessageFormatMenu);

    mPreferHtmlLoadExtAction = new KToggleAction(i18n("Load E&xternal References"), this);
    actionCollection()->addAction(QLatin1String("prefer_html_external_refs"), mPreferHtmlLoadExtAction);
    connect(mPreferHtmlLoadExtAction, &KToggleAction::triggered, this, &KMMainWidget::slotOverrideHtmlLoadExt);

    {
        QAction *action =  mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyCollections);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_C));
    }
    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::Paste);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_V));
    }
    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyItems);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::CTRL + Qt::Key_C));
    }
    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CutItems);
        action->setShortcut(QKeySequence(Qt::ALT + Qt::CTRL + Qt::Key_X));
    }

    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyItemToMenu);
        action->setText(i18n("Copy Message To..."));
        action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveItemToMenu);
        action->setText(i18n("Move Message To..."));
    }

    //----- Message Menu
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("mail-message-new")), i18n("&New Message..."), this);
        actionCollection()->addAction(QLatin1String("new_message"), action);
        action->setIconText(i18nc("@action:intoolbar New Empty Message", "New"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCompose);
        // do not set a New shortcut if kmail is a component
        if (!kmkernel->xmlGuiInstance().isValid()) {
            action->setShortcut(KStandardShortcut::openNew().first());
        }
    }

    mTemplateMenu = new KActionMenu(QIcon::fromTheme(QLatin1String("document-new")), i18n("Message From &Template"),
                                    actionCollection());
    mTemplateMenu->setDelayed(true);
    actionCollection()->addAction(QLatin1String("new_from_template"), mTemplateMenu);
    connect(mTemplateMenu->menu(), SIGNAL(aboutToShow()), this,
            SLOT(slotShowNewFromTemplate()));
    connect(mTemplateMenu->menu(), SIGNAL(triggered(QAction*)), this,
            SLOT(slotNewFromTemplate(QAction*)));

    mMessageNewList = new QAction(QIcon::fromTheme(QLatin1String("mail-message-new-list")),
                                  i18n("New Message t&o Mailing-List..."),
                                  this);
    actionCollection()->addAction(QLatin1String("post_message"),  mMessageNewList);
    connect(mMessageNewList, SIGNAL(triggered(bool)),
            SLOT(slotPostToML()));
    actionCollection()->setDefaultShortcut(mMessageNewList, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));

    mSendAgainAction = new QAction(i18n("Send A&gain..."), this);
    actionCollection()->addAction(QLatin1String("send_again"), mSendAgainAction);
    connect(mSendAgainAction, &QAction::triggered, this, &KMMainWidget::slotResendMsg);

    //----- Create filter actions
    mFilterMenu = new KActionMenu(QIcon::fromTheme(QLatin1String("view-filter")), i18n("&Create Filter"), this);
    actionCollection()->addAction(QLatin1String("create_filter"), mFilterMenu);
    connect(mFilterMenu, SIGNAL(triggered(bool)), this,
            SLOT(slotFilter()));
    {
        QAction *action = new QAction(i18n("Filter on &Subject..."), this);
        actionCollection()->addAction(QLatin1String("subject_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSubjectFilter);
        mFilterMenu->addAction(action);
    }

    {
        QAction *action = new QAction(i18n("Filter on &From..."), this);
        actionCollection()->addAction(QLatin1String("from_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotFromFilter);
        mFilterMenu->addAction(action);
    }
    {
        QAction *action = new QAction(i18n("Filter on &To..."), this);
        actionCollection()->addAction(QLatin1String("to_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotToFilter);
        mFilterMenu->addAction(action);
    }
    mFilterMenu->addAction(mMsgActions->listFilterAction());

    mUseAction = new QAction(QIcon::fromTheme(QLatin1String("document-new")), i18n("New Message From &Template"), this);
    actionCollection()->addAction(QLatin1String("use_template"), mUseAction);
    connect(mUseAction, &QAction::triggered, this, &KMMainWidget::slotUseTemplate);
    actionCollection()->setDefaultShortcut(mUseAction, QKeySequence(Qt::SHIFT + Qt::Key_N));

    //----- "Mark Thread" submenu
    mThreadStatusMenu = new KActionMenu(i18n("Mark &Thread"), this);
    actionCollection()->addAction(QLatin1String("thread_status"), mThreadStatusMenu);

    mMarkThreadAsReadAction = new QAction(QIcon::fromTheme(QLatin1String("mail-mark-read")), i18n("Mark Thread as &Read"), this);
    actionCollection()->addAction(QLatin1String("thread_read"), mMarkThreadAsReadAction);
    connect(mMarkThreadAsReadAction, &QAction::triggered, this, &KMMainWidget::slotSetThreadStatusRead);
    KMail::Util::addQActionHelpText(mMarkThreadAsReadAction, i18n("Mark all messages in the selected thread as read"));
    mThreadStatusMenu->addAction(mMarkThreadAsReadAction);

    mMarkThreadAsUnreadAction = new QAction(QIcon::fromTheme(QLatin1String("mail-mark-unread")), i18n("Mark Thread as &Unread"), this);
    actionCollection()->addAction(QLatin1String("thread_unread"), mMarkThreadAsUnreadAction);
    connect(mMarkThreadAsUnreadAction, &QAction::triggered, this, &KMMainWidget::slotSetThreadStatusUnread);
    KMail::Util::addQActionHelpText(mMarkThreadAsUnreadAction, i18n("Mark all messages in the selected thread as unread"));
    mThreadStatusMenu->addAction(mMarkThreadAsUnreadAction);

    mThreadStatusMenu->addSeparator();

    //----- "Mark Thread" toggle actions
    mToggleThreadImportantAction = new KToggleAction(QIcon::fromTheme(QLatin1String("mail-mark-important")), i18n("Mark Thread as &Important"), this);
    actionCollection()->addAction(QLatin1String("thread_flag"), mToggleThreadImportantAction);
    connect(mToggleThreadImportantAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusImportant);
    mToggleThreadImportantAction->setCheckedState(KGuiItem(i18n("Remove &Important Thread Mark")));
    mThreadStatusMenu->addAction(mToggleThreadImportantAction);

    mToggleThreadToActAction = new KToggleAction(QIcon::fromTheme(QLatin1String("mail-mark-task")), i18n("Mark Thread as &Action Item"), this);
    actionCollection()->addAction(QLatin1String("thread_toact"), mToggleThreadToActAction);
    connect(mToggleThreadToActAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusToAct);
    mToggleThreadToActAction->setCheckedState(KGuiItem(i18n("Remove &Action Item Thread Mark")));
    mThreadStatusMenu->addAction(mToggleThreadToActAction);

    //------- "Watch and ignore thread" actions
    mWatchThreadAction = new KToggleAction(QIcon::fromTheme(QLatin1String("mail-thread-watch")), i18n("&Watch Thread"), this);
    actionCollection()->addAction(QLatin1String("thread_watched"), mWatchThreadAction);
    connect(mWatchThreadAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusWatched);

    mIgnoreThreadAction = new KToggleAction(QIcon::fromTheme(QLatin1String("mail-thread-ignored")), i18n("&Ignore Thread"), this);
    actionCollection()->addAction(QLatin1String("thread_ignored"), mIgnoreThreadAction);
    connect(mIgnoreThreadAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusIgnored);

    mThreadStatusMenu->addSeparator();
    mThreadStatusMenu->addAction(mWatchThreadAction);
    mThreadStatusMenu->addAction(mIgnoreThreadAction);

    mSaveAttachmentsAction = new QAction(QIcon::fromTheme(QLatin1String("mail-attachment")), i18n("Save A&ttachments..."), this);
    actionCollection()->addAction(QLatin1String("file_save_attachments"), mSaveAttachmentsAction);
    connect(mSaveAttachmentsAction, &QAction::triggered, this, &KMMainWidget::slotSaveAttachments);

    mMoveActionMenu = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveItemToMenu);

    mCopyActionMenu = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyItemToMenu);

    mApplyAllFiltersAction =
        new QAction(QIcon::fromTheme(QLatin1String("view-filter")), i18n("Appl&y All Filters"), this);
    actionCollection()->addAction(QLatin1String("apply_filters"), mApplyAllFiltersAction);
    connect(mApplyAllFiltersAction, SIGNAL(triggered(bool)),
            SLOT(slotApplyFilters()));
    actionCollection()->setDefaultShortcut(mApplyAllFiltersAction, QKeySequence(Qt::CTRL + Qt::Key_J));

    mApplyFilterActionsMenu = new KActionMenu(i18n("A&pply Filter"), this);
    actionCollection()->addAction(QLatin1String("apply_filter_actions"), mApplyFilterActionsMenu);

    {
        QAction *action = new QAction(i18nc("View->", "&Expand Thread / Group"), this);
        actionCollection()->addAction(QLatin1String("expand_thread"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Period));
        KMail::Util::addQActionHelpText(action, i18n("Expand the current thread or group"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpandThread);
    }
    {
        QAction *action = new QAction(i18nc("View->", "&Collapse Thread / Group"), this);
        actionCollection()->addAction(QLatin1String("collapse_thread"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Comma));
        KMail::Util::addQActionHelpText(action, i18n("Collapse the current thread or group"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCollapseThread);
    }
    {
        QAction *action = new QAction(i18nc("View->", "Ex&pand All Threads"), this);
        actionCollection()->addAction(QLatin1String("expand_all_threads"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Period));
        KMail::Util::addQActionHelpText(action, i18n("Expand all threads in the current folder"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpandAllThreads);
    }
    {
        QAction *action = new QAction(i18nc("View->", "C&ollapse All Threads"), this);
        actionCollection()->addAction(QLatin1String("collapse_all_threads"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Comma));
        KMail::Util::addQActionHelpText(action, i18n("Collapse all threads in the current folder"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCollapseAllThreads);
    }

    QAction *dukeOfMonmoth = new QAction(i18n("&Display Message"), this);
    actionCollection()->addAction(QLatin1String("display_message"), dukeOfMonmoth);
    connect(dukeOfMonmoth, &QAction::triggered, this, &KMMainWidget::slotDisplayCurrentMessage);
    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(Qt::Key_Enter) << QKeySequence(Qt::Key_Return);
    actionCollection()->setDefaultShortcuts(dukeOfMonmoth, shortcuts);

    //----- Go Menu
    {
        QAction *action = new QAction(i18n("&Next Message"), this);
        actionCollection()->addAction(QLatin1String("go_next_message"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(QLatin1String("N; Right")));
        KMail::Util::addQActionHelpText(action, i18n("Go to the next message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectNextMessage);
    }
    {
        QAction *action = new QAction(i18n("Next &Unread Message"), this);
        actionCollection()->addAction(QLatin1String("go_next_unread_message"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Plus));
        if (QApplication::isRightToLeft()) {
            action->setIcon(QIcon::fromTheme(QLatin1String("go-previous")));
        } else {
            action->setIcon(QIcon::fromTheme(QLatin1String("go-next")));
        }
        action->setIconText(i18nc("@action:inmenu Goto next unread message", "Next"));
        KMail::Util::addQActionHelpText(action, i18n("Go to the next unread message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectNextUnreadMessage);
    }
    {
        QAction *action = new QAction(i18n("&Previous Message"), this);
        actionCollection()->addAction(QLatin1String("go_prev_message"), action);
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous message"));
        actionCollection()->setDefaultShortcut(action, QKeySequence(QLatin1String("P; Left")));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectPreviousMessage);
    }
    {
        QAction *action = new QAction(i18n("Previous Unread &Message"), this);
        actionCollection()->addAction(QLatin1String("go_prev_unread_message"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Minus));
        if (QApplication::isRightToLeft()) {
            action->setIcon(QIcon::fromTheme(QLatin1String("go-next")));
        } else {
            action->setIcon(QIcon::fromTheme(QLatin1String("go-previous")));
        }
        action->setIconText(i18nc("@action:inmenu Goto previous unread message.", "Previous"));
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous unread message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectPreviousUnreadMessage);
    }
    {
        QAction *action = new QAction(i18n("Next Unread &Folder"), this);
        actionCollection()->addAction(QLatin1String("go_next_unread_folder"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotNextUnreadFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Plus));
        KMail::Util::addQActionHelpText(action, i18n("Go to the next folder with unread messages"));
    }
    {
        QAction *action = new QAction(i18n("Previous Unread F&older"), this);
        actionCollection()->addAction(QLatin1String("go_prev_unread_folder"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Minus));
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous folder with unread messages"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotPrevUnreadFolder);
    }
    {
        QAction *action = new QAction(i18nc("Go->", "Next Unread &Text"), this);
        actionCollection()->addAction(QLatin1String("go_next_unread_text"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Space));
        KMail::Util::addQActionHelpText(action, i18n("Go to the next unread text"));
        action->setWhatsThis(i18n("Scroll down current message. "
                                  "If at end of current message, "
                                  "go to next unread message."));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotReadOn);
    }

    //----- Settings Menu
    {
        QAction *action = new QAction(i18n("Configure &Filters..."), this);
        action->setMenuRole(QAction::NoRole);   // do not move to application menu on OS X
        actionCollection()->addAction(QLatin1String("filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotFilter);
    }
    {
        QAction *action = new QAction(i18n("Manage &Sieve Scripts..."), this);
        actionCollection()->addAction(QLatin1String("sieveFilters"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotManageSieveScripts);
    }
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("kmail")), i18n("KMail &Introduction"), this);
        actionCollection()->addAction(QLatin1String("help_kmail_welcomepage"), action);
        KMail::Util::addQActionHelpText(action, i18n("Display KMail's Welcome Page"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotIntro);
    }

    // ----- Standard Actions

    //  KStandardAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("preferences-desktop-notification")),
                                      i18n("Configure &Notifications..."), this);
        action->setMenuRole(QAction::NoRole);   // do not move to application menu on OS X
        actionCollection()->addAction(QLatin1String("kmail_configure_notifications"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotEditNotifications);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("configure")), i18n("&Configure KMail..."), this);
        action->setMenuRole(QAction::PreferencesRole);   // this one should move to the application menu on OS X
        actionCollection()->addAction(QLatin1String("kmail_configure_kmail"), action);
        connect(action, SIGNAL(triggered(bool)), kmkernel, SLOT(slotShowConfigurationDialog()));
    }

    {
        mExpireConfigAction = new QAction(i18n("Expire..."), this);
        actionCollection()->addAction(QLatin1String("expire_settings"), mExpireConfigAction);
        connect( mExpireConfigAction, SIGNAL(triggered(bool)), mManageShowCollectionProperties, SLOT(slotShowExpiryProperties()) );
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("bookmark-new")), i18n("Add Favorite Folder..."), this);
        actionCollection()->addAction(QLatin1String("add_favorite_folder"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAddFavoriteFolder);
    }

    {
        mServerSideSubscription = new QAction(QIcon::fromTheme(QLatin1String("folder-bookmarks")), i18n("Serverside Subscription..."), this);
        actionCollection()->addAction(QLatin1String("serverside_subscription"), mServerSideSubscription);
        connect(mServerSideSubscription, &QAction::triggered, this, &KMMainWidget::slotServerSideSubscription);
    }

    {
        mApplyFiltersOnFolder = new QAction(QIcon::fromTheme(QLatin1String("view-filter")), i18n("Appl&y All Filters On Folder"), this);
        actionCollection()->addAction(QLatin1String("apply_filters_on_folder"), mApplyFiltersOnFolder);
        connect(mApplyFiltersOnFolder, SIGNAL(triggered(bool)),
                SLOT(slotApplyFiltersOnFolder()));

    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("kmail")), i18n("&Export KMail Data..."), this);
        actionCollection()->addAction(QLatin1String("kmail_export_data"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExportData);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QLatin1String("contact-new")), i18n("New AddressBook Contact..."), this);
        actionCollection()->addAction(QLatin1String("kmail_new_addressbook_contact"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCreateAddressBookContact);

    }

    actionCollection()->addAction(KStandardAction::Undo,  QLatin1String("kmail_undo"), this, SLOT(slotUndo()));

    KStandardAction::tipOfDay(this, SLOT(slotShowTip()), actionCollection());

    menutimer = new QTimer(this);
    menutimer->setObjectName(QLatin1String("menutimer"));
    menutimer->setSingleShot(true);
    connect(menutimer, &QTimer::timeout, this, &KMMainWidget::updateMessageActionsDelayed);
    connect(kmkernel->undoStack(),
            SIGNAL(undoStackChanged()), this, SLOT(slotUpdateUndo()));

    updateMessageActions();
    updateFolderMenu();
    mTagActionManager = new KMail::TagActionManager(this, actionCollection(), mMsgActions,
            mGUIClient);
    mFolderShortcutActionManager = new KMail::FolderShortcutActionManager(this, actionCollection());

    {
        QAction *action = new QAction(i18n("Copy Message to Folder"), this);
        actionCollection()->addAction(QLatin1String("copy_message_to_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                SLOT(slotCopySelectedMessagesToFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_C));
    }
    {
        QAction *action = new QAction(i18n("Jump to Folder..."), this);
        actionCollection()->addAction(QLatin1String("jump_to_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                SLOT(slotJumpToFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_J));
    }
    {
        QAction *action = new QAction(i18n("Abort Current Operation"), this);
        actionCollection()->addAction(QLatin1String("cancel"), action);
        connect(action, SIGNAL(triggered(bool)),
                ProgressManager::instance(), SLOT(slotAbortAll()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Escape));
    }
    {
        QAction *action = new QAction(i18n("Focus on Next Folder"), this);
        actionCollection()->addAction(QLatin1String("inc_current_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                mFolderTreeWidget->folderTreeView(), SLOT(slotFocusNextFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Right));
    }
    {
        QAction *action = new QAction(i18n("Focus on Previous Folder"), this);
        actionCollection()->addAction(QLatin1String("dec_current_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                mFolderTreeWidget->folderTreeView(), SLOT(slotFocusPrevFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Left));
    }
    {
        QAction *action = new QAction(i18n("Select Folder with Focus"), this);
        actionCollection()->addAction(QLatin1String("select_current_folder"), action);

        connect(action, SIGNAL(triggered(bool)),
                mFolderTreeWidget->folderTreeView(), SLOT(slotSelectFocusFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Space));
    }
    {
        QAction *action = new QAction(i18n("Focus on First Folder"), this);
        actionCollection()->addAction(QLatin1String("focus_first_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                mFolderTreeWidget->folderTreeView(), SLOT(slotFocusFirstFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Home));
    }
    {
        QAction *action = new QAction(i18n("Focus on Last Folder"), this);
        actionCollection()->addAction(QLatin1String("focus_last_folder"), action);
        connect(action, SIGNAL(triggered(bool)),
                mFolderTreeWidget->folderTreeView(), SLOT(slotFocusLastFolder()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_End));
    }
    {
        QAction *action = new QAction(i18n("Focus on Next Message"), this);
        actionCollection()->addAction(QLatin1String("inc_current_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotFocusOnNextMessage()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Right));
    }
    {
        QAction *action = new QAction(i18n("Focus on Previous Message"), this);
        actionCollection()->addAction(QLatin1String("dec_current_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotFocusOnPrevMessage()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Left));
    }
    {
        QAction *action = new QAction(i18n("Select First Message"), this);
        actionCollection()->addAction(QLatin1String("select_first_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotSelectFirstMessage()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Home));
    }
    {
        QAction *action = new QAction(i18n("Select Last Message"), this);
        actionCollection()->addAction(QLatin1String("select_last_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotSelectLastMessage()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_End));
    }
    {
        QAction *action = new QAction(i18n("Select Message with Focus"), this);
        actionCollection()->addAction(QLatin1String("select_current_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotSelectFocusedMessage()));
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Space));
    }

    {
        mQuickSearchAction = new QAction(i18n("Set Focus to Quick Search"), this);
        //If change shortcut change Panel::setQuickSearchClickMessage(...) message
        actionCollection()->setDefaultShortcut(mQuickSearchAction, QKeySequence(Qt::ALT + Qt::Key_Q));
        actionCollection()->addAction(QLatin1String("focus_to_quickseach"), mQuickSearchAction);
        connect(mQuickSearchAction, SIGNAL(triggered(bool)),
                SLOT(slotFocusQuickSearch()));
        updateQuickSearchLineText();
    }
    {
        QAction *action = new QAction(i18n("Extend Selection to Previous Message"), this);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Left));
        actionCollection()->addAction(QLatin1String("previous_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotExtendSelectionToPreviousMessage()));
    }
    {
        QAction *action = new QAction(i18n("Extend Selection to Next Message"), this);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Right));
        actionCollection()->addAction(QLatin1String("next_message"), action);
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(slotExtendSelectionToNextMessage()));
    }

    {
        mMoveMsgToFolderAction = new QAction(i18n("Move Message to Folder"), this);
        actionCollection()->setDefaultShortcut(mMoveMsgToFolderAction, QKeySequence(Qt::Key_M));
        actionCollection()->addAction(QLatin1String("move_message_to_folder"), mMoveMsgToFolderAction);
        connect(mMoveMsgToFolderAction, SIGNAL(triggered(bool)),
                SLOT(slotMoveSelectedMessageToFolder()));
    }

    mArchiveAction = new QAction(i18nc("@action", "Archive"), this);
    actionCollection()->addAction(QLatin1String("archive_mails"), mArchiveAction);
    connect(mArchiveAction, SIGNAL(triggered(bool)),
            SLOT(slotArchiveMails()));

}

void KMMainWidget::slotAddFavoriteFolder()
{
    if (!mFavoritesModel) {
        return;
    }
    QPointer<MailCommon::FolderSelectionDialog> dialog(selectFromAllFoldersDialog());
    dialog->setWindowTitle(i18n("Add Favorite Folder"));
    if (dialog->exec() && dialog) {
        const Akonadi::Collection collection = dialog->selectedCollection();
        if (collection.isValid()) {
            mFavoritesModel->addCollection(collection);
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEditNotifications()
{
    KMail::KMKnotify notifyDlg(this);
    notifyDlg.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReadOn()
{
    if (!mMsgView) {
        return;
    }
    if (!mMsgView->viewer()->atBottom()) {
        mMsgView->viewer()->slotJumpDown();
        return;
    }
    slotSelectNextUnreadMessage();
}

void KMMainWidget::slotNextUnreadFolder()
{
    if (!mFolderTreeWidget) {
        return;
    }
    mGoToFirstUnreadMessageInSelectedFolder = true;
    mFolderTreeWidget->folderTreeView()->selectNextUnreadFolder();
    mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotPrevUnreadFolder()
{
    if (!mFolderTreeWidget) {
        return;
    }
    mGoToFirstUnreadMessageInSelectedFolder = true;
    mFolderTreeWidget->folderTreeView()->selectPrevUnreadFolder();
    mGoToFirstUnreadMessageInSelectedFolder = false;
}

void KMMainWidget::slotExpandThread()
{
    mMessagePane->setCurrentThreadExpanded(true);
}

void KMMainWidget::slotCollapseThread()
{
    mMessagePane->setCurrentThreadExpanded(false);
}

void KMMainWidget::slotExpandAllThreads()
{
    // TODO: Make this asynchronous ? (if there is enough demand)
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif
    mMessagePane->setAllThreadsExpanded(true);
}

void KMMainWidget::slotCollapseAllThreads()
{
    // TODO: Make this asynchronous ? (if there is enough demand)
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif
    mMessagePane->setAllThreadsExpanded(false);
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
    updateMessageActions(true);

    menutimer->stop();
    menutimer->start(500);
}

void KMMainWidget::updateMessageActions(bool fast)
{
    Akonadi::Item::List selectedItems;
    Akonadi::Item::List selectedVisibleItems;
    bool allSelectedBelongToSameThread = false;
    if (mCurrentFolder && mCurrentFolder->isValid() &&
            mMessagePane->getSelectionStats(selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread)
       ) {
        mMsgActions->setCurrentMessage(mMessagePane->currentItem(), selectedVisibleItems);
    } else {
        mMsgActions->setCurrentMessage(Akonadi::Item());
    }

    if (!fast) {
        updateMessageActionsDelayed();
    }

}

void KMMainWidget::updateMessageActionsDelayed()
{
    int count;
    Akonadi::Item::List selectedItems;
    Akonadi::Item::List selectedVisibleItems;
    bool allSelectedBelongToSameThread = false;
    Akonadi::Item currentMessage;
    if (mCurrentFolder && mCurrentFolder->isValid() &&
            mMessagePane->getSelectionStats(selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread)
       ) {
        count = selectedItems.count();

        currentMessage = mMessagePane->currentItem();

    } else {
        count = 0;
        currentMessage = Akonadi::Item();
    }

    mApplyFiltersOnFolder->setEnabled(mCurrentFolder && mCurrentFolder->isValid());

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

    bool readOnly = mCurrentFolder && mCurrentFolder->isValid() && (mCurrentFolder->rights() & Akonadi::Collection::ReadOnly);
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

    mThreadStatusMenu->setEnabled(thread_actions);
    // these need to be handled individually, the user might have them
    // in the toolbar
    mWatchThreadAction->setEnabled(thread_actions && flags_available);
    mIgnoreThreadAction->setEnabled(thread_actions && flags_available);
    mMarkThreadAsReadAction->setEnabled(thread_actions);
    mMarkThreadAsUnreadAction->setEnabled(thread_actions);
    mToggleThreadToActAction->setEnabled(thread_actions && flags_available);
    mToggleThreadImportantAction->setEnabled(thread_actions && flags_available);
    bool canDeleteMessages = mCurrentFolder && mCurrentFolder->isValid() && (mCurrentFolder->rights() & Akonadi::Collection::CanDeleteItem);

    mTrashThreadAction->setEnabled(thread_actions && canDeleteMessages);
    mDeleteThreadAction->setEnabled(thread_actions && canDeleteMessages);

    if (currentMessage.isValid()) {
        MessageStatus status;
        status.setStatusFromFlags(currentMessage.flags());
        mTagActionManager->updateActionStates(count, mMessagePane->currentItem());
        if (thread_actions) {
            mToggleThreadToActAction->setChecked(status.isToAct());
            mToggleThreadImportantAction->setChecked(status.isImportant());
            mWatchThreadAction->setChecked(status.isWatched());
            mIgnoreThreadAction->setChecked(status.isIgnored());
        }
    }

    mMoveActionMenu->setEnabled(mass_actions && canDeleteMessages);
    if (mMoveMsgToFolderAction) {
        mMoveMsgToFolderAction->setEnabled(mass_actions && canDeleteMessages);
    }
    //mCopyActionMenu->setEnabled( mass_actions );

    mDeleteAction->setEnabled(mass_actions && canDeleteMessages);

    mExpireConfigAction->setEnabled(canDeleteMessages);

    if (mMsgView) {
        mMsgView->findInMessageAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentFolder->collection()));
    }
    mMsgActions->forwardInlineAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentFolder->collection()));
    mMsgActions->forwardAttachedAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentFolder->collection()));
    mMsgActions->forwardMenu()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentFolder->collection()));

    mMsgActions->editAction()->setEnabled(single_actions);
    mUseAction->setEnabled(single_actions && CommonKernel->folderIsTemplates(mCurrentFolder->collection()));
    filterMenu()->setEnabled(single_actions);
    mMsgActions->redirectAction()->setEnabled(/*single_actions &&*/mass_actions && !CommonKernel->folderIsTemplates(mCurrentFolder->collection()));

    if (mMsgActions->customTemplatesMenu()) {
        mMsgActions->customTemplatesMenu()->forwardActionMenu()->setEnabled(mass_actions);
        mMsgActions->customTemplatesMenu()->replyActionMenu()->setEnabled(single_actions);
        mMsgActions->customTemplatesMenu()->replyAllActionMenu()->setEnabled(single_actions);
    }

    // "Print" will act on the current message: it will ignore any hidden selection
    mMsgActions->printAction()->setEnabled(singleVisibleMessageSelected);
    // "Print preview" will act on the current message: it will ignore any hidden selection
    QAction *printPreviewAction = mMsgActions->printPreviewAction();
    if (printPreviewAction) {
        printPreviewAction->setEnabled(singleVisibleMessageSelected);
    }

    // "View Source" will act on the current message: it will ignore any hidden selection
    if (mMsgView) {
        mMsgView->viewSourceAction()->setEnabled(singleVisibleMessageSelected);
    }
    MessageStatus status;
    status.setStatusFromFlags(currentMessage.flags());

    QList< QAction *> actionList;
    bool statusSendAgain = single_actions && ((currentMessage.isValid() && status.isSent()) || (currentMessage.isValid() && CommonKernel->folderIsSentMailFolder(mCurrentFolder->collection())));
    if (statusSendAgain) {
        actionList << mSendAgainAction;
    } else if (single_actions) {
        actionList << messageActions()->editAction();
    }
    actionList << mSaveAttachmentsAction;
    if (mCurrentFolder && FolderArchive::FolderArchiveUtil::resourceSupportArchiving(mCurrentFolder->collection().resource())) {
        actionList << mArchiveAction;
    }
    mGUIClient->unplugActionList(QLatin1String("messagelist_actionlist"));
    mGUIClient->plugActionList(QLatin1String("messagelist_actionlist"), actionList);
    mSendAgainAction->setEnabled(statusSendAgain);

    mSaveAsAction->setEnabled(mass_actions);

    if ((mCurrentFolder && mCurrentFolder->isValid())) {
        updateMoveAction(mCurrentFolder->statistics());
    } else {
        updateMoveAction(false, false);
    }

    const qint64 nbMsgOutboxCollection = MailCommon::Util::updatedCollection(CommonKernel->outboxCollectionFolder()).statistics().count();

    mSendQueued->setEnabled(nbMsgOutboxCollection > 0);
    mSendActionMenu->setEnabled(nbMsgOutboxCollection > 0);

    const bool newPostToMailingList = mCurrentFolder && mCurrentFolder->isMailingListEnabled();
    mMessageNewList->setEnabled(newPostToMailingList);

    slotUpdateOnlineStatus(static_cast<GlobalSettingsBase::EnumNetworkState::type>(GlobalSettings::self()->networkState()));
    if (action(QLatin1String("kmail_undo"))) {
        action(QLatin1String("kmail_undo"))->setEnabled(kmkernel->undoStack()->size() > 0);
    }

    // Enable / disable all filters.
    foreach (QAction *filterAction, mFilterMenuActions) {
        filterAction->setEnabled(count > 0);
    }

    mApplyAllFiltersAction->setEnabled(count);
    mApplyFilterActionsMenu->setEnabled(count);
}

void KMMainWidget::slotAkonadiStandardActionUpdated()
{
    bool multiFolder = false;
    if (mFolderTreeWidget) {
        multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
    }
    if (mCollectionProperties) {
        if (mCurrentFolder && mCurrentFolder->collection().isValid()) {
            const Akonadi::AgentInstance instance =
                Akonadi::AgentManager::self()->instance(mCurrentFolder->collection().resource());

            mCollectionProperties->setEnabled(!multiFolder &&
                                              !mCurrentFolder->isStructural() &&
                                              (instance.status() != Akonadi::AgentInstance::Broken));
        } else {
            mCollectionProperties->setEnabled(false);
        }
        QList< QAction * > collectionProperties;
        if (mCollectionProperties->isEnabled()) {
            collectionProperties << mCollectionProperties;
        }
        mGUIClient->unplugActionList(QLatin1String("akonadi_collection_collectionproperties_actionlist"));
        mGUIClient->plugActionList(QLatin1String("akonadi_collection_collectionproperties_actionlist"), collectionProperties);

    }

    const bool folderWithContent = mCurrentFolder && !mCurrentFolder->isStructural();

    if (mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections)) {

        mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections)->setEnabled(mCurrentFolder
                && !multiFolder
                && (mCurrentFolder->collection().rights() & Collection::CanDeleteCollection)
                && !mCurrentFolder->isSystemFolder()
                && folderWithContent);
    }

    if (mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)) {
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setEnabled(folderWithContent
                && (mCurrentFolder->count() > 0)
                && mCurrentFolder->canDeleteMessages()
                && !multiFolder);
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setText((mCurrentFolder && CommonKernel->folderIsTrash(mCurrentFolder->collection())) ? i18n("E&mpty Trash") : i18n("&Move All Messages to Trash"));
    }

    QList< QAction * > addToFavorite;
    QAction *actionAddToFavoriteCollections = akonadiStandardAction(Akonadi::StandardActionManager::AddToFavoriteCollections);
    if (actionAddToFavoriteCollections) {
        if (mEnableFavoriteFolderView && actionAddToFavoriteCollections->isEnabled()) {
            addToFavorite << actionAddToFavoriteCollections;
        }
        mGUIClient->unplugActionList(QLatin1String("akonadi_collection_add_to_favorites_actionlist"));
        mGUIClient->plugActionList(QLatin1String("akonadi_collection_add_to_favorites_actionlist"), addToFavorite);
    }

    QList< QAction * > syncActionList;
    QAction *actionSync = akonadiStandardAction(Akonadi::StandardActionManager::SynchronizeCollections);
    if (actionSync && actionSync->isEnabled()) {
        syncActionList << actionSync;
    }
    actionSync = akonadiStandardAction(Akonadi::StandardActionManager::SynchronizeCollectionsRecursive);
    if (actionSync && actionSync->isEnabled()) {
        syncActionList << actionSync;
    }
    mGUIClient->unplugActionList(QLatin1String("akonadi_collection_sync_actionlist"));
    mGUIClient->plugActionList(QLatin1String("akonadi_collection_sync_actionlist"), syncActionList);

    QList< QAction * > actionList;

    QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CreateCollection);
    if (action && action->isEnabled()) {
        actionList << action;
    }

    action =  mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveCollectionToMenu);
    if (action && action->isEnabled()) {
        actionList << action;
    }

    action =  mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyCollectionToMenu);
    if (action && action->isEnabled()) {
        actionList << action;
    }
    mGUIClient->unplugActionList(QLatin1String("akonadi_collection_move_copy_menu_actionlist"));
    mGUIClient->plugActionList(QLatin1String("akonadi_collection_move_copy_menu_actionlist"), actionList);

}

void KMMainWidget::updateHtmlMenuEntry()
{
    if (mDisplayMessageFormatMenu && mPreferHtmlLoadExtAction) {
        bool multiFolder = false;
        if (mFolderTreeWidget) {
            multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
        }
        // the visual ones only make sense if we are showing a message list
        const bool enabledAction = (mFolderTreeWidget &&
                                    mFolderTreeWidget->folderTreeView()->currentFolder().isValid() &&
                                    !multiFolder);

        mDisplayMessageFormatMenu->setEnabled(enabledAction);
        const bool isEnabled = (mFolderTreeWidget &&
                                mFolderTreeWidget->folderTreeView()->currentFolder().isValid() &&
                                !multiFolder);
        const bool useHtml = (mFolderDisplayFormatPreference == MessageViewer::Viewer::Html || (mHtmlGlobalSetting && mFolderDisplayFormatPreference == MessageViewer::Viewer::UseGlobalSetting));
        mPreferHtmlLoadExtAction->setEnabled(isEnabled && useHtml);

        mDisplayMessageFormatMenu->setDisplayMessageFormat(mFolderDisplayFormatPreference);

        mPreferHtmlLoadExtAction->setChecked(!multiFolder && (mHtmlLoadExtGlobalSetting ? !mFolderHtmlLoadExtPreference : mFolderHtmlLoadExtPreference));
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFolderMenu()
{
    if (!CommonKernel->outboxCollectionFolder().isValid()) {
        QTimer::singleShot(1000, this, SLOT(updateFolderMenu()));
        return;
    }

    const bool folderWithContent = mCurrentFolder && !mCurrentFolder->isStructural();
    bool multiFolder = false;
    if (mFolderTreeWidget) {
        multiFolder = mFolderTreeWidget->selectedCollections().count() > 1;
    }
    mFolderMailingListPropertiesAction->setEnabled(folderWithContent &&
            !multiFolder &&
            !mCurrentFolder->isSystemFolder());

    QList< QAction * > actionlist;
    if (mCurrentFolder && mCurrentFolder->collection().id() == CommonKernel->outboxCollectionFolder().id() && (mCurrentFolder->collection()).statistics().count() > 0) {
        qDebug() << "Enabling send queued";
        mSendQueued->setEnabled(true);
        actionlist << mSendQueued;
    }
    //   if ( mCurrentFolder && mCurrentFolder->collection().id() != CommonKernel->trashCollectionFolder().id() ) {
    //     actionlist << mTrashAction;
    //   }
    mGUIClient->unplugActionList(QLatin1String("outbox_folder_actionlist"));
    mGUIClient->plugActionList(QLatin1String("outbox_folder_actionlist"), actionlist);
    actionlist.clear();

    const bool isASearchFolder = mCurrentFolder && mCurrentFolder->collection().resource() == QLatin1String("akonadi_search_resource");
    if (isASearchFolder) {
        mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections)->setText(i18n("&Delete Search"));
    }

    mArchiveFolderAction->setEnabled(mCurrentFolder && !multiFolder && folderWithContent);

    bool isInTrashFolder = (mCurrentFolder && CommonKernel->folderIsTrash(mCurrentFolder->collection()));
    akonadiStandardAction(Akonadi::StandardMailActionManager::MoveToTrash)->setText(isInTrashFolder ? i18nc("@action Hard delete, bypassing trash", "&Delete") : i18n("&Move to Trash"));

    mTrashThreadAction->setText(isInTrashFolder ? i18n("Delete T&hread") : i18n("M&ove Thread to Trash"));

    mSearchMessages->setText((mCurrentFolder && mCurrentFolder->collection().resource() == QLatin1String("akonadi_search_resource")) ? i18n("Edit Search...") : i18n("&Find Messages..."));

    mExpireConfigAction->setEnabled(mCurrentFolder &&
                                    !mCurrentFolder->isStructural() &&
                                    !multiFolder &&
                                    mCurrentFolder->canDeleteMessages() &&
                                    folderWithContent &&
                                    !MailCommon::Util::isVirtualCollection(mCurrentFolder->collection()));

    updateHtmlMenuEntry();

    mShowFolderShortcutDialogAction->setEnabled(!multiFolder && folderWithContent);

    actionlist << akonadiStandardAction(Akonadi::StandardActionManager::ManageLocalSubscriptions);
    bool imapFolderIsOnline = false;
    if (mCurrentFolder && kmkernel->isImapFolder(mCurrentFolder->collection(), imapFolderIsOnline)) {
        if (imapFolderIsOnline) {
            actionlist << mServerSideSubscription;
        }
    }

    mGUIClient->unplugActionList(QLatin1String("collectionview_actionlist"));
    mGUIClient->plugActionList(QLatin1String("collectionview_actionlist"), actionlist);

}

//-----------------------------------------------------------------------------
void KMMainWidget::slotIntro()
{
    if (!mMsgView) {
        return;
    }

    mMsgView->clear(true);

    // hide widgets that are in the way:
    if (mMessagePane && mLongFolderList) {
        mMessagePane->hide();
    }
    mMsgView->displayAboutPage();

    mCurrentFolder.clear();
}

void KMMainWidget::slotShowStartupFolder()
{
    connect(MailCommon::FilterManager::instance(), SIGNAL(filtersChanged()),
            this, SLOT(initializeFilterActions()));
    // Plug various action lists. This can't be done in the constructor, as that is called before
    // the main window or Kontact calls createGUI().
    // This function however is called with a single shot timer.
    checkAkonadiServerManagerState();
    mFolderShortcutActionManager->createActions();
    mTagActionManager->createActions();
    messageActions()->setupForwardingActionsList(mGUIClient);

    QString newFeaturesMD5 = KMReaderWin::newFeaturesMD5();
    if (kmkernel->firstStart() ||
            GlobalSettings::self()->previousNewFeaturesMD5() != newFeaturesMD5) {
        GlobalSettings::self()->setPreviousNewFeaturesMD5(newFeaturesMD5);
        slotIntro();
        return;
    }
}

void KMMainWidget::checkAkonadiServerManagerState()
{
    Akonadi::ServerManager::State state = Akonadi::ServerManager::self()->state();
    if (state == Akonadi::ServerManager::Running) {
        initializeFilterActions();
    } else {
        connect(Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
                SLOT(slotServerStateChanged(Akonadi::ServerManager::State)));
    }
}

void KMMainWidget::slotServerStateChanged(Akonadi::ServerManager::State state)
{
    if (state == Akonadi::ServerManager::Running) {
        initializeFilterActions();
        disconnect(Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)));
    }
}

void KMMainWidget::slotShowTip()
{
    KTipDialog::showTip(this, QString(), true);
}

QList<KActionCollection *> KMMainWidget::actionCollections() const
{
    return QList<KActionCollection *>() << actionCollection();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateUndo()
{
    if (actionCollection()->action(QLatin1String("kmail_undo"))) {
        QAction *act = actionCollection()->action(QLatin1String("kmail_undo"));
        act->setEnabled(kmkernel->undoStack()->size() > 0);
        const QString infoStr = kmkernel->undoStack()->undoInfo();
        if (infoStr.isEmpty()) {
            act->setText(i18n("&Undo"));
        } else {
            act->setText(i18n("&Undo: \"%1\"", kmkernel->undoStack()->undoInfo()));
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::clearFilterActions()
{
    if (!mFilterTBarActions.isEmpty())
        if (mGUIClient->factory()) {
            mGUIClient->unplugActionList(QLatin1String("toolbar_filter_actions"));
        }

    if (!mFilterMenuActions.isEmpty())
        if (mGUIClient->factory()) {
            mGUIClient->unplugActionList(QLatin1String("menu_filter_actions"));
        }

    foreach (QAction *a, mFilterMenuActions) {
        actionCollection()->removeAction(a);
    }

    mApplyFilterActionsMenu->menu()->clear();
    mFilterTBarActions.clear();
    mFilterMenuActions.clear();

    qDeleteAll(mFilterCommands);
    mFilterCommands.clear();
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeFilterActions()
{
    clearFilterActions();
    mApplyFilterActionsMenu->menu()->addAction(mApplyAllFiltersAction);
    bool addedSeparator = false;

    foreach (MailFilter *filter, MailCommon::FilterManager::instance()->filters()) {
        if (!filter->isEmpty() && filter->configureShortcut() && filter->isEnabled()) {
            QString filterName = QStringLiteral("Filter %1").arg(filter->name());
            QString normalizedName = filterName.replace(QLatin1Char(' '), QLatin1Char('_'));
            if (action(normalizedName)) {
                continue;
            }
            KMMetaFilterActionCommand *filterCommand = new KMMetaFilterActionCommand(filter->identifier(), this);
            mFilterCommands.append(filterCommand);
            QString displayText = i18n("Filter %1", filter->name());
            QString icon = filter->icon();
            if (icon.isEmpty()) {
                icon = QLatin1String("system-run");
            }
            QAction *filterAction = new QAction(QIcon::fromTheme(icon), displayText, actionCollection());
            filterAction->setIconText(filter->toolbarName());

            // The shortcut configuration is done in the filter dialog.
            // The shortcut set in the shortcut dialog would not be saved back to
            // the filter settings correctly.
            actionCollection()->setShortcutsConfigurable(filterAction, false);
            actionCollection()->addAction(normalizedName,
                                          filterAction);
            connect(filterAction, SIGNAL(triggered(bool)),
                    filterCommand, SLOT(start()));
            actionCollection()->setDefaultShortcut(filterAction, filter->shortcut());
            if (!addedSeparator) {
                QAction *a = mApplyFilterActionsMenu->menu()->addSeparator();
                mFilterMenuActions.append(a);
                addedSeparator = true;
            }
            mApplyFilterActionsMenu->menu()->addAction(filterAction);
            mFilterMenuActions.append(filterAction);
            if (filter->configureToolbar()) {
                mFilterTBarActions.append(filterAction);
            }
        }
    }
    if (!mFilterMenuActions.isEmpty() && mGUIClient->factory()) {
        mGUIClient->plugActionList(QLatin1String("menu_filter_actions"), mFilterMenuActions);
    }
    if (!mFilterTBarActions.isEmpty() && mGUIClient->factory()) {
        mFilterTBarActions.prepend(mToolbarActionSeparator);
        mGUIClient->plugActionList(QLatin1String("toolbar_filter_actions"), mFilterTBarActions);
    }

    // Our filters have changed, now enable/disable them
    updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiSpamWizard()
{
    AntiSpamWizard wiz(AntiSpamWizard::AntiSpam, this);
    wiz.exec();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotAntiVirusWizard()
{
    AntiSpamWizard wiz(AntiSpamWizard::AntiVirus, this);
    wiz.exec();
}
//-----------------------------------------------------------------------------
void KMMainWidget::slotAccountWizard()
{
    KMail::Util::launchAccountWizard(this);
}

void KMMainWidget::slotImportWizard()
{
    const QString path = QStandardPaths::findExecutable(QLatin1String("importwizard"));
    if (!QProcess::startDetached(path))
        KMessageBox::error(this, i18n("Could not start the import wizard. "
                                      "Please check your installation."),
                           i18n("Unable to start import wizard"));
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFilterLogViewer()
{
    MailCommon::FilterManager::instance()->showFilterLogDialog((qlonglong)winId());
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFileMenu()
{
    const bool isEmpty = MailCommon::Util::agentInstances().isEmpty();
    actionCollection()->action(QLatin1String("check_mail"))->setEnabled(!isEmpty);
    actionCollection()->action(QLatin1String("check_mail_in"))->setEnabled(!isEmpty);
}

//-----------------------------------------------------------------------------
const KMMainWidget::PtrList *KMMainWidget::mainWidgetList()
{
    // better safe than sorry; check whether the global static has already been destroyed
    if (theMainWidgetList.isDestroyed()) {
        return 0;
    }
    return theMainWidgetList;
}

QSharedPointer<FolderCollection> KMMainWidget::currentFolder() const
{
    return mCurrentFolder;
}

//-----------------------------------------------------------------------------
QString KMMainWidget::overrideEncoding() const
{
    if (mMsgView) {
        return mMsgView->overrideEncoding();
    } else {
        return MessageCore::GlobalSettings::self()->overrideCharacterEncoding();
    }
}

void KMMainWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    mWasEverShown = true;
}

void KMMainWidget::slotRequestFullSearchFromQuickSearch()
{
    // First, open the search window. If we are currently on a search folder,
    // the search associated with that will be loaded.
    if (!slotSearch()) {
        return;
    }

    assert(mSearchWin);

    // Now we look at the current state of the quick search, and if there's
    // something in there, we add the criteria to the existing search for
    // the search folder, if applicable, or make a new one from it.
    SearchPattern pattern;
    const QString searchString = mMessagePane->currentFilterSearchString();
    if (!searchString.isEmpty()) {
        pattern.append(SearchRule::createInstance("<message>", SearchRule::FuncContains, searchString));
    }
#if 0 //PORT IT
    QList<MessageStatus> status = mMessagePane->currentFilterStatus();
    if (status.hasAttachment()) {
        pattern.append(SearchRule::createInstance("<message>", SearchRule::FuncHasAttachment));
        status.setHasAttachment(false);
    }

    if (!status.isOfUnknownStatus()) {
        pattern.append(SearchRule::Ptr(new SearchRuleStatus(status)));
    }
#endif
    if (!pattern.isEmpty()) {
        mSearchWin->addRulesToSearchPattern(pattern);
    }
}

void KMMainWidget::updateVacationScriptStatus(bool active, const QString &serverName)
{
    mVacationScriptIndicator->setVacationScriptActive(active, serverName);
    mVacationIndicatorActive = mVacationScriptIndicator->hasVacationScriptActive();
}

QWidget *KMMainWidget::vacationScriptIndicator() const
{
    return mVacationScriptIndicator;
}

void KMMainWidget::updateVacationScriptStatus()
{
    updateVacationScriptStatus(mVacationIndicatorActive);
}

KMail::TagActionManager *KMMainWidget::tagActionManager() const
{
    return mTagActionManager;
}

KMail::FolderShortcutActionManager *KMMainWidget::folderShortcutActionManager() const
{
    return mFolderShortcutActionManager;
}

void KMMainWidget::slotMessageSelected(const Akonadi::Item &item)
{
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = 0;
    if (mMsgView) {
        // The current selection was cleared, so we'll remove the previously
        // selected message from the preview pane
        if (!item.isValid()) {
            mMsgView->clear();
        } else {
            mShowBusySplashTimer = new QTimer(this);
            mShowBusySplashTimer->setSingleShot(true);
            connect(mShowBusySplashTimer, &QTimer::timeout, this, &KMMainWidget::slotShowBusySplash);
            mShowBusySplashTimer->start(GlobalSettings::self()->folderLoadingTimeout());   //TODO: check if we need a different timeout setting for this

            Akonadi::ItemFetchJob *itemFetchJob = MessageViewer::Viewer::createFetchJob(item);
            const QString resource = mCurrentFolder->collection().resource();
            itemFetchJob->setProperty("_resource", QVariant::fromValue(resource));
            connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
                    SLOT(itemsReceived(Akonadi::Item::List)));
            connect(itemFetchJob, &Akonadi::ItemFetchJob::result, this, &KMMainWidget::itemsFetchDone);
        }
    }
}

void KMMainWidget::itemsReceived(const Akonadi::Item::List &list)
{
    Q_ASSERT(list.size() == 1);
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = 0;

    if (!mMsgView) {
        return;
    }

    Item item = list.first();

    if (mMessagePane) {
        mMessagePane->show();

        if (mMessagePane->currentItem() != item) {
            // The user has selected another email already, so don't render this one.
            // Mark it as read, though, if the user settings say so.
            if (MessageViewer::GlobalSettings::self()->delayedMarkAsRead() &&
                    MessageViewer::GlobalSettings::self()->delayedMarkTime() == 0) {
                item.setFlag(Akonadi::MessageFlags::Seen);
                Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(item, this);
                modifyJob->disableRevisionCheck();
                modifyJob->setIgnorePayload(true);
            }
            return;
        }
    }

    mMsgView->setMessage(item);
    // reset HTML override to the folder setting
    mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
    mMsgView->setHtmlLoadExtOverride(mFolderHtmlLoadExtPreference);
    mMsgView->setDecryptMessageOverwrite(false);
    mMsgActions->setCurrentMessage(item);
}

void KMMainWidget::itemsFetchDone(KJob *job)
{
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = 0;
    if (job->error()) {
        // Unfortunately job->error() is Job::Unknown in many cases.
        // (see JobPrivate::handleResponse in akonadi/job.cpp)
        // So we show the "offline" page after checking the resource status.
        qDebug() << job->error() << job->errorString();

        const QString resource = job->property("_resource").toString();
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance(resource);
        if (!agentInstance.isOnline()) {
            // The resource is offline
            if (mMsgView) {
                mMsgView->viewer()->enableMessageDisplay();
                mMsgView->clear(true);
            }
            mMessagePane->show();

            if (kmkernel->isOffline()) {
                showOfflinePage();
            } else {
                showResourceOfflinePage();
            }
        } else {
            // Some other error
            BroadcastStatus::instance()->setStatusMsg(job->errorString());
        }
    }
}

QAction *KMMainWidget::akonadiStandardAction(Akonadi::StandardActionManager::Type type)
{
    return mAkonadiStandardActionManager->action(type);
}

QAction *KMMainWidget::akonadiStandardAction(Akonadi::StandardMailActionManager::Type type)
{
    return mAkonadiStandardActionManager->action(type);
}

void KMMainWidget::slotRemoveDuplicates()
{
    RemoveDuplicateMailJob *job = new RemoveDuplicateMailJob(mFolderTreeWidget->folderTreeView()->selectionModel(), this, this);
    job->start();
}

void KMMainWidget::slotServerSideSubscription()
{
    if (!mCurrentFolder) {
        return;
    }
    ManageServerSideSubscriptionJob *job = new ManageServerSideSubscriptionJob(this);
    job->setCurrentFolder(mCurrentFolder);
    job->setParentWidget(this);
    job->start();
}

void KMMainWidget::savePaneSelection()
{
    if (mMessagePane) {
        mMessagePane->saveCurrentSelection();
    }
}

void KMMainWidget::updatePaneTagComboBox()
{
    if (mMessagePane) {
        mMessagePane->updateTagComboBox();
    }
}

void KMMainWidget::slotExportData()
{
    const QString path = QStandardPaths::findExecutable(QLatin1String("pimsettingexporter"));
    if (!QProcess::startDetached(path))
        KMessageBox::error(this, i18n("Could not start \"PIM Setting Exporter\" program. "
                                      "Please check your installation."),
                           i18n("Unable to start \"PIM Setting Exporter\" program"));
}

void KMMainWidget::slotCreateAddressBookContact()
{
    CreateNewContactJob *job = new CreateNewContactJob(this, this);
    job->start();
}

void KMMainWidget::slotOpenRecentMsg(const QUrl &url)
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(this, url, overrideEncoding(), this);
    openCommand->start();
}

void KMMainWidget::addRecentFile(const QUrl &mUrl)
{
    mOpenRecentAction->addUrl(mUrl);
    KConfigGroup grp = mConfig->group(QLatin1String("Recent Files"));
    mOpenRecentAction->saveEntries(grp);
    grp.sync();
}

void KMMainWidget::slotMoveMessageToTrash()
{
    if (messageView() && messageView()->viewer()) {
        KMTrashMsgCommand *command = new KMTrashMsgCommand(mCurrentFolder->collection(), messageView()->viewer()->messageItem(), -1);
        command->start();
    }
}

void KMMainWidget::slotArchiveMails()
{
    const QList<Akonadi::Item> selectedMessages = mMessagePane->selectionAsMessageItemList();
    KMKernel::self()->folderArchiveManager()->setArchiveItems(selectedMessages, mCurrentFolder->collection().resource());
}

void KMMainWidget::updateQuickSearchLineText()
{
    //If change change shortcut
    mMessagePane->setQuickSearchClickMessage(i18nc("Show shortcut for focus quick search. Don't change it", "Search...<%1>", mQuickSearchAction->shortcut().toString()));
}

void KMMainWidget::slotChangeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format)
{
    if (format == MessageViewer::Viewer::Html) {
        const int result = KMessageBox::warningContinueCancel(this,
                           // the warning text is taken from configuredialog.cpp:
                           i18n("Use of HTML in mail will make you more vulnerable to "
                                "\"spam\" and may increase the likelihood that your system will be "
                                "compromised by other present and anticipated security exploits."),
                           i18n("Security Warning"),
                           KGuiItem(i18n("Use HTML")),
                           KStandardGuiItem::cancel(),
                           QLatin1String("OverrideHtmlWarning"), 0);
        if (result == KMessageBox::Cancel) {
            mDisplayMessageFormatMenu->setDisplayMessageFormat(MessageViewer::Viewer::Text);
            return;
        }
    }
    mFolderDisplayFormatPreference = format;

    //Update mPrefererHtmlLoadExtAction
    const bool useHtml = (mFolderDisplayFormatPreference == MessageViewer::Viewer::Html || (mHtmlGlobalSetting && mFolderDisplayFormatPreference == MessageViewer::Viewer::UseGlobalSetting));
    mPreferHtmlLoadExtAction->setEnabled(useHtml);

    if (mMsgView) {
        mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
        mMsgView->update(true);
    }
}

void KMMainWidget::populateMessageListStatusFilterCombo()
{
    mMessagePane->populateStatusFilterCombo();
}
