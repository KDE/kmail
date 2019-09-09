/*   */

#include "kmkernel.h"

#include "settings/kmailsettings.h"
#include "libkdepim/broadcaststatus.h"
#include "job/opencomposerjob.h"
#include "job/newmessagejob.h"
#include "job/opencomposerhiddenjob.h"
#include "job/fillcomposerjob.h"
#include <AkonadiSearch/PIM/indexeditems.h>
#include <LibkdepimAkonadi/ProgressManagerAkonadi>
using KPIM::BroadcastStatus;
#include "kmstartup.h"
#include "kmmainwin.h"
#include "editor/composer.h"
#include "kmreadermainwin.h"
#include "undostack.h"
#include "kmmainwidget.h"

#include "search/checkindexingmanager.h"
#include "libkdepim/recentaddresses.h"
using KPIM::RecentAddresses;
#include "configuredialog/configuredialog.h"
#include "kmcommands.h"
#include "unityservicemanager.h"
#include <MessageCore/StringUtil>
#include "mailcommon/mailutil.h"
#include "pop3settings.h"
#include "MailCommon/FolderTreeView"
#include "MailCommon/KMFilterDialog"
#include "mailcommonsettings_base.h"
#include "mailfilteragentinterface.h"
#include "PimCommon/PimUtil"
#include "folderarchive/folderarchivemanager.h"
#include "sieveimapinterface/kmailsieveimapinstanceinterface.h"
// kdepim includes
#include "kmail-version.h"

// kdepimlibs includes
#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransportakonadi/dispatcherinterface.h>

#include <KSieveUi/SieveImapInstanceInterfaceManager>
#include "mailserviceimpl.h"
using KMail::MailServiceImpl;
#include "mailcommon/jobscheduler.h"

#include <MessageCore/MessageCoreSettings>
#include "messagelistsettings.h"
#include "gravatarsettings.h"
#include "messagelist/messagelistutil.h"
#include "messageviewer/messageviewersettings.h"
#include "MessageComposer/AkonadiSender"
#include "messagecomposer/messagecomposersettings.h"
#include "MessageComposer/MessageHelper"
#include "MessageComposer/MessageComposerSettings"
#include "PimCommon/PimCommonSettings"
#include "PimCommon/AutoCorrection"
#include <PimCommon/NetworkManager>

#include "globalsettings_templateparser.h"
#include "TemplateParser/TemplatesUtil"

#include "mailcommon/foldersettings.h"
#include "editor/codec/codecmanager.h"

#include <kmessagebox.h>
#include <knotification.h>
#include <libkdepim/progressmanager.h>

#include <kconfig.h>
#include <kpassivepopup.h>
#include <kconfiggroup.h>
#include "kmail_debug.h"
#include <kio/jobuidelegate.h>
#include <kprocess.h>
#include <KCrash>

#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>
#include <AkonadiCore/Collection>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/Session>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/CollectionStatisticsJob>
#include <AkonadiCore/ServerManager>

#include <QNetworkConfigurationManager>
#include <QDir>
#include <QWidget>
#include <QFileInfo>
#include <QtDBus>

#include <MailCommon/ResourceReadConfigFile>

#include <kstartupinfo.h>
#include <kmailadaptor.h>
#include <KLocalizedString>
#include <QStandardPaths>
#include "kmailinterface.h"
#include "mailcommon/foldercollectionmonitor.h"
#include "imapresourcesettings.h"
#include "util.h"
#include "MailCommon/MailKernel"

#include "searchdialog/searchdescriptionattribute.h"
#include "kmail_options.h"

using namespace MailCommon;

