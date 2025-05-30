/*   */

#include "kmkernel.h"
#include "job/fillcomposerjob.h"
#include "job/newmessagejob.h"
#include "job/opencomposerhiddenjob.h"
#include "job/opencomposerjob.h"
#include <PimCommon/BroadcastStatus>
#include <PimCommonAkonadi/ProgressManagerAkonadi>
using PimCommon::BroadcastStatus;
#include "commandlineinfo.h"
#include "editor/composer.h"
#include "kmmainwidget.h"
#include "kmmainwin.h"
#include "kmreadermainwin.h"
#include "undostack.h"

#if !KMAIL_FORCE_DISABLE_AKONADI_SEARCH
#include "search/checkindexingmanager.h"
#include <PIM/indexeditems.h>
#endif
#include <PimCommonAkonadi/RecentAddresses>
using PimCommon::RecentAddresses;
#include "configuredialog/configuredialog.h"
#include "folderarchive/folderarchivemanager.h"
#include "kmcommands.h"
#include "mailfilteragentinterface.h"
#include "sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "unityservicemanager.h"
#include <MailCommon/FolderTreeView>
#include <MailCommon/KMFilterDialog>
#include <MailCommon/MailUtil>
#include <MessageCore/StringUtil>
#include <PimCommon/PimUtil>
#include <mailcommon/mailcommonsettings_base.h>
#include <mailcommon/pop3settings.h>

#include <Akonadi/DispatcherInterface>
#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>
#include <MailTransport/Transport>
#include <MailTransport/TransportManager>

#include "mailserviceimpl.h"
#include <KSieveCore/SieveImapInstanceInterfaceManager>
using KMail::MailServiceImpl;
#include <MailCommon/JobScheduler>

#include <MessageComposer/AkonadiSender>
#include <MessageComposer/MessageComposerSettings>
#include <MessageComposer/MessageHelper>
#include <MessageCore/MessageCoreSettings>
#include <MessageList/MessageListUtil>
#include <MessageViewer/MessageViewerSettings>
#include <PimCommon/NetworkManager>
#include <TextAutoCorrectionCore/AutoCorrection>
#include <TextAutoCorrectionCore/TextAutoCorrectionSettings>
#include <gravatar/gravatarsettings.h>
#include <messagelist/messagelistsettings.h>

#include <TemplateParser/TemplatesUtil>
#include <templateparser/globalsettings_templateparser.h>

#include <MailCommon/FolderSettings>

#include <KMessageBox>
#include <KNotification>

#include "kmail_debug.h"
#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KIO/JobUiDelegate>
#include <KMessageBox>

#include <Akonadi/AgentManager>
#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionStatisticsJob>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Session>
#include <KMime/Headers>
#include <KMime/Message>

#include <Libkleo/FileSystemWatcher>
#include <Libkleo/GnuPG>
#include <Libkleo/KeyCache>

#include <QDir>
#include <QFileInfo>
#include <QWidget>

#include <MailCommon/ResourceReadConfigFile>

#include <KLocalizedString>
#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/MailKernel>
#include <QStandardPaths>
#include <kmailadaptor.h>
#include <pimcommon/imapresourcesettings.h>

#include "kmail_options.h"
#include "searchdialog/searchdescriptionattribute.h"
#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include "activities/activitiesmanager.h"
#endif
#if KMAIL_WITH_KUSERFEEDBACK
#include "userfeedback/kmailuserfeedbackprovider.h"
#include <KUserFeedback/Provider>
#endif
#include <chrono>
using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;
// #define DEBUG_SCHEDULER 1
using namespace MailCommon;

static KMKernel *mySelf = nullptr;
static bool s_askingToGoOnline = false;
/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel(QObject *parent)
    : QObject(parent)
    , mJobScheduler(new JobScheduler(this))
    , mFolderArchiveManager(new FolderArchiveManager(this))
{
    // Initialize kmail sieveimap interface
    KSieveCore::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    mDebug = !qEnvironmentVariableIsEmpty("KDEPIM_DEBUGGING");

#if KMAIL_WITH_KUSERFEEDBACK
    mUserFeedbackProvider = new KMailUserFeedbackProvider(this);
#endif
    mSystemNetworkStatus = PimCommon::NetworkManager::self()->isOnline();

    Akonadi::AttributeFactory::registerAttribute<Akonadi::SearchDescriptionAttribute>();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kmail"));
    qCDebug(KMAIL_LOG) << "Starting up...";

    mySelf = this;
    the_firstInstance = true;

    mFilterEditDialog = nullptr;
    // make sure that we check for config updates before doing anything else
    KMKernel::config();
    // this shares the kmailrc parsing too (via KSharedConfig), and reads values from it
    // so better do it here, than in some code where changing the group of config()
    // would be unexpected
    KMailSettings::self();
    mAutoCorrection = new TextAutoCorrectionCore::AutoCorrection();

    auto session = new Akonadi::Session("KMail Kernel ETM", this);

    mFolderCollectionMonitor = new FolderCollectionMonitor(session, this);

    connect(mFolderCollectionMonitor->monitor(), &Akonadi::Monitor::collectionRemoved, this, &KMKernel::slotCollectionRemoved);

    mEntityTreeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor(), this);
    mEntityTreeModel->setListFilter(Akonadi::CollectionFetchScope::Enabled);
    mEntityTreeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);
    connect(mEntityTreeModel, &Akonadi::EntityTreeModel::errorOccurred, this, [this](const QString &message) {
        KMessageBox::error(getKMMainWidget(), message);
    });

    mCollectionModel = new Akonadi::EntityMimeTypeFilterModel(this);
    mCollectionModel->setSourceModel(mEntityTreeModel);
    mCollectionModel->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mCollectionModel->setHeaderGroup(Akonadi::EntityTreeModel::CollectionTreeHeaders);
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    connect(folderCollectionMonitor(),
            qOverload<const Akonadi::Collection &, const QSet<QByteArray> &>(&Akonadi::ChangeRecorder::collectionChanged),
            this,
            &KMKernel::slotCollectionChanged);

    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRemoved, this, &KMKernel::transportRemoved);
    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRenamed, this, &KMKernel::transportRenamed);

    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/MailDispatcherAgent"),
                                          QStringLiteral("org.freedesktop.Akonadi.MailDispatcherAgent"),
                                          QStringLiteral("itemDispatchStarted"),
                                          this,
                                          SLOT(itemDispatchStarted()));
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceStatusChanged, this, &KMKernel::instanceStatusChanged);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceError, this, &KMKernel::slotInstanceError);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceWarning, this, &KMKernel::slotInstanceWarning);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &KMKernel::slotInstanceRemoved);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceAdded, this, &KMKernel::slotInstanceAdded);

    connect(PimCommon::NetworkManager::self(), &PimCommon::NetworkManager::networkStatusChanged, this, &KMKernel::slotSystemNetworkStatusChanged);

    connect(KPIM::ProgressManager::instance(), &KPIM::ProgressManager::progressItemCompleted, this, &KMKernel::slotProgressItemCompletedOrCanceled);
    connect(KPIM::ProgressManager::instance(), &KPIM::ProgressManager::progressItemCanceled, this, &KMKernel::slotProgressItemCompletedOrCanceled);
    connect(identityManager(), &KIdentityManagementCore::IdentityManager::deleted, this, &KMKernel::slotDeleteIdentity);
    CommonKernel->registerKernelIf(this);
    CommonKernel->registerSettingsIf(this);
    CommonKernel->registerFilterIf(this);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    CommonKernel->registerActivitiesBaseManager(ActivitiesManager::self());
    ActivitiesManager::self()->setEnabled(KMailSettings::self()->plasmaActivitySupport());
