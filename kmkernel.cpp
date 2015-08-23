/*   */

#include "kmkernel.h"

#include "settings/globalsettings.h"
#include "misc/broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmstartup.h"
#include "kmmainwin.h"
#include "editor/composer.h"
#include "kmreadermainwin.h"
#include "undostack.h"
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "addressline/recentaddress/recentaddresses.h"
using KPIM::RecentAddresses;
#include "configuredialog/configuredialog.h"
#include "kmcommands.h"
#include "kmsystemtray.h"
#include "utils/stringutil.h"
#include "util/mailutil.h"
#include "mailcommon/pop3settings.h"
#include "mailcommon/folder/foldertreeview.h"
#include "mailcommon/filter/kmfilterdialog.h"
#include "mailcommon/mailcommonsettings_base.h"
#include "pimcommon/util/pimutil.h"
#include "folderarchive/folderarchivemanager.h"
#include "pimcommon/storageservice/storageservicemanager.h"
#include "pimcommon/storageservice/storageservicejobconfig.h"
#include "storageservice/storageservicesettingsjob.h"

// kdepim includes
#include "kdepim-version.h"

// kdepimlibs includes
#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <MailTransport/mailtransport/transport.h>
#include <MailTransport/mailtransport/transportmanager.h>
#include <MailTransport/mailtransport/dispatcherinterface.h>
#include <AkonadiCore/servermanager.h>

#include <kwindowsystem.h>
#include "mailserviceimpl.h"
using KMail::MailServiceImpl;
#include "job/jobscheduler.h"

#include "messagecore/settings/globalsettings.h"
#include "messagelist/core/settings.h"
#include "messagelist/messagelistutil.h"
#include "messageviewer/settings/globalsettings.h"
#include "messagecomposer/sender/akonadisender.h"
#include "settings/messagecomposersettings.h"
#include "messagecomposer/helper/messagehelper.h"
#include "messagecomposer/settings/messagecomposersettings.h"
#include "pimcommon/settings/pimcommonsettings.h"
#include "pimcommon/autocorrection/autocorrection.h"

#include "templateparser/templateparser.h"
#include "templateparser/globalsettings_base.h"
#include "templateparser/templatesutil.h"

#include "foldercollection.h"
#include "editor/codec/codecmanager.h"

#include <kmessagebox.h>
#include <knotification.h>
#include <progresswidget/progressmanager.h>

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

#include <QNetworkConfigurationManager>
#include <QDir>
#include <QWidget>
#include <QFileInfo>
#include <QtDBus/QtDBus>
#include <QMimeDatabase>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kstartupinfo.h>
#include <kmailadaptor.h>
#include <KLocalizedString>
#include <QStandardPaths>
#include "kmailinterface.h"
#include "foldercollectionmonitor.h"
#include "imapresourcesettings.h"
#include "util.h"
#include "mailcommon/kernel/mailkernel.h"

#include "searchdialog/searchdescriptionattribute.h"
#include "kmail_options.h"

using namespace MailCommon;

static KMKernel *mySelf = Q_NULLPTR;
static bool s_askingToGoOnline = false;
static QNetworkConfigurationManager *s_networkConfigMgr = 0;
/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel(QObject *parent) :
    QObject(parent),
    mIdentityManager(Q_NULLPTR),
    mConfigureDialog(Q_NULLPTR),
    mMailService(Q_NULLPTR),
    mSystemNetworkStatus(true),
    mSystemTray(Q_NULLPTR),
    mDebugBaloo(false)
{
    if (!qgetenv("KDEPIM_BALOO_DEBUG").isEmpty()) {
        mDebugBaloo = true;
    }
    if (!s_networkConfigMgr) {
        s_networkConfigMgr = new QNetworkConfigurationManager(QCoreApplication::instance());
    }
    mSystemNetworkStatus = s_networkConfigMgr->isOnline();

    Akonadi::AttributeFactory::registerAttribute<Akonadi::SearchDescriptionAttribute>();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kmail"));
    qCDebug(KMAIL_LOG) << "Starting up...";

    mySelf = this;
    the_firstInstance = true;

    the_undoStack = Q_NULLPTR;
    the_msgSender = Q_NULLPTR;
    mFilterEditDialog = Q_NULLPTR;
    mWin = Q_NULLPTR;
    // make sure that we check for config updates before doing anything else
    KMKernel::config();
    // this shares the kmailrc parsing too (via KSharedConfig), and reads values from it
    // so better do it here, than in some code where changing the group of config()
    // would be unexpected
    GlobalSettings::self();

    mJobScheduler = new JobScheduler(this);
    mXmlGuiInstance = QStringLiteral("kmail2");

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
    if (mNetCodec->name().toLower() == "eucjp"
#if defined Q_OS_WIN || defined Q_OS_MACX
            || netCodec->name().toLower() == "shift-jis" // OK?
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

    connect(folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)),
            SLOT(slotCollectionChanged(Akonadi::Collection,QSet<QByteArray>)));

    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRemoved, this, &KMKernel::transportRemoved);
    connect(MailTransport::TransportManager::self(), &MailTransport::TransportManager::transportRenamed, this, &KMKernel::transportRenamed);

    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/MailDispatcherAgent"), QStringLiteral("org.freedesktop.Akonadi.MailDispatcherAgent"), QStringLiteral("itemDispatchStarted"), this, SLOT(itemDispatchStarted()));
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceStatusChanged, this, &KMKernel::instanceStatusChanged);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceError, this, &KMKernel::slotInstanceError);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceWarning, this, &KMKernel::slotInstanceWarning);

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &KMKernel::slotInstanceRemoved);

    connect(s_networkConfigMgr, &QNetworkConfigurationManager::onlineStateChanged,
            this, &KMKernel::slotSystemNetworkStatusChanged);

    connect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCompleted(KPIM::ProgressItem*)),
            this, SLOT(slotProgressItemCompletedOrCanceled(KPIM::ProgressItem*)));
    connect(KPIM::ProgressManager::instance(), SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
            this, SLOT(slotProgressItemCompletedOrCanceled(KPIM::ProgressItem*)));
    connect(identityManager(), SIGNAL(deleted(uint)), this, SLOT(slotDeleteIdentity(uint)));
    CommonKernel->registerKernelIf(this);
    CommonKernel->registerSettingsIf(this);
    CommonKernel->registerFilterIf(this);
    mFolderArchiveManager = new FolderArchiveManager(this);
    mStorageManager = new PimCommon::StorageServiceManager(this);
    StorageServiceSettingsJob *settingsJob = new StorageServiceSettingsJob;
    PimCommon::StorageServiceJobConfig *configJob = PimCommon::StorageServiceJobConfig::self();
    configJob->registerConfigIf(settingsJob);
}