static KMKernel *mySelf = nullptr;
static bool s_askingToGoOnline = false;
/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel(QObject *parent)
    : QObject(parent)
{
    //Initialize kmail sieveimap interface
    KSieveUi::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    mDebug = !qEnvironmentVariableIsEmpty("KDEPIM_DEBUGGING");

    mSystemNetworkStatus = PimCommon::NetworkManager::self()->networkConfigureManager()->isOnline();

    Akonadi::AttributeFactory::registerAttribute<Akonadi::SearchDescriptionAttribute>();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kmail"));
    qCDebug(KMAIL_LOG) << "Starting up...";

    mySelf = this;
    the_firstInstance = true;

    the_undoStack = nullptr;
    the_msgSender = nullptr;
    mFilterEditDialog = nullptr;
    // make sure that we check for config updates before doing anything else
    KMKernel::config();
    // this shares the kmailrc parsing too (via KSharedConfig), and reads values from it
    // so better do it here, than in some code where changing the group of config()
    // would be unexpected
    KMailSettings::self();

    mJobScheduler = new JobScheduler(this);

    mAutoCorrection = new PimCommon::AutoCorrection();
    KMime::setUseOutlookAttachmentEncoding(MessageComposer::MessageComposerSettings::self()->outlookCompatibleAttachments());

    // cberzan: this crap moved to CodecManager ======================
    mNetCodec = QTextCodec::codecForLocale();

    // In the case of Japan. Japanese locale name is "eucjp" but
    // The Japanese mail systems normally used "iso-2022-jp" of locale name.
    // We want to change locale name from eucjp to iso-2022-jp at KMail only.

    // (Introduction to i18n, 6.6 Limit of Locale technology):
    // EUC-JP is the de-facto standard for UNIX systems, ISO 2022-JP
    // is the standard for Internet, and Shift-JIS is the encoding
    // for Windows and Macintosh.
    const QByteArray netCodecLower = mNetCodec->name().toLower();
    if (netCodecLower == "eucjp"
#if defined Q_OS_WIN || defined Q_OS_MACX
        || netCodecLower == "shift-jis"     // OK?
#endif
        ) {
        mNetCodec = QTextCodec::codecForName("jis7");
    }
    // until here ================================================

    Akonadi::Session *session = new Akonadi::Session("KMail Kernel ETM", this);

    mFolderCollectionMonitor = new FolderCollectionMonitor(session, this);

    connect(mFolderCollectionMonitor->monitor(), &Akonadi::Monitor::collectionRemoved, this, &KMKernel::slotCollectionRemoved);

    mEntityTreeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor(), this);
    mEntityTreeModel->setListFilter(Akonadi::CollectionFetchScope::Enabled);
    mEntityTreeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    mCollectionModel = new Akonadi::EntityMimeTypeFilterModel(this);
    mCollectionModel->setSourceModel(mEntityTreeModel);
    mCollectionModel->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mCollectionModel->setHeaderGroup(Akonadi::EntityTreeModel::CollectionTreeHeaders);
    mCollectionModel->setDynamicSortFilter(true);
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    connect(folderCollectionMonitor(), qOverload<const Akonadi::Collection &, const QSet<QByteArray> &>(&Akonadi::ChangeRecorder::collectionChanged), this,
            &KMKernel::slotCollectionChanged);

    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRemoved, this, &KMKernel::transportRemoved);
    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRenamed, this, &KMKernel::transportRenamed);

    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/MailDispatcherAgent"), QStringLiteral("org.freedesktop.Akonadi.MailDispatcherAgent"), QStringLiteral(
                                              "itemDispatchStarted"), this, SLOT(itemDispatchStarted()));
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceStatusChanged, this, &KMKernel::instanceStatusChanged);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceError, this, &KMKernel::slotInstanceError);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceWarning, this, &KMKernel::slotInstanceWarning);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &KMKernel::slotInstanceRemoved);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceAdded, this, &KMKernel::slotInstanceAdded);

    connect(PimCommon::NetworkManager::self()->networkConfigureManager(), &QNetworkConfigurationManager::onlineStateChanged,
            this, &KMKernel::slotSystemNetworkStatusChanged);

    connect(KPIM::ProgressManager::instance(), &KPIM::ProgressManager::progressItemCompleted,
            this, &KMKernel::slotProgressItemCompletedOrCanceled);
    connect(KPIM::ProgressManager::instance(), &KPIM::ProgressManager::progressItemCanceled,
            this, &KMKernel::slotProgressItemCompletedOrCanceled);
    connect(identityManager(), &KIdentityManagement::IdentityManager::deleted, this, &KMKernel::slotDeleteIdentity);
    CommonKernel->registerKernelIf(this);
    CommonKernel->registerSettingsIf(this);
    CommonKernel->registerFilterIf(this);
    mFolderArchiveManager = new FolderArchiveManager(this);
    mIndexedItems = new Akonadi::Search::PIM::IndexedItems(this);
    mCheckIndexingManager = new CheckIndexingManager(mIndexedItems, this);
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

static QUrl makeAbsoluteUrl(const QString &str, const QString &cwd)
{
    return QUrl::fromUserInput(str, cwd, QUrl::AssumeLocalFile);
}