#endif

#if !KMAIL_FORCE_DISABLE_AKONADI_SEARCH
    mIndexedItems = new Akonadi::Search::PIM::IndexedItems(this);
    mCheckIndexingManager = new CheckIndexingManager(mIndexedItems, this);
#endif
    mUnityServiceManager = new KMail::UnityServiceManager(this);
}

KMKernel::~KMKernel()
{
    delete mMailService;
    mMailService = nullptr;

    stopAgentInstance();
    saveConfig();

    delete mAutoCorrection;
    delete mMailCommonSettings;
    mySelf = nullptr;
}

Akonadi::ChangeRecorder *KMKernel::folderCollectionMonitor() const
{
    return mFolderCollectionMonitor->monitor();
}

Akonadi::EntityTreeModel *KMKernel::entityTreeModel() const
{
    return mEntityTreeModel;
}

Akonadi::EntityMimeTypeFilterModel *KMKernel::collectionModel() const
{
    return mCollectionModel;
}

void KMKernel::setupDBus()
{
    (void)new KmailAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KMail"), this);
    mMailService = new MailServiceImpl();
}

bool KMKernel::handleCommandLine(bool noArgsOpensReader, const QStringList &args, const QString &workingDir)
{
    // qDebug() << " args " << args;
    CommandLineInfo commandLineInfo;
    commandLineInfo.parseCommandLine(args, workingDir);

    if (commandLineInfo.startInTray()) {
        KMailSettings::self()->setSystemTrayEnabled(true);
    }

    if (!noArgsOpensReader && !commandLineInfo.mailto() && !commandLineInfo.checkMail() && !commandLineInfo.viewOnly()) {
        return false;
    }

    if (commandLineInfo.viewOnly()) {
        viewMessage(commandLineInfo.messageFile());
    } else {
        action(commandLineInfo.mailto(),
               commandLineInfo.checkMail(),
               commandLineInfo.startInTray(),
               commandLineInfo.htmlBody(),
               commandLineInfo.to(),
               commandLineInfo.cc(),
               commandLineInfo.bcc(),
               commandLineInfo.subject(),
               commandLineInfo.body(),
               commandLineInfo.messageFile(),
               commandLineInfo.attachURLs(),
               commandLineInfo.customHeaders(),
               commandLineInfo.replyTo(),
               commandLineInfo.inReplyTo(),
               commandLineInfo.identity());
    }
    return true;
}

// It may be able be possible to use this in AccountsPageReceivingTab,
// but check the use of resourceGroupPattern there - there may not be
// a KMKernel.
// Move to KMail::Util instead?
KConfigGroup KMKernel::resourceConfigGroup(const QString &id)
{
    return (KConfigGroup(KMKernel::config(), QStringLiteral("Resource %1").arg(id)));
}

/********************************************************************/
/*             D-Bus-callable, and command line actions              */
/********************************************************************/
void KMKernel::checkMail() // might create a new reader but won't show!!
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance &agent : lst) {
        const QString id = agent.identifier();
        const KConfigGroup group = resourceConfigGroup(id);
        if (group.readEntry("IncludeInManualChecks", true)) {
            if (!agent.isOnline()) {
                agent.setIsOnline(true);
            }
            if (mResourcesBeingChecked.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Starting manual mail check";
                Q_EMIT startCheckMail();
            }

            if (!mResourcesBeingChecked.contains(id)) {
                mResourcesBeingChecked.append(id);
            }
            agent.synchronize();
        }
    }
}

void KMKernel::openReader()
{
    openReader(false, false);
}

QStringList KMKernel::accounts() const
{
    QStringList accountLst;
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    accountLst.reserve(lst.count());
    for (const Akonadi::AgentInstance &type : lst) {
        // Explicitly make a copy, as we're not changing values of the list but only
        // the local copy which is passed to action.
        accountLst << type.identifier();
    }
    return accountLst;
}

void KMKernel::checkAccount(const QString &account) // might create a new reader but won't show!!
{
    if (account.isEmpty()) {
        checkMail();
    } else {
        Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(account);
        if (agent.isValid()) {
            agent.synchronize();
        } else {
            qCDebug(KMAIL_LOG) << "- account with name '" << account << "' not found";
        }
    }
}

void KMKernel::openReader(bool onlyCheck, bool startInTray)
{
    KMainWindow *ktmw = nullptr;

    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        if (::qobject_cast<KMMainWin *>(window)) {
            ktmw = window;
            break;
        }
    }

    bool activate;
    if (ktmw) {
        auto win = static_cast<KMMainWin *>(ktmw);
        activate = !onlyCheck; // existing window: only activate if not --check
        if (activate) {
            win->showAndActivateWindow();
        }
    } else {
        auto win = new KMMainWin;
        if (!startInTray && !KMailSettings::self()->startInTray()) {
            win->showAndActivateWindow();
        }
        activate = false; // new window: no explicit activation (#73591)
    }
}

void KMKernel::openComposer(const QString &to,
                            const QString &cc,
                            const QString &bcc,
                            const QString &subject,
                            const QString &body,
                            bool hidden,
                            const QString &messageFile,
                            const QStringList &attachmentPaths,
                            const QStringList &customHeaders,
                            const QString &replyTo,
                            const QString &inReplyTo,
                            const QString &identity,
                            bool htmlBody)
{
    const OpenComposerSettings
        settings(to, cc, bcc, subject, body, hidden, messageFile, attachmentPaths, customHeaders, replyTo, inReplyTo, identity, htmlBody);
    auto job = new OpenComposerJob(this);
    job->setOpenComposerSettings(std::move(settings));
    job->start();
}

void KMKernel::openComposer(const QString &to,
                            const QString &cc,
                            const QString &bcc,
                            const QString &subject,
                            const QString &body,
                            bool hidden,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset,
                            unsigned int identity,
                            bool htmlBody)
{
    fillComposer(hidden,
                 to,
                 cc,
                 bcc,
                 subject,
                 body,
                 attachName,
                 attachCte,
                 attachData,
                 attachType,
                 attachSubType,
                 attachParamAttr,
                 attachParamValue,
                 attachContDisp,
                 attachCharset,
                 identity,
                 false,
                 htmlBody);
}

void KMKernel::openComposer(const QString &to,
                            const QString &cc,
                            const QString &bcc,
                            const QString &subject,
                            const QString &body,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset,
                            unsigned int identity,
                            bool htmlBody)
{
    fillComposer(false,
                 to,
                 cc,
                 bcc,
                 subject,
                 body,
                 attachName,
                 attachCte,
                 attachData,
                 attachType,
                 attachSubType,
                 attachParamAttr,
                 attachParamValue,
                 attachContDisp,
                 attachCharset,
                 identity,
                 true,
                 htmlBody);
}

void KMKernel::fillComposer(bool hidden,
                            const QString &to,
                            const QString &cc,
                            const QString &bcc,
                            const QString &subject,
                            const QString &body,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset,
                            unsigned int identity,
                            bool forceShowWindow,
                            bool htmlBody)
{
    const FillComposerJobSettings settings(hidden,
                                           to,
                                           cc,
                                           bcc,
                                           subject,
                                           body,
                                           attachName,
                                           attachCte,
                                           attachData,
                                           attachType,
                                           attachSubType,
                                           attachParamAttr,
                                           attachParamValue,
                                           attachContDisp,
                                           attachCharset,
                                           identity,
                                           forceShowWindow,
                                           htmlBody);
    auto job = new FillComposerJob;
    job->setSettings(std::move(settings));
    job->start();
}

