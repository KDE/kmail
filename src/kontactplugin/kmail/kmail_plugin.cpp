/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2003-2013 Kontact Developer <kde-pim@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kmail_plugin.h"

#include "kmailinterface.h"
#include "summarywidget.h"

#include <KCalUtils/ICalDrag>
#include <KCalUtils/VCalDrag>
#include <KCalendarCore/FileStorage>
#include <KCalendarCore/MemoryCalendar>
#include <KContacts/VCardDrag>

#include <KontactInterface/Core>

#include "kmailplugin_debug.h"
#include <KActionCollection>
#include <KLocalizedString>
#include <QAction>
#include <QIcon>
#include <QTemporaryFile>

#include <QDropEvent>
#include <QStandardPaths>

using namespace KCalUtils;
using namespace KCalendarCore;
using namespace Qt::Literals::StringLiterals;

EXPORT_KONTACT_PLUGIN_WITH_JSON(KMailPlugin, "kmailplugin.json")

KMailPlugin::KMailPlugin(KontactInterface::Core *core, const KPluginMetaData &data, const QVariantList &)
    : KontactInterface::Plugin(core, core, data, "kmail2")
{
    setComponentName(QStringLiteral("kmail2"), i18n("KMail2"));

    auto action = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), i18nc("@action:inmenu", "New Message…"), this);
    actionCollection()->addAction(QStringLiteral("new_mail"), action);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    // action->setHelpText(
    //            i18nc( "@info:status", "Create a new mail message" ) );
    action->setWhatsThis(i18nc("@info:whatsthis",
                               "You will be presented with a dialog where you can create "
                               "and send a new email message."));
    connect(action, &QAction::triggered, this, &KMailPlugin::slotNewMail);
    insertNewAction(action);

    auto syncAction = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18nc("@action:inmenu", "Sync Mail"), this);
    // syncAction->setHelpText(
    //            i18nc( "@info:status", "Synchronize groupware mail" ) );
    syncAction->setWhatsThis(i18nc("@info:whatsthis", "Choose this option to synchronize your groupware email."));
    connect(syncAction, &QAction::triggered, this, &KMailPlugin::slotSyncFolders);
    actionCollection()->addAction(QStringLiteral("sync_mail"), syncAction);
    insertSyncAction(syncAction);

    mUniqueAppWatcher = new KontactInterface::UniqueAppWatcher(new KontactInterface::UniqueAppHandlerFactory<KMailUniqueAppHandler>(), this);
}

bool KMailPlugin::canDecodeMimeData(const QMimeData *mimeData) const
{
    return ICalDrag::canDecode(mimeData) || VCalDrag::canDecode(mimeData) || KContacts::VCardDrag::canDecode(mimeData);
}

void KMailPlugin::shortcutChanged()
{
    KParts::Part *localPart = part();
    if (localPart) {
        if (localPart->metaObject()->indexOfMethod("updateQuickSearchText()") == -1) {
            qCWarning(KMAILPLUGIN_LOG) << "KMailPart part is missing slot updateQuickSearchText()";
            return;
        }
        QMetaObject::invokeMethod(localPart, "updateQuickSearchText");
    }
}

void KMailPlugin::processDropEvent(QDropEvent *de)
{
    MemoryCalendar::Ptr cal(new MemoryCalendar(QTimeZone::utc()));
    KContacts::Addressee::List list;
    const QMimeData *md = de->mimeData();

    if (VCalDrag::fromMimeData(md, cal) || ICalDrag::fromMimeData(md, cal)) {
        QTemporaryFile tmp(QStringLiteral("incidences-kmail_XXXXXX.ics"));
        if (tmp.open()) {
            tmp.setAutoRemove(false);
            FileStorage storage(cal, tmp.fileName());
            if (!storage.save()) {
                qCWarning(KMAILPLUGIN_LOG) << " Impossible to save data in filestorage";
                return;
            }
            openComposer(QUrl::fromLocalFile(tmp.fileName()));
        } else {
            qCWarning(KMAILPLUGIN_LOG) << " Impossible to create temporary file";
        }
    } else if (KContacts::VCardDrag::fromMimeData(md, list)) {
        QStringList to;
        to.reserve(list.count());
        for (const auto &s : std::as_const(list)) {
            to.append(s.fullEmail());
        }
        openComposer(to.join(", "_L1));
    }

    qCWarning(KMAILPLUGIN_LOG) << QStringLiteral("Cannot handle drop events of type '%1'.").arg(de->mimeData()->formats().join(QLatin1Char(';')));
}

void KMailPlugin::openComposer(const QUrl &attach)
{
    (void)part(); // ensure part is loaded
    Q_ASSERT(mInstance);
    if (mInstance) {
        if (attach.isValid()) {
            mInstance->newMessage(QString(), QString(), QString(), false, true, QString(), attach.isLocalFile() ? attach.toLocalFile() : attach.path());
        } else {
            mInstance->newMessage(QString(), QString(), QString(), false, true, QString(), QString());
        }
    }
}

void KMailPlugin::openComposer(const QString &to)
{
    (void)part(); // ensure part is loaded
    Q_ASSERT(mInstance);
    if (mInstance) {
        mInstance->newMessage(to, QString(), QString(), false, true, QString(), QString());
    }
}

void KMailPlugin::slotNewMail()
{
    openComposer(QString());
}

void KMailPlugin::slotSyncFolders()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kmail"),
                                                          QStringLiteral("/KMail"),
                                                          QStringLiteral("org.kde.kmail.kmail"),
                                                          QStringLiteral("checkMail"));
    QDBusConnection::sessionBus().send(message);
}

KMailPlugin::~KMailPlugin()
{
    delete mInstance;
    mInstance = nullptr;
}

KParts::Part *KMailPlugin::createPart()
{
    KParts::Part *part = loadPart();
    if (!part) {
        return nullptr;
    }

    mInstance = new OrgKdeKmailKmailInterface(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());

    return part;
}

QStringList KMailPlugin::invisibleToolbarActions() const
{
    return QStringList() << QStringLiteral("new_message");
}

bool KMailPlugin::isRunningStandalone() const
{
    return mUniqueAppWatcher->isRunningStandalone();
}

KontactInterface::Summary *KMailPlugin::createSummaryWidget(QWidget *parent)
{
    return new SummaryWidget(this, parent);
}

int KMailPlugin::weight() const
{
    return 200;
}

////

#include "../../kmail_options.h"
void KMailUniqueAppHandler::loadCommandLineOptions(QCommandLineParser *parser)
{
    kmail_options(parser);
}

int KMailUniqueAppHandler::activate(const QStringList &args, const QString &workingDir)
{
    // Ensure part is loaded
    (void)plugin()->part();
    org::kde::kmail::kmail kmail(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
    QDBusReply<bool> reply = kmail.handleCommandLine(false, args, workingDir);

    if (reply.isValid()) {
        bool handled = reply;
        if (!handled) { // no args -> simply bring kmail plugin to front
            return KontactInterface::UniqueAppHandler::activate(args, workingDir);
        }
    }
    return 0;
}

bool KMailPlugin::queryClose() const
{
    org::kde::kmail::kmail kmail(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
    QDBusReply<bool> canClose = kmail.canQueryClose();
    return canClose;
}

#include "kmail_plugin.moc"

#include "moc_kmail_plugin.cpp"