bool KMKernel::handleCommandLine(bool noArgsOpensReader, const QStringList &args, const QString &workingDir)
{
    QString to, cc, bcc, subj, body, inReplyTo, replyTo;
    QStringList customHeaders;
    QUrl messageFile;
    QList<QUrl> attachURLs;
    QString identity;
    bool startInTray = false;
    bool mailto = false;
    bool checkMail = false;
    bool viewOnly = false;
    bool calledWithSession = false; // for ignoring '-session foo'

    // process args:
    QCommandLineParser parser;
    kmail_options(&parser);
    QStringList newargs;
    bool addAttachmentAttribute = false;
    for (const QString &argument : qAsConst(args)) {
        if (argument == QLatin1String("--attach")) {
            addAttachmentAttribute = true;
        } else {
            if (argument.startsWith(QLatin1String("--"))) {
                addAttachmentAttribute = false;
            }
            if (argument.contains(QLatin1Char('@')) || argument.startsWith(QLatin1String("mailto:"))) { //address mustn't be trade as a attachment
                addAttachmentAttribute = false;
            }
            if (addAttachmentAttribute) {
                newargs.append(QStringLiteral("--attach"));
                newargs.append(argument);
            } else {
                newargs.append(argument);
            }
        }
    }

    parser.process(newargs);
    if (parser.isSet(QStringLiteral("subject"))) {
        subj = parser.value(QStringLiteral("subject"));
        // if kmail is called with 'kmail -session abc' then this doesn't mean
        // that the user wants to send a message with subject "ession" but
        // (most likely) that the user clicked on KMail's system tray applet
        // which results in KMKernel::raise() calling "kmail kmail newInstance"
        // via D-Bus which apparently executes the application with the original
        // command line arguments and those include "-session ..." if
        // kmail/kontact was restored by session management
        if (subj == QLatin1String("ession")) {
            subj.clear();
            calledWithSession = true;
        } else {
            mailto = true;
        }
    }

    if (parser.isSet(QStringLiteral("cc"))) {
        mailto = true;
        cc = parser.value(QStringLiteral("cc"));
    }

    if (parser.isSet(QStringLiteral("bcc"))) {
        mailto = true;
        bcc = parser.value(QStringLiteral("bcc"));
    }

    if (parser.isSet(QStringLiteral("replyTo"))) {
        mailto = true;
        replyTo = parser.value(QStringLiteral("replyTo"));
    }

    if (parser.isSet(QStringLiteral("msg"))) {
        mailto = true;
        const QString file = parser.value(QStringLiteral("msg"));
        messageFile = makeAbsoluteUrl(file, workingDir);
    }

    if (parser.isSet(QStringLiteral("body"))) {
        mailto = true;
        body = parser.value(QStringLiteral("body"));
    }

    const QStringList attachList = parser.values(QStringLiteral("attach"));
    if (!attachList.isEmpty()) {
        mailto = true;
        QStringList::ConstIterator end = attachList.constEnd();
        for (QStringList::ConstIterator it = attachList.constBegin();
             it != end; ++it) {
            if (!(*it).isEmpty()) {
                if ((*it) != QLatin1String("--")) {
                    attachURLs.append(makeAbsoluteUrl(*it, workingDir));
                }
            }
        }
    }

    customHeaders = parser.values(QStringLiteral("header"));

    if (parser.isSet(QStringLiteral("composer"))) {
        mailto = true;
    }

    if (parser.isSet(QStringLiteral("check"))) {
        checkMail = true;
    }

    if (parser.isSet(QStringLiteral("startintray"))) {
        KMailSettings::self()->setSystemTrayEnabled(true);
        startInTray = true;
    }

    if (parser.isSet(QStringLiteral("identity"))) {
        identity = parser.value(QStringLiteral("identity"));
    }

    if (parser.isSet(QStringLiteral("view"))) {
        viewOnly = true;
        const QString filename
            = parser.value(QStringLiteral("view"));
        messageFile = QUrl::fromUserInput(filename, workingDir);
    }

    if (!calledWithSession) {
        // only read additional command line arguments if kmail/kontact is
        // not called with "-session foo"
        for (const QString &arg : parser.positionalArguments()) {
            if (arg.startsWith(QLatin1String("mailto:"), Qt::CaseInsensitive)) {
                const QUrl urlDecoded(QUrl::fromPercentEncoding(arg.toUtf8()));
                const QList<QPair<QString, QString> > values = MessageCore::StringUtil::parseMailtoUrl(urlDecoded);
                QString previousKey;
                for (int i = 0; i < values.count(); ++i) {
                    const QPair<QString, QString> element = values.at(i);
                    const QString key = element.first.toLower();
                    if (key == QLatin1String("to")) {
                        if (!element.second.isEmpty()) {
                            to += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == QLatin1String("cc")) {
                        if (!element.second.isEmpty()) {
                            cc += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == QLatin1String("bcc")) {
                        if (!element.second.isEmpty()) {
                            bcc += element.second + QStringLiteral(", ");
                        }
                        previousKey.clear();
                    } else if (key == QLatin1String("subject")) {
                        subj = element.second;
                        previousKey.clear();
                    } else if (key == QLatin1String("body")) {
                        body = element.second;
                        previousKey = key;
                    } else if (key == QLatin1String("in-reply-to")) {
                        inReplyTo = element.second;
                        previousKey.clear();
                    } else if (key == QLatin1String("attachment") || key == QLatin1String("attach")) {
                        if (!element.second.isEmpty()) {
                            attachURLs << makeAbsoluteUrl(element.second, workingDir);
                        }
                        previousKey.clear();
                    } else {
                        qCWarning(KMAIL_LOG) << "unknown key" << key;
                        //Workaround: https://bugs.kde.org/show_bug.cgi?id=390939
                        //QMap<QString, QString> parseMailtoUrl(const QUrl &url) parses correctly url
                        //But if we have a "&" unknown key we lost it.
                        if (previousKey == QLatin1String("body")) {
                            body += QLatin1Char('&') + key + QLatin1Char('=') + element.second;
                        }
                        //Don't clear previous key.
                    }
                }
            } else {
                QUrl url(arg);
                if (url.isValid() && !url.scheme().isEmpty()) {
                    attachURLs += url;
                } else {
                    to += arg + QStringLiteral(", ");
                }
            }
            mailto = true;
        }
        if (!to.isEmpty()) {
            // cut off the superfluous trailing ", "
            to.truncate(to.length() - 2);
        }
    }

    if (!noArgsOpensReader && !mailto && !checkMail && !viewOnly) {
        return false;
    }

    if (viewOnly) {
        viewMessage(messageFile);
    } else {
        action(mailto, checkMail, startInTray, to, cc, bcc, subj, body, messageFile,
               attachURLs, customHeaders, replyTo, inReplyTo, identity);
    }
    return true;
}

/********************************************************************/
/*             D-Bus-callable, and command line actions              */
/********************************************************************/
void KMKernel::checkMail()  //might create a new reader but won't show!!
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    const QString resourceGroupPattern(QStringLiteral("Resource %1"));

    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance type : lst) {
        const QString id = type.identifier();
        KConfigGroup group(KMKernel::config(), resourceGroupPattern.arg(id));
        if (group.readEntry("IncludeInManualChecks", true)) {
            if (!type.isOnline()) {
                type.setIsOnline(true);
            }
            if (mResourcesBeingChecked.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Starting manual mail check";
                Q_EMIT startCheckMail();
            }

            if (!mResourcesBeingChecked.contains(id)) {
                mResourcesBeingChecked.append(id);
            }
            type.synchronize();
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

void KMKernel::checkAccount(const QString &account)   //might create a new reader but won't show!!
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
        KMMainWin *win = static_cast<KMMainWin *>(ktmw);
        activate = !onlyCheck; // existing window: only activate if not --check
        if (activate) {
            win->show();
        }
    } else {
        KMMainWin *win = new KMMainWin;
        if (!startInTray && !KMailSettings::self()->startInTray()) {
            win->show();
        }
        activate = false; // new window: no explicit activation (#73591)
    }
}

void KMKernel::openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden, const QString &messageFile, const QStringList &attachmentPaths, const QStringList &customHeaders, const QString &replyTo, const QString &inReplyTo, const QString &identity)
{
    const OpenComposerSettings settings(to, cc,
                                        bcc, subject,
                                        body, hidden,
                                        messageFile,
                                        attachmentPaths,
                                        customHeaders,
                                        replyTo, inReplyTo, identity);
    OpenComposerJob *job = new OpenComposerJob(this);
    job->setOpenComposerSettings(settings);
    job->start();
}