void KMKernel::openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden)
{
    const OpenComposerHiddenJobSettings settings(to, cc, bcc, subject, body, hidden);
    auto job = new OpenComposerHiddenJob(this);
    job->setSettings(settings);
    job->start();
}

void KMKernel::newMessage(const QString &to,
                          const QString &cc,
                          const QString &bcc,
                          bool hidden,
                          bool useFolderId,
                          const QString & /*messageFile*/,
                          const QString &_attachURL)
{
    QSharedPointer<FolderSettings> folder;
    Akonadi::Collection col;
    uint id = 0;
    if (useFolderId) {
        // create message with required folder identity
        folder = currentFolderCollection();
        id = folder ? folder->identity() : 0;
        col = currentCollection();
    }

    const NewMessageJobSettings settings(to, cc, bcc, hidden, _attachURL, folder, id, col);

    auto job = new NewMessageJob(this);
    job->setNewMessageJobSettings(settings);
    job->start();
}

void KMKernel::viewMessage(const QUrl &url)
{
    auto openCommand = new KMOpenMsgCommand(nullptr, url);

    openCommand->start();
}

int KMKernel::viewMessage(const QString &messageFile)
{
    auto openCommand = new KMOpenMsgCommand(nullptr, QUrl::fromLocalFile(messageFile));

    openCommand->start();
    return 1;
}

void KMKernel::raise()
{
    QDBusInterface iface(QStringLiteral("org.kde.kmail"),
                         QStringLiteral("/MainApplication"),
                         QStringLiteral("org.kde.PIMUniqueApplication"),
                         QDBusConnection::sessionBus());
    QDBusReply<int> reply;
    if (!iface.isValid() || !(reply = iface.call(QStringLiteral("newInstance"))).isValid()) {
        QDBusError err = iface.lastError();
        qCritical() << "Communication problem with KMail. "
                    << "Error message was:" << err.name() << ": \"" << err.message() << "\"";
    }
}

bool KMKernel::showMail(qint64 serialNumber)
{
    KMMainWidget *mainWidget = nullptr;

    // First look for a KMainWindow.
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        // Then look for a KMMainWidget.
        QList<KMMainWidget *> l = window->findChildren<KMMainWidget *>();
        if (!l.isEmpty() && l.first()) {
            mainWidget = l.first();
            if (window->isActiveWindow()) {
                break;
            }
        }
    }
    if (mainWidget) {
        auto job = new Akonadi::ItemFetchJob(Akonadi::Item(serialNumber), this);
        job->fetchScope().fetchFullPayload();
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        if (job->exec()) {
            if (job->items().count() >= 1) {
                auto win = new KMReaderMainWin(MessageViewer::Viewer::UseGlobalSetting, false);
                const auto item = job->items().at(0);
                win->showMessage(MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding(), item, item.parentCollection());
                win->showAndActivateWindow();
                return true;
            }
        }
    }
    return false;
}

void KMKernel::pauseBackgroundJobs()
{
    mBackgroundTasksTimer->stop();
    mJobScheduler->pause();
}

void KMKernel::resumeBackgroundJobs()
{
    mJobScheduler->resume();
    mBackgroundTasksTimer->start(4h);
}

void KMKernel::stopNetworkJobs()
{
    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Offline) {
        return;
    }

    setAccountStatus(false);

    KMailSettings::setNetworkState(KMailSettings::EnumNetworkState::Offline);
    BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be offline; all network jobs are suspended"));
    Q_EMIT onlineStatusChanged((KMailSettings::EnumNetworkState::type)KMailSettings::networkState());
}

void KMKernel::setAccountStatus(bool goOnline)
{
    Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances(false);
    for (Akonadi::AgentInstance &agent : lst) {
        const QString identifier(agent.identifier());
        if (PimCommon::Util::isImapResource(identifier) || identifier.contains(POP3_RESOURCE_IDENTIFIER)
            || identifier.contains("akonadi_maildispatcher_agent"_L1) || agent.type().capabilities().contains("NeedsNetwork"_L1)) {
            agent.setIsOnline(goOnline);
        }
    }
    if (goOnline && MessageComposer::MessageComposerSettings::self()->sendImmediate()) {
        const auto col = CommonKernel->collectionFromId(CommonKernel->outboxCollectionFolder().id());
        const qint64 nbMsgOutboxCollection = col.statistics().count();
        if (nbMsgOutboxCollection > 0) {
            if (!kmkernel->msgSender()->sendQueued()) {
                KNotification::event(QStringLiteral("sent-mail-error"),
                                     i18n("Send Email"),
                                     i18n("Impossible to send email"),
                                     QStringLiteral("kmail"),
                                     KNotification::CloseOnTimeout);
            }
        }
    }
}

const QString KMKernel::xmlGuiInstanceName() const
{
    return mXmlGuiInstance;
}

void KMKernel::setXmlGuiInstanceName(const QString &instance)
{
    mXmlGuiInstance = instance;
}

KMail::UndoStack *KMKernel::undoStack() const
{
    return the_undoStack;
}

void KMKernel::resumeNetworkJobs()
{
    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Online) {
        return;
    }

    if (mSystemNetworkStatus) {
        setAccountStatus(true);
        BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be online; all network jobs resumed"));
    } else {
        BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be online; all network jobs will resume when a network connection is detected"));
    }
    KMailSettings::setNetworkState(KMailSettings::EnumNetworkState::Online);
    Q_EMIT onlineStatusChanged((KMailSettings::EnumNetworkState::type)KMailSettings::networkState());
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        widget->refreshMessageListSelection();
    }
}

bool KMKernel::isOffline()
{
    if ((KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Offline) || !PimCommon::NetworkManager::self()->isOnline()) {
        return true;
    } else {
        return false;
    }
}

void KMKernel::verifyAccounts()
{
    Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance &agent : lst) {
        const KConfigGroup group = resourceConfigGroup(agent.identifier());
        if (group.readEntry("CheckOnStartup", false)) {
            if (!agent.isOnline()) {
                agent.setIsOnline(true);
            }
            agent.synchronize();
        }

        // "false" is also hardcoded in ConfigureDialog, don't forget to change there.
        if (group.readEntry("OfflineOnShutdown", false)) {
            if (!agent.isOnline()) {
                agent.setIsOnline(true);
            }
        }
    }
}

void KMKernel::slotCheckAccounts(Akonadi::ServerManager::State state)
{
    if (state == Akonadi::ServerManager::Running) {
        disconnect(Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)));
        verifyAccounts();
    }
}

void KMKernel::checkMailOnStartup()
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    if (Akonadi::ServerManager::state() != Akonadi::ServerManager::Running) {
        connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, &KMKernel::slotCheckAccounts);
    } else {
        verifyAccounts();
    }
}

bool KMKernel::askToGoOnline()
{
    // already asking means we are offline and need to wait anyhow
    if (s_askingToGoOnline) {
        return false;
    }

    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Offline) {
        s_askingToGoOnline = true;
        int rc = KMessageBox::questionTwoActions(KMKernel::self()->mainWin(),
                                                 i18n("KMail is currently in offline mode. "
                                                      "How do you want to proceed?"),
                                                 i18nc("@title:window", "Online/Offline"),
                                                 KGuiItem(i18nc("@action:button", "Work Online")),
                                                 KGuiItem(i18nc("@action:button", "Work Offline")));

        s_askingToGoOnline = false;
        if (rc == KMessageBox::ButtonCode::SecondaryAction) {
            return false;
        } else {
            kmkernel->resumeNetworkJobs();
        }
    }
    if (kmkernel->isOffline()) {
        return false;
    }

    return true;
}

