/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2002 Don Sanders <sanders@kde.org>
  Copyright (c) 2009-2018 Montel Laurent <montel@kde.org>

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
#include "job/composenewmessagejob.h"
#include "editor/composer.h"
#include "searchdialog/searchwindow.h"
#include "widgets/vacationscriptindicatorwidget.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmainwin.h"
#include <TemplateParser/CustomTemplatesMenu>
#include "MailCommon/FolderSelectionDialog"
#include "MailCommon/FolderTreeWidget"
#include "PimCommonAkonadi/MailUtil"
#include "util.h"

#include "mailcommon/mailutil.h"
#include "mailcommon/mailkernel.h"
#include "dialog/archivefolderdialog.h"
#include "settings/kmailsettings.h"
#include "MailCommon/FolderTreeView"
#include "tag/tagactionmanager.h"
#include "foldershortcutactionmanager.h"
#include "widgets/collectionpane.h"
#include "manageshowcollectionproperties.h"
#include "widgets/kactionmenutransport.h"
#include "widgets/kactionmenuaccount.h"
#include "mailcommon/searchrulestatus.h"
#include "plugininterface/kmailplugininterface.h"
#include "PimCommon/NetworkUtil"
#include "kpimtextedit/texttospeech.h"
#include "job/markallmessagesasreadinfolderandsubfolderjob.h"
#include "job/removeduplicatemessageinfolderandsubfolderjob.h"
#include "sieveimapinterface/kmsieveimappasswordprovider.h"
#include <KSieveUi/SieveDebugDialog>

#include <AkonadiWidgets/CollectionMaintenancePage>
#include "collectionpage/collectionquotapage.h"
#include "collectionpage/collectiontemplatespage.h"
#include "collectionpage/collectionshortcutpage.h"
#include "collectionpage/collectionviewpage.h"
#include "collectionpage/collectionmailinglistpage.h"
#include "tag/tagselectdialog.h"
#include "job/createnewcontactjob.h"
#include "folderarchive/folderarchiveutil.h"
#include "folderarchive/folderarchivemanager.h"

#include <PimCommonAkonadi/CollectionAclPage>
#include "PimCommon/PimUtil"
#include "MailCommon/CollectionGeneralPage"
#include "MailCommon/CollectionExpiryPage"
#include "MailCommon/ExpireCollectionAttribute"
#include "MailCommon/FilterManager"
#include "MailCommon/MailFilter"
#include "MailCommon/FavoriteCollectionWidget"
#include "MailCommon/FavoriteCollectionOrderProxyModel"
#include "MailCommon/FolderTreeWidget"
#include "MailCommon/FolderTreeView"
#include "mailcommonsettings_base.h"
#include "kmmainwidget.h"

// Other PIM includes
#include "kmail-version.h"

#include "messageviewer/messageviewersettings.h"
#include "messageviewer/viewer.h"
#include "messageviewer/headerstyleplugin.h"
#include "messageviewer/headerstyle.h"

#include <MessageViewer/AttachmentStrategy>

#ifndef QT_NO_CURSOR
#include "Libkdepim/KCursorSaver"
#endif

#include <MessageComposer/MessageSender>
#include "MessageComposer/MessageHelper"

#include <MessageCore/MessageCoreSettings>
#include "MessageCore/MailingList"

#include "dialog/kmknotify.h"
#include "widgets/displaymessageformatactionmenu.h"

#include "ksieveui/vacationmanager.h"
#include "kmlaunchexternalcomponent.h"

// LIBKDEPIM includes
#include "libkdepim/progressmanager.h"
#include "libkdepim/broadcaststatus.h"

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
#include <AkonadiWidgets/controlgui.h>
#include <AkonadiWidgets/collectiondialog.h>
#include <AkonadiCore/collectionstatistics.h>
#include <AkonadiCore/EntityMimeTypeFilterModel>
#include <Akonadi/KMime/MessageFlags>
#include <AkonadiCore/CachePolicy>

#include <kidentitymanagement/identity.h>
#include <kidentitymanagement/identitymanager.h>
#include <KEmailAddress>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>
#include <kmime/kmime_mdn.h>
#include <kmime/kmime_header_parsing.h>
#include <kmime/kmime_message.h>
#include <ksieveui/managesievescriptsdialog.h>
#include <ksieveui/util.h>

#include <PimCommon/LogActivitiesManager>

// KDELIBS includes
#include <kwindowsystem.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <QMenu>
#include <kacceleratormanager.h>
#include <kstandardshortcut.h>
#include <kcharsets.h>
#include "kmail_debug.h"
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
#include <KRecentFilesAction>

// Qt includes
#include <QByteArray>
#include <QHeaderView>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QShortcut>
#include <QProcess>
#include <WebEngineViewer/WebHitTestResult>
// System includes
#include <AkonadiWidgets/standardactionmanager.h>
#include <QStandardPaths>

#include "PimCommonAkonadi/ManageServerSideSubscriptionJob"
#include <job/removeduplicatemailjob.h>
#include <job/removecollectionjob.h>

using namespace KMime;
using namespace Akonadi;
using namespace MailCommon;
using KPIM::ProgressManager;
using KPIM::BroadcastStatus;
using KMail::SearchWindow;
using KMime::Types::AddrSpecList;
using MessageViewer::AttachmentStrategy;

Q_GLOBAL_STATIC(KMMainWidget::PtrList, theMainWidgetList)

//-----------------------------------------------------------------------------
KMMainWidget::KMMainWidget(QWidget *parent, KXMLGUIClient *aGUIClient, KActionCollection *actionCollection, KSharedConfig::Ptr config)
    : QWidget(parent)
    , mManageShowCollectionProperties(new ManageShowCollectionProperties(this, this))
{
    mLaunchExternalComponent = new KMLaunchExternalComponent(this, this);
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
    mFolderTreeWidget = nullptr;
    mPreferHtmlLoadExtAction = nullptr;
    Akonadi::ControlGui::widgetNeedsAkonadi(this);
    mFavoritesModel = nullptr;
    mSievePasswordProvider = new KMSieveImapPasswordProvider(winId());
    mVacationManager = new KSieveUi::VacationManager(mSievePasswordProvider, this);
    connect(mVacationManager, &KSieveUi::VacationManager::updateVacationScriptStatus, this, QOverload<bool, const QString &>::of(&KMMainWidget::updateVacationScriptStatus));

    mToolbarActionSeparator = new QAction(this);
    mToolbarActionSeparator->setSeparator(true);

    KMailPluginInterface::self()->setActionCollection(mActionCollection);
    KMailPluginInterface::self()->initializePlugins();
    KMailPluginInterface::self()->setMainWidget(this);

    theMainWidgetList->append(this);

    readPreConfig();
    createWidgets();
    setupActions();

    readConfig();

    if (!kmkernel->isOffline()) {   //kmail is set to online mode, make sure the agents are also online
        kmkernel->setAccountStatus(true);
    }

    QTimer::singleShot(0, this, &KMMainWidget::slotShowStartupFolder);

    connect(kmkernel, &KMKernel::startCheckMail,
            this, &KMMainWidget::slotStartCheckMail);

    connect(kmkernel, &KMKernel::endCheckMail,
            this, &KMMainWidget::slotEndCheckMail);

    connect(kmkernel, &KMKernel::configChanged,
            this, &KMMainWidget::slotConfigChanged);

    connect(kmkernel, &KMKernel::onlineStatusChanged,
            this, &KMMainWidget::slotUpdateOnlineStatus);

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

    KMainWindow *mainWin = qobject_cast<KMainWindow *>(window());
    QStatusBar *sb = mainWin ? mainWin->statusBar() : nullptr;
    mVacationScriptIndicator = new KMail::VacationScriptIndicatorWidget(sb);
    mVacationScriptIndicator->hide();
    connect(mVacationScriptIndicator, &KMail::VacationScriptIndicatorWidget::clicked, this, &KMMainWidget::slotEditVacation);
    if (KSieveUi::Util::checkOutOfOfficeOnStartup()) {
        QTimer::singleShot(0, this, &KMMainWidget::slotCheckVacation);
    }

    connect(mFolderTreeWidget->folderTreeView()->model(), &QAbstractItemModel::modelReset,
            this, &KMMainWidget::restoreCollectionFolderViewConfig);
    restoreCollectionFolderViewConfig();

    if (kmkernel->firstStart()) {
        const QStringList listOfMailerFound = MailCommon::Util::foundMailer();
        if (!listOfMailerFound.isEmpty()) {
            if (KMessageBox::questionYesNoList(this, i18n("Another mailer was found on system. Do you want to import data from it?"), listOfMailerFound) == KMessageBox::Yes) {
                const QString path = QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard"));
                if (!QProcess::startDetached(path)) {
                    KMessageBox::error(this, i18n("Could not start the import wizard. "
                                                  "Please check your installation."),
                                       i18n("Unable to start import wizard"));
                }
            } else {
                mLaunchExternalComponent->slotAccountWizard();
            }
        } else {
            mLaunchExternalComponent->slotAccountWizard();
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
    if (mCurrentCollection.isValid()) {
        id = mCurrentCollection.id();
    }

    if (id == -1) {
        if (KMailSettings::self()->startSpecificFolderAtStartup()) {
            Akonadi::Collection::Id startupFolder = KMailSettings::self()->startupFolder();
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
    disconnect(mFolderTreeWidget->folderTreeView()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &KMMainWidget::updateFolderMenu);
    writeConfig(false); /* don't force kmkernel sync when close BUG: 289287 */
    writeFolderConfig();
    deleteWidgets();
    clearCurrentFolder();
    delete mMoveOrCopyToDialog;
    delete mSelectFromAllFoldersDialog;
    delete mSievePasswordProvider;

    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), this, nullptr);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemRemoved(Akonadi::Item)), this, nullptr);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)), this, nullptr);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), this, nullptr);
    disconnect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), this, nullptr);

    mDestructed = true;
}

void KMMainWidget::clearCurrentFolder()
{
    mCurrentFolderSettings.clear();
    mCurrentCollection = Akonadi::Collection();
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
    const bool sendOnAll
        = KMailSettings::self()->sendOnCheck() == KMailSettings::EnumSendOnCheck::SendOnAllChecks;
    const bool sendOnManual
        = KMailSettings::self()->sendOnCheck() == KMailSettings::EnumSendOnCheck::SendOnManualChecks;
    if (!kmkernel->isOffline() && (sendOnAll || sendOnManual)) {
        slotSendQueued();
    }
    // update folder menus in case some mail got filtered to trash/current folder
    // and we can enable "empty trash/move all to trash" action etc.
    updateFolderMenu();
}

void KMMainWidget::setCurrentCollection(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    if (mCurrentFolderSettings) {
        mCurrentFolderSettings->setCollection(col);
    }
}

void KMMainWidget::slotCollectionFetched(int collectionId)
{
    // Called when a collection is fetched for the first time by the ETM.
    // This is the right time to update the caption (which still says "Loading...")
    // and to update the actions that depend on the number of mails in the folder.
    if (collectionId == mCurrentCollection.id()) {
        setCurrentCollection(CommonKernel->collectionFromId(mCurrentCollection.id()));
        updateMessageActions();
        updateFolderMenu();
    }
    // We call this for any collection, it could be one of our parents...
    if (mCurrentCollection.isValid()) {
        Q_EMIT captionChangeRequest(fullCollectionPath());
    }
}

QString KMMainWidget::fullCollectionPath() const
{
    if (mCurrentCollection.isValid()) {
        return MailCommon::Util::fullCollectionPath(mCurrentCollection);
    }
    return {};
}

// Connected to the currentChanged signals from the folderTreeView and favorites view.
void KMMainWidget::slotFolderChanged(const Akonadi::Collection &collection)
{
    if (mCurrentCollection == collection)
        return;
    folderSelected(collection);
    if (collection.cachePolicy().syncOnDemand()) {
        AgentManager::self()->synchronizeCollection(collection, false);
    }
    mMsgActions->setCurrentMessage(Akonadi::Item());
    Q_EMIT captionChangeRequest(MailCommon::Util::fullCollectionPath(collection));
}

// Called by slotFolderChanged (no particular reason for this method to be split out)
void KMMainWidget::folderSelected(const Akonadi::Collection &col)
{
    if (mGoToFirstUnreadMessageInSelectedFolder) {
        // the default action has been overridden from outside
        mPreSelectionMode = MessageList::Core::PreSelectFirstUnreadCentered;
    } else {
        // use the default action
        switch (KMailSettings::self()->actionEnterFolder()) {
        case KMailSettings::EnumActionEnterFolder::SelectFirstUnread:
            mPreSelectionMode = MessageList::Core::PreSelectFirstUnreadCentered;
            break;
        case KMailSettings::EnumActionEnterFolder::SelectLastSelected:
            mPreSelectionMode = MessageList::Core::PreSelectLastSelected;
            break;
        case KMailSettings::EnumActionEnterFolder::SelectNewest:
            mPreSelectionMode = MessageList::Core::PreSelectNewestCentered;
            break;
        case KMailSettings::EnumActionEnterFolder::SelectOldest:
            mPreSelectionMode = MessageList::Core::PreSelectOldestCentered;
            break;
        default:
            mPreSelectionMode = MessageList::Core::PreSelectNone;
            break;
        }
    }

    mGoToFirstUnreadMessageInSelectedFolder = false;
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif

    if (mMsgView) {
        mMsgView->clear(true);
    }
    const bool newFolder = mCurrentCollection != col;

    // Delete any pending timer, if needed it will be recreated below
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = nullptr;
    if (newFolder) {
        // We're changing folder: write configuration for the old one
        writeFolderConfig();
    }

    mCurrentFolderSettings = FolderSettings::forCollection(col);
    mCurrentCollection = col;

    readFolderConfig();
    if (mMsgView) {
        mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
        mMsgView->setHtmlLoadExtDefault(mFolderHtmlLoadExtPreference);
    }

    if (!mCurrentFolderSettings->isValid() && (mMessagePane->count() < 2)) {
        slotIntro();
    } else {
        if (mMessagePane->isHidden()) {
            mMessagePane->show();
        }
    }

    // The message pane uses the selection model of the folder view to load the correct aggregation model and theme
    //  settings. At this point the selection model hasn't been updated yet to the user's new choice, so it would load
    //  the old folder settings instead.
    QTimer::singleShot(0, this, &KMMainWidget::slotShowSelectedFolderInPane);
}

void KMMainWidget::slotShowSelectedFolderInPane()
{
    if (mCurrentCollection.isValid()) {
        QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(KMKernel::self()->entityTreeModel(), mCurrentCollection);
        mMessagePane->setCurrentFolder(mCurrentCollection, idx, false, mPreSelectionMode);
    }
    updateMessageActions();
    updateFolderMenu();
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
    mLongFolderList = KMailSettings::self()->folderList() == KMailSettings::EnumFolderList::longlist;
    mReaderWindowActive = KMailSettings::self()->readerWindowMode() != KMailSettings::EnumReaderWindowMode::hide;
    mReaderWindowBelow = KMailSettings::self()->readerWindowMode() == KMailSettings::EnumReaderWindowMode::below;

    mHtmlGlobalSetting = MessageViewer::MessageViewerSettings::self()->htmlMail();
    mHtmlLoadExtGlobalSetting = MessageViewer::MessageViewerSettings::self()->htmlLoadExternal();

    mEnableFavoriteFolderView = (KMKernel::self()->mailCommonSettings()->favoriteCollectionViewMode() != MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode);
    mEnableFolderQuickSearch = KMailSettings::self()->enableFolderQuickSearch();

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
    if (!mCurrentCollection.isValid()) {
        return;
    }
    KSharedConfig::Ptr config = KMKernel::self()->config();
    KConfigGroup group(config, MailCommon::FolderSettings::configGroupName(mCurrentCollection));
    if (group.hasKey("htmlMailOverride")) {
        const bool useHtml = group.readEntry("htmlMailOverride", false);
        mFolderDisplayFormatPreference = useHtml ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
        group.deleteEntry("htmlMailOverride");
        group.sync();
    } else {
        mFolderDisplayFormatPreference = static_cast<MessageViewer::Viewer::DisplayFormatMessage>(group.readEntry("displayFormatOverride", static_cast<int>(MessageViewer::Viewer::UseGlobalSetting)));
    }
    mFolderHtmlLoadExtPreference
        = group.readEntry("htmlLoadExternalOverride", false);
}