void KMKernel::openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden, const QString &attachName, const QByteArray &attachCte, const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue, const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity)
{
    fillComposer(hidden, to, cc, bcc,
                 subject, body,
                 attachName, attachCte, attachData,
                 attachType, attachSubType, attachParamAttr, attachParamValue,
                 attachContDisp, attachCharset, identity, false);
}

void KMKernel::openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, const QString &attachName, const QByteArray &attachCte, const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue, const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity)
{
    fillComposer(false, to, cc, bcc,
                 subject, body,
                 attachName, attachCte, attachData,
                 attachType, attachSubType, attachParamAttr, attachParamValue,
                 attachContDisp, attachCharset, identity, true);
}

void KMKernel::fillComposer(bool hidden, const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, const QString &attachName, const QByteArray &attachCte, const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue, const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity, bool forceShowWindow)
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
                                           forceShowWindow);
    FillComposerJob *job = new FillComposerJob;
    job->setSettings(settings);
    job->start();
}

void KMKernel::openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden)
{
    const OpenComposerHiddenJobSettings settings(to, cc,
                                                 bcc,
                                                 subject,
                                                 body, hidden);
    OpenComposerHiddenJob *job = new OpenComposerHiddenJob(this);
    job->setSettings(settings);
    job->start();
}