void KMKernel::slotSystemNetworkStatusChanged(bool isOnline)
{
    mSystemNetworkStatus = isOnline;
    if (KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Offline) {
        return;
    }

    if (isOnline) {
        BroadcastStatus::instance()->setStatusMsg(i18n("Network connection detected, all network jobs resumed"));
        kmkernel->setAccountStatus(true);
    } else {
        BroadcastStatus::instance()->setStatusMsg(i18n("No network connection detected, all network jobs are suspended"));
        kmkernel->setAccountStatus(false);
    }
}

/********************************************************************/
/*                        Kernel methods                            */
/********************************************************************/

void KMKernel::quit()
{
    // Called when all windows are closed. Will take care of compacting,
    // sending... should handle session management too!!
}

/* TODO later:
   Assuming that:
     - msgsender is nonblocking
       (our own, QSocketNotifier based. Pops up errors and sends signal
        senderFinished when done)

   o If we are getting mail, stop it (but don't lose something!)
         [Done already, see mailCheckAborted]
   o If we are sending mail, go on UNLESS this was called by SM,
       in which case stop ASAP that too (can we warn? should we continue
       on next start?)
   o If we are compacting, or expunging, go on UNLESS this was SM call.
       In that case stop compacting ASAP and continue on next start, before
       touching any folders. [Not needed anymore with CompactionJob]

   KMKernel::quit ()
   {
     SM call?
       if compacting, stop;
       if sending, stop;
       if receiving, stop;
       Windows will take care of themselves (composer should dump
        its messages, if any but not in deadMail)
       declare us ready for the End of the Session

     No, normal quit call
       All windows are off. Anything to do, should compact or sender sends?
         Yes, maybe put an icon in panel as a sign of life
         if sender sending, connect us to his finished slot, declare us ready
                            for quit and wait for senderFinished
         if not, Folder manager, go compact sent-mail and outbox
}                (= call slotFinished())

void KMKernel::slotSenderFinished()
{
  good, Folder manager go compact sent-mail and outbox
  clean up stage1 (release folders and config, unregister from dcop)
    -- another kmail may start now ---
  qApp->quit();
}
*/

/********************************************************************/
/*            Init, Exit, and handler  methods                      */
/********************************************************************/

//-----------------------------------------------------------------------------
// Open a composer for each message found in the dead.letter folder
void KMKernel::recoverDeadLetters()
{
    const QString pathName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kmail2/"_L1;
    QDir dir(pathName);
    if (!dir.exists(QStringLiteral("autosave"))) {
        return;
    }

    dir.cd(pathName + "autosave"_L1);
    const QFileInfoList autoSaveFiles = dir.entryInfoList();
    for (const QFileInfo &file : autoSaveFiles) {
        // Disregard the '.' and '..' folders
        const QString filename = file.fileName();
        if (filename == QLatin1Char('.') || filename == ".."_L1 || file.isDir()) {
            continue;
        }
        qCDebug(KMAIL_LOG) << "Opening autosave file:" << file.absoluteFilePath();
        QFile autoSaveFile(file.absoluteFilePath());
        if (autoSaveFile.open(QIODevice::ReadOnly)) {
            const KMime::Message::Ptr autoSaveMessage(new KMime::Message());
            const QByteArray msgData = autoSaveFile.readAll();
            autoSaveMessage->setContent(msgData);
            autoSaveMessage->parse();

            // Show the a new composer dialog for the message
            KMail::Composer *autoSaveWin = KMail::makeComposer();
            autoSaveWin->setMessage(autoSaveMessage, false, false, false);
            autoSaveWin->setAutoSaveFileName(filename);
            autoSaveWin->show();
            autoSaveFile.close();
        } else {
            KMessageBox::error(nullptr,
                               i18n("Failed to open autosave file at %1.\nReason: %2", file.absoluteFilePath(), autoSaveFile.errorString()),
                               i18nc("@title:window", "Opening Autosave File Failed"));
        }
    }
}

void KMKernel::akonadiStateChanged(Akonadi::ServerManager::State state)
{
    qCDebug(KMAIL_LOG) << "KMKernel has akonadi state changed to:" << int(state);

    if (state == Akonadi::ServerManager::Running) {
        CommonKernel->initFolders();
    }
}

static void kmCrashHandler(int sigId)
{
    fprintf(stderr, "*** KMail got signal %d (Exiting)\n", sigId);
    // try to cleanup all windows
    if (kmkernel) {
        kmkernel->dumpDeadLetters();
        fprintf(stderr, "*** Dead letters dumped.\n");
        kmkernel->stopAgentInstance();
        kmkernel->cleanupTemporaryFiles();
    }
}

namespace
{
auto initKeyCache()
{
    using namespace Kleo;

    // set up automatic update of the key cache on changes in the key ring
    const auto keyCache = KeyCache::mutableInstance();
    auto watcher = std::make_shared<FileSystemWatcher>();
    watcher->whitelistFiles(gnupgFileWhitelist());
    watcher->addPath(gnupgHomeDirectory());
    watcher->setDelay(1000);
    keyCache->addFileSystemWatcher(watcher);

    return KeyCache::instance();
}
}

void KMKernel::init()
{
    the_shuttingDown = false;

    the_firstStart = KMailSettings::self()->firstStart();
    KMailSettings::self()->setFirstStart(false);

    // keep a reference on the key cache to avoid expensive reinitialization on each use
    mKeyCache = initKeyCache();

    the_undoStack = new KMail::UndoStack(20);

    the_msgSender = new MessageComposer::AkonadiSender;
    // filterMgr->dump();

    mBackgroundTasksTimer = new QTimer(this);
    mBackgroundTasksTimer->setSingleShot(true);
    connect(mBackgroundTasksTimer, &QTimer::timeout, this, &KMKernel::slotRunBackgroundTasks);
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
    mBackgroundTasksTimer->start(10s);
#else
    mBackgroundTasksTimer->start(5min);
#endif

    KCrash::setEmergencySaveFunction(kmCrashHandler);

    qCDebug(KMAIL_LOG) << "KMail init with akonadi server state:" << int(Akonadi::ServerManager::state());
    if (Akonadi::ServerManager::state() == Akonadi::ServerManager::Running) {
        CommonKernel->initFolders();
    }

    connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, &KMKernel::akonadiStateChanged);
}

void KMKernel::doSessionManagement()
{
    // Do session management
    if (qApp->isSessionRestored()) {
        int n = 1;
        while (KMMainWin::canBeRestored(n)) {
            // only restore main windows! (Matthias);
            if (KMMainWin::classNameOfToplevel(n) == "KMMainWin"_L1) {
                (new KMMainWin)->restoreDockedState(n);
            }
            ++n;
        }
    }
}

bool KMKernel::firstInstance() const
{
    return the_firstInstance;
}

void KMKernel::setFirstInstance(bool value)
{
    the_firstInstance = value;
}

void KMKernel::closeAllKMailWindows()
{
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        if (::qobject_cast<KMMainWin *>(window) || ::qobject_cast<KMail::SecondaryWindow *>(window)) {
            // close and delete the window
            window->setAttribute(Qt::WA_DeleteOnClose);
            window->close();
        }
    }
}