KMKernel::~KMKernel()
{
    delete mMailService;
    mMailService = Q_NULLPTR;

    mSystemTray = Q_NULLPTR;

    stopAgentInstance();
    slotSyncConfig();

    delete mAutoCorrection;
    mySelf = Q_NULLPTR;
    qCDebug(KMAIL_LOG);
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
    (void) new KmailAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KMail"), this);
    mMailService = new MailServiceImpl();
}

static QUrl makeAbsoluteUrl(const QString &str, const QString &cwd)
{
    QUrl url = QUrl::fromLocalFile(str);
    if (url.scheme().isEmpty()) {
        const QString newUrl = cwd + QLatin1Char('/') + url.fileName();
        return QUrl::fromLocalFile(newUrl);
    } else {
        return url;
    }
}

bool KMKernel::handleCommandLine(bool noArgsOpensReader, const QStringList &args, const QString &workingDir)
{
    QString to, cc, bcc, subj, body, inReplyTo, replyTo;
    QStringList customHeaders;
    QUrl messageFile;
    QList<QUrl> attachURLs;
    bool mailto = false;
    bool checkMail = false;
    bool viewOnly = false;
    bool calledWithSession = false; // for ignoring '-session foo'

    // process args:
    QCommandLineParser parser;
    kmail_options(&parser);
    parser.process(args);

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

    if (parser.isSet(QStringLiteral("view"))) {
        viewOnly = true;
        const QString filename =
            parser.value(QStringLiteral("view"));
        messageFile = QUrl::fromLocalFile(filename);
        if (!messageFile.isValid()) {
            messageFile = QUrl();
            messageFile.setPath(filename);
        }
    }

    if (!calledWithSession) {
        // only read additional command line arguments if kmail/kontact is
        // not called with "-session foo"
        for (const QString &arg : parser.positionalArguments()) {
            if (arg.startsWith(QStringLiteral("mailto:"), Qt::CaseInsensitive)) {
                QMap<QString, QString> values = MessageCore::StringUtil::parseMailtoUrl(QUrl::fromUserInput(arg));
                if (!values.value(QStringLiteral("to")).isEmpty()) {
                    to += values.value(QStringLiteral("to")) + QStringLiteral(", ");
                }
                if (!values.value(QStringLiteral("cc")).isEmpty()) {
                    cc += values.value(QStringLiteral("cc")) + QStringLiteral(", ");
                }
                if (!values.value(QStringLiteral("subject")).isEmpty()) {
                    subj = values.value(QStringLiteral("subject"));
                }
                if (!values.value(QStringLiteral("body")).isEmpty()) {
                    body = values.value(QStringLiteral("body"));
                }
                if (!values.value(QStringLiteral("in-reply-to")).isEmpty()) {
                    inReplyTo = values.value(QStringLiteral("in-reply-to"));
                }
                const QString attach = values.value(QStringLiteral("attachment"));
                if (!attach.isEmpty()) {
                    attachURLs << makeAbsoluteUrl(attach, workingDir);
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
        viewMessage(messageFile.url());
    } else
        action(mailto, checkMail, to, cc, bcc, subj, body, messageFile,
               attachURLs, customHeaders, replyTo, inReplyTo);
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
    foreach (Akonadi::AgentInstance type, lst) {
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

void KMKernel::setSystrayIconNotificationsEnabled(bool enabled)
{
    if (mSystemTray) {
        mSystemTray->setSystrayIconNotificationsEnabled(enabled);
    }
}

QStringList KMKernel::accounts()
{
    QStringList accountLst;
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    accountLst.reserve(lst.count());
    foreach (const Akonadi::AgentInstance &type, lst) {
        // Explicitly make a copy, as we're not changing values of the list but only
        // the local copy which is passed to action.
        accountLst << type.identifier();
    }
    return accountLst;
}

void KMKernel::checkAccount(const QString &account)   //might create a new reader but won't show!!
{
    qCDebug(KMAIL_LOG);
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

void KMKernel::openReader(bool onlyCheck)
{
    mWin = Q_NULLPTR;
    KMainWindow *ktmw = Q_NULLPTR;
    qCDebug(KMAIL_LOG);

    foreach (KMainWindow *window, KMainWindow::memberList()) {
        if (::qobject_cast<KMMainWin *>(window)) {
            ktmw = window;
            break;
        }
    }

    bool activate;
    if (ktmw) {
        mWin = static_cast<KMMainWin *>(ktmw);
        activate = !onlyCheck; // existing window: only activate if not --check
        if (activate) {
            mWin->show();
        }
    } else {
        mWin = new KMMainWin;
        mWin->show();
        activate = false; // new window: no explicit activation (#73591)
    }

    if (activate) {
        // Activate window - doing this instead of KWindowSystem::activateWindow(mWin->winId());
        // so that it also works when called from KMailApplication::newInstance()
#if defined Q_OS_X11 && ! defined K_WS_QTONLY
        KStartupInfo::setNewStartupId(mWin, KStartupInfo::startupId());
#endif
    }
}

int KMKernel::openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
                           const QString &body, bool hidden,
                           const QString &messageFile,
                           const QStringList &attachmentPaths,
                           const QStringList &customHeaders,
                           const QString &replyTo, const QString &inReplyTo)
{
    qCDebug(KMAIL_LOG);
    KMail::Composer::TemplateContext context = KMail::Composer::New;
    KMime::Message::Ptr msg(new KMime::Message);
    MessageHelper::initHeader(msg, identityManager());
    msg->contentType()->setCharset("utf-8");
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to, "utf-8");
    }

    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc, "utf-8");
    }

    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc, "utf-8");
    }

    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject, "utf-8");
    }

    QUrl messageUrl = QUrl::fromLocalFile(messageFile);
    if (!messageUrl.isEmpty() && messageUrl.isLocalFile()) {
        QFile f(messageUrl.toLocalFile());
        QByteArray str;
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(KMAIL_LOG) << "Failed to load message: " << f.errorString();
        } else {
            str = f.readAll();
            f.close();
        }
        if (!str.isEmpty()) {
            context = KMail::Composer::NoTemplate;
            msg->setBody(QString::fromLocal8Bit(str.data(), str.size()).toUtf8());
        } else {
            TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
            parser.setIdentityManager(KMKernel::self()->identityManager());
            parser.process(msg);
        }
    } else if (!body.isEmpty()) {
        context = KMail::Composer::NoTemplate;
        msg->setBody(body.toUtf8());
    } else {
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(msg);
    }

    if (!inReplyTo.isEmpty()) {
        KMime::Headers::InReplyTo *header = new KMime::Headers::InReplyTo;
        header->fromUnicodeString(inReplyTo, "utf-8");
        msg->setHeader(header);
    }

    msg->assemble();
    KMail::Composer *cWin = KMail::makeComposer(msg, false, false, context);
    if (!to.isEmpty()) {
        cWin->setFocusToSubject();
    }
    QList<QUrl> attachURLs = QUrl::fromStringList(attachmentPaths);
    QList<QUrl>::ConstIterator endAttachment(attachURLs.constEnd());
    for (QList<QUrl>::ConstIterator it = attachURLs.constBegin() ; it != endAttachment; ++it) {
        QMimeDatabase mimeDb;
        if (mimeDb.mimeTypeForUrl(*it).name() == QLatin1String("inode/directory")) {
            if (KMessageBox::questionYesNo(Q_NULLPTR, i18n("Do you want to attach this folder \"%1\"?", (*it).toDisplayString()), i18n("Attach Folder")) == KMessageBox::No) {
                continue;
            }
        }
        cWin->addAttachment((*it), QString());
    }
    if (!replyTo.isEmpty()) {
        cWin->setCurrentReplyTo(replyTo);
    }

    if (!customHeaders.isEmpty()) {
        QMap<QByteArray, QString> extraCustomHeaders;
        QStringList::ConstIterator end = customHeaders.constEnd();
        for (QStringList::ConstIterator it = customHeaders.constBegin() ; it != end ; ++it) {
            if (!(*it).isEmpty()) {
                const int pos = (*it).indexOf(QLatin1Char(':'));
                if (pos > 0) {
                    const QString header = (*it).left(pos).trimmed();
                    const QString value = (*it).mid(pos + 1).trimmed();
                    if (!header.isEmpty() && !value.isEmpty()) {
                        extraCustomHeaders.insert(header.toUtf8(), value);
                    }
                }
            }
        }
        if (!extraCustomHeaders.isEmpty()) {
            cWin->addExtraCustomHeaders(extraCustomHeaders);
        }
    }
    if (!hidden) {
        cWin->show();
        // Activate window - doing this instead of KWindowSystem::activateWindow(cWin->winId());
        // so that it also works when called from KMailApplication::newInstance()
#if defined Q_OS_X11 && ! defined K_WS_QTONLY
        KStartupInfo::setNewStartupId(cWin, KStartupInfo::startupId());
#endif
    }
    return 1;
}