void KMKernel::newMessage(const QString &to, const QString &cc, const QString &bcc, bool hidden, bool useFolderId, const QString & /*messageFile*/, const QString &_attachURL)
{
    QSharedPointer<FolderSettings> folder;
    Akonadi::Collection col;
    uint id = 0;
    if (useFolderId) {
        //create message with required folder identity
        folder = currentFolderCollection();
        id = folder ? folder->identity() : 0;
        col = currentCollection();
    }

    const NewMessageJobSettings settings(to,
                                         cc,
                                         bcc,
                                         hidden,
                                         _attachURL,
                                         folder,
                                         id,
                                         col);

    NewMessageJob *job = new NewMessageJob(this);
    job->setNewMessageJobSettings(settings);
    job->start();
}

void KMKernel::viewMessage(const QUrl &url)
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(nullptr, url);

    openCommand->start();
}

int KMKernel::viewMessage(const QString &messageFile)
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(nullptr, QUrl::fromLocalFile(messageFile));

    openCommand->start();
    return 1;
}

void KMKernel::raise()
{
    QDBusInterface iface(QStringLiteral("org.kde.kmail"), QStringLiteral("/MainApplication"),
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
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(Akonadi::Item(serialNumber), this);
        job->fetchScope().fetchFullPayload();
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        if (job->exec()) {
            if (job->items().count() >= 1) {
                KMReaderMainWin *win = new KMReaderMainWin(MessageViewer::Viewer::UseGlobalSetting, false);
                const auto item = job->items().at(0);
                win->showMessage(MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding(),
                                 item, item.parentCollection());
                win->show();
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
    mBackgroundTasksTimer->start(4 * 60 * 60 * 1000);
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
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances(false);
    for (Akonadi::AgentInstance type : lst) {
        const QString identifier(type.identifier());
        if (PimCommon::Util::isImapResource(identifier)
            || identifier.contains(POP3_RESOURCE_IDENTIFIER)
            || identifier.contains(QLatin1String("akonadi_maildispatcher_agent"))
            || type.type().capabilities().contains(QLatin1String("NeedsNetwork"))) {
            type.setIsOnline(goOnline);
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
                                     KMKernel::self()->mainWin(),
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
    if ((KMailSettings::self()->networkState() == KMailSettings::EnumNetworkState::Offline)
        || !PimCommon::NetworkManager::self()->networkConfigureManager()->isOnline()) {
        return true;
    } else {
        return false;
    }
}

void KMKernel::verifyAccount()
{
    const QString resourceGroupPattern(QStringLiteral("Resource %1"));

    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance type : lst) {
        KConfigGroup group(KMKernel::config(), resourceGroupPattern.arg(type.identifier()));
        if (group.readEntry("CheckOnStartup", false)) {
            if (!type.isOnline()) {
                type.setIsOnline(true);
            }
            type.synchronize();
        }

        // "false" is also hardcoded in ConfigureDialog, don't forget to change there.
        if (group.readEntry("OfflineOnShutdown", false)) {
            if (!type.isOnline()) {
                type.setIsOnline(true);
            }
        }
    }
}

void KMKernel::slotCheckAccount(Akonadi::ServerManager::State state)
{
    if (state == Akonadi::ServerManager::Running) {
        disconnect(Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)));
        verifyAccount();
    }
}

void KMKernel::checkMailOnStartup()
{
    if (!kmkernel->askToGoOnline()) {
        return;
    }

    if (Akonadi::ServerManager::state() != Akonadi::ServerManager::Running) {
        connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, &KMKernel::slotCheckAccount);
    } else {
        verifyAccount();
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
        int rc
            = KMessageBox::questionYesNo(KMKernel::self()->mainWin(),
                                         i18n("KMail is currently in offline mode. "
                                              "How do you want to proceed?"),
                                         i18n("Online/Offline"),
                                         KGuiItem(i18n("Work Online")),
                                         KGuiItem(i18n("Work Offline")));

        s_askingToGoOnline = false;
        if (rc == KMessageBox::No) {
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
        BroadcastStatus::instance()->setStatusMsg(i18n(
                                                      "Network connection detected, all network jobs resumed"));
        kmkernel->setAccountStatus(true);
    } else {
        BroadcastStatus::instance()->setStatusMsg(i18n(
                                                      "No network connection detected, all network jobs are suspended"));
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
   Asuming that:
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
    const QString pathName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail2/");
    QDir dir(pathName);
    if (!dir.exists(QStringLiteral("autosave"))) {
        return;
    }

    dir.cd(pathName + QLatin1String("autosave"));
    const QFileInfoList autoSaveFiles = dir.entryInfoList();
    for (const QFileInfo &file : autoSaveFiles) {
        // Disregard the '.' and '..' folders
        const QString filename = file.fileName();
        if (filename == QLatin1String(".")
            || filename == QLatin1String("..")
            || file.isDir()) {
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
            KMessageBox::sorry(nullptr, i18n("Failed to open autosave file at %1.\nReason: %2",
                                             file.absoluteFilePath(), autoSaveFile.errorString()),
                               i18n("Opening Autosave File Failed"));
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

void KMKernel::init()
{
    the_shuttingDown = false;

    the_firstStart = KMailSettings::self()->firstStart();
    KMailSettings::self()->setFirstStart(false);

    the_undoStack = new KMail::UndoStack(20);

    the_msgSender = new MessageComposer::AkonadiSender;
    // filterMgr->dump();

    mBackgroundTasksTimer = new QTimer(this);
    mBackgroundTasksTimer->setSingleShot(true);
    connect(mBackgroundTasksTimer, &QTimer::timeout, this, &KMKernel::slotRunBackgroundTasks);
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
    mBackgroundTasksTimer->start(10000);   // 10s, singleshot
#else
    mBackgroundTasksTimer->start(5 * 60000);   // 5 minutes, singleshot
#endif

    KCrash::setEmergencySaveFunction(kmCrashHandler);

    qCDebug(KMAIL_LOG) << "KMail init with akonadi server state:" << int(Akonadi::ServerManager::state());
    if (Akonadi::ServerManager::state() == Akonadi::ServerManager::Running) {
        CommonKernel->initFolders();
    }

    connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, &KMKernel::akonadiStateChanged);
}

bool KMKernel::doSessionManagement()
{
    // Do session management
    if (qApp->isSessionRestored()) {
        int n = 1;
        while (KMMainWin::canBeRestored(n)) {
            //only restore main windows! (Matthias);
            if (KMMainWin::classNameOfToplevel(n) == QLatin1String("KMMainWin")) {
                (new KMMainWin)->restoreDockedState(n);
            }
            ++n;
        }
        return true; // we were restored by SM
    }
    return false;  // no, we were not restored
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
        if (::qobject_cast<KMMainWin *>(window)
            || ::qobject_cast<KMail::SecondaryWindow *>(window)) {
            // close and delete the window
            window->setAttribute(Qt::WA_DeleteOnClose);
            window->close();
        }
    }
}

void KMKernel::cleanup()
{
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceStatusChanged(Akonadi::AgentInstance)));
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceError(Akonadi::AgentInstance,QString)));
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceWarning(Akonadi::AgentInstance,QString)));
    disconnect(Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)));
    disconnect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCompleted(KPIM::ProgressItem*)));
    disconnect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCanceled(KPIM::ProgressItem*)));

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
        return;    //All documents should be saved before shutting down is set!
    }

    // make all composer windows autosave their contents
    const auto lst = KMainWindow::memberList();
    for (KMainWindow *window : lst) {
        if (KMail::Composer *win = ::qobject_cast<KMail::Composer *>(window)) {
            win->autoSaveMessage(true);

            while (win->isComposing()) {
                qCWarning(KMAIL_LOG) << "Danger, using an event loop, this should no longer be happening!";
                qApp->processEvents();
            }
        }
    }
}