void KMKernel::cleanup()
{
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceStatusChanged(Akonadi::AgentInstance)));
    // clang-format off
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceError(Akonadi::AgentInstance,QString)));
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceWarning(Akonadi::AgentInstance,QString)));
    // clang-format on
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)));
    // clang-format off
    disconnect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCompleted(KPIM::ProgressItem*)));
    disconnect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCanceled(KPIM::ProgressItem*)));
    // clang-format on

    dumpDeadLetters();
    the_shuttingDown = true;
    closeAllKMailWindows();

    // Flush the cache of foldercollection objects. This results
    // in configuration writes, so we need to do it early enough.
    MailCommon::FolderSettings::clearCache();

    // Write the config while all other managers are alive
    delete the_msgSender;
    the_msgSender = nullptr;
    delete the_undoStack;
    the_undoStack = nullptr;
    delete mConfigureDialog;
    mConfigureDialog = nullptr;

    KSharedConfig::Ptr config = KMKernel::config();
    if (RecentAddresses::exists()) {
        RecentAddresses::self(config.data())->save(config.data());
    }

    Akonadi::Collection trashCollection = CommonKernel->trashCollectionFolder();
    if (trashCollection.isValid()) {
        if (KMailSettings::self()->emptyTrashOnExit()) {
            const auto service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_mailfilter_agent"));
            OrgFreedesktopAkonadiMailFilterAgentInterface mailFilterInterface(service, QStringLiteral("/MailFilterAgent"), QDBusConnection::sessionBus(), this);
            if (mailFilterInterface.isValid()) {
                mailFilterInterface.expunge(static_cast<qlonglong>(trashCollection.id()));
            } else {
                qCWarning(KMAIL_LOG) << "Mailfilter is not active";
            }
        }
    }
}

void KMKernel::dumpDeadLetters()
{
    if (shuttingDown()) {
        return; // All documents should be saved before shutting down is set!
    }

    // make all composer windows autosave their contents
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        if (auto win = ::qobject_cast<KMail::Composer *>(window)) {
            win->autoSaveMessage(true);

            while (win->isComposing()) {
                qCWarning(KMAIL_LOG) << "Danger, using an event loop, this should no longer be happening!";
                qApp->processEvents();
            }
        }
    }
}

void KMKernel::action(bool mailto,
                      bool check,
                      bool startInTray,
                      bool htmlBody,
                      const QString &to,
                      const QString &cc,
                      const QString &bcc,
                      const QString &subj,
                      const QString &body,
                      const QUrl &messageFile,
                      const QList<QUrl> &attachURLs,
                      const QStringList &customHeaders,
                      const QString &replyTo,
                      const QString &inReplyTo,
                      const QString &identity)
{
    if (mailto) {
        openComposer(to,
                     cc,
                     bcc,
                     subj,
                     body,
                     false,
                     messageFile.toLocalFile(),
                     QUrl::toStringList(attachURLs),
                     customHeaders,
                     replyTo,
                     inReplyTo,
                     identity,
                     htmlBody);
    } else {
        openReader(check, startInTray);
    }

    if (check) {
        checkMail();
    }
    // Anything else?
}

void KMKernel::slotRequestConfigSync()
{
    // ### FIXME: delay as promised in the kdoc of this function ;-)
    slotSyncConfig();
}

void KMKernel::slotSyncConfig()
{
    saveConfig();
    // Laurent investigate why we need to reload them.
    TextAutoCorrectionCore::TextAutoCorrectionSettings::self()->load();
    MessageCore::MessageCoreSettings::self()->load();
    MessageViewer::MessageViewerSettings::self()->load();
    MessageComposer::MessageComposerSettings::self()->load();
    TemplateParser::TemplateParserSettings::self()->load();
    MessageList::MessageListSettings::self()->load();
    mMailCommonSettings->load();
    Gravatar::GravatarSettings::self()->load();
    KMailSettings::self()->load();
    KMKernel::config()->reparseConfiguration();
    mUnityServiceManager->updateCount();
}

void KMKernel::saveConfig()
{
    TextAutoCorrectionCore::TextAutoCorrectionSettings::self()->save();
    MessageCore::MessageCoreSettings::self()->save();
    MessageViewer::MessageViewerSettings::self()->save();
    MessageComposer::MessageComposerSettings::self()->save();
    TemplateParser::TemplateParserSettings::self()->save();
    MessageList::MessageListSettings::self()->save();
    mMailCommonSettings->save();
    Gravatar::GravatarSettings::self()->save();
    KMailSettings::self()->save();
}

void KMKernel::updateConfig()
{
    slotConfigChanged();
}

void KMKernel::slotShowConfigurationDialog()
{
    if (KMKernel::getKMMainWidget() == nullptr) {
        // ensure that there is a main widget available
        // as parts of the configure dialog (identity) rely on this
        // and this slot can be called when there is only a KMComposeWin showing
        auto win = new KMMainWin;
        win->show();
    }

    if (!mConfigureDialog) {
        mConfigureDialog = new ConfigureDialog(nullptr, false);
        mConfigureDialog->setObjectName("configure"_L1);
        connect(mConfigureDialog, &ConfigureDialog::configChanged, this, &KMKernel::slotConfigChanged);
    }

    // Save all current settings.
    if (getKMMainWidget()) {
        getKMMainWidget()->writeReaderConfig();
    }

    if (mConfigureDialog->isHidden()) {
        mConfigureDialog->show();
    }
    mConfigureDialog->raise();
    mConfigureDialog->activateWindow();
}

void KMKernel::slotConfigChanged()
{
    Q_EMIT configChanged();
}

//-------------------------------------------------------------------------------

bool KMKernel::haveSystemTrayApplet() const
{
    return mUnityServiceManager->haveSystemTrayApplet();
}

void KMKernel::setSystemTryAssociatedWindow(QWindow *window)
{
    mUnityServiceManager->setSystemTryAssociatedWindow(window);
}

void KMKernel::updateSystemTray()
{
    if (!the_shuttingDown) {
        mUnityServiceManager->initListOfCollection();
    }
}

KIdentityManagementCore::IdentityManager *KMKernel::identityManager()
{
    return KIdentityManagementCore::IdentityManager::self();
}

JobScheduler *KMKernel::jobScheduler() const
{
    return mJobScheduler;
}

KMainWindow *KMKernel::mainWin()
{
    // First look for a KMMainWin.
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        if (::qobject_cast<KMMainWin *>(window)) {
            return window;
        }
    }

    // There is no KMMainWin. Use any other KMainWindow instead (e.g. in
    // case we are running inside Kontact) because we anyway only need
    // it for modal message boxes and for KNotify events.
    if (!KMainWindow::memberList().isEmpty()) {
        KMainWindow *kmWin = KMainWindow::memberList().constFirst();
        if (kmWin) {
            return kmWin;
        }
    }

    // There's not a single KMainWindow. Create a KMMainWin.
    // This could happen if we want to pop up an error message
    // while we are still doing the startup wizard and no other
    // KMainWindow is running.
    return new KMMainWin;
}

KMKernel *KMKernel::self()
{
    return mySelf;
}