int KMKernel::openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
                           const QString &body, bool hidden,
                           const QString &attachName,
                           const QByteArray &attachCte,
                           const QByteArray &attachData,
                           const QByteArray &attachType,
                           const QByteArray &attachSubType,
                           const QByteArray &attachParamAttr,
                           const QString &attachParamValue,
                           const QByteArray &attachContDisp,
                           const QByteArray &attachCharset,
                           unsigned int identity)
{
    KMail::Composer *cWin;
    bool iCalAutoSend = fillComposer(cWin, to, cc, bcc,
                                     subject, body,
                                     attachName, attachCte, attachData,
                                     attachType, attachSubType, attachParamAttr, attachParamValue,
                                     attachContDisp, attachCharset, identity);

    if (!hidden && !iCalAutoSend) {
        cWin->show();
        // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
        // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
        KStartupInfo::setNewStartupId(cWin, KStartupInfo::startupId());
#endif
    } else {

        // Always disable word wrap when we don't show the composer, since otherwise QTextEdit
        // gets the widget size wrong and wraps much too early.
        cWin->disableWordWrap();
        cWin->slotSendNow();
    }
    return 1;
}

int KMKernel::openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
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
                           unsigned int identity)
{
    KMail::Composer *cWin;
    fillComposer(cWin, to, cc, bcc,
                 subject, body,
                 attachName, attachCte, attachData,
                 attachType, attachSubType, attachParamAttr, attachParamValue,
                 attachContDisp, attachCharset, identity);
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId(cWin, KStartupInfo::startupId());
#endif

    return 1;
}

