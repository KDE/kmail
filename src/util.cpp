/*
    SPDX-FileCopyrightText: 2005 Till Adam <adam@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "util.h"
#include "kmkernel.h"

#include <MessageCore/StringUtil>
#include <MessageComposer/MessageHelper>
#include "job/handleclickedurljob.h"

#include <kmime/kmime_message.h>
#include <KLocalizedString>
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
        action->setIcon(isInTrashFolder ? QIcon::fromTheme(QStringLiteral("edit-delete-shred")) : QIcon::fromTheme(QStringLiteral("edit-delete")));
        //Use same text as in Text property. Change it in kf5
        action->setToolTip(isInTrashFolder ? i18nc("@action Hard delete, bypassing trash", "Delete") : i18n("Move to Trash"));
    }
}
