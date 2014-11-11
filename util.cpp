/*******************************************************************************
**
** Filename   : util
** Created on : 03 April, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : <adam@kde.org>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   It is distributed in the hope that it will be useful, but
**   WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**   General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/
#include "util.h"
#include "kmkernel.h"

#include "messagecore/utils/stringutil.h"
#include "messagecomposer/helper/messagehelper.h"

#include "templateparser/templateparser.h"

#include <kmime/kmime_message.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <KProcess>
#include <QDebug>

#include <QProcess>
#include <QFileInfo>
#include <QAction>
#include <QStandardPaths>

#include "foldercollection.h"

using namespace MailCommon;

KMime::Types::Mailbox::List KMail::Util::mailingListsFromMessage(const Akonadi::Item &item)
{
    KMime::Types::Mailbox::List addresses;
    // determine the mailing list posting address
    Akonadi::Collection parentCollection = item.parentCollection();
    if (parentCollection.isValid()) {
        const QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(parentCollection, false);
        if (fd->isMailingListEnabled() && !fd->mailingListPostAddress().isEmpty()) {
            addresses << MessageCore::StringUtil::mailboxFromUnicodeString(fd->mailingListPostAddress());
        }
    }

    return addresses;
}

Akonadi::Item::Id KMail::Util::putRepliesInSameFolder(const Akonadi::Item &item)
{
    Akonadi::Collection parentCollection = item.parentCollection();
    if (parentCollection.isValid()) {
        const QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(parentCollection, false);
        if (fd->putRepliesInSameFolder()) {
            return parentCollection.id();
        }
    }
    return -1;
}

void KMail::Util::launchAccountWizard(QWidget *w)
{
    QStringList lst;
    lst.append(QLatin1String("--type"));
    lst.append(QLatin1String("message/rfc822"));

    const QString path = QStandardPaths::findExecutable(QLatin1String("accountwizard"));
    if (!QProcess::startDetached(path, lst))
        KMessageBox::error(w, i18n("Could not start the account wizard. "
                                   "Please check your installation."),
                           i18n("Unable to start account wizard"));

}

bool KMail::Util::handleClickedURL(const KUrl &url, const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    if (url.protocol() == QLatin1String("mailto")) {
        KMime::Message::Ptr msg(new KMime::Message);
        uint identity = !folder.isNull() ? folder->identity() : 0;
        MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), identity);
        msg->contentType()->setCharset("utf-8");

        QMap<QString, QString> fields =  MessageCore::StringUtil::parseMailtoUrl(url);

        msg->to()->fromUnicodeString(fields.value(QLatin1String("to")), "utf-8");
        if (!fields.value(QLatin1String("subject")).isEmpty()) {
            msg->subject()->fromUnicodeString(fields.value(QLatin1String("subject")), "utf-8");
        }
        if (!fields.value(QLatin1String("body")).isEmpty()) {
            msg->setBody(fields.value(QLatin1String("body")).toUtf8());
        }
        if (!fields.value(QLatin1String("cc")).isEmpty()) {
            msg->cc()->fromUnicodeString(fields.value(QLatin1String("cc")), "utf-8");
        }

        if (!folder.isNull()) {
            TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
            parser.setIdentityManager(KMKernel::self()->identityManager());
            parser.process(msg, folder->collection());
        }

        KMail::Composer *win = KMail::makeComposer(msg, false, false, KMail::Composer::New, identity);
        win->setFocusToSubject();
        if (!folder.isNull()) {
            win->setCollectionForNewMessage(folder->collection());
        }
        win->show();
        return true;
    } else {
        qWarning() << "Can't handle URL:" << url;
        return false;
    }
}

bool KMail::Util::mailingListsHandleURL(const KUrl::List &lst, const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    const QString handler = (folder->mailingList().handler() == MailingList::KMail)
                            ? QLatin1String("mailto") : QLatin1String("https");

    KUrl urlToHandle;
    KUrl::List::ConstIterator end(lst.constEnd());
    for (KUrl::List::ConstIterator itr = lst.constBegin(); itr != end; ++itr) {
        if (handler == (*itr).protocol()) {
            urlToHandle = *itr;
            break;
        }
    }
    if (urlToHandle.isEmpty() && !lst.empty()) {
        urlToHandle = lst.first();
    }

    if (!urlToHandle.isEmpty()) {
        return KMail::Util::handleClickedURL(urlToHandle, folder);
    } else {
        qWarning() << "Can't handle url";
        return false;
    }
}

bool KMail::Util::mailingListPost(const QSharedPointer<MailCommon::FolderCollection> &fd)
{
    if (fd) {
        return KMail::Util::mailingListsHandleURL(fd->mailingList().postUrls(), fd);
    }
    return false;
}

bool KMail::Util::mailingListSubscribe(const QSharedPointer<MailCommon::FolderCollection> &fd)
{
    if (fd) {
        return KMail::Util::mailingListsHandleURL(fd->mailingList().subscribeUrls(), fd);
    }
    return false;
}

bool KMail::Util::mailingListUnsubscribe(const QSharedPointer<MailCommon::FolderCollection> &fd)
{
    if (fd) {
        return KMail::Util::mailingListsHandleURL(fd->mailingList().unsubscribeUrls(), fd);
    }
    return false;
}

bool KMail::Util::mailingListArchives(const QSharedPointer<MailCommon::FolderCollection> &fd)
{
    if (fd) {
        return KMail::Util::mailingListsHandleURL(fd->mailingList().archiveUrls(), fd);
    }
    return false;
}

bool KMail::Util::mailingListHelp(const QSharedPointer<MailCommon::FolderCollection> &fd)
{
    if (fd) {
        return KMail::Util::mailingListsHandleURL(fd->mailingList().helpUrls(), fd);
    }
    return false;
}

void KMail::Util::lastEncryptAndSignState(bool &lastEncrypt, bool &lastSign, const KMime::Message::Ptr &msg)
{
    lastSign = KMime::isSigned(msg.get());
    lastEncrypt = KMime::isEncrypted(msg.get());
}

QColor KMail::Util::misspelledColor()
{
    return QColor(Qt::red);
}

QColor KMail::Util::quoteL1Color()
{
    return QColor(0x00, 0x80, 0x00);
}

QColor KMail::Util::quoteL2Color()
{
    return QColor(0x00, 0x70, 0x00);
}

QColor KMail::Util::quoteL3Color()
{
    return QColor(0x00, 0x60, 0x00);
}

void KMail::Util::reduceQuery(QString &query)
{
    QRegExp rx(QLatin1String("<[\\w]+://[\\w\\d-_.]+(/[\\d\\w/-._]+/)*([\\w\\d-._]+)#([\\w\\d]+)>"));
    query.replace(rx, QLatin1String("\\2:\\3"));
    query.replace(QLatin1String("rdf-schema:"), QLatin1String("rdfs:"));
    query.replace(QLatin1String("22-rdf-syntax-ns:"), QLatin1String("rdf:"));
    query.replace(QLatin1String("XMLSchema:"), QLatin1String("xsd:"));
    query = query.simplified();
}

void KMail::Util::migrateFromKMail1()
{
    // Akonadi migration
    // check if there is something to migrate at all
    bool needMigration = true;
    KConfig oldKMailConfig(QLatin1String("kmailrc"), KConfig::NoGlobals);
    if (oldKMailConfig.hasGroup("General") ||
            (oldKMailConfig.groupList().count() == 1 &&
             oldKMailConfig.groupList().first() == QLatin1String("$Version"))) {
        const QFileInfo oldDataDirFileInfo(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kmail")) ;
        if (!oldDataDirFileInfo.exists() || !oldDataDirFileInfo.isDir()) {
            // neither config or data, the migrator cannot do anything useful anyways
            needMigration = false;
        }
    } else {
        needMigration = false;
    }

    KConfig config(QLatin1String("kmail-migratorrc"));
    KConfigGroup migrationCfg(&config, "Migration");
    if (needMigration) {
        const bool enabled = migrationCfg.readEntry("Enabled", false);
        const int currentVersion = migrationCfg.readEntry("Version", 0);
        const int targetVersion = migrationCfg.readEntry("TargetVersion", 1);
        if (enabled && currentVersion < targetVersion) {
            const int choice = KMessageBox::questionYesNoCancel(0, i18n(
                                   "<b>Thanks for using KMail2!</b>"
                                   "<p>KMail2 uses a new storage technology that requires migration of your current KMail data and configuration.</p>\n"
                                   "<p>The conversion process can take a lot of time (depending on the amount of email you have) and it <em>must not be interrupted</em>.</p>\n"
                                   "<p>You can:</p><ul>"
                                   "<li>Migrate now (be prepared to wait)</li>"
                                   "<li>Skip the migration and start with fresh data and configuration</li>"
                                   "<li>Cancel and exit KMail2.</li>"
                                   "</ul>"
                                   "<p><a href=\"http://userbase.kde.org/Akonadi\">More Information...</a></p>"
                               ), i18n("KMail Migration"), KGuiItem(i18n("Migrate Now")), KGuiItem(i18n("Skip Migration")), KStandardGuiItem::cancel(),
                               QString(), KMessageBox::Notify | KMessageBox::Dangerous | KMessageBox::AllowLink);
            if (choice == KMessageBox::Cancel) {
                exit(1);
            }

            if (choice != KMessageBox::Yes) {    // user skipped migration
                // we only will make one attempt at this
                migrationCfg.writeEntry("Version", targetVersion);
                migrationCfg.sync();

                return;
            }

            qDebug() << "Performing Akonadi migration. Good luck!";
            KProcess proc;
            QStringList args = QStringList() << QLatin1String("--interactive-on-change");
            const QString path = QStandardPaths::findExecutable(QLatin1String("kmail-migrator"));
            proc.setProgram(path, args);
            proc.start();
            bool result = proc.waitForStarted();
            if (result) {
                result = proc.waitForFinished(-1);
            }
            if (result && proc.exitCode() == 0) {
                qDebug() << "Akonadi migration has been successful";
            } else {
                // exit code 1 means it is already running, so we are probably called by a migrator instance
                qCritical() << "Akonadi migration failed!";
                qCritical() << "command was: " << proc.program();
                qCritical() << "exit code: " << proc.exitCode();
                qCritical() << "stdout: " << proc.readAllStandardOutput();
                qCritical() << "stderr: " << proc.readAllStandardError();

                KMessageBox::error(0, i18n("Migration to KMail 2 failed. In case you want to try again, run 'kmail-migrator --interactive' manually."),
                                   i18n("Migration Failed"));
                return;
            }
        }
    } else {
        if (migrationCfg.hasKey("Enabled") && (migrationCfg.readEntry("Enabled", false) == false)) {
            return;
        }
        migrationCfg.writeEntry("Enabled", false);
        migrationCfg.sync();
    }
}

void KMail::Util::addQActionHelpText(QAction *action, const QString &text)
{
    action->setStatusTip(text);
    action->setToolTip(text);
    if (action->whatsThis().isEmpty()) {
        action->setWhatsThis(text);
    }
}
