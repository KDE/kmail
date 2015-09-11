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

#include "utils/stringutil.h"
#include "messagecomposer/helper/messagehelper.h"

#include "TemplateParser/TemplateParser"

#include <kmime/kmime_message.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <KProcess>
#include "kmail_debug.h"

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
            KMime::Types::Mailbox mailbox;
            mailbox.fromUnicodeString(fd->mailingListPostAddress());
            addresses << mailbox;
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

bool KMail::Util::handleClickedURL(const QUrl &url, const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    if (url.scheme() == QLatin1String("mailto")) {
        KMime::Message::Ptr msg(new KMime::Message);
        uint identity = !folder.isNull() ? folder->identity() : 0;
        MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), identity);
        msg->contentType()->setCharset("utf-8");

        QMap<QString, QString> fields =  MessageCore::StringUtil::parseMailtoUrl(url);

        msg->to()->fromUnicodeString(fields.value(QStringLiteral("to")), "utf-8");
        if (!fields.value(QStringLiteral("subject")).isEmpty()) {
            msg->subject()->fromUnicodeString(fields.value(QStringLiteral("subject")), "utf-8");
        }
        if (!fields.value(QStringLiteral("body")).isEmpty()) {
            msg->setBody(fields.value(QStringLiteral("body")).toUtf8());
        }
        if (!fields.value(QStringLiteral("cc")).isEmpty()) {
            msg->cc()->fromUnicodeString(fields.value(QStringLiteral("cc")), "utf-8");
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
        qCWarning(KMAIL_LOG) << "Can't handle URL:" << url;
        return false;
    }
}

bool KMail::Util::mailingListsHandleURL(const QList<QUrl> &lst, const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    const QString handler = (folder->mailingList().handler() == MailingList::KMail)
                            ? QStringLiteral("mailto") : QStringLiteral("https");

    QUrl urlToHandle;
    QList<QUrl>::ConstIterator end(lst.constEnd());
    for (QList<QUrl>::ConstIterator itr = lst.constBegin(); itr != end; ++itr) {
        if (handler == (*itr).scheme()) {
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
        qCWarning(KMAIL_LOG) << "Can't handle url";
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
    lastSign = KMime::isSigned(msg.data());
    lastEncrypt = KMime::isEncrypted(msg.data());
}

void KMail::Util::addQActionHelpText(QAction *action, const QString &text)
{
    action->setStatusTip(text);
    action->setToolTip(text);
    if (action->whatsThis().isEmpty()) {
        action->setWhatsThis(text);
    }
}