//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig()
{
    if (mCurrentCollection.isValid()) {
        KSharedConfig::Ptr config = KMKernel::self()->config();
        KConfigGroup group(config, MailCommon::FolderSettings::configGroupName(mCurrentCollection));
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
    if (mMsgView) {
        disconnect(mMsgView->copyAction(), &QAction::triggered,
                   mMsgView, &KMReaderWin::slotCopySelectedText);
    }

    // If long folder list is enabled, the splitters are:
    // Splitter 1: FolderView vs (HeaderAndSearch vs MessageViewer)
    // Splitter 2: HeaderAndSearch vs MessageViewer
    //
    // If long folder list is disabled, the splitters are:
    // Splitter 1: (FolderView vs HeaderAndSearch) vs MessageViewer
    // Splitter 2: FolderView vs HeaderAndSearch

    // The folder view is both the folder tree and the favorite folder view, if
    // enabled

    const bool readerWindowAtSide = !mReaderWindowBelow && mReaderWindowActive;
    const bool readerWindowBelow = mReaderWindowBelow && mReaderWindowActive;

    mSplitter1 = new QSplitter(this);
    mSplitter2 = new QSplitter(mSplitter1);

    QWidget *folderTreeWidget = mFolderTreeWidget;
    if (mFavoriteCollectionsView) {
        mFolderViewSplitter = new QSplitter(Qt::Vertical);
        mFolderViewSplitter->setChildrenCollapsible(false);
        mFolderViewSplitter->addWidget(mFavoriteCollectionsView);
        mFolderViewSplitter->addWidget(mFolderTreeWidget);
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
    mSplitter1->setObjectName(QStringLiteral("splitter1"));
    mSplitter1->setChildrenCollapsible(false);
    mSplitter2->setObjectName(QStringLiteral("splitter2"));
    mSplitter2->setChildrenCollapsible(false);

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

    const int folderViewWidth = KMailSettings::self()->folderViewWidth();
    int ftHeight = KMailSettings::self()->folderTreeHeight();
    int headerHeight = KMailSettings::self()->searchAndHeaderHeight();
    const int messageViewerWidth = KMailSettings::self()->readerWindowWidth();
    int headerWidth = KMailSettings::self()->searchAndHeaderWidth();
    int messageViewerHeight = KMailSettings::self()->readerWindowHeight();

    int ffvHeight = mFolderViewSplitter ? KMKernel::self()->mailCommonSettings()->favoriteCollectionViewHeight() : 0;

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
    if (mMsgView) {
        connect(mMsgView->copyAction(), &QAction::triggered,
                mMsgView, &KMReaderWin::slotCopySelectedText);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::refreshFavoriteFoldersViewProperties()
{
    if (mFavoriteCollectionsView) {
        if (KMKernel::self()->mailCommonSettings()->favoriteCollectionViewMode() == MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode) {
            mFavoriteCollectionsView->changeViewMode(QListView::IconMode);
        } else if (KMKernel::self()->mailCommonSettings()->favoriteCollectionViewMode() == MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode) {
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

        layoutChanged = (oldLongFolderList != mLongFolderList)
                        || (oldReaderWindowActive != mReaderWindowActive)
                        || (oldReaderWindowBelow != mReaderWindowBelow)
                        || (oldFavoriteFolderView != mEnableFavoriteFolderView);

        if (layoutChanged) {
            deleteWidgets();
            createWidgets();
            restoreCollectionFolderViewConfig();
            Q_EMIT recreateGui();
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
            QTimer::singleShot(0, this, &KMMainWidget::slotCheckMailOnStartup);
        }
    }

    if (layoutChanged) {
        layoutSplitters();
    }

    updateMessageMenu();
    updateFileMenu();
    kmkernel->toggleSystemTray();
    mAccountActionMenu->setAccountOrder(KMKernel::self()->mailCommonSettings()->order());

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

        KMailSettings::self()->setSearchAndHeaderHeight(headersHeight);
        KMailSettings::self()->setSearchAndHeaderWidth(mMessagePane->width());
        if (mFavoriteCollectionsView) {
            KMKernel::self()->mailCommonSettings()->setFavoriteCollectionViewHeight(mFavoriteCollectionsView->height());
            KMailSettings::self()->setFolderTreeHeight(mFolderTreeWidget->height());
            if (!mLongFolderList) {
                KMailSettings::self()->setFolderViewHeight(mFolderViewSplitter->height());
            }
        } else if (!mLongFolderList && mFolderTreeWidget) {
            KMailSettings::self()->setFolderTreeHeight(mFolderTreeWidget->height());
        }
        if (mFolderTreeWidget) {
            KMailSettings::self()->setFolderViewWidth(mFolderTreeWidget->width());
            KSharedConfig::Ptr config = KMKernel::self()->config();
            KConfigGroup group(config, "CollectionFolderView");

            ETMViewStateSaver saver;
            saver.setView(mFolderTreeWidget->folderTreeView());
            saver.saveState(group);

            group.writeEntry("HeaderState", mFolderTreeWidget->folderTreeView()->header()->saveState());
            //Work around from startup folder
            group.deleteEntry("Selection");
#if 0
            if (!KMailSettings::self()->startSpecificFolderAtStartup()) {
                group.deleteEntry("Current");
            }
#endif
            group.sync();
        }

        if (mMsgView) {
            if (!mReaderWindowBelow) {
                KMailSettings::self()->setReaderWindowWidth(mMsgView->width());
            }
            mMsgView->viewer()->writeConfig(force);
            KMailSettings::self()->setReaderWindowHeight(mMsgView->height());
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

KMReaderWin *KMMainWidget::messageView() const
{
    return mMsgView;
}

CollectionPane *KMMainWidget::messageListPane() const
{
    return mMessagePane;
}

Collection KMMainWidget::currentCollection() const
{
    return mCurrentCollection;
}

//-----------------------------------------------------------------------------
void KMMainWidget::deleteWidgets()
{
    // Simply delete the top splitter, which always is mSplitter1, regardless
    // of the layout. This deletes all children.
    // akonadi action manager is created in createWidgets(), parented to this
    //  so not autocleaned up.
    delete mAkonadiStandardActionManager;
    mAkonadiStandardActionManager = nullptr;
    delete mSplitter1;
    mMsgView = nullptr;
    mFolderViewSplitter = nullptr;
    mFavoriteCollectionsView = nullptr;
    mFolderTreeWidget = nullptr;
    mSplitter1 = nullptr;
    mSplitter2 = nullptr;
    mFavoritesModel = nullptr;
}

//-----------------------------------------------------------------------------
void KMMainWidget::createWidgets()
{
    // Note that all widgets we create in this function have the parent 'this'.
    // They will be properly reparented in layoutSplitters()

    //
    // Create the folder tree
    //
    FolderTreeWidget::TreeViewOptions opt = FolderTreeWidget::ShowUnreadCount;
    opt |= FolderTreeWidget::UseLineEditForFiltering;
    opt |= FolderTreeWidget::ShowCollectionStatisticAnimation;
    opt |= FolderTreeWidget::DontKeyFilter;
    mFolderTreeWidget = new FolderTreeWidget(this, mGUIClient, opt);

    connect(mFolderTreeWidget->folderTreeView(), QOverload<const Akonadi::Collection &>::of(&EntityTreeView::currentChanged), this, &KMMainWidget::slotFolderChanged);

    connect(mFolderTreeWidget->folderTreeView()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &KMMainWidget::updateFolderMenu);

    connect(mFolderTreeWidget->folderTreeView(), &FolderTreeView::newTabRequested, this, &KMMainWidget::slotCreateNewTab);

    //
    // Create the message pane
    //
    mMessagePane = new CollectionPane(!KMailSettings::self()->startSpecificFolderAtStartup(), KMKernel::self()->entityTreeModel(),
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

    connect(mMessagePane, &MessageList::Pane::statusMessage,
            this, &KMMainWidget::showMessageActivities);

    connect(mMessagePane, &MessageList::Pane::forceLostFocus,
            this, &KMMainWidget::slotSetFocusToViewer);

    //
    // Create the reader window
    //
    if (mReaderWindowActive) {
        mMsgView = new KMReaderWin(this, this, actionCollection());
        if (mMsgActions) {
            mMsgActions->setMessageView(mMsgView);
        }
        connect(mMsgView->viewer(), &MessageViewer::Viewer::displayPopupMenu, this, &KMMainWidget::slotMessagePopup);
        connect(mMsgView->viewer(), &MessageViewer::Viewer::moveMessageToTrash,
                this, &KMMainWidget::slotMoveMessageToTrash);
        connect(mMsgView->viewer(), &MessageViewer::Viewer::pageIsScrolledToBottom,
                this, &KMMainWidget::slotPageIsScrolledToBottom);
        connect(mMsgView->viewer(), &MessageViewer::Viewer::replyMessageTo,
                this, &KMMainWidget::slotReplyMessageTo);
        if (mShowIntroductionAction) {
            mShowIntroductionAction->setEnabled(true);
        }
    } else {
        if (mMsgActions) {
            mMsgActions->setMessageView(nullptr);
        }
        if (mShowIntroductionAction) {
            mShowIntroductionAction->setEnabled(false);
        }
    }
    if (!KMailSettings::self()->enableFolderQuickSearch()) {
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
        mFavoriteCollectionsView = new FavoriteCollectionWidget(KMKernel::self()->mailCommonSettings(), mGUIClient, this);
        refreshFavoriteFoldersViewProperties();
        connect(mFavoriteCollectionsView, QOverload<const Akonadi::Collection &>::of(&EntityListView::currentChanged), this, &KMMainWidget::slotFolderChanged);
        connect(mFavoriteCollectionsView, &FavoriteCollectionWidget::newTabRequested, this, &KMMainWidget::slotCreateNewTab);
        mFavoritesModel = new Akonadi::FavoriteCollectionsModel(
            mFolderTreeWidget->folderTreeWidgetProxyModel(),
            KMKernel::self()->config()->group("FavoriteCollections"), mFavoriteCollectionsView);

        auto *orderProxy = new MailCommon::FavoriteCollectionOrderProxyModel(this);
        orderProxy->setOrderConfig(KMKernel::self()->config()->group("FavoriteCollectionsOrder"));
        orderProxy->setSourceModel(mFavoritesModel);
        orderProxy->sort(0, Qt::AscendingOrder);

        mFavoriteCollectionsView->setModel(orderProxy);

        mAkonadiStandardActionManager->setFavoriteCollectionsModel(mFavoritesModel);
        mAkonadiStandardActionManager->setFavoriteSelectionModel(mFavoriteCollectionsView->selectionModel());
    }

    //Don't use mMailActionManager->createAllActions() to save memory by not
    //creating actions that doesn't make sense.
    const auto standardActions = {
        StandardActionManager::CreateCollection,
        StandardActionManager::CopyCollections,
        StandardActionManager::DeleteCollections,
        StandardActionManager::SynchronizeCollections,
        StandardActionManager::CollectionProperties,
        StandardActionManager::CopyItems,
        StandardActionManager::Paste,
        StandardActionManager::DeleteItems,
        StandardActionManager::ManageLocalSubscriptions,
        StandardActionManager::CopyCollectionToMenu,
        StandardActionManager::CopyItemToMenu,
        StandardActionManager::MoveItemToMenu,
        StandardActionManager::MoveCollectionToMenu,
        StandardActionManager::CutItems,
        StandardActionManager::CutCollections,
        StandardActionManager::CreateResource,
        StandardActionManager::DeleteResources,
        StandardActionManager::ResourceProperties,
        StandardActionManager::SynchronizeResources,
        StandardActionManager::ToggleWorkOffline,
        StandardActionManager::SynchronizeCollectionsRecursive,
    };

    for (StandardActionManager::Type standardAction : standardActions) {
        mAkonadiStandardActionManager->createAction(standardAction);
    }

    if (mEnableFavoriteFolderView) {
        const auto favoriteActions = {
            StandardActionManager::AddToFavoriteCollections,
            StandardActionManager::RemoveFromFavoriteCollections,
            StandardActionManager::RenameFavoriteCollection,
            StandardActionManager::SynchronizeFavoriteCollections,
        };
        for (StandardActionManager::Type favoriteAction : favoriteActions) {
            mAkonadiStandardActionManager->createAction(favoriteAction);
        }
    }

    const auto mailActions = {
        StandardMailActionManager::MarkAllMailAsRead,
        StandardMailActionManager::MoveToTrash,
        StandardMailActionManager::MoveAllToTrash,
        StandardMailActionManager::RemoveDuplicates,
        StandardMailActionManager::EmptyAllTrash,
        StandardMailActionManager::MarkMailAsRead,
        StandardMailActionManager::MarkMailAsUnread,
        StandardMailActionManager::MarkMailAsImportant,
        StandardMailActionManager::MarkMailAsActionItem
    };

    for (StandardMailActionManager::Type mailAction : mailActions) {
        mAkonadiStandardActionManager->createAction(mailAction);
    }

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::CollectionProperties);
    connect(mAkonadiStandardActionManager->action(
                Akonadi::StandardActionManager::CollectionProperties), &QAction::triggered, mManageShowCollectionProperties, &ManageShowCollectionProperties::slotCollectionProperties);

    //
    // Create all kinds of actions
    //
    mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::RemoveDuplicates)->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Asterisk));
    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::RemoveDuplicates);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::RemoveDuplicates), &QAction::triggered, this, &KMMainWidget::slotRemoveDuplicates);

    mCollectionProperties = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CollectionProperties);
    connect(kmkernel->folderCollectionMonitor(), &Monitor::collectionRemoved,
            this, &KMMainWidget::slotCollectionRemoved);
    connect(kmkernel->folderCollectionMonitor(), &Monitor::itemAdded,
            this, &KMMainWidget::slotItemAdded);
    connect(kmkernel->folderCollectionMonitor(), &Monitor::itemRemoved,
            this, &KMMainWidget::slotItemRemoved);
    connect(kmkernel->folderCollectionMonitor(), &Monitor::itemMoved,
            this, &KMMainWidget::slotItemMoved);
    connect(kmkernel->folderCollectionMonitor(), QOverload<const Akonadi::Collection &, const QSet<QByteArray> &>::of(&ChangeRecorder::collectionChanged),
            this, &KMMainWidget::slotCollectionChanged);

    connect(kmkernel->folderCollectionMonitor(), &Monitor::collectionStatisticsChanged, this, &KMMainWidget::slotCollectionStatisticsChanged);
}

void KMMainWidget::updateMoveAction(const Akonadi::CollectionStatistics &statistic)
{
    const bool hasUnreadMails = (statistic.unreadCount() > 0);
    updateMoveAction(hasUnreadMails);
}

void KMMainWidget::updateMoveAction(bool hasUnreadMails)
{
    const bool enable_goto_unread = hasUnreadMails
                                    || (KMailSettings::self()->loopOnGotoUnread() == KMailSettings::EnumLoopOnGotoUnread::LoopInAllFolders)
                                    || (KMailSettings::self()->loopOnGotoUnread() == KMailSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders);
    actionCollection()->action(QStringLiteral("go_next_unread_message"))->setEnabled(enable_goto_unread);
    actionCollection()->action(QStringLiteral("go_prev_unread_message"))->setEnabled(enable_goto_unread);
    if (mAkonadiStandardActionManager && mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MarkAllMailAsRead)) {
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MarkAllMailAsRead)->setEnabled(hasUnreadMails);
    }
}

void KMMainWidget::updateAllToTrashAction(qint64 statistics)
{
    if (mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)) {
        const bool folderWithContent = mCurrentFolderSettings && !mCurrentFolderSettings->isStructural();
        mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setEnabled(folderWithContent
                                                                                                              && (statistics > 0)
                                                                                                              && mCurrentFolderSettings->canDeleteMessages());
    }
}

void KMMainWidget::slotCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistic)
{
    if (id == CommonKernel->outboxCollectionFolder().id()) {
        const bool enableAction = (statistic.count() > 0);
        mSendQueued->setEnabled(enableAction);
        mSendActionMenu->setEnabled(enableAction);
    } else if (id == mCurrentCollection.id()) {
        updateMoveAction(statistic);
        updateAllToTrashAction(statistic.count());
        setCurrentCollection(CommonKernel->collectionFromId(mCurrentCollection.id()));
    }
}

void KMMainWidget::slotCreateNewTab(bool preferNewTab)
{
    mMessagePane->setPreferEmptyTab(preferNewTab);
}

