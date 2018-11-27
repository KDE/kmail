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

#include <MessageCore/StringUtil>
#include "MessageComposer/MessageHelper"
#include "job/handleclickedurljob.h"

#include <kmime/kmime_message.h>
#include <kmessagebox.h>
#include <klocalizedstring.h>
#include "kmail_debug.h"

#include <QAction>
#include <QStandardPaths>

#include <MailCommon/FolderSettings>

using namespace MailCommon;
using namespace KMail;
KMime::Types::Mailbox::List Util::mailingListsFromMessage(const Akonadi::Item &item)
{
    KMime::Types::Mailbox::List addresses;
    // determine the mailing list posting address
    Akonadi::Collection parentCollection = item.parentCollection();
    if (parentCollection.isValid()) {
        const QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(parentCollection, false);
        if (fd->isMailingListEnabled() && !fd->mailingListPostAddress().isEmpty()) {
            KMime::Types::Mailbox mailbox;
            mailbox.fromUnicodeString(fd->mailingListPostAddress());
            addresses << mailbox;
        }
    }

    return addresses;
}

Akonadi::Item::Id Util::putRepliesInSameFolder(const Akonadi::Item &item)
{
    Akonadi::Collection parentCollection = item.parentCollection();
    if (parentCollection.isValid()) {
        const QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(parentCollection, false);
        if (fd->putRepliesInSameFolder()) {
            return parentCollection.id();
        }
    }
    return -1;
}

bool Util::handleClickedURL(const QUrl &url, const QSharedPointer<MailCommon::FolderSettings> &folder, const Akonadi::Collection &collection)
{
    if (url.scheme() == QLatin1String("mailto")) {
        HandleClickedUrlJob *job = new HandleClickedUrlJob;
        job->setUrl(url);
        job->setFolder(folder);
        job->setCurrentCollection(collection);
        job->start();
        return true;
    } else {
        qCWarning(KMAIL_LOG) << "Can't handle URL:" << url;
        return false;
    }
}

bool Util::mailingListsHandleURL(const QList<QUrl> &lst, const QSharedPointer<MailCommon::FolderSettings> &folder, const Akonadi::Collection &collection)
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
        urlToHandle = lst.constFirst();
    }

    if (!urlToHandle.isEmpty()) {
        return Util::handleClickedURL(urlToHandle, folder, collection);
    } else {
        qCWarning(KMAIL_LOG) << "Can't handle url";
        return false;
    }
}

bool Util::mailingListPost(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col)
{
    if (fd) {
        return Util::mailingListsHandleURL(fd->mailingList().postUrls(), fd, col);
    }
    return false;
}

bool Util::mailingListSubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col)
{
    if (fd) {
        return Util::mailingListsHandleURL(fd->mailingList().subscribeUrls(), fd, col);
    }
    return false;
}

bool Util::mailingListUnsubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col)
{
    if (fd) {
        return Util::mailingListsHandleURL(fd->mailingList().unsubscribeUrls(), fd, col);
    }
    return false;
}

bool Util::mailingListArchives(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col)
{
    if (fd) {
        return Util::mailingListsHandleURL(fd->mailingList().archiveUrls(), fd, col);
    }
    return false;
}

bool Util::mailingListHelp(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col)
{
    if (fd) {
        return Util::mailingListsHandleURL(fd->mailingList().helpUrls(), fd, col);
    }
    return false;
}

void Util::lastEncryptAndSignState(bool &lastEncrypt, bool &lastSign, const KMime::Message::Ptr &msg)
{
    lastSign = KMime::isSigned(msg.data());
    lastEncrypt = KMime::isEncrypted(msg.data());
}

void Util::addQActionHelpText(QAction *action, const QString &text)
{
    action->setStatusTip(text);
    action->setToolTip(text);
    if (action->whatsThis().isEmpty()) {
        action->setWhatsThis(text);
    }
}

void Util::setActionTrashOrDelete(QAction *action, bool isInTrashFolder)
{
    if (action) {
        action->setText(isInTrashFolder ? i18nc("@action Hard delete, bypassing trash", "&Delete") : i18n("&Move to Trash"));
        action->setIcon(isInTrashFolder ? QIcon::fromTheme(QStringLiteral("edit-delete")) : QIcon::fromTheme(QStringLiteral("edit-delete-shred")));
        //Use same text as in Text property. Change it in kf5
        action->setToolTip(isInTrashFolder ? i18nc("@action Hard delete, bypassing trash", "Delete") : i18n("Move to Trash"));
    }
}