KSharedConfig::Ptr KMKernel::config()
{
    assert(mySelf);
    if (!mySelf->mConfig) {
        mySelf->mConfig = KSharedConfig::openConfig(QStringLiteral("kmail2rc"));
        // Check that all updates have been run on the config file:
        MessageList::MessageListSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageList::MessageListSettings::self()->load();
        TemplateParser::TemplateParserSettings::self()->setSharedConfig(mySelf->mConfig);
        TemplateParser::TemplateParserSettings::self()->load();
        MessageComposer::MessageComposerSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageComposer::MessageComposerSettings::self()->load();
        MessageCore::MessageCoreSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageCore::MessageCoreSettings::self()->load();
        MessageViewer::MessageViewerSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageViewer::MessageViewerSettings::self()->load();
        mMailCommonSettings = new MailCommon::MailCommonSettings;

        mMailCommonSettings->setSharedConfig(mySelf->mConfig);
        mMailCommonSettings->load();

        TextAutoCorrectionCore::TextAutoCorrectionSettings::self()->setSharedConfig(mySelf->mConfig);
        TextAutoCorrectionCore::TextAutoCorrectionSettings::self()->load();
        Gravatar::GravatarSettings::self()->setSharedConfig(mySelf->mConfig);
        Gravatar::GravatarSettings::self()->load();
    }
    return mySelf->mConfig;
}

void KMKernel::syncConfig()
{
    slotRequestConfigSync();
}

void KMKernel::selectCollectionFromId(Akonadi::Collection::Id id)
{
    KMMainWidget *widget = getKMMainWidget();
    Q_ASSERT(widget);
    if (!widget) {
        return;
    }

    Akonadi::Collection colFolder = CommonKernel->collectionFromId(id);

    if (colFolder.isValid()) {
        widget->slotSelectCollectionFolder(colFolder);
    }
}

bool KMKernel::selectFolder(const QString &folder)
{
    KMMainWidget *widget = getKMMainWidget();
    Q_ASSERT(widget);
    if (!widget) {
        return false;
    }

    const Akonadi::Collection colFolder = CommonKernel->collectionFromId(folder.toLongLong());

    if (colFolder.isValid()) {
        widget->slotSelectCollectionFolder(colFolder);
        return true;
    }
    return false;
}

KMMainWidget *KMKernel::getKMMainWidget() const
{
    // This could definitely use a speadup
    const QWidgetList l = QApplication::topLevelWidgets();

    for (QWidget *wid : l) {
        QList<KMMainWidget *> l2 = wid->window()->findChildren<KMMainWidget *>();
        if (!l2.isEmpty() && l2.first()) {
            return l2.first();
        }
    }
    return nullptr;
}

void KMKernel::slotRunBackgroundTasks() // called regularly by timer
{
    // Hidden KConfig keys. Not meant to be used, but a nice fallback in case
    // a stable kmail release goes out with a nasty bug in CompactionJob...
    if (KMailSettings::self()->autoExpiring()) {
        mFolderCollectionMonitor->expireAllFolders(false /*scheduled, not immediate*/, entityTreeModel());
    }
#if !KMAIL_FORCE_DISABLE_AKONADI_SEARCH
    if (KMailSettings::self()->checkCollectionsIndexing()) {
        mCheckIndexingManager->start(entityTreeModel());
    }
#endif
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
    mBackgroundTasksTimer->start(1m);
#else
    mBackgroundTasksTimer->start(4h);
#endif
}