bool KMKernel::fillComposer(KMail::Composer *&cWin,
                            const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
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
                            unsigned int identity)
{
    KMail::Composer::TemplateContext context = KMail::Composer::New;
    KMime::Message::Ptr msg(new KMime::Message);
    KMime::Content *msgPart = Q_NULLPTR;
    MessageHelper::initHeader(msg, identityManager());
    msg->contentType()->setCharset("utf-8");
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc, "utf-8");
    }
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc, "utf-8");
    }
    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject, "utf-8");
    }
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to, "utf-8");
    }
    if (identity > 0) {
        KMime::Headers::Generic *h = new KMime::Headers::Generic("X-KMail-Identity");
        h->from7BitString(QByteArray::number(identity));
        msg->setHeader(h);
    }
    if (!body.isEmpty()) {
        msg->setBody(body.toUtf8());
    } else {
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(KMime::Message::Ptr());
    }

    bool iCalAutoSend = false;
    bool noWordWrap = false;
    bool isICalInvitation = false;
    if (!attachData.isEmpty()) {
        isICalInvitation = (attachName == QLatin1String("cal.ics")) &&
                           attachType == "text" &&
                           attachSubType == "calendar" &&
                           attachParamAttr == "method";
        // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
        if (isICalInvitation && bcc.isEmpty()) {
            msg->bcc()->clear();
        }
        if (isICalInvitation &&
                MessageViewer::GlobalSettings::self()->legacyBodyInvites()) {
            // KOrganizer invitation caught and to be sent as body instead
            msg->setBody(attachData);
            msg->contentType()->from7BitString(
                QStringLiteral("text/calendar; method=%1; "
                               "charset=\"utf-8\"").
                arg(attachParamValue).toLatin1());

            iCalAutoSend = true; // no point in editing raw ICAL
            noWordWrap = true; // we shant word wrap inline invitations
        } else {
            // Just do what we're told to do
            msgPart = new KMime::Content;
            msgPart->contentTransferEncoding()->fromUnicodeString(QLatin1String(attachCte), "utf-8");
            msgPart->setBody(attachData);   //TODO: check if was setBodyEncoded
            msgPart->contentType()->setMimeType(attachType + '/' +  attachSubType);
            msgPart->contentType()->setParameter(QLatin1String(attachParamAttr), attachParamValue);   //TODO: Check if the content disposition parameter needs to be set!
            if (! MessageViewer::GlobalSettings::self()->exchangeCompatibleInvitations()) {
                msgPart->contentDisposition()->fromUnicodeString(QLatin1String(attachContDisp), "utf-8");
            }
            if (!attachCharset.isEmpty()) {
                // qCDebug(KMAIL_LOG) << "Set attachCharset to" << attachCharset;
                msgPart->contentType()->setCharset(attachCharset);
            }

            msgPart->contentType()->setName(attachName, "utf-8");
            msgPart->assemble();
            // Don't show the composer window if the automatic sending is checked
            iCalAutoSend = MessageViewer::GlobalSettings::self()->automaticSending();
        }
    }

    msg->assemble();

    if (!msg->body().isEmpty()) {
        context = KMail::Composer::NoTemplate;
    }

    cWin = KMail::makeComposer(KMime::Message::Ptr(), false, false, context);
    cWin->setMessage(msg, false, false, !isICalInvitation /* mayAutoSign */);
    cWin->setSigningAndEncryptionDisabled(isICalInvitation
                                          && MessageViewer::GlobalSettings::self()->legacyBodyInvites());
    if (noWordWrap) {
        cWin->disableWordWrap();
    }
    if (msgPart) {
        cWin->addAttach(msgPart);
    }
    if (isICalInvitation) {
        cWin->disableWordWrap();
        cWin->forceDisableHtml();
        //cWin->disableRecipientNumberCheck();
        cWin->disableForgottenAttachmentsCheck();
    }
    return iCalAutoSend;
}

QDBusObjectPath KMKernel::openComposer(const QString &to, const QString &cc,
                                       const QString &bcc,
                                       const QString &subject,
                                       const QString &body, bool hidden)
{
    KMime::Message::Ptr msg(new KMime::Message);
    MessageHelper::initHeader(msg, identityManager());
    msg->contentType()->setCharset("utf-8");
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc, "utf-8");
    }
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc, "utf-8");
    }
    if (!subject.isEmpty()) {
        msg->subject()->fromUnicodeString(subject, "utf-8");
    }
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to, "utf-8");
    }
    if (!body.isEmpty()) {
        msg->setBody(body.toUtf8());
    } else {
        TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
        parser.setIdentityManager(KMKernel::self()->identityManager());
        parser.process(KMime::Message::Ptr());
    }

    msg->assemble();
    const KMail::Composer::TemplateContext context = body.isEmpty() ? KMail::Composer::New :
            KMail::Composer::NoTemplate;
    KMail::Composer *cWin = KMail::makeComposer(msg, false, false, context);
    if (!hidden) {
        cWin->show();
        // Activate window - doing this instead of KWindowSystem::activateWindow(cWin->winId());
        // so that it also works when called from KMailApplication::newInstance()
#if defined Q_OS_X11 && ! defined K_WS_QTONLY
        KStartupInfo::setNewStartupId(cWin, KStartupInfo::startupId());
#endif
    } else {
        // Always disable word wrap when we don't show the composer; otherwise,
        // QTextEdit gets the widget size wrong and wraps much too early.
        cWin->disableWordWrap();
        cWin->slotSendNow();
    }

    return QDBusObjectPath(cWin->dbusObjectPath());
}