void KMKernel::action(bool mailto, bool check, bool startInTray, const QString &to, const QString &cc, const QString &bcc, const QString &subj, const QString &body, const QUrl &messageFile, const QList<QUrl> &attachURLs, const QStringList &customHeaders, const QString &replyTo, const QString &inReplyTo, const QString &identity)
{
    if (mailto) {
        openComposer(to, cc, bcc, subj, body, 0,
                     messageFile.toLocalFile(), QUrl::toStringList(attachURLs),
                     customHeaders, replyTo, inReplyTo, identity);
    } else {
        openReader(check, startInTray);
    }

    if (check) {
        checkMail();
    }
    //Anything else?
}

void KMKernel::slotRequestConfigSync()
{
    // ### FIXME: delay as promised in the kdoc of this function ;-)
    slotSyncConfig();
}

void KMKernel::slotSyncConfig()
{
    saveConfig();
    //Laurent investigate why we need to reload them.
    PimCommon::PimCommonSettings::self()->load();
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
    PimCommon::PimCommonSettings::self()->save();
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
        KMMainWin *win = new KMMainWin;
        win->show();
    }

    if (!mConfigureDialog) {
        mConfigureDialog = new ConfigureDialog(nullptr, false);
        mConfigureDialog->setObjectName(QStringLiteral("configure"));
        connect(mConfigureDialog, &ConfigureDialog::configChanged, this, &KMKernel::slotConfigChanged);
    }

    // Save all current settings.
    if (getKMMainWidget()) {
        getKMMainWidget()->writeReaderConfig();
    }

    if (mConfigureDialog->isHidden()) {
        mConfigureDialog->show();
    } else {
        mConfigureDialog->raise();
    }
}