static Akonadi::Collection::List collect_collections(const QAbstractItemModel *model, const QModelIndex &parent)
{
    Akonadi::Collection::List collections;
    QStack<QModelIndex> stack;
    stack.push(parent);
    while (!stack.isEmpty()) {
        const QModelIndex idx = stack.pop();
        if (idx.isValid()) {
            collections << model->data(idx, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            for (int i = model->rowCount(idx) - 1; i >= 0; --i) {
                stack.push(model->index(i, 0, idx));
            }
        }
    }

    return collections;
}

Akonadi::Collection::List KMKernel::allFolders() const
{
    return collect_collections(collectionModel(), QModelIndex());
}

Akonadi::Collection::List KMKernel::subfolders(const Akonadi::Collection &col) const
{
    const auto idx = collectionModel()->match({}, Akonadi::EntityTreeModel::CollectionRole, QVariant::fromValue(col), 1, Qt::MatchExactly);
    if (!idx.isEmpty()) {
        return collect_collections(collectionModel(), idx[0]);
    }

    return {};
}

void KMKernel::expireAllFoldersNow() // called by the GUI
{
    mFolderCollectionMonitor->expireAllFolders(true /*immediate*/, entityTreeModel());
}

bool KMKernel::canQueryClose()
{
    if (!KMMainWidget::mainWidgetList()) {
        return true;
    }
    return mUnityServiceManager->canQueryClose();
}

Akonadi::Collection KMKernel::currentCollection() const
{
    KMMainWidget *widget = getKMMainWidget();
    Akonadi::Collection col;
    if (widget) {
        col = widget->currentCollection();
    }
    return col;
}

QSharedPointer<FolderSettings> KMKernel::currentFolderCollection()
{
    KMMainWidget *widget = getKMMainWidget();
    QSharedPointer<FolderSettings> folder;
    if (widget) {
        folder = widget->currentFolder();
    }
    return folder;
}

MailCommon::MailCommonSettings *KMKernel::mailCommonSettings() const
{
    return mMailCommonSettings;
}

#if !KMAIL_FORCE_DISABLE_AKONADI_SEARCH
Akonadi::Search::PIM::IndexedItems *KMKernel::indexedItems() const
{
    return mIndexedItems;
}
#endif
// can't be inline, since KMSender isn't known to implement
// KMail::MessageSender outside this .cpp file
MessageComposer::MessageSender *KMKernel::msgSender()
{
    return the_msgSender;
}

void KMKernel::transportRemoved(int id, const QString &name)
{
    Q_UNUSED(id)

    // reset all identities using the deleted transport
    QStringList changedIdents;
    KIdentityManagementCore::IdentityManager *im = identityManager();
    KIdentityManagementCore::IdentityManager::Iterator end = im->modifyEnd();
    for (KIdentityManagementCore::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it) {
        if (name == (*it).transport()) {
            (*it).setTransport(QString());
            changedIdents += (*it).identityName();
        }
    }

    // if the deleted transport is the currently used transport reset it to default
    const QString &currentTransport = KMailSettings::self()->currentTransport();
    if (name == currentTransport) {
        KMailSettings::self()->setCurrentTransport(QString());
    }

    if (!changedIdents.isEmpty()) {
        QString information = i18np("This identity has been changed to use the default transport:",
                                    "These %1 identities have been changed to use the default transport:",
                                    changedIdents.count());
        // Don't set parent otherwise we will switch to current KMail and we configure it. So not good
        KMessageBox::informationList(nullptr, information, changedIdents);
        im->commit();
    }
}

void KMKernel::transportRenamed(int id, const QString &oldName, const QString &newName)
{
    Q_UNUSED(id)

    QStringList changedIdents;
    KIdentityManagementCore::IdentityManager *im = identityManager();
    KIdentityManagementCore::IdentityManager::Iterator end = im->modifyEnd();
    for (KIdentityManagementCore::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it) {
        if (oldName == (*it).transport()) {
            (*it).setTransport(newName);
            changedIdents << (*it).identityName();
        }
    }

    if (!changedIdents.isEmpty()) {
        const QString information = i18np("This identity has been changed to use the modified transport:",
                                          "These %1 identities have been changed to use the modified transport:",
                                          changedIdents.count());
        // Don't set parent otherwise we will switch to current KMail and we configure it. So not good
        KMessageBox::informationList(nullptr, information, changedIdents);
        im->commit();
    }
}

void KMKernel::itemDispatchStarted()
{
    // Watch progress of the MDA.
    PimCommon::ProgressManagerAkonadi::createProgressItem(nullptr,
                                                          Akonadi::DispatcherInterface().dispatcherInstance(),
                                                          QStringLiteral("Sender"),
                                                          i18n("Sending messages"),
                                                          i18n("Initiating sending process…"),
                                                          true,
                                                          KPIM::ProgressItem::Unknown);
}

void KMKernel::instanceStatusChanged(const Akonadi::AgentInstance &instance)
{
    if (instance.identifier() == "akonadi_mailfilter_agent"_L1) {
        // Creating a progress item twice is ok, it will simply return the already existing
        // item
        KPIM::ProgressItem *progress = PimCommon::ProgressManagerAkonadi::createProgressItem(nullptr,
                                                                                             instance,
                                                                                             instance.identifier(),
                                                                                             instance.name(),
                                                                                             instance.statusMessage(),
                                                                                             false,
                                                                                             KPIM::ProgressItem::Encrypted);
        progress->setProperty("AgentIdentifier", instance.identifier());
        return;
    }
    if (MailCommon::Util::agentInstances(true).contains(instance)) {
        if (instance.status() == Akonadi::AgentInstance::Running) {
            if (mResourcesBeingChecked.isEmpty()) {
                qCDebug(KMAIL_LOG) << "A Resource started to synchronize, starting a mail check.";
                Q_EMIT startCheckMail();
            }

            const QString identifier(instance.identifier());
            if (!mResourcesBeingChecked.contains(identifier)) {
                mResourcesBeingChecked.append(identifier);
            }

            KPIM::ProgressItem::CryptoStatus cryptoStatus = KPIM::ProgressItem::Unencrypted;
            if (mResourceCryptoSettingCache.contains(identifier)) {
                cryptoStatus = mResourceCryptoSettingCache.value(identifier);
            } else {
                if (PimCommon::Util::isImapResource(identifier)) {
                    MailCommon::ResourceReadConfigFile resourceFile(identifier);
                    const KConfigGroup grp = resourceFile.group(QStringLiteral("network"));
                    if (grp.isValid()) {
                        const QString imapSafety = grp.readEntry(QStringLiteral("Safety"));
                        if (imapSafety == "None"_L1) {
                            cryptoStatus = KPIM::ProgressItem::Unencrypted;
                        } else {
                            cryptoStatus = KPIM::ProgressItem::Encrypted;
                        }

                        mResourceCryptoSettingCache.insert(identifier, cryptoStatus);
                    }
                } else if (identifier.contains(POP3_RESOURCE_IDENTIFIER)) {
                    MailCommon::ResourceReadConfigFile resourceFile(identifier);
                    const KConfigGroup grp = resourceFile.group(QStringLiteral("General"));
                    if (grp.isValid()) {
                        if (grp.readEntry(QStringLiteral("useSSL"), false) || grp.readEntry(QStringLiteral("useTLS"), false)) {
                            cryptoStatus = KPIM::ProgressItem::Encrypted;
                        }
                        mResourceCryptoSettingCache.insert(identifier, cryptoStatus);
                    }
                }
            }

            // Creating a progress item twice is ok, it will simply return the already existing
            // item
            KPIM::ProgressItem *progress = PimCommon::ProgressManagerAkonadi::createProgressItem(nullptr,
                                                                                                 instance,
                                                                                                 instance.identifier(),
                                                                                                 instance.name(),
                                                                                                 instance.statusMessage(),
                                                                                                 true,
                                                                                                 cryptoStatus);
            progress->setProperty("AgentIdentifier", instance.identifier());
        } else if (instance.status() == Akonadi::AgentInstance::Broken) {
            agentInstanceBroken(instance);
        }
    }
}

void KMKernel::agentInstanceBroken(const Akonadi::AgentInstance &instance)
{
    const QString summary = i18n("Resource %1 is broken.\n%2", instance.name(), instance.statusMessage());
    KNotification::event(QStringLiteral("akonadi-resource-broken"), QString(), summary, QStringLiteral("kmail"), KNotification::CloseOnTimeout);
}

void KMKernel::slotProgressItemCompletedOrCanceled(KPIM::ProgressItem *item)
{
    const QString identifier = item->property("AgentIdentifier").toString();
    const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(identifier);
    if (agent.isValid()) {
        mResourcesBeingChecked.removeAll(identifier);
        if (mResourcesBeingChecked.isEmpty()) {
            qCDebug(KMAIL_LOG) << "Last resource finished syncing, mail check done";
            Q_EMIT endCheckMail();
        }
    }
}

void KMKernel::updatedTemplates()
{
    Q_EMIT customTemplatesChanged();
}

void KMKernel::cleanupTemporaryFiles()
{
    QDir dir(QDir::tempPath());
    const QStringList lst = dir.entryList(QStringList{QStringLiteral("messageviewer_*")});
    qCDebug(KMAIL_LOG) << " list file to delete " << lst;
    for (const QString &file : lst) {
        QFile tempFile(QDir::tempPath() + QLatin1Char('/') + file);
        if (!tempFile.remove()) {
            fprintf(stderr, "%s was not removed .\n", qPrintable(tempFile.fileName()));
        } else {
            fprintf(stderr, "%s was removed .\n", qPrintable(tempFile.fileName()));
        }
    }
    const QStringList lstRepo = dir.entryList(QStringList{QStringLiteral("messageviewer_*.index.*")});
    qCDebug(KMAIL_LOG) << " list repo to delete " << lstRepo;
    for (const QString &file : lstRepo) {
        QDir tempDir(QDir::tempPath() + QLatin1Char('/') + file);
        if (!tempDir.removeRecursively()) {
            fprintf(stderr, "%s was not removed .\n", qPrintable(tempDir.path()));
        } else {
            fprintf(stderr, "%s was removed .\n", qPrintable(tempDir.path()));
        }
    }
}

void KMKernel::stopAgentInstance()
{
    Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance &agent : lst) {
        const QString identifier = agent.identifier();
        const KConfigGroup group = resourceConfigGroup(identifier);

        // Keep sync in ConfigureDialog, don't forget to change there.
        if (group.readEntry("OfflineOnShutdown", identifier.startsWith("akonadi_pop3_resource"_L1) ? true : false)) {
            agent.setIsOnline(false);
        }
    }
}

void KMKernel::slotCollectionRemoved(const Akonadi::Collection &col)
{
    KConfigGroup group(KMKernel::config(), MailCommon::FolderSettings::configGroupName(col));
    group.deleteGroup();
    group.sync();
    const QString colStr = QString::number(col.id());
    TemplateParser::Util::deleteTemplate(colStr);
    MessageList::Util::deleteConfig(colStr);
}

void KMKernel::slotDeleteIdentity(uint identity)
{
    TemplateParser::Util::deleteTemplate(QStringLiteral("IDENTITY_%1").arg(identity));
}

bool KMKernel::showPopupAfterDnD()
{
    return KMailSettings::self()->showPopupAfterDnD();
}

bool KMKernel::excludeImportantMailFromExpiry()
{
    return KMailSettings::self()->excludeImportantMailFromExpiry();
}

qreal KMKernel::closeToQuotaThreshold()
{
    return KMailSettings::self()->closeToQuotaThreshold();
}

Akonadi::Collection::Id KMKernel::lastSelectedFolder()
{
    return KMailSettings::self()->lastSelectedFolder();
}

