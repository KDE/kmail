/*
  This file is part of Kontact.

  Copyright (c) 2003-2013 Kontact Developer <kde-pim@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "kmail_plugin.h"
#include "kmailinterface.h"
#include "summarywidget.h"

#include <KContacts/VCardDrag>
#include <KCalCore/MemoryCalendar>
#include <KCalCore/FileStorage>
#include <KCalUtils/ICalDrag>
#include <KCalUtils/VCalDrag>

#include <KontactInterface/Core>

#include <QAction>
#include <KActionCollection>
#include "kmailplugin_debug.h"
#include <QIcon>
#include <KLocalizedString>
#include <QTemporaryFile>

#include <QDropEvent>
#include <QStandardPaths>

using namespace KCalUtils;
using namespace KCalCore;

EXPORT_KONTACT_PLUGIN(KMailPlugin, kmail)

KMailPlugin::KMailPlugin(KontactInterface::Core *core, const QVariantList &)
    : KontactInterface::Plugin(core, core, "kmail2")
    , m_instance(nullptr)
{
    setComponentName(QStringLiteral("kmail2"), i18n("KMail2"));

    QAction *action
        = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")),
                      i18nc("@action:inmenu", "New Message..."), this);
    actionCollection()->addAction(QStringLiteral("new_mail"), action);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    //action->setHelpText(
    //            i18nc( "@info:status", "Create a new mail message" ) );
    action->setWhatsThis(
        i18nc("@info:whatsthis",
              "You will be presented with a dialog where you can create "
              "and send a new email message."));
    connect(action, &QAction::triggered, this, &KMailPlugin::slotNewMail);
    insertNewAction(action);

    QAction *syncAction
        = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
                      i18nc("@action:inmenu", "Sync Mail"), this);
    //syncAction->setHelpText(
    //            i18nc( "@info:status", "Synchronize groupware mail" ) );
    syncAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choose this option to synchronize your groupware email."));
    connect(syncAction, &QAction::triggered, this, &KMailPlugin::slotSyncFolders);
    actionCollection()->addAction(QStringLiteral("sync_mail"), syncAction);
    insertSyncAction(syncAction);

    mUniqueAppWatcher = new KontactInterface::UniqueAppWatcher(
        new KontactInterface::UniqueAppHandlerFactory<KMailUniqueAppHandler>(), this);
}

bool KMailPlugin::canDecodeMimeData(const QMimeData *mimeData) const
{
    return
        ICalDrag::canDecode(mimeData)
        || VCalDrag::canDecode(mimeData)
        || KContacts::VCardDrag::canDecode(mimeData);
}

void KMailPlugin::shortcutChanged()
{
    KParts::ReadOnlyPart *localPart = part();
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
        tmp.setAutoRemove(false);
        tmp.open();
        FileStorage storage(cal, tmp.fileName());
        storage.save();
        openComposer(QUrl::fromLocalFile(tmp.fileName()));
    } else if (KContacts::VCardDrag::fromMimeData(md, list)) {
        KContacts::Addressee::List::ConstIterator it;
        QStringList to;
        to.reserve(list.count());
        KContacts::Addressee::List::ConstIterator end(list.constEnd());
        for (it = list.constBegin(); it != end; ++it) {
            to.append((*it).fullEmail());
        }
        openComposer(to.join(QStringLiteral(", ")));
    }

    qCWarning(KMAILPLUGIN_LOG) << QStringLiteral("Cannot handle drop events of type '%1'.").arg(de->mimeData()->formats().join(QLatin1Char(';')));
}

void KMailPlugin::openComposer(const QUrl &attach)
{
    (void)part();  // ensure part is loaded
    Q_ASSERT(m_instance);
    if (m_instance) {
        if (attach.isValid()) {
            m_instance->newMessage(QString(), QString(), QString(), false, true, QString(), attach.isLocalFile()
                                   ? attach.toLocalFile() : attach.path());
        } else {
            m_instance->newMessage(QString(), QString(), QString(), false, true, QString(), QString());
        }
    }
}

void KMailPlugin::openComposer(const QString &to)
{
    (void)part();  // ensure part is loaded
    Q_ASSERT(m_instance);
    if (m_instance) {
        m_instance->newMessage(to, QString(), QString(), false, true, QString(), QString());
    }
}

void KMailPlugin::slotNewMail()
{
    openComposer(QString());
}

void KMailPlugin::slotSyncFolders()
{
    QDBusMessage message
        = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"),
                                         QStringLiteral("org.kde.kmail.kmail"),
                                         QStringLiteral("checkMail"));
    QDBusConnection::sessionBus().send(message);
}

KMailPlugin::~KMailPlugin()
{
    delete m_instance;
    m_instance = nullptr;
}

KParts::ReadOnlyPart *KMailPlugin::createPart()
{
    KParts::ReadOnlyPart *part = loadPart();
    if (!part) {
        return nullptr;
    }

    m_instance = new OrgKdeKmailKmailInterface(
        QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());

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
        if (!handled) {   // no args -> simply bring kmail plugin to front
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