void KMKernel::slotConfigChanged()
{
    CodecManager::self()->updatePreferredCharsets();
    Q_EMIT configChanged();
}

//-------------------------------------------------------------------------------

bool KMKernel::haveSystemTrayApplet() const
{
    return mUnityServiceManager->haveSystemTrayApplet();
}

QTextCodec *KMKernel::networkCodec() const
{
    return mNetCodec;
}

void KMKernel::updateSystemTray()
{
    if (!the_shuttingDown) {
        mUnityServiceManager->initListOfCollection();
    }
}

KIdentityManagement::IdentityManager *KMKernel::identityManager()
{
    return KIdentityManagement::IdentityManager::self();
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
        PimCommon::PimCommonSettings::self()->setSharedConfig(mySelf->mConfig);
        PimCommon::PimCommonSettings::self()->load();
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

KMMainWidget *KMKernel::getKMMainWidget()
{
    //This could definitely use a speadup
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
    if (KMailSettings::self()->checkCollectionsIndexing()) {
        mCheckIndexingManager->start(entityTreeModel());
    }
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
    mBackgroundTasksTimer->start(60 * 1000);   // check again in 1 minute
#else
    mBackgroundTasksTimer->start(4 * 60 * 60 * 1000);   // check again in 4 hours
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
    const auto idx = collectionModel()->match({}, Akonadi::EntityTreeModel::CollectionRole,
                                              QVariant::fromValue(col), 1, Qt::MatchExactly);
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
    if (KMMainWidget::mainWidgetList()
        && KMMainWidget::mainWidgetList()->count() > 1) {
        return true;
    }
    return mUnityServiceManager->canQueryClose();
}

Akonadi::Collection KMKernel::currentCollection()
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

Akonadi::Search::PIM::IndexedItems *KMKernel::indexedItems() const
{
    return mIndexedItems;
}

// can't be inline, since KMSender isn't known to implement
// KMail::MessageSender outside this .cpp file
MessageComposer::MessageSender *KMKernel::msgSender()
{
    return the_msgSender;
}

void KMKernel::transportRemoved(int id, const QString &name)
{
    Q_UNUSED(id);

    // reset all identities using the deleted transport
    QStringList changedIdents;
    KIdentityManagement::IdentityManager *im = identityManager();
    KIdentityManagement::IdentityManager::Iterator end = im->modifyEnd();
    for (KIdentityManagement::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it) {
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
        //Don't set parent otherwise we will swith to current KMail and we configure it. So not good
        KMessageBox::informationList(nullptr, information, changedIdents);
        im->commit();
    }
}

void KMKernel::transportRenamed(int id, const QString &oldName, const QString &newName)
{
    Q_UNUSED(id);

    QStringList changedIdents;
    KIdentityManagement::IdentityManager *im = identityManager();
    KIdentityManagement::IdentityManager::Iterator end = im->modifyEnd();
    for (KIdentityManagement::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it) {
        if (oldName == (*it).transport()) {
            (*it).setTransport(newName);
            changedIdents << (*it).identityName();
        }
    }

    if (!changedIdents.isEmpty()) {
        const QString information
            = i18np("This identity has been changed to use the modified transport:",
                    "These %1 identities have been changed to use the modified transport:",
                    changedIdents.count());
        //Don't set parent otherwise we will swith to current KMail and we configure it. So not good
        KMessageBox::informationList(nullptr, information, changedIdents);
        im->commit();
    }
}

void KMKernel::itemDispatchStarted()
{
    // Watch progress of the MDA.
    KPIM::ProgressManagerAkonadi::createProgressItem(nullptr,
                                                     MailTransport::DispatcherInterface().dispatcherInstance(),
                                                     QStringLiteral("Sender"),
                                                     i18n("Sending messages"),
                                                     i18n("Initiating sending process..."),
                                                     true, KPIM::ProgressItem::Unknown);
}

void KMKernel::instanceStatusChanged(const Akonadi::AgentInstance &instance)
{
    if (instance.identifier() == QLatin1String("akonadi_mailfilter_agent")) {
        // Creating a progress item twice is ok, it will simply return the already existing
        // item
        KPIM::ProgressItem *progress = KPIM::ProgressManagerAkonadi::createProgressItem(nullptr, instance,
                                                                                        instance.identifier(), instance.name(), instance.statusMessage(),
                                                                                        false, KPIM::ProgressItem::Encrypted);
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
                        if (imapSafety == QLatin1String("None")) {
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
            KPIM::ProgressItem *progress = KPIM::ProgressManagerAkonadi::createProgressItem(nullptr, instance,
                                                                                            instance.identifier(), instance.name(), instance.statusMessage(),
                                                                                            true, cryptoStatus);
            progress->setProperty("AgentIdentifier", instance.identifier());
        } else if (instance.status() == Akonadi::AgentInstance::Broken) {
            agentInstanceBroken(instance);
        }
    }
}

void KMKernel::agentInstanceBroken(const Akonadi::AgentInstance &instance)
{
    const QString summary = i18n("Resource %1 is broken.", instance.name());
    KNotification::event(QStringLiteral("akonadi-resource-broken"),
                         QString(),
                         summary,
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout);
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
        QDir tempDir(QDir::tempPath() + QLatin1Char('/') + file);
        if (!tempDir.removeRecursively()) {
            fprintf(stderr, "%s was not removed .\n", qPrintable(tempDir.absolutePath()));
        } else {
            fprintf(stderr, "%s was removed .\n", qPrintable(tempDir.absolutePath()));
        }
    }
}