void KMMainWidget::slotCollectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &set)
{
    if ((collection == mCurrentCollection)
        && (set.contains("MESSAGEFOLDER") || set.contains("expirationcollectionattribute"))) {
        if (set.contains("MESSAGEFOLDER")) {
            mMessagePane->resetModelStorage();
        } else {
            setCurrentCollection(collection);
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
bool KMMainWidget::showSearchDialog()
{
    if (!mSearchWin) {
        mSearchWin = new SearchWindow(this, mCurrentCollection);
        mSearchWin->setModal(false);
        mSearchWin->setObjectName(QStringLiteral("Search"));
    } else {
        mSearchWin->activateFolder(mCurrentCollection);
    }

    mSearchWin->show();
    KWindowSystem::activateWindow(mSearchWin->winId());
    return true;
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

    mManageSieveDialog = new KSieveUi::ManageSieveScriptsDialog(mSievePasswordProvider);
    connect(mManageSieveDialog.data(), &KSieveUi::ManageSieveScriptsDialog::finished, this, &KMMainWidget::slotCheckVacation);
    mManageSieveDialog->show();
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

void KMMainWidget::slotCompose()
{
    ComposeNewMessageJob *job = new ComposeNewMessageJob;
    job->setFolderSettings(mCurrentFolderSettings);
    job->setCurrentCollection(mCurrentCollection);
    job->start();
}

//-----------------------------------------------------------------------------
// TODO: do we want the list sorted alphabetically?
void KMMainWidget::slotShowNewFromTemplate()
{
    if (mCurrentFolderSettings) {
        const KIdentityManagement::Identity &ident
            = kmkernel->identityManager()->identityForUoidOrDefault(mCurrentFolderSettings->identity());
        mTemplateFolder = CommonKernel->collectionFromId(ident.templates().toLongLong());
    }

    if (!mTemplateFolder.isValid()) {
        mTemplateFolder = CommonKernel->templatesCollectionFolder();
    }
    if (!mTemplateFolder.isValid()) {
        qCWarning(KMAIL_LOG) << "Template folder not found";
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
        KMime::Message::Ptr msg = MessageComposer::Util::message(items.at(idx));
        if (msg) {
            QString subj = msg->subject()->asUnicodeString();
            if (subj.isEmpty()) {
                subj = i18n("No Subject");
            }

            QAction *templateAction = mTemplateMenu->menu()->addAction(KStringHandler::rsqueeze(subj.replace(QLatin1Char('&'), QStringLiteral("&&"))));
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
    if (mCurrentFolderSettings && mCurrentFolderSettings->isMailingListEnabled()) {
        if (KMail::Util::mailingListPost(mCurrentFolderSettings, mCurrentCollection)) {
            return;
        }
    }
    slotCompose();
}

void KMMainWidget::slotExpireFolder()
{
    if (!mCurrentFolderSettings) {
        return;
    }
    bool mustDeleteExpirationAttribute = false;
    MailCommon::ExpireCollectionAttribute *attr = MailCommon::Util::expirationCollectionAttribute(mCurrentCollection, mustDeleteExpirationAttribute);
    bool canBeExpired = true;
    if (!attr->isAutoExpire()) {
        canBeExpired = false;
    } else if (attr->unreadExpireUnits() == MailCommon::ExpireCollectionAttribute::ExpireNever
               && attr->readExpireUnits() == MailCommon::ExpireCollectionAttribute::ExpireNever) {
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

    if (KMailSettings::self()->warnBeforeExpire()) {
        const QString message = i18n("<qt>Are you sure you want to expire the folder <b>%1</b>?</qt>",
                                     mCurrentFolderSettings->name().toHtmlEscaped());
        if (KMessageBox::warningContinueCancel(this, message, i18n("Expire Folder"),
                                               KGuiItem(i18n("&Expire")))
            != KMessageBox::Continue) {
            if (mustDeleteExpirationAttribute) {
                delete attr;
            }
            return;
        }
    }

    MailCommon::Util::expireOldMessages(mCurrentCollection, true /*immediate*/);
    if (mustDeleteExpirationAttribute) {
        delete attr;
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEmptyFolder()
{
    if (!mCurrentCollection.isValid()) {
        return;
    }
    const bool isTrash = CommonKernel->folderIsTrash(mCurrentCollection);
    const QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
    const QString text = (isTrash)
                         ? i18n("Are you sure you want to empty the trash folder?")
                         : i18n("<qt>Are you sure you want to move all messages from "
                                "folder <b>%1</b> to the trash?</qt>", mCurrentCollection.name().toHtmlEscaped());

    if (KMessageBox::warningContinueCancel(this, text, title, KGuiItem(title, QStringLiteral("user-trash")))
        != KMessageBox::Continue) {
        return;
    }
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    slotSelectAllMessages();
    if (isTrash) {
        /* Don't ask for confirmation again when deleting, the user has already
        confirmed. */
        deleteSelectedMessages(false);
    } else {
        slotTrashSelectedMessages();
    }

    if (mMsgView) {
        mMsgView->clearCache();
    }

    if (!isTrash) {
        const QString str = i18n("Moved all messages to the trash");
        showMessageActivities(str);
    }

    updateMessageActions();

    // Disable empty trash/move all to trash action - we've just deleted/moved
    // all folder contents.
    mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)->setEnabled(false);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotArchiveFolder()
{
    if (mCurrentCollection.isValid()) {
        QPointer<KMail::ArchiveFolderDialog> archiveDialog = new KMail::ArchiveFolderDialog(this);
        archiveDialog->setFolder(mCurrentCollection);
        archiveDialog->exec();
        delete archiveDialog;
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotRemoveFolder()
{
    if (!mCurrentFolderSettings) {
        return;
    }
    if (!mCurrentFolderSettings->isValid()) {
        return;
    }
    if (mCurrentFolderSettings->isSystemFolder()) {
        return;
    }
    if (mCurrentFolderSettings->isReadOnly()) {
        return;
    }

    RemoveCollectionJob *job = new RemoveCollectionJob(this);
    connect(job, &RemoveCollectionJob::clearCurrentFolder, this, &KMMainWidget::slotClearCurrentFolder);
    job->setMainWidget(this);
    job->setCurrentFolder(mCurrentCollection);
    job->start();
}

void KMMainWidget::slotClearCurrentFolder()
{
    clearCurrentFolder();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireAll()
{
    if (KMailSettings::self()->warnBeforeExpire()) {
        const int ret = KMessageBox::warningContinueCancel(KMainWindow::memberList().constFirst(),
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
                                                        QStringLiteral("OverrideHtmlLoadExtWarning"), nullptr);
        if (result == KMessageBox::Cancel) {
            mPreferHtmlLoadExtAction->setChecked(false);
            return;
        }
    }
    mFolderHtmlLoadExtPreference = !mFolderHtmlLoadExtPreference;

    if (mMsgView) {
        mMsgView->setHtmlLoadExtDefault(mFolderHtmlLoadExtPreference);
        mMsgView->update(true);
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardInlineMsg()
{
    if (!mCurrentFolderSettings) {
        return;
    }

    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    const QString text = mMsgView ? mMsgView->copyText() : QString();
    KMForwardCommand *command = new KMForwardCommand(
        this, selectedMessages, mCurrentFolderSettings->identity(), QString(), text
        );

    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMessage()
{
    if (!mCurrentFolderSettings) {
        return;
    }

    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    KMForwardAttachedCommand *command = new KMForwardAttachedCommand(
        this, selectedMessages, mCurrentFolderSettings->identity()
        );

    command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotUseTemplate()
{
    newFromTemplate(mMessagePane->currentItem());
}

//-----------------------------------------------------------------------------
// Message moving and permanent deletion
//

void KMMainWidget::moveMessageSelected(MessageList::Core::MessageItemSetReference ref, const Akonadi::Collection &dest, bool confirmOnDeletion)
{
    Akonadi::Item::List selectMsg = mMessagePane->itemListFromPersistentSet(ref);
    // If this is a deletion, ask for confirmation
    if (confirmOnDeletion) {
        const int selectedMessageCount = selectMsg.count();
        int ret = KMessageBox::warningContinueCancel(
            this,
            i18np(
                "<qt>Do you really want to delete the selected message?<br />"
                "Once deleted, it cannot be restored.</qt>",
                "<qt>Do you really want to delete the %1 selected messages?<br />"
                "Once deleted, they cannot be restored.</qt>",
                selectedMessageCount
                ),
            selectedMessageCount > 1 ? i18n("Delete Messages") : i18n("Delete Message"),
            KStandardGuiItem::del(),
            KStandardGuiItem::cancel()
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
        command, &KMMoveCommand::moveDone,
        this, &KMMainWidget::slotMoveMessagesCompleted
        );
    command->start();

    if (dest.isValid()) {
        showMessageActivities(i18n("Moving messages..."));
    } else {
        showMessageActivities(i18n("Deleting messages..."));
    }
}

void KMMainWidget::slotMoveMessagesCompleted(KMMoveCommand *command)
{
    Q_ASSERT(command);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(command->refSet(), false);
    mMessagePane->deletePersistentSet(command->refSet());
    // Bleah :D
    const bool moveWasReallyADelete = !command->destFolder().isValid();

    QString str;
    if (command->result() == KMCommand::OK) {
        if (moveWasReallyADelete) {
            str = i18n("Messages deleted successfully.");
        } else {
            str = i18n("Messages moved successfully.");
        }
    } else {
        if (moveWasReallyADelete) {
            if (command->result() == KMCommand::Failed) {
                str = i18n("Deleting messages failed.");
            } else {
                str = i18n("Deleting messages canceled.");
            }
        } else {
            if (command->result() == KMCommand::Failed) {
                str = i18n("Moving messages failed.");
            } else {
                str = i18n("Moving messages canceled.");
            }
        }
    }
    showMessageActivities(str);
    // The command will autodelete itself and will also kill the set.
}

void KMMainWidget::slotDeleteMessages()
{
    deleteSelectedMessages(true);
}

Akonadi::Item::List KMMainWidget::currentSelection() const
{
    Akonadi::Item::List selectMsg;
    MessageList::Core::MessageItemSetReference ref = mMessagePane->selectionAsPersistentSet();
    if (ref != -1) {
        selectMsg = mMessagePane->itemListFromPersistentSet(ref);
    }
    return selectMsg;
}

void KMMainWidget::deleteSelectedMessages(bool confirmDelete)
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

void KMMainWidget::copyMessageSelected(const Akonadi::Item::List &selectMsg, const Akonadi::Collection &dest)
{
    if (selectMsg.isEmpty()) {
        return;
    }
    // And stuff them into a KMCopyCommand :)
    KMCommand *command = new KMCopyCommand(dest, selectMsg);
    QObject::connect(
        command, &KMCommand::completed,
        this, &KMMainWidget::slotCopyMessagesCompleted
        );
    command->start();
    showMessageActivities(i18n("Copying messages..."));
}

void KMMainWidget::slotCopyMessagesCompleted(KMCommand *command)
{
    Q_ASSERT(command);
    QString str;
    if (command->result() == KMCommand::OK) {
        str = i18n("Messages copied successfully.");
    } else {
        if (command->result() == KMCommand::Failed) {
            str = i18n("Copying messages failed.");
        } else {
            str = i18n("Copying messages canceled.");
        }
    }
    showMessageActivities(str);
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
    const Akonadi::Item::List lstMsg = mMessagePane->selectionAsMessageItemList();
    if (!lstMsg.isEmpty()) {
        copyMessageSelected(lstMsg, dest);
    }
}

//-----------------------------------------------------------------------------
// Message trashing
//
void KMMainWidget::trashMessageSelected(MessageList::Core::MessageItemSetReference ref)
{
    if (!mCurrentCollection.isValid()) {
        return;
    }

    const Akonadi::Item::List select = mMessagePane->itemListFromPersistentSet(ref);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(ref, true);

    // FIXME: Why we don't use KMMoveCommand( trashFolder(), selectedMessages ); ?
    // And stuff them into a KMTrashMsgCommand :)
    KMTrashMsgCommand *command = new KMTrashMsgCommand(mCurrentCollection, select, ref);

    QObject::connect(command, &KMTrashMsgCommand::moveDone,
                     this, &KMMainWidget::slotTrashMessagesCompleted);
    command->start();
    switch (command->operation()) {
    case KMTrashMsgCommand::MoveToTrash:
        showMessageActivities(i18n("Moving messages to trash..."));
        break;
    case KMTrashMsgCommand::Delete:
        showMessageActivities(i18n("Deleting messages..."));
        break;
    case KMTrashMsgCommand::Both:
    case KMTrashMsgCommand::Unknown:
        showMessageActivities(i18n("Deleting and moving messages to trash..."));
        break;
    }
}

void KMMainWidget::slotTrashMessagesCompleted(KMTrashMsgCommand *command)
{
    Q_ASSERT(command);
    mMessagePane->markMessageItemsAsAboutToBeRemoved(command->refSet(), false);
    mMessagePane->deletePersistentSet(command->refSet());
    if (command->result() == KMCommand::OK) {
        switch (command->operation()) {
        case KMTrashMsgCommand::MoveToTrash:
            showMessageActivities(i18n("Messages moved to trash successfully."));
            break;
        case KMTrashMsgCommand::Delete:
            showMessageActivities(i18n("Messages deleted successfully."));
            break;
        case KMTrashMsgCommand::Both:
        case KMTrashMsgCommand::Unknown:
            showMessageActivities(i18n("Messages moved to trash or deleted successfully"));
            break;
        }
    } else if (command->result() == KMCommand::Failed) {
        switch (command->operation()) {
        case KMTrashMsgCommand::MoveToTrash:
            showMessageActivities(i18n("Moving messages to trash failed."));
            break;
        case KMTrashMsgCommand::Delete:
            showMessageActivities(i18n("Deleting messages failed."));
            break;
        case KMTrashMsgCommand::Both:
        case KMTrashMsgCommand::Unknown:
            showMessageActivities(i18n("Deleting or moving messages to trash failed."));
            break;
        }
    } else {
        switch (command->operation()) {
        case KMTrashMsgCommand::MoveToTrash:
            showMessageActivities(i18n("Moving messages to trash canceled."));
            break;
        case KMTrashMsgCommand::Delete:
            showMessageActivities(i18n("Deleting messages canceled."));
            break;
        case KMTrashMsgCommand::Both:
        case KMTrashMsgCommand::Unknown:
            showMessageActivities(i18n("Deleting or moving messages to trash canceled."));
            break;
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
void KMMainWidget::toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag)
{
    if (select.isEmpty()) {
        return;
    }
    KMCommand *command = new KMSetTagCommand(Akonadi::Tag::List() << tag, select, KMSetTagCommand::Toggle);
    command->start();
}

void KMMainWidget::slotSelectMoreMessageTagList()
{
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }

    QPointer<TagSelectDialog> dlg = new TagSelectDialog(this, selectedMessages.count(), selectedMessages.first());
    dlg->setActionCollection(QList<KActionCollection *> { actionCollection() });
    if (dlg->exec()) {
        const Akonadi::Tag::List lst = dlg->selectedTag();
        KMCommand *command = new KMSetTagCommand(lst, selectedMessages, KMSetTagCommand::CleanExistingAndAddNew);
        command->start();
    }
    delete dlg;
}

void KMMainWidget::slotUpdateMessageTagList(const Akonadi::Tag &tag)
{
    // Create a persistent set from the current thread.
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    toggleMessageSetTag(selectedMessages, tag);
}

void KMMainWidget::refreshMessageListSelection()
{
    mAkonadiStandardActionManager->setItemSelectionModel(mMessagePane->currentItemSelectionModel());
    slotMessageSelected(mMessagePane->currentItem());
    Q_EMIT captionChangeRequest(MailCommon::Util::fullCollectionPath(mMessagePane->currentFolder()));
}

//-----------------------------------------------------------------------------
// Status setting for threads
//
// FIXME: The "selection" version of these functions is in MessageActions.
//        We should probably move everything there....
void KMMainWidget::setMessageSetStatus(const Akonadi::Item::List &select, const Akonadi::MessageStatus &status, bool toggle)
{
    KMCommand *command = new KMSetStatusCommand(status, select, toggle);
    command->start();
}

void KMMainWidget::setCurrentThreadStatus(const Akonadi::MessageStatus &status, bool toggle)
{
    const Akonadi::Item::List select = mMessagePane->currentThreadAsMessageList();
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
void KMMainWidget::slotRedirectMessage()
{
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
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

    qCDebug(KMAIL_LOG) << "Reply with template:" << tmpl;

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

    qCDebug(KMAIL_LOG) << "Reply to All with template:" << tmpl;

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
    if (!mCurrentFolderSettings) {
        return;
    }

    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    const QString text = mMsgView ? mMsgView->copyText() : QString();
    qCDebug(KMAIL_LOG) << "Forward with template:" << tmpl;
    KMForwardCommand *command = new KMForwardCommand(
        this, selectedMessages, mCurrentFolderSettings->identity(), tmpl, text
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
        openFilterDialog("From", msg->from()->asUnicodeString());
    } else {
        openFilterDialog("From", al.front().asString());
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToFilter()
{
    KMime::Message::Ptr msg = mMessagePane->currentMessage();
    if (!msg) {
        return;
    }
    openFilterDialog("To", msg->to()->asUnicodeString());
}

void KMMainWidget::slotCcFilter()
{
    KMime::Message::Ptr msg = mMessagePane->currentMessage();
    if (!msg) {
        return;
    }
    openFilterDialog("Cc", msg->cc()->asUnicodeString());
}

void KMMainWidget::slotBandwidth(bool b)
{
    PimCommon::NetworkUtil::self()->setLowBandwidth(b);
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
        slotFolderChanged(col); // call it explicitly in case the collection is filtered out in the foldertreeview
    }
}

void KMMainWidget::slotApplyFilters()
{
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    applyFilters(selectedMessages);
}

Akonadi::Collection::List KMMainWidget::applyFilterOnCollection(bool recursive)
{
    Akonadi::Collection::List cols;
    if (recursive) {
        cols = KMKernel::self()->subfolders(mCurrentCollection);
    } else {
        cols << mCurrentCollection;
    }
    return cols;
}

void KMMainWidget::slotApplyFiltersOnFolder(bool recursive)
{
    if (mCurrentCollection.isValid()) {
        const Akonadi::Collection::List cols = applyFilterOnCollection(recursive);
        applyFilters(cols);
    }
}

void KMMainWidget::slotApplyFilterOnFolder(bool recursive)
{
    if (mCurrentCollection.isValid()) {
        const Akonadi::Collection::List cols = applyFilterOnCollection(recursive);
        QAction *action = qobject_cast<QAction *>(sender());
        applyFilter(cols, action->property("filter_id").toString());
    }
}

void KMMainWidget::applyFilters(const Akonadi::Item::List &selectedMessages)
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif

    MailCommon::FilterManager::instance()->filter(selectedMessages);
}

void KMMainWidget::applyFilters(const Akonadi::Collection::List &selectedCols)
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    MailCommon::FilterManager::instance()->filter(selectedCols);
}

void KMMainWidget::applyFilter(const Akonadi::Collection::List &selectedCols, const QString &filter)
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    MailCommon::FilterManager::instance()->filter(selectedCols, { filter });
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckVacation()
{
    updateVacationScriptStatus(false);
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    mVacationManager->checkVacation();
}

void KMMainWidget::slotEditCurrentVacation()
{
    slotEditVacation(QString());
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
    if (kmkernel->allowToDebug()) {
        QPointer<KSieveUi::SieveDebugDialog> mSieveDebugDialog = new KSieveUi::SieveDebugDialog(mSievePasswordProvider, this);
        mSieveDebugDialog->exec();
        delete mSieveDebugDialog;
    }
}

void KMMainWidget::slotConfigChanged()
{
    readConfig();
    mMsgActions->setupForwardActions(actionCollection());
    mMsgActions->setupForwardingActionsList(mGUIClient);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveMsg()
{
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
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
    const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
    if (selectedMessages.isEmpty()) {
        return;
    }
    // Avoid re-downloading in the common case that only one message is selected, and the message
    // is also displayed in the viewer. For this, create a dummy item without a parent collection / item id,
    // so that KMCommand doesn't download it.
    KMSaveAttachmentsCommand *saveCommand = nullptr;
    if (mMsgView && selectedMessages.size() == 1
        && mMsgView->message().hasPayload<KMime::Message::Ptr>()
        && selectedMessages.first().id() == mMsgView->message().id()) {
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
    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Online) {
        // if online; then toggle and set it offline.
        kmkernel->stopNetworkJobs();
    } else {
        kmkernel->resumeNetworkJobs();
        slotCheckVacation();
    }
}

void KMMainWidget::slotUpdateOnlineStatus(KMailSettings::EnumNetworkState::type)
{
    if (!mAkonadiStandardActionManager) {
        return;
    }
    QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::ToggleWorkOffline);
    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Online) {
        action->setText(i18n("Work Offline"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("user-offline")));
    } else {
        action->setText(i18n("Work Online"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("user-online")));
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueued()
{
    if (kmkernel->msgSender()) {
        if (!kmkernel->msgSender()->sendQueued()) {
            KMessageBox::error(this, i18n("Impossible to send email"), i18n("Send Email"));
        }
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueuedVia(MailTransport::Transport *transport)
{
    if (transport) {
        if (kmkernel->msgSender()) {
            if (!kmkernel->msgSender()->sendQueued(transport->id())) {
                KMessageBox::error(this, i18n("Impossible to send email"), i18n("Send Email"));
            }
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
            true,      // center item
            KMailSettings::self()->loopOnGotoUnread() != KMailSettings::EnumLoopOnGotoUnread::DontLoop
            )) {
        // no next unread message was found in the current folder
        if ((KMailSettings::self()->loopOnGotoUnread()
             == KMailSettings::EnumLoopOnGotoUnread::LoopInAllFolders)
            || (KMailSettings::self()->loopOnGotoUnread()
                == KMailSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders)) {
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
            true,      // center item
            KMailSettings::self()->loopOnGotoUnread() == KMailSettings::EnumLoopOnGotoUnread::LoopInCurrentFolder
            )) {
        // no next unread message was found in the current folder
        if ((KMailSettings::self()->loopOnGotoUnread()
             == KMailSettings::EnumLoopOnGotoUnread::LoopInAllFolders)
            || (KMailSettings::self()->loopOnGotoUnread()
                == KMailSettings::EnumLoopOnGotoUnread::LoopInAllMarkedFolders)) {
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
    if (!mCurrentCollection.isValid() || !msg.isValid()) {
        return;
    }

    if (CommonKernel->folderIsDraftOrOutbox(mCurrentCollection)) {
        mMsgActions->setCurrentMessage(msg);
        mMsgActions->editCurrentMessage();
        return;
    }

    if (CommonKernel->folderIsTemplates(mCurrentCollection)) {
        slotUseTemplate();
        return;
    }

    KMReaderMainWin *win = nullptr;
    if (!mMsgView) {
        win = new KMReaderMainWin(mFolderDisplayFormatPreference, mFolderHtmlLoadExtPreference);
    }
    // Try to fetch the mail, even in offline mode, it might be cached
    KMFetchMessageCommand *cmd = new KMFetchMessageCommand(this, msg, win ? win->viewer() : mMsgView->viewer(), win);
    connect(cmd, &KMCommand::completed,
            this, &KMMainWidget::slotItemsFetchedForActivation);
    cmd->start();
}

void KMMainWidget::slotItemsFetchedForActivation(KMCommand *command)
{
    KMCommand::Result result = command->result();
    if (result != KMCommand::OK) {
        qCDebug(KMAIL_LOG) << "slotItemsFetchedForActivation result:" << result;
        return;
    }

    KMFetchMessageCommand *fetchCmd = qobject_cast<KMFetchMessageCommand *>(command);
    const Item msg = fetchCmd->item();
    KMReaderMainWin *win = fetchCmd->readerMainWin();

    if (!win) {
        win = new KMReaderMainWin(mFolderDisplayFormatPreference, mFolderHtmlLoadExtPreference);
    }
    win->viewer()->setWebViewZoomFactor(mMsgView->viewer()->webViewZoomFactor());
    const bool useFixedFont = mMsgView ? mMsgView->isFixedFont()
                              : MessageViewer::MessageViewerSettings::self()->useFixedFont();
    win->setUseFixedFont(useFixedFont);
    win->showMessage(overrideEncoding(), msg, CommonKernel->collectionFromId(msg.parentCollection().id()));
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
void KMMainWidget::slotSelectAllMessages()
{
    mMessagePane->selectAll();
    updateMessageActions();
}

void KMMainWidget::slotMessagePopup(const Akonadi::Item &msg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint)
{
    QUrl aUrl = result.linkUrl();
    QUrl imageUrl = result.imageUrl();
    updateMessageMenu();

    const QString email = KEmailAddress::firstEmailAddress(aUrl.path()).toLower();
    if (aUrl.scheme() == QLatin1String("mailto") && !email.isEmpty()) {
        Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob(this);
        job->setLimit(1);
        job->setQuery(Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch);
        job->setProperty("msg", QVariant::fromValue(msg));
        job->setProperty("point", aPoint);
        job->setProperty("imageUrl", imageUrl);
        job->setProperty("url", aUrl);
        job->setProperty("webhitresult", QVariant::fromValue(result));
        connect(job, &Akonadi::ContactSearchJob::result, this, &KMMainWidget::slotContactSearchJobForMessagePopupDone);
    } else {
        showMessagePopup(msg, aUrl, imageUrl, aPoint, false, false, result);
    }
}

void KMMainWidget::slotContactSearchJobForMessagePopupDone(KJob *job)
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);
    const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

    const Akonadi::Item::List listContact = searchJob->items();
    const bool uniqueContactFound = (listContact.count() == 1);
    if (uniqueContactFound) {
        mMsgView->setContactItem(listContact.first(), searchJob->contacts().at(0));
    } else {
        mMsgView->clearContactItem();
    }
    const Akonadi::Item msg = job->property("msg").value<Akonadi::Item>();
    const QPoint aPoint = job->property("point").toPoint();
    const QUrl imageUrl = job->property("imageUrl").toUrl();
    const QUrl url = job->property("url").toUrl();
    const WebEngineViewer::WebHitTestResult result = job->property("webhitresult").value<WebEngineViewer::WebHitTestResult>();

    showMessagePopup(msg, url, imageUrl, aPoint, contactAlreadyExists, uniqueContactFound, result);
}

void KMMainWidget::showMessagePopup(const Akonadi::Item &msg, const QUrl &url, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound,
                                    const WebEngineViewer::WebHitTestResult &result)
{
    QMenu menu(this);
    bool urlMenuAdded = false;

    if (!url.isEmpty()) {
        if (url.scheme() == QLatin1String("mailto")) {
            // popup on a mailto URL
            menu.addAction(mMsgView->mailToComposeAction());
            menu.addAction(mMsgView->mailToReplyAction());
            menu.addAction(mMsgView->mailToForwardAction());

            menu.addSeparator();

            if (contactAlreadyExists) {
                if (uniqueContactFound) {
                    menu.addAction(mMsgView->editContactAction());
                } else {
                    menu.addAction(mMsgView->openAddrBookAction());
                }
            } else {
                menu.addAction(mMsgView->addAddrBookAction());
                menu.addAction(mMsgView->addToExistingContactAction());
            }
            menu.addSeparator();
            menu.addMenu(mMsgView->viewHtmlOption());
            menu.addSeparator();
            menu.addAction(mMsgView->copyURLAction());
            urlMenuAdded = true;
        } else if (url.scheme() != QLatin1String("attachment")) {
            // popup on a not-mailto URL
            menu.addAction(mMsgView->urlOpenAction());
            menu.addAction(mMsgView->addBookmarksAction());
            menu.addAction(mMsgView->urlSaveAsAction());
            menu.addAction(mMsgView->copyURLAction());
            menu.addSeparator();
            menu.addAction(mMsgView->shareServiceUrlMenu());
            menu.addActions(mMsgView->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedUrl));
            if (!imageUrl.isEmpty()) {
                menu.addSeparator();
                menu.addAction(mMsgView->copyImageLocation());
                menu.addAction(mMsgView->downloadImageToDiskAction());
                menu.addAction(mMsgView->shareImage());
            }
            urlMenuAdded = true;
        }
        qCDebug(KMAIL_LOG) << "URL is:" << url;
    }
    const QString selectedText = mMsgView ? mMsgView->copyText() : QString();
    if (mMsgView && !selectedText.isEmpty()) {
        if (urlMenuAdded) {
            menu.addSeparator();
        }
        menu.addAction(mMsgActions->replyMenu());
        menu.addSeparator();

        menu.addAction(mMsgView->copyAction());
        menu.addAction(mMsgView->selectAllAction());
        menu.addSeparator();
        mMsgActions->addWebShortcutsMenu(&menu, selectedText);
        menu.addSeparator();
        menu.addActions(mMsgView->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedSelection));
        if (KPIMTextEdit::TextToSpeech::self()->isReady()) {
            menu.addSeparator();
            menu.addAction(mMsgView->speakTextAction());
        }
    } else if (!urlMenuAdded) {
        // popup somewhere else (i.e., not a URL) on the message
        if (!mMessagePane->currentMessage()) {
            // no messages
            return;
        }
        Akonadi::Collection parentCol = msg.parentCollection();
        if (parentCol.isValid() && CommonKernel->folderIsTemplates(parentCol)) {
            menu.addAction(mMsgActions->newMessageFromTemplateAction());
        } else {
            menu.addAction(mMsgActions->replyMenu());
            menu.addAction(mMsgActions->forwardMenu());
        }
        if (parentCol.isValid() && CommonKernel->folderIsSentMailFolder(parentCol)) {
            menu.addAction(mMsgActions->sendAgainAction());
        }
        menu.addAction(mailingListActionMenu());
        menu.addSeparator();

        menu.addAction(mCopyActionMenu);
        menu.addAction(mMoveActionMenu);

        menu.addSeparator();

        menu.addAction(mMsgActions->messageStatusMenu());
        menu.addSeparator();
        if (mMsgView) {
            if (!imageUrl.isEmpty()) {
                menu.addSeparator();
                menu.addAction(mMsgView->copyImageLocation());
                menu.addAction(mMsgView->downloadImageToDiskAction());
                menu.addAction(mMsgView->shareImage());
                menu.addSeparator();
            }
        }
        menu.addSeparator();
        menu.addAction(mMsgActions->printPreviewAction());
        menu.addAction(mMsgActions->printAction());
        menu.addSeparator();
        menu.addAction(mSaveAsAction);
        menu.addAction(mSaveAttachmentsAction);
        menu.addSeparator();
        if (parentCol.isValid() && CommonKernel->folderIsTrash(parentCol)) {
            menu.addAction(mDeleteAction);
        } else {
            menu.addAction(akonadiStandardAction(Akonadi::StandardMailActionManager::MoveToTrash));
        }
        menu.addSeparator();

        if (mMsgView) {
            menu.addActions(mMsgView->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedMessage));
            menu.addSeparator();
        }
#if 0
        menu.addAction(mMsgActions->annotateAction());

        menu.addSeparator();
#endif
        menu.addAction(mMsgActions->addFollowupReminderAction());
        if (kmkernel->allowToDebug()) {
            menu.addSeparator();
            menu.addAction(mMsgActions->debugAkonadiSearchAction());
        }
    }
    if (mMsgView) {
        const QList<QAction *> interceptorUrlActions = mMsgView->interceptorUrlActions(result);
        if (!interceptorUrlActions.isEmpty()) {
            menu.addSeparator();
            menu.addActions(interceptorUrlActions);
        }
    }

    KAcceleratorManager::manage(&menu);
    menu.exec(aPoint, nullptr);
}

void KMMainWidget::setupActions()
{
    KMailPluginInterface::self()->setParentWidget(this);
    KMailPluginInterface::self()->createPluginInterface();
    mMsgActions = new KMail::MessageActions(actionCollection(), this);
    mMsgActions->setMessageView(mMsgView);

    //----- File Menu
    mSaveAsAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save &As..."), this);
    actionCollection()->addAction(QStringLiteral("file_save_as"), mSaveAsAction);
    connect(mSaveAsAction, &QAction::triggered, this, &KMMainWidget::slotSaveMsg);
    actionCollection()->setDefaultShortcut(mSaveAsAction, KStandardShortcut::save().first());

    mOpenAction = KStandardAction::open(this, &KMMainWidget::slotOpenMsg,
                                        actionCollection());

    mOpenRecentAction = KStandardAction::openRecent(this, &KMMainWidget::slotOpenRecentMessage,
                                                    actionCollection());
    KConfigGroup grp = mConfig->group(QStringLiteral("Recent Files"));
    mOpenRecentAction->loadEntries(grp);

    {
        QAction *action = new QAction(i18n("&Expire All Folders"), this);
        actionCollection()->addAction(QStringLiteral("expire_all_folders"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpireAll);
    }
    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("mail-receive")), i18n("Check &Mail"), this);
        actionCollection()->addAction(QStringLiteral("check_mail"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCheckMail);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_L));
    }

    mAccountActionMenu = new KActionMenuAccount(this);
    mAccountActionMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-receive")));
    mAccountActionMenu->setText(i18n("Check Mail In"));

    mAccountActionMenu->setIconText(i18n("Check Mail"));
    mAccountActionMenu->setToolTip(i18n("Check Mail"));
    actionCollection()->addAction(QStringLiteral("check_mail_in"), mAccountActionMenu);
    connect(mAccountActionMenu, &KActionMenu::triggered, this, &KMMainWidget::slotCheckMail);

    mSendQueued = new QAction(QIcon::fromTheme(QStringLiteral("mail-send")), i18n("&Send Queued Messages"), this);
    actionCollection()->addAction(QStringLiteral("send_queued"), mSendQueued);
    connect(mSendQueued, &QAction::triggered, this, &KMMainWidget::slotSendQueued);
    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::ToggleWorkOffline);
        mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::ToggleWorkOffline);
        action->setCheckable(false);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotOnlineStatus);
        action->setText(i18n("Online status (unknown)"));
    }

    mSendActionMenu = new KActionMenuTransport(this);
    mSendActionMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-send")));
    mSendActionMenu->setText(i18n("Send Queued Messages Via"));
    actionCollection()->addAction(QStringLiteral("send_queued_via"), mSendActionMenu);

    connect(mSendActionMenu, &KActionMenuTransport::transportSelected, this, &KMMainWidget::slotSendQueuedVia);

    //----- Tools menu
    if (parent()->inherits("KMMainWin")) {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("x-office-address-book")), i18n("&Address Book"), this);
        actionCollection()->addAction(QStringLiteral("addressbook"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotRunAddressBook);
        if (QStandardPaths::findExecutable(QStringLiteral("kaddressbook")).isEmpty()) {
            action->setEnabled(false);
        }
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("pgp-keys")), i18n("Certificate Manager"), this);
        actionCollection()->addAction(QStringLiteral("tools_start_certman"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotStartCertManager);
        // disable action if no certman binary is around
        if (QStandardPaths::findExecutable(QStringLiteral("kleopatra")).isEmpty()) {
            action->setEnabled(false);
        }
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("document-import")), i18n("&Import Messages..."), this);
        actionCollection()->addAction(QStringLiteral("import"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotImport);
        if (QStandardPaths::findExecutable(QStringLiteral("akonadiimportwizard")).isEmpty()) {
            action->setEnabled(false);
        }
    }

    {
        if (kmkernel->allowToDebug()) {
            QAction *action = new QAction(i18n("&Debug Sieve..."), this);
            actionCollection()->addAction(QStringLiteral("tools_debug_sieve"), action);
            connect(action, &QAction::triggered, this, &KMMainWidget::slotDebugSieve);
        }
    }

    {
        QAction *action = new QAction(i18n("Filter &Log Viewer..."), this);
        actionCollection()->addAction(QStringLiteral("filter_log_viewer"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotFilterLogViewer);
    }
    {
        QAction *action = new QAction(i18n("&Import from another Email Client..."), this);
        actionCollection()->addAction(QStringLiteral("importWizard"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotImportWizard);
    }
    if (KSieveUi::Util::allowOutOfOfficeSettings()) {
        QAction *action = new QAction(i18n("Edit \"Out of Office\" Replies..."), this);
        actionCollection()->addAction(QStringLiteral("tools_edit_vacation"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotEditCurrentVacation);
    }

    {
        QAction *action = new QAction(i18n("&Configure Automatic Archiving..."), this);
        actionCollection()->addAction(QStringLiteral("tools_automatic_archiving"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotConfigureAutomaticArchiving);
    }

    {
        QAction *action = new QAction(i18n("Delayed Messages..."), this);
        actionCollection()->addAction(QStringLiteral("message_delayed"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotConfigureSendLater);
    }

    {
        QAction *action = new QAction(i18n("Followup Reminder Messages..."), this);
        actionCollection()->addAction(QStringLiteral("followup_reminder_messages"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotConfigureFollowupReminder);
    }

    // Disable the standard action delete key sortcut.
    QAction *const standardDelAction = akonadiStandardAction(Akonadi::StandardActionManager::DeleteItems);
    standardDelAction->setShortcut(QKeySequence());

    //----- Edit Menu

    mDeleteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action Hard delete, bypassing trash", "&Delete"), this);
    actionCollection()->addAction(QStringLiteral("delete"), mDeleteAction);
    connect(mDeleteAction, &QAction::triggered, this, &KMMainWidget::slotDeleteMessages);
    actionCollection()->setDefaultShortcut(mDeleteAction, QKeySequence(Qt::SHIFT + Qt::Key_Delete));

    mTrashThreadAction = new QAction(i18n("M&ove Thread to Trash"), this);
    actionCollection()->addAction(QStringLiteral("move_thread_to_trash"), mTrashThreadAction);
    actionCollection()->setDefaultShortcut(mTrashThreadAction, QKeySequence(Qt::CTRL + Qt::Key_Delete));
    mTrashThreadAction->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
    KMail::Util::addQActionHelpText(mTrashThreadAction, i18n("Move thread to trashcan"));
    connect(mTrashThreadAction, &QAction::triggered, this, &KMMainWidget::slotTrashThread);

    mDeleteThreadAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete T&hread"), this);
    actionCollection()->addAction(QStringLiteral("delete_thread"), mDeleteThreadAction);
    //Don't use new connect api.
    connect(mDeleteThreadAction, &QAction::triggered, this, &KMMainWidget::slotDeleteThread);
    actionCollection()->setDefaultShortcut(mDeleteThreadAction, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Delete));

    mSearchMessages = new QAction(QIcon::fromTheme(QStringLiteral("edit-find-mail")), i18n("&Find Messages..."), this);
    actionCollection()->addAction(QStringLiteral("search_messages"), mSearchMessages);
    connect(mSearchMessages, &QAction::triggered, this, &KMMainWidget::slotRequestFullSearchFromQuickSearch);
    actionCollection()->setDefaultShortcut(mSearchMessages, QKeySequence(Qt::Key_S));

    mSelectAllMessages = new QAction(i18n("Select &All Messages"), this);
    actionCollection()->addAction(QStringLiteral("mark_all_messages"), mSelectAllMessages);
    connect(mSelectAllMessages, &QAction::triggered, this, &KMMainWidget::slotSelectAllMessages);
    actionCollection()->setDefaultShortcut(mSelectAllMessages, QKeySequence(Qt::CTRL + Qt::Key_A));

    //----- Folder Menu

    mFolderMailingListPropertiesAction = new QAction(i18n("&Mailing List Management..."), this);
    actionCollection()->addAction(QStringLiteral("folder_mailinglist_properties"), mFolderMailingListPropertiesAction);
    connect(mFolderMailingListPropertiesAction, &QAction::triggered, mManageShowCollectionProperties, &ManageShowCollectionProperties::slotFolderMailingListProperties);
    // mFolderMailingListPropertiesAction->setIcon(QIcon::fromTheme("document-properties-mailing-list"));

    mShowFolderShortcutDialogAction = new QAction(QIcon::fromTheme(QStringLiteral("configure-shortcuts")), i18n("&Assign Shortcut..."), this);
    actionCollection()->addAction(QStringLiteral("folder_shortcut_command"), mShowFolderShortcutDialogAction);
    connect(mShowFolderShortcutDialogAction, &QAction::triggered, mManageShowCollectionProperties,
            &ManageShowCollectionProperties::slotShowFolderShortcutDialog);
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
    actionCollection()->addAction(QStringLiteral("expire"), mExpireFolderAction);
    connect(mExpireFolderAction, &QAction::triggered, this, &KMMainWidget::slotExpireFolder);

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::MoveToTrash);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveToTrash), &QAction::triggered, this, &KMMainWidget::slotTrashSelectedMessages);

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardMailActionManager::MoveAllToTrash);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash), &QAction::triggered, this, &KMMainWidget::slotEmptyFolder);

    mAkonadiStandardActionManager->interceptAction(Akonadi::StandardActionManager::DeleteCollections);
    connect(mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections), &QAction::triggered, this, &KMMainWidget::slotRemoveFolder);

    // ### PORT ME: Add this to the context menu. Not possible right now because
    //              the context menu uses XMLGUI, and that would add the entry to
    //              all collection context menus
    mArchiveFolderAction = new QAction(i18n("&Archive Folder..."), this);
    actionCollection()->addAction(QStringLiteral("archive_folder"), mArchiveFolderAction);
    connect(mArchiveFolderAction, &QAction::triggered, this, &KMMainWidget::slotArchiveFolder);

    mDisplayMessageFormatMenu = new DisplayMessageFormatActionMenu(this);
    connect(mDisplayMessageFormatMenu, &DisplayMessageFormatActionMenu::changeDisplayMessageFormat, this, &KMMainWidget::slotChangeDisplayMessageFormat);
    actionCollection()->addAction(QStringLiteral("display_format_message"), mDisplayMessageFormatMenu);

    mPreferHtmlLoadExtAction = new KToggleAction(i18n("Load E&xternal References"), this);
    actionCollection()->addAction(QStringLiteral("prefer_html_external_refs"), mPreferHtmlLoadExtAction);
    connect(mPreferHtmlLoadExtAction, &KToggleAction::triggered, this, &KMMainWidget::slotOverrideHtmlLoadExt);

    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyCollections);
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
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::CTRL + Qt::Key_X));
    }

    {
        QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyItemToMenu);
        action->setText(i18n("Copy Message To..."));
        action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveItemToMenu);
        action->setText(i18n("Move Message To..."));
    }

    //----- Message Menu
    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), i18n("&New Message..."), this);
        actionCollection()->addAction(QStringLiteral("new_message"), action);
        action->setIconText(i18nc("@action:intoolbar New Empty Message", "New"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCompose);
        // do not set a New shortcut if kmail is a component
        if (kmkernel->xmlGuiInstanceName().isEmpty()) {
            actionCollection()->setDefaultShortcut(action, KStandardShortcut::openNew().first());
        }
    }

    mTemplateMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("document-new")), i18n("Message From &Template"),
                                    actionCollection());
    mTemplateMenu->setDelayed(true);
    actionCollection()->addAction(QStringLiteral("new_from_template"), mTemplateMenu);
    connect(mTemplateMenu->menu(), &QMenu::aboutToShow, this,
            &KMMainWidget::slotShowNewFromTemplate);
    connect(mTemplateMenu->menu(), &QMenu::triggered, this,
            &KMMainWidget::slotNewFromTemplate);

    mMessageNewList = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new-list")),
                                  i18n("New Message t&o Mailing-List..."),
                                  this);
    actionCollection()->addAction(QStringLiteral("post_message"), mMessageNewList);
    connect(mMessageNewList, &QAction::triggered,
            this, &KMMainWidget::slotPostToML);
    actionCollection()->setDefaultShortcut(mMessageNewList, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));

    //----- Create filter actions
    mFilterMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("view-filter")), i18n("&Create Filter"), this);
    actionCollection()->addAction(QStringLiteral("create_filter"), mFilterMenu);
    connect(mFilterMenu, &QAction::triggered, this,
            &KMMainWidget::slotFilter);
    {
        QAction *action = new QAction(i18n("Filter on &Subject..."), this);
        actionCollection()->addAction(QStringLiteral("subject_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSubjectFilter);
        mFilterMenu->addAction(action);
    }

    {
        QAction *action = new QAction(i18n("Filter on &From..."), this);
        actionCollection()->addAction(QStringLiteral("from_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotFromFilter);
        mFilterMenu->addAction(action);
    }
    {
        QAction *action = new QAction(i18n("Filter on &To..."), this);
        actionCollection()->addAction(QStringLiteral("to_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotToFilter);
        mFilterMenu->addAction(action);
    }
    {
        QAction *action = new QAction(i18n("Filter on &Cc..."), this);
        actionCollection()->addAction(QStringLiteral("cc_filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCcFilter);
        mFilterMenu->addAction(action);
    }
    mFilterMenu->addAction(mMsgActions->listFilterAction());

    //----- "Mark Thread" submenu
    mThreadStatusMenu = new KActionMenu(i18n("Mark &Thread"), this);
    actionCollection()->addAction(QStringLiteral("thread_status"), mThreadStatusMenu);

    mMarkThreadAsReadAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-mark-read")), i18n("Mark Thread as &Read"), this);
    actionCollection()->addAction(QStringLiteral("thread_read"), mMarkThreadAsReadAction);
    connect(mMarkThreadAsReadAction, &QAction::triggered, this, &KMMainWidget::slotSetThreadStatusRead);
    KMail::Util::addQActionHelpText(mMarkThreadAsReadAction, i18n("Mark all messages in the selected thread as read"));
    mThreadStatusMenu->addAction(mMarkThreadAsReadAction);

    mMarkThreadAsUnreadAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-mark-unread")), i18n("Mark Thread as &Unread"), this);
    actionCollection()->addAction(QStringLiteral("thread_unread"), mMarkThreadAsUnreadAction);
    connect(mMarkThreadAsUnreadAction, &QAction::triggered, this, &KMMainWidget::slotSetThreadStatusUnread);
    KMail::Util::addQActionHelpText(mMarkThreadAsUnreadAction, i18n("Mark all messages in the selected thread as unread"));
    mThreadStatusMenu->addAction(mMarkThreadAsUnreadAction);

    mThreadStatusMenu->addSeparator();

    //----- "Mark Thread" toggle actions
    mToggleThreadImportantAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("mail-mark-important")), i18n("Mark Thread as &Important"), this);
    actionCollection()->addAction(QStringLiteral("thread_flag"), mToggleThreadImportantAction);
    connect(mToggleThreadImportantAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusImportant);
    mToggleThreadImportantAction->setCheckedState(KGuiItem(i18n("Remove &Important Thread Mark")));
    mThreadStatusMenu->addAction(mToggleThreadImportantAction);

    mToggleThreadToActAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("mail-mark-task")), i18n("Mark Thread as &Action Item"), this);
    actionCollection()->addAction(QStringLiteral("thread_toact"), mToggleThreadToActAction);
    connect(mToggleThreadToActAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusToAct);
    mToggleThreadToActAction->setCheckedState(KGuiItem(i18n("Remove &Action Item Thread Mark")));
    mThreadStatusMenu->addAction(mToggleThreadToActAction);

    //------- "Watch and ignore thread" actions
    mWatchThreadAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("mail-thread-watch")), i18n("&Watch Thread"), this);
    actionCollection()->addAction(QStringLiteral("thread_watched"), mWatchThreadAction);
    connect(mWatchThreadAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusWatched);

    mIgnoreThreadAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("mail-thread-ignored")), i18n("&Ignore Thread"), this);
    actionCollection()->addAction(QStringLiteral("thread_ignored"), mIgnoreThreadAction);
    connect(mIgnoreThreadAction, &KToggleAction::triggered, this, &KMMainWidget::slotSetThreadStatusIgnored);

    mThreadStatusMenu->addSeparator();
    mThreadStatusMenu->addAction(mWatchThreadAction);
    mThreadStatusMenu->addAction(mIgnoreThreadAction);

    mSaveAttachmentsAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), i18n("Save A&ttachments..."), this);
    actionCollection()->addAction(QStringLiteral("file_save_attachments"), mSaveAttachmentsAction);
    connect(mSaveAttachmentsAction, &QAction::triggered, this, &KMMainWidget::slotSaveAttachments);

    mMoveActionMenu = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveItemToMenu);

    mCopyActionMenu = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyItemToMenu);

    mCopyDecryptedActionMenu = new KActionMenu(i18n("Copy Decrypted To..."), this);
    actionCollection()->addAction(QStringLiteral("copy_decrypted_to_menu"), mCopyDecryptedActionMenu);
    connect(mCopyDecryptedActionMenu->menu(), &QMenu::triggered,
            this, &KMMainWidget::slotCopyDecryptedTo);

    mApplyAllFiltersAction
        = new QAction(QIcon::fromTheme(QStringLiteral("view-filter")), i18n("Appl&y All Filters"), this);
    actionCollection()->addAction(QStringLiteral("apply_filters"), mApplyAllFiltersAction);
    connect(mApplyAllFiltersAction, &QAction::triggered,
            this, &KMMainWidget::slotApplyFilters);
    actionCollection()->setDefaultShortcut(mApplyAllFiltersAction, QKeySequence(Qt::CTRL + Qt::Key_J));

    mApplyFilterActionsMenu = new KActionMenu(i18n("A&pply Filter"), this);
    actionCollection()->addAction(QStringLiteral("apply_filter_actions"), mApplyFilterActionsMenu);

    {
        QAction *action = new QAction(i18nc("View->", "&Expand Thread / Group"), this);
        actionCollection()->addAction(QStringLiteral("expand_thread"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Period));
        KMail::Util::addQActionHelpText(action, i18n("Expand the current thread or group"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpandThread);
    }
    {
        QAction *action = new QAction(i18nc("View->", "&Collapse Thread / Group"), this);
        actionCollection()->addAction(QStringLiteral("collapse_thread"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Comma));
        KMail::Util::addQActionHelpText(action, i18n("Collapse the current thread or group"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCollapseThread);
    }
    {
        QAction *action = new QAction(i18nc("View->", "Ex&pand All Threads"), this);
        actionCollection()->addAction(QStringLiteral("expand_all_threads"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Period));
        KMail::Util::addQActionHelpText(action, i18n("Expand all threads in the current folder"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotExpandAllThreads);
    }
    {
        QAction *action = new QAction(i18nc("View->", "C&ollapse All Threads"), this);
        actionCollection()->addAction(QStringLiteral("collapse_all_threads"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Comma));
        KMail::Util::addQActionHelpText(action, i18n("Collapse all threads in the current folder"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCollapseAllThreads);
    }

    QAction *dukeOfMonmoth = new QAction(i18n("&Display Message"), this);
    actionCollection()->addAction(QStringLiteral("display_message"), dukeOfMonmoth);
    connect(dukeOfMonmoth, &QAction::triggered, this, &KMMainWidget::slotDisplayCurrentMessage);
    actionCollection()->setDefaultShortcuts(dukeOfMonmoth, QList<QKeySequence> {
        QKeySequence(Qt::Key_Enter),
        QKeySequence(Qt::Key_Return)
    });

    //----- Go Menu
    {
        QAction *action = new QAction(i18n("&Next Message"), this);
        actionCollection()->addAction(QStringLiteral("go_next_message"), action);
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::Key_N),
            QKeySequence(Qt::Key_Right)
        });
        KMail::Util::addQActionHelpText(action, i18n("Go to the next message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectNextMessage);
    }
    {
        QAction *action = new QAction(i18n("Next &Unread Message"), this);
        actionCollection()->addAction(QStringLiteral("go_next_unread_message"), action);
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::Key_Plus),
            QKeySequence(Qt::Key_Plus + Qt::KeypadModifier)
        });
        if (QApplication::isRightToLeft()) {
            action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
        } else {
            action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
        }
        action->setIconText(i18nc("@action:inmenu Goto next unread message", "Next"));
        KMail::Util::addQActionHelpText(action, i18n("Go to the next unread message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectNextUnreadMessage);
    }
    {
        QAction *action = new QAction(i18n("&Previous Message"), this);
        actionCollection()->addAction(QStringLiteral("go_prev_message"), action);
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous message"));
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::Key_P),
            QKeySequence(Qt::Key_Left)
        });
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectPreviousMessage);
    }
    {
        QAction *action = new QAction(i18n("Previous Unread &Message"), this);
        actionCollection()->addAction(QStringLiteral("go_prev_unread_message"), action);
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::Key_Minus),
            QKeySequence(Qt::Key_Minus + Qt::KeypadModifier)
        });
        if (QApplication::isRightToLeft()) {
            action->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
        } else {
            action->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
        }
        action->setIconText(i18nc("@action:inmenu Goto previous unread message.", "Previous"));
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous unread message"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotSelectPreviousUnreadMessage);
    }
    {
        QAction *action = new QAction(i18n("Next Unread &Folder"), this);
        actionCollection()->addAction(QStringLiteral("go_next_unread_folder"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotNextUnreadFolder);
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::ALT + Qt::Key_Plus),
            QKeySequence(Qt::ALT + Qt::Key_Plus + Qt::KeypadModifier)
        });
        KMail::Util::addQActionHelpText(action, i18n("Go to the next folder with unread messages"));
    }
    {
        QAction *action = new QAction(i18n("Previous Unread F&older"), this);
        actionCollection()->addAction(QStringLiteral("go_prev_unread_folder"), action);
        actionCollection()->setDefaultShortcuts(action, QList<QKeySequence> {
            QKeySequence(Qt::ALT + Qt::Key_Minus),
            QKeySequence(Qt::ALT + Qt::Key_Minus + Qt::KeypadModifier)
        });
        KMail::Util::addQActionHelpText(action, i18n("Go to the previous folder with unread messages"));
        connect(action, &QAction::triggered, this, &KMMainWidget::slotPrevUnreadFolder);
    }
    {
        QAction *action = new QAction(i18nc("Go->", "Next Unread &Text"), this);
        actionCollection()->addAction(QStringLiteral("go_next_unread_text"), action);
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
        actionCollection()->addAction(QStringLiteral("filter"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotFilter);
    }
    {
        QAction *action = new QAction(i18n("Manage &Sieve Scripts..."), this);
        actionCollection()->addAction(QStringLiteral("sieveFilters"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotManageSieveScripts);
    }
    {
        QAction *action = new QAction(i18n("&Add Account..."), this);
        actionCollection()->addAction(QStringLiteral("accountWizard"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotAccountWizard);
    }
    {
        mShowIntroductionAction = new QAction(QIcon::fromTheme(QStringLiteral("kmail")), i18n("KMail &Introduction"), this);
        actionCollection()->addAction(QStringLiteral("help_kmail_welcomepage"), mShowIntroductionAction);
        KMail::Util::addQActionHelpText(mShowIntroductionAction, i18n("Display KMail's Welcome Page"));
        connect(mShowIntroductionAction, &QAction::triggered, this, &KMMainWidget::slotIntro);
        mShowIntroductionAction->setEnabled(mMsgView != nullptr);
    }

    // ----- Standard Actions
    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("preferences-desktop-notification")),
                                      i18n("Configure &Notifications..."), this);
        action->setMenuRole(QAction::NoRole);   // do not move to application menu on OS X
        actionCollection()->addAction(QStringLiteral("kmail_configure_notifications"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotEditNotifications);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Configure KMail..."), this);
        action->setMenuRole(QAction::PreferencesRole);   // this one should move to the application menu on OS X
        actionCollection()->addAction(QStringLiteral("kmail_configure_kmail"), action);
        connect(action, &QAction::triggered, kmkernel, &KMKernel::slotShowConfigurationDialog);
    }

    {
        mExpireConfigAction = new QAction(i18n("Expire..."), this);
        actionCollection()->addAction(QStringLiteral("expire_settings"), mExpireConfigAction);
        connect(mExpireConfigAction, &QAction::triggered, mManageShowCollectionProperties, &ManageShowCollectionProperties::slotShowExpiryProperties);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add Favorite Folder..."), this);
        actionCollection()->addAction(QStringLiteral("add_favorite_folder"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotAddFavoriteFolder);
    }

    {
        mServerSideSubscription = new QAction(QIcon::fromTheme(QStringLiteral("folder-bookmarks")), i18n("Serverside Subscription..."), this);
        actionCollection()->addAction(QStringLiteral("serverside_subscription"), mServerSideSubscription);
        connect(mServerSideSubscription, &QAction::triggered, this, &KMMainWidget::slotServerSideSubscription);
    }

    {
        mApplyAllFiltersFolderAction = new QAction(QIcon::fromTheme(QStringLiteral("view-filter")), i18n("Apply All Filters"), this);
        actionCollection()->addAction(QStringLiteral("apply_filters_folder"), mApplyAllFiltersFolderAction);
        connect(mApplyAllFiltersFolderAction, &QAction::triggered,
                this, [this] {
            slotApplyFiltersOnFolder(/* recursive */ false);
        });
    }

    {
        mApplyAllFiltersFolderRecursiveAction = new QAction(QIcon::fromTheme(QStringLiteral("view-filter")), i18n("Apply All Filters"), this);
        actionCollection()->addAction(QStringLiteral("apply_filters_folder_recursive"), mApplyAllFiltersFolderRecursiveAction);
        connect(mApplyAllFiltersFolderRecursiveAction, &QAction::triggered,
                this, [this] {
            slotApplyFiltersOnFolder(/* recursive */ true);
        });
    }

    {
        mApplyFilterFolderActionsMenu = new KActionMenu(i18n("Apply Filters on Folder"), this);
        actionCollection()->addAction(QStringLiteral("apply_filters_on_folder_actions"), mApplyFilterFolderActionsMenu);
    }

    {
        mApplyFilterFolderRecursiveActionsMenu = new KActionMenu(i18n("Apply Filters on Folder and all its Subfolders"), this);
        actionCollection()->addAction(QStringLiteral("apply_filters_on_folder_recursive_actions"), mApplyFilterFolderRecursiveActionsMenu);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("kontact")), i18n("Import/Export KMail Data..."), this);
        actionCollection()->addAction(QStringLiteral("kmail_export_data"), action);
        connect(action, &QAction::triggered, mLaunchExternalComponent, &KMLaunchExternalComponent::slotExportData);
    }

    {
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")), i18n("New AddressBook Contact..."), this);
        actionCollection()->addAction(QStringLiteral("kmail_new_addressbook_contact"), action);
        connect(action, &QAction::triggered, this, &KMMainWidget::slotCreateAddressBookContact);
    }

    QAction *act = actionCollection()->addAction(KStandardAction::Undo, QStringLiteral("kmail_undo"));
    connect(act, &QAction::triggered, this, &KMMainWidget::slotUndo);

    menutimer = new QTimer(this);
    menutimer->setObjectName(QStringLiteral("menutimer"));
    menutimer->setSingleShot(true);
    connect(menutimer, &QTimer::timeout, this, &KMMainWidget::updateMessageActionsDelayed);
    connect(kmkernel->undoStack(),
            &KMail::UndoStack::undoStackChanged, this, &KMMainWidget::slotUpdateUndo);

    updateMessageActions();
    updateFolderMenu();
    mTagActionManager = new KMail::TagActionManager(this, actionCollection(), mMsgActions,
                                                    mGUIClient);
    mFolderShortcutActionManager = new KMail::FolderShortcutActionManager(this, actionCollection());

    {
        QAction *action = new QAction(i18n("Copy Message to Folder"), this);
        actionCollection()->addAction(QStringLiteral("copy_message_to_folder"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotCopySelectedMessagesToFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_C));
    }
    {
        QAction *action = new QAction(i18n("Jump to Folder..."), this);
        actionCollection()->addAction(QStringLiteral("jump_to_folder"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotJumpToFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_J));
    }
    {
        QAction *action = new QAction(i18n("Abort Current Operation"), this);
        actionCollection()->addAction(QStringLiteral("cancel"), action);
        connect(action, &QAction::triggered,
                ProgressManager::instance(), &KPIM::ProgressManager::slotAbortAll);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::Key_Escape));
    }
    {
        QAction *action = new QAction(i18n("Focus on Next Folder"), this);
        actionCollection()->addAction(QStringLiteral("inc_current_folder"), action);
        connect(action, &QAction::triggered,
                mFolderTreeWidget->folderTreeView(), &FolderTreeView::slotFocusNextFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Right));
    }
    {
        QAction *action = new QAction(i18n("Focus on Previous Folder"), this);
        actionCollection()->addAction(QStringLiteral("dec_current_folder"), action);
        connect(action, &QAction::triggered,
                mFolderTreeWidget->folderTreeView(), &FolderTreeView::slotFocusPrevFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Left));
    }
    {
        QAction *action = new QAction(i18n("Select Folder with Focus"), this);
        actionCollection()->addAction(QStringLiteral("select_current_folder"), action);

        connect(action, &QAction::triggered,
                mFolderTreeWidget->folderTreeView(), &FolderTreeView::slotSelectFocusFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Space));
    }
    {
        QAction *action = new QAction(i18n("Focus on First Folder"), this);
        actionCollection()->addAction(QStringLiteral("focus_first_folder"), action);
        connect(action, &QAction::triggered,
                mFolderTreeWidget->folderTreeView(), &FolderTreeView::slotFocusFirstFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Home));
    }
    {
        QAction *action = new QAction(i18n("Focus on Last Folder"), this);
        actionCollection()->addAction(QStringLiteral("focus_last_folder"), action);
        connect(action, &QAction::triggered,
                mFolderTreeWidget->folderTreeView(), &FolderTreeView::slotFocusLastFolder);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_End));
    }
    {
        QAction *action = new QAction(i18n("Focus on Next Message"), this);
        actionCollection()->addAction(QStringLiteral("inc_current_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotFocusOnNextMessage);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Right));
    }
    {
        QAction *action = new QAction(i18n("Focus on Previous Message"), this);
        actionCollection()->addAction(QStringLiteral("dec_current_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotFocusOnPrevMessage);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Left));
    }
    {
        QAction *action = new QAction(i18n("Select First Message"), this);
        actionCollection()->addAction(QStringLiteral("select_first_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotSelectFirstMessage);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Home));
    }
    {
        QAction *action = new QAction(i18n("Select Last Message"), this);
        actionCollection()->addAction(QStringLiteral("select_last_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotSelectLastMessage);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_End));
    }
    {
        QAction *action = new QAction(i18n("Select Message with Focus"), this);
        actionCollection()->addAction(QStringLiteral("select_current_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotSelectFocusedMessage);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Space));
    }

    {
        mQuickSearchAction = new QAction(i18n("Set Focus to Quick Search"), this);
        //If change shortcut change Panel::setQuickSearchClickMessage(...) message
        actionCollection()->setDefaultShortcut(mQuickSearchAction, QKeySequence(Qt::ALT + Qt::Key_Q));
        actionCollection()->addAction(QStringLiteral("focus_to_quickseach"), mQuickSearchAction);
        connect(mQuickSearchAction, &QAction::triggered,
                this, &KMMainWidget::slotFocusQuickSearch);
        updateQuickSearchLineText();
    }
    {
        QAction *action = new QAction(i18n("Extend Selection to Previous Message"), this);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Left));
        actionCollection()->addAction(QStringLiteral("previous_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotExtendSelectionToPreviousMessage);
    }
    {
        QAction *action = new QAction(i18n("Extend Selection to Next Message"), this);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::SHIFT + Qt::Key_Right));
        actionCollection()->addAction(QStringLiteral("next_message"), action);
        connect(action, &QAction::triggered,
                this, &KMMainWidget::slotExtendSelectionToNextMessage);
    }

    {
        mMoveMsgToFolderAction = new QAction(i18n("Move Message to Folder"), this);
        actionCollection()->setDefaultShortcut(mMoveMsgToFolderAction, QKeySequence(Qt::Key_M));
        actionCollection()->addAction(QStringLiteral("move_message_to_folder"), mMoveMsgToFolderAction);
        connect(mMoveMsgToFolderAction, &QAction::triggered,
                this, &KMMainWidget::slotMoveSelectedMessageToFolder);
    }

    mArchiveAction = new QAction(i18nc("@action", "Archive"), this);
    actionCollection()->addAction(QStringLiteral("archive_mails"), mArchiveAction);
    connect(mArchiveAction, &QAction::triggered, this, &KMMainWidget::slotArchiveMails);

    mUseLessBandwidth = new KToggleAction(i18n("Use Less Bandwidth"), this);
    actionCollection()->addAction(QStringLiteral("low_bandwidth"), mUseLessBandwidth);
    connect(mUseLessBandwidth, &KToggleAction::triggered, this, &KMMainWidget::slotBandwidth);

    mMarkAllMessageAsReadAndInAllSubFolder = new QAction(i18n("Mark All Messages As Read in This Folder and All its Subfolder"), this);
    mMarkAllMessageAsReadAndInAllSubFolder->setIcon(QIcon::fromTheme(QStringLiteral("mail-mark-read")));
    actionCollection()->addAction(QStringLiteral("markallmessagereadcurentfolderandsubfolder"), mMarkAllMessageAsReadAndInAllSubFolder);
    connect(mMarkAllMessageAsReadAndInAllSubFolder, &KToggleAction::triggered, this, &KMMainWidget::slotMarkAllMessageAsReadInCurrentFolderAndSubfolder);

    mRemoveDuplicateRecursiveAction = new QAction(i18n("Remove Duplicates in This Folder and All its Subfolder"), this);
    actionCollection()->addAction(QStringLiteral("remove_duplicate_recursive"), mRemoveDuplicateRecursiveAction);
    connect(mRemoveDuplicateRecursiveAction, &KToggleAction::triggered, this, &KMMainWidget::slotRemoveDuplicateRecursive);
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
    QPointer<KMail::KMKnotify> notifyDlg = new KMail::KMKnotify(this);
    notifyDlg->exec();
    delete notifyDlg;
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReadOn()
{
    if (!mMsgView) {
        return;
    }
    mMsgView->viewer()->atBottom();
}

void KMMainWidget::slotPageIsScrolledToBottom(bool isAtBottom)
{
    if (isAtBottom) {
        slotSelectNextUnreadMessage();
    } else {
        mMsgView->viewer()->slotJumpDown();
    }
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
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    mMessagePane->setAllThreadsExpanded(true);
}

void KMMainWidget::slotCollapseAllThreads()
{
    // TODO: Make this asynchronous ? (if there is enough demand)
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
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
    if (mCurrentFolderSettings && mCurrentFolderSettings->isValid()
        && mMessagePane->getSelectionStats(selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread)
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
    bool currentFolderSettingsIsValid = mCurrentFolderSettings && mCurrentFolderSettings->isValid();
    if (currentFolderSettingsIsValid
        && mMessagePane->getSelectionStats(selectedItems, selectedVisibleItems, &allSelectedBelongToSameThread)
        ) {
        count = selectedItems.count();

        currentMessage = mMessagePane->currentItem();
    } else {
        count = 0;
        currentMessage = Akonadi::Item();
    }

    mApplyFilterActionsMenu->setEnabled(currentFolderSettingsIsValid);
    mApplyFilterFolderRecursiveActionsMenu->setEnabled(currentFolderSettingsIsValid);

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

    const bool readOnly = currentFolderSettingsIsValid && (mCurrentFolderSettings->rights() & Akonadi::Collection::ReadOnly);
    // can we apply strictly single message actions ? (this is false if the whole selection contains more than one message)
    const bool single_actions = count == 1;
    // can we apply loosely single message actions ? (this is false if the VISIBLE selection contains more than one message)
    const bool singleVisibleMessageSelected = selectedVisibleItems.count() == 1;
    // can we apply "mass" actions to the selection ? (this is actually always true if the selection is non-empty)
    const bool mass_actions = count >= 1;
    // does the selection identify a single thread ?
    const bool thread_actions = mass_actions && allSelectedBelongToSameThread && mMessagePane->isThreaded();
    // can we apply flags to the selected messages ?
    const bool flags_available = KMailSettings::self()->allowLocalFlags() || !(currentFolderSettingsIsValid ? readOnly : true);

    mThreadStatusMenu->setEnabled(thread_actions);
    // these need to be handled individually, the user might have them
    // in the toolbar
    mWatchThreadAction->setEnabled(thread_actions && flags_available);
    mIgnoreThreadAction->setEnabled(thread_actions && flags_available);
    mMarkThreadAsReadAction->setEnabled(thread_actions);
    mMarkThreadAsUnreadAction->setEnabled(thread_actions);
    mToggleThreadToActAction->setEnabled(thread_actions && flags_available);
    mToggleThreadImportantAction->setEnabled(thread_actions && flags_available);
    bool canDeleteMessages = currentFolderSettingsIsValid && (mCurrentFolderSettings->rights() & Akonadi::Collection::CanDeleteItem);

    mTrashThreadAction->setEnabled(thread_actions && canDeleteMessages);
    mDeleteThreadAction->setEnabled(thread_actions && canDeleteMessages);
    if (messageView() && messageView()->viewer() && messageView()->viewer()->headerStylePlugin()) {
        messageView()->viewer()->headerStylePlugin()->headerStyle()->setReadOnlyMessage(!canDeleteMessages);
    }

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
        mMsgView->findInMessageAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentCollection));
    }
    mMsgActions->forwardInlineAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentCollection));
    mMsgActions->forwardAttachedAction()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentCollection));
    mMsgActions->forwardMenu()->setEnabled(mass_actions && !CommonKernel->folderIsTemplates(mCurrentCollection));

    mMsgActions->newMessageFromTemplateAction()->setEnabled(single_actions && CommonKernel->folderIsTemplates(mCurrentCollection));
    filterMenu()->setEnabled(single_actions);
    mMsgActions->redirectAction()->setEnabled(/*single_actions &&*/ mass_actions && !CommonKernel->folderIsTemplates(mCurrentCollection));

    if (auto *menuCustom = mMsgActions->customTemplatesMenu()) {
        menuCustom->forwardActionMenu()->setEnabled(mass_actions);
        menuCustom->replyActionMenu()->setEnabled(single_actions);
        menuCustom->replyAllActionMenu()->setEnabled(single_actions);
    }

    // "Print" will act on the current message: it will ignore any hidden selection
    mMsgActions->printAction()->setEnabled(singleVisibleMessageSelected);
    // "Print preview" will act on the current message: it will ignore any hidden selection
    if (QAction *printPreviewAction = mMsgActions->printPreviewAction()) {
        printPreviewAction->setEnabled(singleVisibleMessageSelected);
    }

    // "View Source" will act on the current message: it will ignore any hidden selection
    if (mMsgView) {
        mMsgView->viewSourceAction()->setEnabled(singleVisibleMessageSelected);
        mMsgView->selectAllAction()->setEnabled(count);
    }
    MessageStatus status;
    status.setStatusFromFlags(currentMessage.flags());

    QList< QAction *> actionList;
    bool statusSendAgain = single_actions && ((currentMessage.isValid() && status.isSent()) || (currentMessage.isValid() && CommonKernel->folderIsSentMailFolder(mCurrentCollection)));
    if (statusSendAgain) {
        actionList << mMsgActions->sendAgainAction();
    }
    if (single_actions) {
        actionList << mSaveAttachmentsAction;
    }
    if (mCurrentCollection.isValid() && FolderArchive::FolderArchiveUtil::resourceSupportArchiving(mCurrentCollection.resource())) {
        actionList << mArchiveAction;
    }
    mGUIClient->unplugActionList(QStringLiteral("messagelist_actionlist"));
    mGUIClient->plugActionList(QStringLiteral("messagelist_actionlist"), actionList);
    mMsgActions->sendAgainAction()->setEnabled(statusSendAgain);

    mSaveAsAction->setEnabled(single_actions);

    if (currentFolderSettingsIsValid) {
        updateMoveAction(mCurrentFolderSettings->statistics());
    } else {
        updateMoveAction(false);
    }

    const auto col = CommonKernel->collectionFromId(CommonKernel->outboxCollectionFolder().id());
    const bool nbMsgOutboxCollectionIsNotNull = (col.statistics().count() > 0);

    mSendQueued->setEnabled(nbMsgOutboxCollectionIsNotNull);
    mSendActionMenu->setEnabled(nbMsgOutboxCollectionIsNotNull);

    const bool newPostToMailingList = mCurrentFolderSettings && mCurrentFolderSettings->isMailingListEnabled();
    mMessageNewList->setEnabled(newPostToMailingList);

    slotUpdateOnlineStatus(static_cast<GlobalSettingsBase::EnumNetworkState::type>(KMailSettings::self()->networkState()));
    if (QAction *act = action(QStringLiteral("kmail_undo"))) {
        act->setEnabled(kmkernel->undoStack() && !kmkernel->undoStack()->isEmpty());
    }

    // Enable / disable all filters.
    for (QAction *filterAction : qAsConst(mFilterMenuActions)) {
        filterAction->setEnabled(count > 0);
    }

    mApplyAllFiltersAction->setEnabled(count);
    mApplyFilterActionsMenu->setEnabled(count);
    mSelectAllMessages->setEnabled(count);

    if (currentMessage.hasFlag(Akonadi::MessageFlags::Encrypted) || count > 1) {
        mCopyDecryptedActionMenu->setVisible(true);
        mCopyDecryptedActionMenu->menu()->clear();
        mAkonadiStandardActionManager->standardActionManager()->createActionFolderMenu(
            mCopyDecryptedActionMenu->menu(),
            Akonadi::StandardActionManager::CopyItemToMenu);
    } else {
        mCopyDecryptedActionMenu->setVisible(false);
    }
}

void KMMainWidget::slotAkonadiStandardActionUpdated()
{
    if (mCollectionProperties) {
        if (mCurrentCollection.isValid()) {
            const Akonadi::AgentInstance instance
                = Akonadi::AgentManager::self()->instance(mCurrentCollection.resource());

            mCollectionProperties->setEnabled(!mCurrentFolderSettings->isStructural()
                                              && (instance.status() != Akonadi::AgentInstance::Broken));
        } else {
            mCollectionProperties->setEnabled(false);
        }
        QList< QAction * > collectionProperties;
        if (mCollectionProperties->isEnabled()) {
            collectionProperties << mCollectionProperties;
        }
        mGUIClient->unplugActionList(QStringLiteral("akonadi_collection_collectionproperties_actionlist"));
        mGUIClient->plugActionList(QStringLiteral("akonadi_collection_collectionproperties_actionlist"), collectionProperties);
    }

    const bool folderWithContent = mCurrentFolderSettings && !mCurrentFolderSettings->isStructural();

    if (QAction *act = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections)) {
        act->setEnabled(mCurrentFolderSettings
                        && (mCurrentCollection.rights() & Collection::CanDeleteCollection)
                        && !mCurrentFolderSettings->isSystemFolder()
                        && folderWithContent);
    }

    if (QAction *act = mAkonadiStandardActionManager->action(Akonadi::StandardMailActionManager::MoveAllToTrash)) {
        act->setEnabled(folderWithContent
                        && (mCurrentFolderSettings->count() > 0)
                        && mCurrentFolderSettings->canDeleteMessages());
        act->setText((mCurrentFolderSettings
                      && CommonKernel->folderIsTrash(mCurrentCollection)) ? i18n("E&mpty Trash") : i18n(
                         "&Move All Messages to Trash"));
    }

    QList< QAction * > addToFavorite;
    QAction *actionAddToFavoriteCollections = akonadiStandardAction(Akonadi::StandardActionManager::AddToFavoriteCollections);
    if (actionAddToFavoriteCollections) {
        if (mEnableFavoriteFolderView && actionAddToFavoriteCollections->isEnabled()) {
            addToFavorite << actionAddToFavoriteCollections;
        }
        mGUIClient->unplugActionList(QStringLiteral("akonadi_collection_add_to_favorites_actionlist"));
        mGUIClient->plugActionList(QStringLiteral("akonadi_collection_add_to_favorites_actionlist"), addToFavorite);
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
    mGUIClient->unplugActionList(QStringLiteral("akonadi_collection_sync_actionlist"));
    mGUIClient->plugActionList(QStringLiteral("akonadi_collection_sync_actionlist"), syncActionList);

    QList< QAction * > actionList;

    QAction *action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CreateCollection);
    if (action && action->isEnabled()) {
        actionList << action;
    }

    action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::MoveCollectionToMenu);
    if (action && action->isEnabled()) {
        actionList << action;
    }

    action = mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::CopyCollectionToMenu);
    if (action && action->isEnabled()) {
        actionList << action;
    }
    mGUIClient->unplugActionList(QStringLiteral("akonadi_collection_move_copy_menu_actionlist"));
    mGUIClient->plugActionList(QStringLiteral("akonadi_collection_move_copy_menu_actionlist"), actionList);
}

