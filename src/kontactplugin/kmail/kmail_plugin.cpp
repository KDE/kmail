/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2003-2013 Kontact Developer <kde-pim@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "kmail_plugin.h"

#include "kmailinterface.h"
#include "summarywidget.h"

#include <kcalendarcore_version.h>
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
#include <KCalUtils/ICalDrag>
#include <KCalUtils/VCalDrag>
#else
#include <KCalendarCore/MimeData>
#endif
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
#include <QUrl>

using namespace KCalendarCore;
using namespace Qt::Literals::StringLiterals;

EXPORT_KONTACT_PLUGIN_WITH_JSON(KMailPlugin, "kmailplugin.json")

KMailPlugin::KMailPlugin(KontactInterface::Core *core, const KPluginMetaData &data, const QVariantList &)
    : KontactInterface::Plugin(core, core, data, "kmail2")
{
    setComponentName(u"kmail2"_s, i18n("KMail2"));

    auto action = new QAction(QIcon::fromTheme(u"mail-message-new"_s), i18nc("@action:inmenu", "New Message…"), this);
    actionCollection()->addAction(u"new_mail"_s, action);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    // action->setHelpText(
    //            i18nc( "@info:status", "Create a new mail message" ) );
    action->setWhatsThis(i18nc("@info:whatsthis",
                               "You will be presented with a dialog where you can create "
                               "and send a new email message."));
    connect(action, &QAction::triggered, this, &KMailPlugin::slotNewMail);
    insertNewAction(action);

    auto syncAction = new QAction(QIcon::fromTheme(u"view-refresh"_s), i18nc("@action:inmenu", "Sync Mail"), this);
    // syncAction->setHelpText(
    //            i18nc( "@info:status", "Synchronize groupware mail" ) );
    syncAction->setWhatsThis(i18nc("@info:whatsthis", "Choose this option to synchronize your groupware email."));
    connect(syncAction, &QAction::triggered, this, &KMailPlugin::slotSyncFolders);
    actionCollection()->addAction(u"sync_mail"_s, syncAction);
    insertSyncAction(syncAction);

    mUniqueAppWatcher = new KontactInterface::UniqueAppWatcher(new KontactInterface::UniqueAppHandlerFactory<KMailUniqueAppHandler>(), this);
}

bool KMailPlugin::canDecodeMimeData(const QMimeData *mimeData) const
{
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    return KCalUtils::ICalDrag::canDecode(mimeData) || KCalUtils::VCalDrag::canDecode(mimeData) || KContacts::VCardDrag::canDecode(mimeData);
#else
    return KCalendarCore::MimeData::canDecode(mimeData) || KContacts::VCardDrag::canDecode(mimeData);
#endif
}

void KMailPlugin::shortcutChanged()
{
    if (KParts::Part *localPart = part()) {
        if (localPart->metaObject()->indexOfMethod("updateQuickSearchText()") == -1) {
            qCWarning(KMAILPLUGIN_LOG) << "KMailPart part is missing slot updateQuickSearchText()";
            return;
        }
        QMetaObject::invokeMethod(localPart, "updateQuickSearchText");
    }
}

void KMailPlugin::processDropEvent(QDropEvent *de)
{
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    MemoryCalendar::Ptr cal(new MemoryCalendar(QTimeZone::utc()));
#endif
    KContacts::Addressee::List list;
    const QMimeData *md = de->mimeData();

#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 29, 0)
    if (KCalUtils::VCalDrag::fromMimeData(md, cal) || KCalUtils::ICalDrag::fromMimeData(md, cal)) {
#else
    if (const auto cal = KCalendarCore::MimeData::decodeCalendar(md); cal) {
#endif
        if (QTemporaryFile tmp(u"incidences-kmail_XXXXXX.ics"_s); tmp.open()) {
            tmp.setAutoRemove(false);
            if (FileStorage storage(cal, tmp.fileName()); !storage.save()) {
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

    qCWarning(KMAILPLUGIN_LOG) << u"Cannot handle drop events of type '%1'."_s.arg(de->mimeData()->formats().join(u';'));
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
    QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.kmail"_s, u"/KMail"_s, u"org.kde.kmail.kmail"_s, u"checkMail"_s);
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

    mInstance = new OrgKdeKmailKmailInterface(u"org.kde.kmail"_s, u"/KMail"_s, QDBusConnection::sessionBus());

    return part;
}

QStringList KMailPlugin::invisibleToolbarActions() const
{
    return QStringList() << u"new_message"_s;
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
    org::kde::kmail::kmail kmail(u"org.kde.kmail"_s, u"/KMail"_s, QDBusConnection::sessionBus());
    if (QDBusReply<bool> reply = kmail.handleCommandLine(false, args, workingDir); reply.isValid()) {
        if (bool handled = reply; !handled) { // no args -> simply bring kmail plugin to front
            return KontactInterface::UniqueAppHandler::activate(args, workingDir);
        }
    }
    return 0;
}

bool KMailPlugin::queryClose() const
{
    org::kde::kmail::kmail kmail(u"org.kde.kmail"_s, u"/KMail"_s, QDBusConnection::sessionBus());
    QDBusReply<bool> canClose = kmail.canQueryClose();
    return canClose;
}

#include "kmail_plugin.moc"

#include "moc_kmail_plugin.cpp"