void KMKernel::stopAgentInstance()
{
    const QString resourceGroupPattern(QStringLiteral("Resource %1"));

    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    for (Akonadi::AgentInstance type : lst) {
        const QString identifier = type.identifier();
        KConfigGroup group(KMKernel::config(), resourceGroupPattern.arg(identifier));

        // Keep sync in ConfigureDialog, don't forget to change there.
        if (group.readEntry("OfflineOnShutdown", identifier.startsWith(QLatin1String("akonadi_pop3_resource")) ? true : false)) {
            type.setIsOnline(false);
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
        mFilterEditDialog->setObjectName(QStringLiteral("filterdialog"));
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
                        //Use default trash
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
                for (const Akonadi::Collection &collection : qAsConst(collectionList)) {
                    const Akonadi::Collection::Id collectionId = collection.id();
                    if (iface->targetCollection() == collectionId) {
                        //Use default inbox
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
    KNotification::event(QStringLiteral("akonadi-instance-warning"),
                         QString(),
                         summary,
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout);
}

void KMKernel::slotInstanceError(const Akonadi::AgentInstance &instance, const QString &message)
{
    const QString summary = i18nc("<source>: <error message>", "%1: %2", instance.name(), message);
    KNotification::event(QStringLiteral("akonadi-instance-error"),
                         QString(),
                         summary,
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout);
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

PimCommon::AutoCorrection *KMKernel::composerAutoCorrection()
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
    Q_UNUSED(col);
    Q_UNUSED(sync);
}