void KMMainWidget::updateHtmlMenuEntry()
{
    if (mDisplayMessageFormatMenu && mPreferHtmlLoadExtAction) {
        // the visual ones only make sense if we are showing a message list
        const bool enabledAction = (mFolderTreeWidget
                                    && mFolderTreeWidget->folderTreeView()->currentFolder().isValid());

        mDisplayMessageFormatMenu->setEnabled(enabledAction);
        const bool isEnabled = (mFolderTreeWidget
                                && mFolderTreeWidget->folderTreeView()->currentFolder().isValid());
        const bool useHtml = (mFolderDisplayFormatPreference == MessageViewer::Viewer::Html || (mHtmlGlobalSetting && mFolderDisplayFormatPreference == MessageViewer::Viewer::UseGlobalSetting));
        mPreferHtmlLoadExtAction->setEnabled(isEnabled && useHtml);

        mDisplayMessageFormatMenu->setDisplayMessageFormat(mFolderDisplayFormatPreference);

        mPreferHtmlLoadExtAction->setChecked((mHtmlLoadExtGlobalSetting ? !mFolderHtmlLoadExtPreference : mFolderHtmlLoadExtPreference));
    }
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFolderMenu()
{
    if (!CommonKernel->outboxCollectionFolder().isValid()) {
        QTimer::singleShot(1000, this, &KMMainWidget::updateFolderMenu);
        return;
    }

    const bool folderWithContent = mCurrentFolderSettings
                                   && !mCurrentFolderSettings->isStructural()
                                   && mCurrentFolderSettings->isValid();
    mFolderMailingListPropertiesAction->setEnabled(folderWithContent
                                                   && !mCurrentFolderSettings->isSystemFolder());

    QList< QAction * > actionlist;
    if (mCurrentCollection.id() == CommonKernel->outboxCollectionFolder().id() && (mCurrentCollection).statistics().count() > 0) {
        qCDebug(KMAIL_LOG) << "Enabling send queued";
        mSendQueued->setEnabled(true);
        actionlist << mSendQueued;
    }
    //   if ( mCurrentCollection.id() != CommonKernel->trashCollectionFolder().id() ) {
    //     actionlist << mTrashAction;
    //   }
    mGUIClient->unplugActionList(QStringLiteral("outbox_folder_actionlist"));
    mGUIClient->plugActionList(QStringLiteral("outbox_folder_actionlist"), actionlist);
    actionlist.clear();

    const bool isASearchFolder = mCurrentCollection.resource() == QLatin1String("akonadi_search_resource");
    if (isASearchFolder) {
        mAkonadiStandardActionManager->action(Akonadi::StandardActionManager::DeleteCollections)->setText(i18n("&Delete Search"));
    }

    mArchiveFolderAction->setEnabled(mCurrentFolderSettings && folderWithContent);

    const bool isInTrashFolder = (mCurrentFolderSettings && CommonKernel->folderIsTrash(mCurrentCollection));
    QAction *moveToTrash = akonadiStandardAction(Akonadi::StandardMailActionManager::MoveToTrash);
    KMail::Util::setActionTrashOrDelete(moveToTrash, isInTrashFolder);

    mTrashThreadAction->setIcon(isInTrashFolder ? QIcon::fromTheme(QStringLiteral("edit-delete")) : QIcon::fromTheme(QStringLiteral("user-trash")));
    mTrashThreadAction->setText(isInTrashFolder ? i18n("Delete T&hread") : i18n("M&ove Thread to Trash"));

    mSearchMessages->setText((mCurrentCollection.resource() == QLatin1String("akonadi_search_resource")) ? i18n("Edit Search...") : i18n("&Find Messages..."));

    mExpireConfigAction->setEnabled(mCurrentFolderSettings
                                    && !mCurrentFolderSettings->isStructural()
                                    && mCurrentFolderSettings->canDeleteMessages()
                                    && folderWithContent
                                    && !MailCommon::Util::isVirtualCollection(mCurrentCollection));

    updateHtmlMenuEntry();

    mShowFolderShortcutDialogAction->setEnabled(folderWithContent);
    actionlist << akonadiStandardAction(Akonadi::StandardActionManager::ManageLocalSubscriptions);
    bool imapFolderIsOnline = false;
    if (mCurrentFolderSettings && PimCommon::MailUtil::isImapFolder(mCurrentCollection, imapFolderIsOnline)) {
        if (imapFolderIsOnline) {
            actionlist << mServerSideSubscription;
        }
    }

    mGUIClient->unplugActionList(QStringLiteral("collectionview_actionlist"));
    mGUIClient->plugActionList(QStringLiteral("collectionview_actionlist"), actionlist);

    const bool folderIsValid = folderWithContent;
    mApplyAllFiltersFolderAction->setEnabled(folderIsValid);
    mApplyFilterFolderActionsMenu->setEnabled(folderIsValid);
    mApplyFilterFolderRecursiveActionsMenu->setEnabled(folderIsValid);
    for (auto a : qAsConst(mFilterFolderMenuActions)) {
        a->setEnabled(folderIsValid);
    }
    for (auto a : qAsConst(mFilterFolderMenuRecursiveActions)) {
        a->setEnabled(folderIsValid);
    }
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

    clearCurrentFolder();
}

void KMMainWidget::slotShowStartupFolder()
{
    connect(MailCommon::FilterManager::instance(), &FilterManager::filtersChanged,
            this, &KMMainWidget::initializeFilterActions);
    // Plug various action lists. This can't be done in the constructor, as that is called before
    // the main window or Kontact calls createGUI().
    // This function however is called with a single shot timer.
    checkAkonadiServerManagerState();
    mFolderShortcutActionManager->createActions();
    mTagActionManager->createActions();
    messageActions()->setupForwardingActionsList(mGUIClient);
    initializePluginActions();

    const QString newFeaturesMD5 = KMReaderWin::newFeaturesMD5();
    if (kmkernel->firstStart()
        || KMailSettings::self()->previousNewFeaturesMD5() != newFeaturesMD5) {
        KMailSettings::self()->setPreviousNewFeaturesMD5(newFeaturesMD5);
        slotIntro();
    }
}

void KMMainWidget::checkAkonadiServerManagerState()
{
    Akonadi::ServerManager::State state = Akonadi::ServerManager::self()->state();
    if (state == Akonadi::ServerManager::Running) {
        initializeFilterActions();
    } else {
        connect(Akonadi::ServerManager::self(), &ServerManager::stateChanged,
                this, &KMMainWidget::slotServerStateChanged);
    }
}

void KMMainWidget::slotServerStateChanged(Akonadi::ServerManager::State state)
{
    if (state == Akonadi::ServerManager::Running) {
        initializeFilterActions();
        disconnect(Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)));
    }
}