void KMKernel::setLastSelectedFolder(Akonadi::Collection::Id col)
{
    KMailSettings::self()->setLastSelectedFolder(col);
}

QStringList KMKernel::customTemplates()
{
    return GlobalSettingsBase::self()->customTemplates();
}

void KMKernel::openFilterDialog(bool createDummyFilter)
{
    if (!mFilterEditDialog) {
        mFilterEditDialog = new MailCommon::KMFilterDialog(getKMMainWidget()->actionCollections(), nullptr, createDummyFilter);
        mFilterEditDialog->setObjectName("filterdialog"_L1);
    }
    mFilterEditDialog->show();
    mFilterEditDialog->raise();
    mFilterEditDialog->activateWindow();
}

void KMKernel::createFilter(const QByteArray &field, const QString &value)
{
    mFilterEditDialog->createFilter(field, value);
}

void KMKernel::checkFolderFromResources(const Akonadi::Collection::List &collectionList)
{
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (const Akonadi::AgentInstance &type : lst) {
        if (type.status() == Akonadi::AgentInstance::Broken) {
            continue;
        }
        const QString typeIdentifier(type.identifier());
        if (PimCommon::Util::isImapResource(typeIdentifier)) {
            OrgKdeAkonadiImapSettingsInterface *iface = PimCommon::Util::createImapSettingsInterface(typeIdentifier);
            if (iface && iface->isValid()) {
                const Akonadi::Collection::Id imapTrashId = iface->trashCollection();
                for (const Akonadi::Collection &collection : collectionList) {
                    const Akonadi::Collection::Id collectionId = collection.id();
                    if (imapTrashId == collectionId) {
                        // Use default trash
                        iface->setTrashCollection(CommonKernel->trashCollectionFolder().id());
                        iface->save();
                        break;
                    }
                }
            }
            delete iface;
        } else if (typeIdentifier.contains(POP3_RESOURCE_IDENTIFIER)) {
            OrgKdeAkonadiPOP3SettingsInterface *iface = MailCommon::Util::createPop3SettingsInterface(typeIdentifier);
            if (iface->isValid()) {
                for (const Akonadi::Collection &collection : std::as_const(collectionList)) {
                    const Akonadi::Collection::Id collectionId = collection.id();
                    if (iface->targetCollection() == collectionId) {
                        // Use default inbox
                        iface->setTargetCollection(CommonKernel->inboxCollectionFolder().id());
                        iface->save();
                        break;
                    }
                }
            }
            delete iface;
        }
    }
}

const QAbstractItemModel *KMKernel::treeviewModelSelection()
{
    if (getKMMainWidget()) {
        return getKMMainWidget()->folderTreeView()->selectionModel()->model();
    } else {
        return entityTreeModel();
    }
}

void KMKernel::slotInstanceWarning(const Akonadi::AgentInstance &instance, const QString &message)
{
    const QString summary = i18nc("<source>: <error message>", "%1: %2", instance.name(), message);
    KNotification::event(QStringLiteral("akonadi-instance-warning"), QString(), summary, QStringLiteral("kmail"), KNotification::CloseOnTimeout);
}

void KMKernel::slotInstanceError(const Akonadi::AgentInstance &instance, const QString &message)
{
    const QString summary = i18nc("<source>: <error message>", "%1: %2", instance.name(), message);
    KNotification::event(QStringLiteral("akonadi-instance-error"), QString(), summary, QStringLiteral("kmail"), KNotification::CloseOnTimeout);
}

void KMKernel::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    const QString identifier(instance.identifier());
    const QString resourceGroup = QStringLiteral("Resource %1").arg(identifier);
    if (KMKernel::config()->hasGroup(resourceGroup)) {
        KConfigGroup group(KMKernel::config(), resourceGroup);
        group.deleteGroup();
        group.sync();
    }
    if (mResourceCryptoSettingCache.contains(identifier)) {
        mResourceCryptoSettingCache.remove(identifier);
    }
    mFolderArchiveManager->slotInstanceRemoved(instance);

    if (MailCommon::Util::isMailAgent(instance)) {
        Q_EMIT incomingAccountsChanged();
    }
}

void KMKernel::slotInstanceAdded(const Akonadi::AgentInstance &instance)
{
    if (MailCommon::Util::isMailAgent(instance)) {
        Q_EMIT incomingAccountsChanged();
    }
}

void KMKernel::savePaneSelection()
{
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        widget->savePaneSelection();
    }
}

void KMKernel::updatePaneTagComboBox()
{
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        widget->updatePaneTagComboBox();
    }
}

void KMKernel::resourceGoOnLine()
{
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        if (widget->currentCollection().isValid()) {
            Akonadi::Collection collection = widget->currentCollection();
            Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(collection.resource());
            instance.setIsOnline(true);
            widget->refreshMessageListSelection();
        }
    }
}

void KMKernel::makeResourceOnline(MessageViewer::Viewer::ResourceOnlineMode mode)
{
    switch (mode) {
    case MessageViewer::Viewer::AllResources:
        resumeNetworkJobs();
        break;
    case MessageViewer::Viewer::SelectedResource:
        resourceGoOnLine();
        break;
    }
}
TextAutoCorrectionCore::AutoCorrection *KMKernel::composerAutoCorrection()
{
    return mAutoCorrection;
}

void KMKernel::toggleSystemTray()
{
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        mUnityServiceManager->toggleSystemTray(widget);
    }
}

void KMKernel::showFolder(const QString &collectionId)
{
    if (!collectionId.isEmpty()) {
        const Akonadi::Collection::Id id = collectionId.toLongLong();
        selectCollectionFromId(id);
    }
}

void KMKernel::reloadFolderArchiveConfig()
{
    mFolderArchiveManager->reloadConfig();
}

bool KMKernel::replyMail(qint64 serialNumber, bool replyToAll)
{
    KMMainWidget *mainWidget = nullptr;

    // First look for a KMainWindow.
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        // Then look for a KMMainWidget.
        QList<KMMainWidget *> l = window->findChildren<KMMainWidget *>();
        if (!l.isEmpty() && l.first()) {
            mainWidget = l.first();
            if (window->isActiveWindow()) {
                break;
            }
        }
    }
    if (mainWidget) {
        auto job = new Akonadi::ItemFetchJob(Akonadi::Item(serialNumber), this);
        job->fetchScope().fetchFullPayload();
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        if (job->exec()) {
            if (job->items().count() >= 1) {
                const auto item = job->items().at(0);
                mainWidget->replyMessageTo(item, replyToAll);
                return true;
            }
        }
    }
    return false;
}

void KMKernel::slotCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &set)
{
    if (set.contains("newmailnotifierattribute")) {
        mUnityServiceManager->initListOfCollection();
    }
}

FolderArchiveManager *KMKernel::folderArchiveManager() const
{
    return mFolderArchiveManager;
}

bool KMKernel::allowToDebug() const
{
    return mDebug;
}

bool KMKernel::firstStart() const
{
    return the_firstStart;
}

bool KMKernel::shuttingDown() const
{
    return the_shuttingDown;
}

void KMKernel::setShuttingDown(bool flag)
{
    the_shuttingDown = flag;
}

void KMKernel::expunge(Akonadi::Collection::Id col, bool sync)
{
    Q_UNUSED(col)
    Q_UNUSED(sync)
}

#if KMAIL_WITH_KUSERFEEDBACK
KUserFeedback::Provider *KMKernel::userFeedbackProvider() const
{
    return mUserFeedbackProvider;
}

#endif

#include "moc_kmkernel.cpp"