QDBusObjectPath KMKernel::newMessage(const QString &to,
                                     const QString &cc,
                                     const QString &bcc,
                                     bool hidden,
                                     bool useFolderId,
                                     const QString & /*messageFile*/,
                                     const QString &_attachURL)
{
    QUrl attachURL = QUrl::fromLocalFile(_attachURL);
    KMime::Message::Ptr msg(new KMime::Message);
    QSharedPointer<FolderCollection> folder;
    uint id = 0;

    if (useFolderId) {
        //create message with required folder identity
        folder = currentFolderCollection();
        id = folder ? folder->identity() : 0;
    }
    MessageHelper::initHeader(msg, identityManager(), id);
    msg->contentType()->setCharset("utf-8");
    //set basic headers
    if (!cc.isEmpty()) {
        msg->cc()->fromUnicodeString(cc, "utf-8");
    }
    if (!bcc.isEmpty()) {
        msg->bcc()->fromUnicodeString(bcc, "utf-8");
    }
    if (!to.isEmpty()) {
        msg->to()->fromUnicodeString(to, "utf-8");
    }

    msg->assemble();
    TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
    parser.setIdentityManager(identityManager());
    Akonadi::Collection col = folder ? folder->collection() : Akonadi::Collection();
    parser.process(msg, col);

    KMail::Composer *win = makeComposer(msg, false, false, KMail::Composer::New, id);

    win->setCollectionForNewMessage(col);
    //Add the attachment if we have one
    if (!attachURL.isEmpty() && attachURL.isValid()) {
        win->addAttachment(attachURL, QString());
    }

    //only show window when required
    if (!hidden) {
        win->show();
    }
    return QDBusObjectPath(win->dbusObjectPath());
}