QList<KActionCollection *> KMMainWidget::actionCollections() const
{
    return QList<KActionCollection *>() << actionCollection();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateUndo()
{
    if (actionCollection()->action(QStringLiteral("kmail_undo"))) {
        QAction *act = actionCollection()->action(QStringLiteral("kmail_undo"));
        act->setEnabled(!kmkernel->undoStack()->isEmpty());
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
    if (mGUIClient->factory()) {
        if (!mFilterTBarActions.isEmpty()) {
            mGUIClient->unplugActionList(QStringLiteral("toolbar_filter_actions"));
        }
        if (!mFilterMenuActions.isEmpty()) {
            mGUIClient->unplugActionList(QStringLiteral("menu_filter_actions"));
        }
        if (!mFilterFolderMenuActions.isEmpty()) {
            mGUIClient->unplugActionList(QStringLiteral("menu_filter_folder_actions"));
        }
        if (!mFilterFolderMenuRecursiveActions.isEmpty()) {
            mGUIClient->unplugActionList(QStringLiteral("menu_filter_folder_recursive_actions"));
        }
    }

    for (QAction *a : qAsConst(mFilterMenuActions)) {
        actionCollection()->removeAction(a);
    }
    for (QAction *a : qAsConst(mFilterFolderMenuActions)) {
        actionCollection()->removeAction(a);
    }
    for (QAction *a : qAsConst(mFilterFolderMenuRecursiveActions)) {
        actionCollection()->removeAction(a);
    }

    mApplyFilterActionsMenu->menu()->clear();
    mApplyFilterFolderActionsMenu->menu()->clear();
    mApplyFilterFolderRecursiveActionsMenu->menu()->clear();
    mFilterTBarActions.clear();
    mFilterMenuActions.clear();
    mFilterFolderMenuActions.clear();
    mFilterFolderMenuRecursiveActions.clear();

    qDeleteAll(mFilterCommands);
    mFilterCommands.clear();
}

void KMMainWidget::initializePluginActions()
{
    KMailPluginInterface::self()->initializePluginActions(QStringLiteral("kmail"), mGUIClient);
}

QAction *KMMainWidget::filterToAction(MailCommon::MailFilter *filter)
{
    QString displayText = i18n("Filter %1", filter->name());
    QString icon = filter->icon();
    if (icon.isEmpty()) {
        icon = QStringLiteral("system-run");
    }
    QAction *filterAction = new QAction(QIcon::fromTheme(icon), displayText, actionCollection());
    filterAction->setProperty("filter_id", filter->identifier());
    filterAction->setIconText(filter->toolbarName());

    // The shortcut configuration is done in the filter dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the filter settings correctly.
    actionCollection()->setShortcutsConfigurable(filterAction, false);

    return filterAction;
}

//-----------------------------------------------------------------------------
void KMMainWidget::initializeFilterActions()
{
    clearFilterActions();
    mApplyFilterActionsMenu->menu()->addAction(mApplyAllFiltersAction);
    mApplyFilterFolderActionsMenu->menu()->addAction(mApplyAllFiltersFolderAction);
    mApplyFilterFolderRecursiveActionsMenu->menu()->addAction(mApplyAllFiltersFolderRecursiveAction);
    bool addedSeparator = false;

    const QList<MailFilter *> lstFilters = MailCommon::FilterManager::instance()->filters();
    for (MailFilter *filter : lstFilters) {
        if (!filter->isEmpty() && filter->configureShortcut() && filter->isEnabled()) {
            QString filterName = QStringLiteral("Filter %1").arg(filter->name());
            QString normalizedName = filterName.replace(QLatin1Char(' '), QLatin1Char('_'));
            if (action(normalizedName)) {
                continue;
            }

            if (!addedSeparator) {
                QAction *a = mApplyFilterActionsMenu->menu()->addSeparator();
                mFilterMenuActions.append(a);
                a = mApplyFilterFolderActionsMenu->menu()->addSeparator();
                mFilterFolderMenuActions.append(a);
                a = mApplyFilterFolderRecursiveActionsMenu->menu()->addSeparator();
                mFilterFolderMenuRecursiveActions.append(a);
                addedSeparator = true;
            }

            KMMetaFilterActionCommand *filterCommand = new KMMetaFilterActionCommand(filter->identifier(), this);
            mFilterCommands.append(filterCommand);

            auto filterAction = filterToAction(filter);
            actionCollection()->addAction(normalizedName, filterAction);
            connect(filterAction, &QAction::triggered,
                    filterCommand, &KMMetaFilterActionCommand::start);
            actionCollection()->setDefaultShortcut(filterAction, filter->shortcut());
            mApplyFilterActionsMenu->menu()->addAction(filterAction);
            mFilterMenuActions.append(filterAction);
            if (filter->configureToolbar()) {
                mFilterTBarActions.append(filterAction);
            }

            filterAction = filterToAction(filter);
            actionCollection()->addAction(normalizedName + QStringLiteral("___folder"), filterAction);
            connect(filterAction, &QAction::triggered,
                    this, [this] {
                slotApplyFilterOnFolder(/* recursive */ false);
            });
            mApplyFilterFolderActionsMenu->menu()->addAction(filterAction);
            mFilterFolderMenuActions.append(filterAction);

            filterAction = filterToAction(filter);
            actionCollection()->addAction(normalizedName + QStringLiteral("___folder_recursive"), filterAction);
            connect(filterAction, &QAction::triggered,
                    this, [this] {
                slotApplyFilterOnFolder(/* recursive */ true);
            });
            mApplyFilterFolderRecursiveActionsMenu->menu()->addAction(filterAction);
            mFilterFolderMenuRecursiveActions.append(filterAction);
        }
    }

    if (mGUIClient->factory()) {
        if (!mFilterMenuActions.isEmpty()) {
            mGUIClient->plugActionList(QStringLiteral("menu_filter_actions"), mFilterMenuActions);
        }
        if (!mFilterTBarActions.isEmpty()) {
            mFilterTBarActions.prepend(mToolbarActionSeparator);
            mGUIClient->plugActionList(QStringLiteral("toolbar_filter_actions"), mFilterTBarActions);
        }
        if (!mFilterFolderMenuActions.isEmpty()) {
            mGUIClient->plugActionList(QStringLiteral("menu_filter_folder_actions"), mFilterFolderMenuActions);
        }
        if (!mFilterFolderMenuRecursiveActions.isEmpty()) {
            mGUIClient->plugActionList(QStringLiteral("menu_filter_folder_recursive_actions"), mFilterFolderMenuRecursiveActions);
        }
    }

    // Our filters have changed, now enable/disable them
    updateMessageActions();
}

void KMMainWidget::updateFileMenu()
{
    const bool isEmpty = MailCommon::Util::agentInstances().isEmpty();
    actionCollection()->action(QStringLiteral("check_mail"))->setEnabled(!isEmpty);
    actionCollection()->action(QStringLiteral("check_mail_in"))->setEnabled(!isEmpty);
}

//-----------------------------------------------------------------------------
const KMMainWidget::PtrList *KMMainWidget::mainWidgetList()
{
    // better safe than sorry; check whether the global static has already been destroyed
    if (theMainWidgetList.isDestroyed()) {
        return nullptr;
    }
    return theMainWidgetList;
}

QSharedPointer<FolderSettings> KMMainWidget::currentFolder() const
{
    return mCurrentFolderSettings;
}

QAction *KMMainWidget::action(const QString &name)
{
    return mActionCollection->action(name);
}

KActionMenu *KMMainWidget::filterMenu() const
{
    return mFilterMenu;
}

KActionMenu *KMMainWidget::mailingListActionMenu() const
{
    return mMsgActions->mailingListActionMenu();
}

QAction *KMMainWidget::sendQueuedAction() const
{
    return mSendQueued;
}

KActionMenuTransport *KMMainWidget::sendQueueViaMenu() const
{
    return mSendActionMenu;
}

KMail::MessageActions *KMMainWidget::messageActions() const
{
    return mMsgActions;
}

//-----------------------------------------------------------------------------
QString KMMainWidget::overrideEncoding() const
{
    if (mMsgView) {
        return mMsgView->overrideEncoding();
    } else {
        return MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding();
    }
}

void KMMainWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    mWasEverShown = true;
}

KActionCollection *KMMainWidget::actionCollection() const
{
    return mActionCollection;
}

void KMMainWidget::slotRequestFullSearchFromQuickSearch()
{
    // First, open the search window. If we are currently on a search folder,
    // the search associated with that will be loaded.
    if (!showSearchDialog()) {
        return;
    }

    assert(mSearchWin);

    // Now we look at the current state of the quick search, and if there's
    // something in there, we add the criteria to the existing search for
    // the search folder, if applicable, or make a new one from it.
    SearchPattern pattern;
    const QString searchString = mMessagePane->currentFilterSearchString();
    if (!searchString.isEmpty()) {
        MessageList::Core::QuickSearchLine::SearchOptions options = mMessagePane->currentOptions();
        QByteArray searchStringVal;
        if (options & MessageList::Core::QuickSearchLine::SearchEveryWhere) {
            searchStringVal = QByteArrayLiteral("<message>");
        } else if (options & MessageList::Core::QuickSearchLine::SearchAgainstSubject) {
            searchStringVal = QByteArrayLiteral("subject");
        } else if (options & MessageList::Core::QuickSearchLine::SearchAgainstBody) {
            searchStringVal = QByteArrayLiteral("<body>");
        } else if (options & MessageList::Core::QuickSearchLine::SearchAgainstFrom) {
            searchStringVal = QByteArrayLiteral("from");
        } else if (options & MessageList::Core::QuickSearchLine::SearchAgainstBcc) {
            searchStringVal = QByteArrayLiteral("bcc");
        } else if (options & MessageList::Core::QuickSearchLine::SearchAgainstTo) {
            searchStringVal = QByteArrayLiteral("to");
        } else {
            searchStringVal = QByteArrayLiteral("<message>");
        }
        pattern.append(SearchRule::createInstance(searchStringVal, SearchRule::FuncContains, searchString));
        const QList<MessageStatus> statusList = mMessagePane->currentFilterStatus();
        for (MessageStatus status : statusList) {
            if (status.hasAttachment()) {
                pattern.append(SearchRule::createInstance(searchStringVal, SearchRule::FuncHasAttachment, searchString));
                status.setHasAttachment(false);
            }
            if (!status.isOfUnknownStatus()) {
                pattern.append(SearchRule::Ptr(new SearchRuleStatus(status)));
            }
        }
    }
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

FolderTreeView *KMMainWidget::folderTreeView() const
{
    return mFolderTreeWidget->folderTreeView();
}

KXMLGUIClient *KMMainWidget::guiClient() const
{
    return mGUIClient;
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
    mShowBusySplashTimer = nullptr;
    if (mMsgView) {
        // The current selection was cleared, so we'll remove the previously
        // selected message from the preview pane
        if (!item.isValid()) {
            mMsgView->clear();
        } else {
            mShowBusySplashTimer = new QTimer(this);
            mShowBusySplashTimer->setSingleShot(true);
            connect(mShowBusySplashTimer, &QTimer::timeout, this, &KMMainWidget::slotShowBusySplash);
            mShowBusySplashTimer->start(1000);

            Akonadi::ItemFetchJob *itemFetchJob = mMsgView->viewer()->createFetchJob(item);
            if (mCurrentCollection.isValid()) {
                const QString resource = mCurrentCollection.resource();
                itemFetchJob->setProperty("_resource", QVariant::fromValue(resource));
                connect(itemFetchJob, &ItemFetchJob::itemsReceived,
                        this, &KMMainWidget::itemsReceived);
                connect(itemFetchJob, &Akonadi::ItemFetchJob::result, this, &KMMainWidget::itemsFetchDone);
            }
        }
    }
}

void KMMainWidget::itemsReceived(const Akonadi::Item::List &list)
{
    Q_ASSERT(list.size() == 1);
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = nullptr;

    if (!mMsgView) {
        return;
    }

    Item item = list.first();

    if (mMessagePane) {
        mMessagePane->show();

        if (mMessagePane->currentItem() != item) {
            // The user has selected another email already, so don't render this one.
            // Mark it as read, though, if the user settings say so.
            if (MessageViewer::MessageViewerSettings::self()->delayedMarkAsRead()
                && MessageViewer::MessageViewerSettings::self()->delayedMarkTime() == 0) {
                item.setFlag(Akonadi::MessageFlags::Seen);
                Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(item, this);
                modifyJob->disableRevisionCheck();
                modifyJob->setIgnorePayload(true);
            }
            return;
        }
    }

    Akonadi::Item copyItem(item);
    if (mCurrentCollection.isValid()) {
        copyItem.setParentCollection(mCurrentCollection);
    }

    mMsgView->setMessage(copyItem);
    // reset HTML override to the folder setting
    mMsgView->setDisplayFormatMessageOverwrite(mFolderDisplayFormatPreference);
    mMsgView->setHtmlLoadExtDefault(mFolderHtmlLoadExtPreference);
    mMsgView->setDecryptMessageOverwrite(false);
    mMsgActions->setCurrentMessage(copyItem);
}

void KMMainWidget::itemsFetchDone(KJob *job)
{
    delete mShowBusySplashTimer;
    mShowBusySplashTimer = nullptr;
    if (job->error()) {
        // Unfortunately job->error() is Job::Unknown in many cases.
        // (see JobPrivate::handleResponse in akonadi/job.cpp)
        // So we show the "offline" page after checking the resource status.
        qCDebug(KMAIL_LOG) << job->error() << job->errorString();

        const QString resource = job->property("_resource").toString();
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance(resource);
        if (!agentInstance.isOnline()) {
            // The resource is offline
            mMessagePane->show();
            if (mMsgView) {
                mMsgView->viewer()->enableMessageDisplay();
                mMsgView->clear(true);
                if (kmkernel->isOffline()) {
                    showOfflinePage();
                } else {
                    showResourceOfflinePage();
                }
            }
        } else {
            // Some other error
            showMessageActivities(job->errorString());
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

StandardMailActionManager *KMMainWidget::standardMailActionManager() const
{
    return mAkonadiStandardActionManager;
}

void KMMainWidget::slotRemoveDuplicates()
{
    RemoveDuplicateMailJob *job = new RemoveDuplicateMailJob(mFolderTreeWidget->folderTreeView()->selectionModel(), this, this);
    job->start();
}

void KMMainWidget::slotServerSideSubscription()
{
    if (!mCurrentCollection.isValid()) {
        return;
    }
    PimCommon::ManageServerSideSubscriptionJob *job = new PimCommon::ManageServerSideSubscriptionJob(this);
    job->setCurrentCollection(mCurrentCollection);
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

void KMMainWidget::slotCreateAddressBookContact()
{
    CreateNewContactJob *job = new CreateNewContactJob(this, this);
    job->start();
}

void KMMainWidget::slotOpenRecentMessage(const QUrl &url)
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(this, url, overrideEncoding(), this);
    openCommand->start();
}

void KMMainWidget::addRecentFile(const QUrl &mUrl)
{
    mOpenRecentAction->addUrl(mUrl);
    KConfigGroup grp = mConfig->group(QStringLiteral("Recent Files"));
    mOpenRecentAction->saveEntries(grp);
    grp.sync();
}

void KMMainWidget::slotMoveMessageToTrash()
{
    if (messageView() && messageView()->viewer() && mCurrentCollection.isValid()) {
        KMTrashMsgCommand *command = new KMTrashMsgCommand(mCurrentCollection, messageView()->viewer()->messageItem(), -1);
        command->start();
    }
}

void KMMainWidget::slotArchiveMails()
{
    if (mCurrentCollection.isValid()) {
        const Akonadi::Item::List selectedMessages = mMessagePane->selectionAsMessageItemList();
        KMKernel::self()->folderArchiveManager()->setArchiveItems(selectedMessages, mCurrentCollection.resource());
    }
}

void KMMainWidget::updateQuickSearchLineText()
{
    //If change change shortcut
    mMessagePane->setQuickSearchClickMessage(i18nc("Show shortcut for focus quick search. Don't change it", "Search... <%1>", mQuickSearchAction->shortcut().toString()));
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
                                                              QStringLiteral("OverrideHtmlWarning"), nullptr);
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
    writeFolderConfig();
}

void KMMainWidget::populateMessageListStatusFilterCombo()
{
    mMessagePane->populateStatusFilterCombo();
}

void KMMainWidget::slotCollectionRemoved(const Akonadi::Collection &col)
{
    if (mFavoritesModel) {
        mFavoritesModel->removeCollection(col);
    }
}

void KMMainWidget::slotMarkAllMessageAsReadInCurrentFolderAndSubfolder()
{
    if (mCurrentCollection.isValid()) {
        MarkAllMessagesAsReadInFolderAndSubFolderJob *job = new MarkAllMessagesAsReadInFolderAndSubFolderJob(this);
        job->setTopLevelCollection(mCurrentCollection);
        job->start();
    }
}

void KMMainWidget::slotRemoveDuplicateRecursive()
{
    if (mCurrentCollection.isValid()) {
        RemoveDuplicateMessageInFolderAndSubFolderJob *job = new RemoveDuplicateMessageInFolderAndSubFolderJob(this, this);
        job->setTopLevelCollection(mCurrentCollection);
        job->start();
    }
}

void KMMainWidget::slotUpdateConfig()
{
    readFolderConfig();
    updateHtmlMenuEntry();
}

void KMMainWidget::printCurrentMessage(bool preview)
{
    bool result = false;
    if (messageView() && messageView()->viewer()) {
        if (MessageViewer::MessageViewerSettings::self()->printSelectedText()) {
            result = messageView()->printSelectedText(preview);
        }
    }
    if (!result) {
        const bool useFixedFont = MessageViewer::MessageViewerSettings::self()->useFixedFont();
        const QString overrideEncoding = MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding();

        const Akonadi::Item currentItem = messageView()->viewer()->messageItem();

        KMPrintCommandInfo commandInfo;
        commandInfo.mMsg = currentItem;
        commandInfo.mHeaderStylePlugin = messageView()->viewer()->headerStylePlugin();
        commandInfo.mFormat = messageView()->viewer()->displayFormatMessageOverwrite();
        commandInfo.mHtmlLoadExtOverride =  messageView()->viewer()->htmlLoadExternal();
        commandInfo.mPrintPreview = preview;
        commandInfo.mUseFixedFont = useFixedFont;
        commandInfo.mOverrideFont = overrideEncoding;
        commandInfo.mShowSignatureDetails = messageView()->viewer()->showSignatureDetails();
        commandInfo.mShowEncryptionDetails = messageView()->viewer()->showEncryptionDetails();

        KMPrintCommand *command
            = new KMPrintCommand(this, commandInfo);
        command->start();
    }
}

void KMMainWidget::slotRedirectCurrentMessage()
{
    if (messageView() && messageView()->viewer()) {
        const Akonadi::Item currentItem = messageView()->viewer()->messageItem();
        if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
            return;
        }
        KMCommand *command = new KMRedirectCommand(this, currentItem);
        command->start();
    }
}

void KMMainWidget::replyCurrentMessageCommand(MessageComposer::ReplyStrategy strategy)
{
    if (messageView() && messageView()->viewer()) {
        Akonadi::Item currentItem = messageView()->viewer()->messageItem();
        if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
            return;
        }
        const QString text = messageView()->copyText();
        KMReplyCommand *command = new KMReplyCommand(this, currentItem, strategy, text);
        command->start();
    }
}

void KMMainWidget::slotReplyMessageTo(const KMime::Message::Ptr &message, bool replyToAll)
{
    Akonadi::Item item;

    item.setPayload<KMime::Message::Ptr>(message);
    item.setMimeType(KMime::Message::mimeType());
    KMReplyCommand *command = new KMReplyCommand(this, item, replyToAll ? MessageComposer::ReplyAll : MessageComposer::ReplyAuthor);
    command->start();
}

void KMMainWidget::showMessageActivities(const QString &str)
{
    BroadcastStatus::instance()->setStatusMsg(str);
    PimCommon::LogActivitiesManager::self()->appendLog(str);
}

void KMMainWidget::slotCopyDecryptedTo(QAction *action)
{
    if (action) {
        const auto index = action->data().toModelIndex();
        const auto collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

        auto command = new KMCopyDecryptedCommand(collection, mMessagePane->selectionAsMessageItemList());
        command->start();
    }
}

void KMMainWidget::slotSetFocusToViewer()
{
    if (messageView() && messageView()->viewer()) {
        messageView()->viewer()->setFocus();
    }
}