int KMKernel::viewMessage(const QString &messageFile)
{
    KMOpenMsgCommand *openCommand = new KMOpenMsgCommand(Q_NULLPTR, QUrl::fromLocalFile(messageFile));

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
    KMMainWidget *mainWidget = Q_NULLPTR;

    // First look for a KMainWindow.
    foreach (KMainWindow *window, KMainWindow::memberList()) {
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
        if (job->exec()) {
            if (job->items().count() >= 1) {
                KMReaderMainWin *win = new KMReaderMainWin(MessageViewer::Viewer::UseGlobalSetting, false);
                win->showMessage(MessageCore::GlobalSettings::self()->overrideCharacterEncoding(),
                                 job->items().at(0));
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
    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline) {
        return;
    }

    setAccountStatus(false);

    GlobalSettings::setNetworkState(GlobalSettings::EnumNetworkState::Offline);
    BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be offline; all network jobs are suspended"));
    Q_EMIT onlineStatusChanged((GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState());

}

void KMKernel::setAccountStatus(bool goOnline)
{
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances(false);
    foreach (Akonadi::AgentInstance type, lst) {
        const QString identifier(type.identifier());
        if (PimCommon::Util::isImapResource(identifier) ||
                identifier.contains(POP3_RESOURCE_IDENTIFIER) ||
                identifier.contains(QStringLiteral("akonadi_maildispatcher_agent")) ||
                type.type().capabilities().contains(QStringLiteral("NeedsNetwork"))) {
            type.setIsOnline(goOnline);
        }
    }
    if (goOnline &&  MessageComposer::MessageComposerSettings::self()->sendImmediate()) {
        const qint64 nbMsgOutboxCollection = MailCommon::Util::updatedCollection(CommonKernel->outboxCollectionFolder()).statistics().count();
        if (nbMsgOutboxCollection > 0) {
            kmkernel->msgSender()->sendQueued();
        }
    }
}

void KMKernel::resumeNetworkJobs()
{
    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online) {
        return;
    }

    if (mSystemNetworkStatus) {
        setAccountStatus(true);
        BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be online; all network jobs resumed"));
    } else {
        BroadcastStatus::instance()->setStatusMsg(i18n("KMail is set to be online; all network jobs will resume when a network connection is detected"));
    }
    GlobalSettings::setNetworkState(GlobalSettings::EnumNetworkState::Online);
    Q_EMIT onlineStatusChanged((GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState());
    KMMainWidget *widget = getKMMainWidget();
    if (widget) {
        widget->clearViewer();
    }
}

bool KMKernel::isOffline()
{
    if ((GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline) || !s_networkConfigMgr->isOnline()) {
        return true;
    } else {
        return false;
    }
}

void KMKernel::verifyAccount()
{
    const QString resourceGroupPattern(QStringLiteral("Resource %1"));

    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    foreach (Akonadi::AgentInstance type, lst) {
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

    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline) {
        s_askingToGoOnline = true;
        int rc =
            KMessageBox::questionYesNo(KMKernel::self()->mainWin(),
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
    if (GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline) {
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
    const QString pathName = localDataPath();
    QDir dir(pathName);
    if (!dir.exists(QStringLiteral("autosave"))) {
        return;
    }

    dir.cd(localDataPath() + QLatin1String("autosave"));
    const QFileInfoList autoSaveFiles = dir.entryInfoList();
    foreach (const QFileInfo &file, autoSaveFiles) {
        // Disregard the '.' and '..' folders
        const QString filename = file.fileName();
        if (filename == QLatin1String(".") ||
                filename == QLatin1String("..")
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
            KMessageBox::sorry(Q_NULLPTR, i18n("Failed to open autosave file at %1.\nReason: %2" ,
                                               file.absoluteFilePath(), autoSaveFile.errorString()),
                               i18n("Opening Autosave File Failed"));
        }
    }
}

void  KMKernel::akonadiStateChanged(Akonadi::ServerManager::State state)
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
    }
}

void KMKernel::init()
{
    the_shuttingDown = false;

    the_firstStart = GlobalSettings::self()->firstStart();
    GlobalSettings::self()->setFirstStart(false);
    the_previousVersion = GlobalSettings::self()->previousVersion();
    GlobalSettings::self()->setPreviousVersion(QStringLiteral(KDEPIM_VERSION));

    the_undoStack = new UndoStack(20);

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
    QList<KMainWindow *> windowsToDelete;

    foreach (KMainWindow *window, KMainWindow::memberList()) {
        if (::qobject_cast<KMMainWin *>(window) ||
                ::qobject_cast<KMail::SecondaryWindow *>(window)) {
            // close and delete the window
            window->setAttribute(Qt::WA_DeleteOnClose);
            window->close();
            windowsToDelete.append(window);
        }
    }

    // We delete all main windows here. Above we called close(), but that calls
    // deleteLater() internally, therefore does not delete it immediately.
    // This would lead to problems when closing Kontact when a composer window
    // is open, because the destruction order is:
    //
    // 1. destructor of the Kontact mainwinow
    // 2.   delete all parts
    // 3.     the KMail part destructor calls KMKernel::cleanup(), which calls
    //        this function
    // 4. delete all other mainwindows
    //
    // Deleting the composer windows here will make sure that step 4 will not delete
    // any composer window, which would fail because the kernel is already deleted.
    qDeleteAll(windowsToDelete);
    windowsToDelete.clear();
}

void KMKernel::cleanup(void)
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
    MailCommon::FolderCollection::clearCache();

    // Write the config while all other managers are alive
    delete the_msgSender;
    the_msgSender = Q_NULLPTR;
    delete the_undoStack;
    the_undoStack = Q_NULLPTR;

    KSharedConfig::Ptr config =  KMKernel::config();
    Akonadi::Collection trashCollection = CommonKernel->trashCollectionFolder();
    if (trashCollection.isValid()) {
        if (GlobalSettings::self()->emptyTrashOnExit()) {
            Akonadi::CollectionStatisticsJob *jobStatistics = new Akonadi::CollectionStatisticsJob(trashCollection);
            if (jobStatistics->exec()) {
                if (jobStatistics->statistics().count() > 0) {
                    mFolderCollectionMonitor->expunge(trashCollection, true /*sync*/);
                }
            }
        }
    }
    delete mConfigureDialog;
    mConfigureDialog = Q_NULLPTR;
    // do not delete, because mWin may point to an existing window
    // delete mWin;
    mWin = Q_NULLPTR;

    if (RecentAddresses::exists()) {
        RecentAddresses::self(config.data())->save(config.data());
    }
}

void KMKernel::dumpDeadLetters()
{
    if (shuttingDown()) {
        return;    //All documents should be saved before shutting down is set!
    }

    // make all composer windows autosave their contents
    foreach (KMainWindow *window, KMainWindow::memberList()) {
        if (KMail::Composer *win = ::qobject_cast<KMail::Composer *>(window)) {
            win->autoSaveMessage(true);

            while (win->isComposing()) {
                qCWarning(KMAIL_LOG) << "Danger, using an event loop, this should no longer be happening!";
                qApp->processEvents();
            }
        }
    }
}

void KMKernel::action(bool mailto, bool check, const QString &to,
                      const QString &cc, const QString &bcc,
                      const QString &subj, const QString &body,
                      const QUrl &messageFile,
                      const QList<QUrl> &attachURLs,
                      const QStringList &customHeaders,
                      const QString &replyTo,
                      const QString &inReplyTo)
{
    if (mailto) {
        openComposer(to, cc, bcc, subj, body, 0,
                     messageFile.toDisplayString(), QUrl::toStringList(attachURLs),
                     customHeaders, replyTo, inReplyTo);
    } else {
        openReader(check);
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
    PimCommon::PimCommonSettings::self()->save();
    MessageCore::GlobalSettings::self()->save();
    MessageViewer::GlobalSettings::self()->save();
    MessageComposer::MessageComposerSettings::self()->save();
    TemplateParser::GlobalSettings::self()->save();
    MessageList::Core::Settings::self()->save();
    MailCommon::MailCommonSettings::self()->save();
    GlobalSettings::self()->save();
    KMKernel::config()->sync();
    //Laurent investigate why we need to reload them.
    PimCommon::PimCommonSettings::self()->load();
    MessageCore::GlobalSettings::self()->load();
    MessageViewer::GlobalSettings::self()->load();
    MessageComposer::MessageComposerSettings::self()->load();
    TemplateParser::GlobalSettings::self()->load();
    MessageList::Core::Settings::self()->load();
    MailCommon::MailCommonSettings::self()->load();
    GlobalSettings::self()->load();
    KMKernel::config()->reparseConfiguration();
}

void KMKernel::updateConfig()
{
    slotConfigChanged();
}

void KMKernel::slotShowConfigurationDialog()
{
    if (KMKernel::getKMMainWidget() == Q_NULLPTR) {
        // ensure that there is a main widget available
        // as parts of the configure dialog (identity) rely on this
        // and this slot can be called when there is only a KMComposeWin showing
        KMMainWin *win = new KMMainWin;
        win->show();

    }

    if (!mConfigureDialog) {
        mConfigureDialog = new ConfigureDialog(Q_NULLPTR, false);
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
//static
QString KMKernel::localDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail2/") ;
}

//-------------------------------------------------------------------------------

bool KMKernel::haveSystemTrayApplet() const
{
    return (mSystemTray != Q_NULLPTR);
}

void KMKernel::updateSystemTray()
{
    if (mSystemTray && !the_shuttingDown) {
        mSystemTray->updateSystemTray();
    }
}

KIdentityManagement::IdentityManager *KMKernel::identityManager()
{
    if (!mIdentityManager) {
        qCDebug(KMAIL_LOG);
        mIdentityManager = new KIdentityManagement::IdentityManager(false, this, "mIdentityManager");
    }
    return mIdentityManager;
}

KMainWindow *KMKernel::mainWin()
{
    // First look for a KMMainWin.
    foreach (KMainWindow *window, KMainWindow::memberList())
        if (::qobject_cast<KMMainWin *>(window)) {
            return window;
        }

    // There is no KMMainWin. Use any other KMainWindow instead (e.g. in
    // case we are running inside Kontact) because we anyway only need
    // it for modal message boxes and for KNotify events.
    if (!KMainWindow::memberList().isEmpty()) {
        KMainWindow *kmWin = KMainWindow::memberList().first();
        if (kmWin) {
            return kmWin;
        }
    }

    // There's not a single KMainWindow. Create a KMMainWin.
    // This could happen if we want to pop up an error message
    // while we are still doing the startup wizard and no other
    // KMainWindow is running.
    mWin = new KMMainWin;
    return mWin;
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
        MessageList::Core::Settings::self()->setSharedConfig(mySelf->mConfig);
        MessageList::Core::Settings::self()->load();
        TemplateParser::GlobalSettings::self()->setSharedConfig(mySelf->mConfig);
        TemplateParser::GlobalSettings::self()->load();
        MessageComposer::MessageComposerSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageComposer::MessageComposerSettings::self()->load();
        MessageCore::GlobalSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageCore::GlobalSettings::self()->load();
        MessageViewer::GlobalSettings::self()->setSharedConfig(mySelf->mConfig);
        MessageViewer::GlobalSettings::self()->load();
        MailCommon::MailCommonSettings::self()->setSharedConfig(mySelf->mConfig);
        MailCommon::MailCommonSettings::self()->load();
        PimCommon::PimCommonSettings::self()->setSharedConfig(mySelf->mConfig);
        PimCommon::PimCommonSettings::self()->load();
    }
    return mySelf->mConfig;
}

void KMKernel::syncConfig()
{
    slotRequestConfigSync();
}

void KMKernel::selectCollectionFromId(const Akonadi::Collection::Id id)
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
    QWidgetList l = QApplication::topLevelWidgets();
    QWidget *wid;

    Q_FOREACH (wid, l) {
        QList<KMMainWidget *> l2 = wid->window()->findChildren<KMMainWidget *>();
        if (!l2.isEmpty() && l2.first()) {
            return l2.first();
        }
    }
    return Q_NULLPTR;
}

void KMKernel::slotRunBackgroundTasks() // called regularly by timer
{
    // Hidden KConfig keys. Not meant to be used, but a nice fallback in case
    // a stable kmail release goes out with a nasty bug in CompactionJob...
    if (GlobalSettings::self()->autoExpiring()) {
        mFolderCollectionMonitor->expireAllFolders(false /*scheduled, not immediate*/, entityTreeModel());
    }

#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
    mBackgroundTasksTimer->start(60 * 1000);   // check again in 1 minute
#else
    mBackgroundTasksTimer->start(4 * 60 * 60 * 1000);   // check again in 4 hours
#endif

}

static Akonadi::Collection::List collect_collections(const QAbstractItemModel *model,
        const QModelIndex &parent)
{
    Akonadi::Collection::List collections;
    const int numberOfCollection(model->rowCount(parent));
    for (int i = 0; i < numberOfCollection; ++i) {
        const QModelIndex child = model->index(i, 0, parent);
        Akonadi::Collection collection =
            model->data(child, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.isValid()) {
            collections << collection;
        }
        collections += collect_collections(model, child);
    }
    return collections;
}

Akonadi::Collection::List KMKernel::allFolders() const
{
    return collect_collections(collectionModel(), QModelIndex());
}

void KMKernel::expireAllFoldersNow() // called by the GUI
{
    mFolderCollectionMonitor->expireAllFolders(true /*immediate*/, entityTreeModel());
}

bool KMKernel::canQueryClose()
{
    if (KMMainWidget::mainWidgetList() &&
            KMMainWidget::mainWidgetList()->count() > 1) {
        return true;
    }
    if (!mSystemTray) {
        return true;
    }
    if (mSystemTray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowAlways) {
        mSystemTray->hideKMail();
        return false;
    } else if ((mSystemTray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread)) {
        if (mSystemTray->hasUnreadMail()) {
            mSystemTray->setStatus(KStatusNotifierItem::Active);
        }
        mSystemTray->hideKMail();
        return false;
    }
    return true;
}

QSharedPointer<FolderCollection> KMKernel::currentFolderCollection()
{
    KMMainWidget *widget = getKMMainWidget();
    QSharedPointer<FolderCollection> folder;
    if (widget) {
        folder = widget->currentFolder();
    }
    return folder;
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
    const QString &currentTransport = GlobalSettings::self()->currentTransport();
    if (name == currentTransport) {
        GlobalSettings::self()->setCurrentTransport(QString());
    }

    if (!changedIdents.isEmpty()) {
        QString information = i18np("This identity has been changed to use the default transport:",
                                    "These %1 identities have been changed to use the default transport:",
                                    changedIdents.count());
        //Don't set parent otherwise we will swith to current KMail and we configure it. So not good
        KMessageBox::informationList(Q_NULLPTR, information, changedIdents);
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
        const QString information =
            i18np("This identity has been changed to use the modified transport:",
                  "These %1 identities have been changed to use the modified transport:",
                  changedIdents.count());
        //Don't set parent otherwise we will swith to current KMail and we configure it. So not good
        KMessageBox::informationList(Q_NULLPTR, information, changedIdents);
        im->commit();
    }
}

void KMKernel::itemDispatchStarted()
{
    // Watch progress of the MDA.
    KPIM::ProgressManager::createProgressItem(Q_NULLPTR,
            MailTransport::DispatcherInterface().dispatcherInstance(),
            QStringLiteral("Sender"),
            i18n("Sending messages"),
            i18n("Initiating sending process..."),
            true);
}

void KMKernel::instanceStatusChanged(const Akonadi::AgentInstance &instance)
{
    if (instance.identifier() == QLatin1String("akonadi_mailfilter_agent")) {
        // Creating a progress item twice is ok, it will simply return the already existing
        // item
        KPIM::ProgressItem *progress =  KPIM::ProgressManager::createProgressItem(Q_NULLPTR, instance,
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
                    OrgKdeAkonadiImapSettingsInterface *iface = PimCommon::Util::createImapSettingsInterface(identifier);
                    if (iface && iface->isValid()) {
                        const QString imapSafety = iface->safety();
                        if ((imapSafety == QLatin1String("SSL") || imapSafety == QLatin1String("STARTTLS"))) {
                            cryptoStatus = KPIM::ProgressItem::Encrypted;
                        }

                        mResourceCryptoSettingCache.insert(identifier, cryptoStatus);
                    }
                    delete iface;
                } else if (identifier.contains(POP3_RESOURCE_IDENTIFIER)) {
                    OrgKdeAkonadiPOP3SettingsInterface *iface = MailCommon::Util::createPop3SettingsInterface(identifier);
                    if (iface->isValid()) {
                        if ((iface->useSSL() || iface->useTLS())) {
                            cryptoStatus = KPIM::ProgressItem::Encrypted;
                        }
                        mResourceCryptoSettingCache.insert(identifier, cryptoStatus);
                    }
                    delete iface;
                }
            }

            // Creating a progress item twice is ok, it will simply return the already existing
            // item
            KPIM::ProgressItem *progress =  KPIM::ProgressManager::createProgressItem(Q_NULLPTR, instance,
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
    const QString summary = i18n("Resource %1 is broken.",  instance.name());
    KNotification::event(QStringLiteral("akonadi-resource-broken"),
                         summary,
                         QPixmap(),
                         Q_NULLPTR,
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

void KMKernel::stopAgentInstance()
{
    const QString resourceGroupPattern(QStringLiteral("Resource %1"));

    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    foreach (Akonadi::AgentInstance type, lst) {
        const QString identifier = type.identifier();
        KConfigGroup group(KMKernel::config(), resourceGroupPattern.arg(identifier));

        // Keep sync in ConfigureDialog, don't forget to change there.
        if (group.readEntry("OfflineOnShutdown", identifier.startsWith(QStringLiteral("akonadi_pop3_resource")) ? true : false)) {
            type.setIsOnline(false);
        }
    }
}

void KMKernel::slotCollectionRemoved(const Akonadi::Collection &col)
{
    KConfigGroup group(KMKernel::config(), MailCommon::FolderCollection::configGroupName(col));
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
    return GlobalSettings::self()->showPopupAfterDnD();
}

bool KMKernel::excludeImportantMailFromExpiry()
{
    return GlobalSettings::self()->excludeImportantMailFromExpiry();
}

qreal KMKernel::closeToQuotaThreshold()
{
    return GlobalSettings::self()->closeToQuotaThreshold();
}

Akonadi::Entity::Id KMKernel::lastSelectedFolder()
{
    return GlobalSettings::self()->lastSelectedFolder();
}

void KMKernel::setLastSelectedFolder(const Akonadi::Entity::Id &col)
{
    GlobalSettings::self()->setLastSelectedFolder(col);
}

QStringList KMKernel::customTemplates()
{
    return GlobalSettingsBase::self()->customTemplates();
}

void KMKernel::openFilterDialog(bool createDummyFilter)
{
    if (!mFilterEditDialog) {
        mFilterEditDialog = new MailCommon::KMFilterDialog(getKMMainWidget()->actionCollections(), Q_NULLPTR, createDummyFilter);
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
    foreach (const Akonadi::AgentInstance &type, lst) {
        if (type.status() == Akonadi::AgentInstance::Broken) {
            continue;
        }
        const QString typeIdentifier(type.identifier());
        if (PimCommon::Util::isImapResource(typeIdentifier)) {
            OrgKdeAkonadiImapSettingsInterface *iface = PimCommon::Util::createImapSettingsInterface(type.identifier());
            if (iface && iface->isValid()) {
                foreach (const Akonadi::Collection &collection, collectionList) {
                    const Akonadi::Collection::Id collectionId = collection.id();
                    if (iface->trashCollection() == collectionId) {
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
                foreach (const Akonadi::Collection &collection, collectionList) {
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

void KMKernel::slotInstanceWarning(const Akonadi::AgentInstance &instance , const QString &message)
{
    const QString summary = i18nc("<source>: <error message>", "%1: %2", instance.name(), message);
    KNotification::event(QStringLiteral("akonadi-instance-warning"),
                         summary,
                         QPixmap(),
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout);
}

void KMKernel::slotInstanceError(const Akonadi::AgentInstance &instance, const QString &message)
{
    const QString summary = i18nc("<source>: <error message>", "%1: %2", instance.name(), message);
    KNotification::event(QStringLiteral("akonadi-instance-error"),
                         summary,
                         QPixmap(),
                         Q_NULLPTR,
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
        if (widget->currentFolder()) {
            Akonadi::Collection collection = widget->currentFolder()->collection();
            Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(collection.resource());
            instance.setIsOnline(true);
            widget->clearViewer();
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
        if (!mSystemTray && GlobalSettings::self()->systemTrayEnabled()) {
            mSystemTray = new KMail::KMSystemTray(widget);
        } else if (mSystemTray && !GlobalSettings::self()->systemTrayEnabled()) {
            // Get rid of system tray on user's request
            qCDebug(KMAIL_LOG) << "deleting systray";
            delete mSystemTray;
            mSystemTray = Q_NULLPTR;
        }

        // Set mode of systemtray. If mode has changed, tray will handle this.
        if (mSystemTray) {
            mSystemTray->setMode(GlobalSettings::self()->systemTrayPolicy());
            mSystemTray->setShowUnreadCount(GlobalSettings::self()->systemTrayShowUnread());
        }

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
        if (mSystemTray) {
            mSystemTray->updateSystemTray();
        }
    }
}

FolderArchiveManager *KMKernel::folderArchiveManager() const
{
    return mFolderArchiveManager;
}

PimCommon::StorageServiceManager *KMKernel::storageServiceManager() const
{
    return mStorageManager;
}

bool KMKernel::allowToDebugBalooSupport() const
{
    return mDebugBaloo;
}
