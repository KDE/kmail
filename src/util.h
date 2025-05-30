/*
    SPDX-FileCopyrightText: 2005 Till Adam <adam@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <MailCommon/FolderSettings>

namespace KMail
{
/**
 * The Util namespace contains a collection of helper functions use in
 * various places.
 */
namespace Util
{
/**
 * Returns any mailing list post addresses set on the
 *  parent collection (the mail folder) of the item.
 */
KMAILTESTS_TESTS_EXPORT KMime::Types::Mailbox::List mailingListsFromMessage(const Akonadi::Item &item);

/**
 * Whether or not the mail item has the keep-reply-in-folder
 *  attribute set.
 */
KMAILTESTS_TESTS_EXPORT Akonadi::Item::Id putRepliesInSameFolder(const Akonadi::Item &item);

/**
 * Handles a clicked URL, but only in case the viewer didn't handle it.
 * Currently only support mailto.
 */
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool
handleClickedURL(const QUrl &url,
                 const QSharedPointer<MailCommon::FolderSettings> &folder = QSharedPointer<MailCommon::FolderSettings>(),
                 const Akonadi::Collection &collection = Akonadi::Collection(),
                 QWidget *parentWidget = nullptr);

[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool
mailingListsHandleURL(const QList<QUrl> &lst, const QSharedPointer<MailCommon::FolderSettings> &folder, const Akonadi::Collection &collection);

[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool mailingListPost(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool mailingListSubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool mailingListUnsubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool mailingListArchives(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
[[nodiscard]] KMAILTESTS_TESTS_EXPORT bool mailingListHelp(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);

KMAILTESTS_TESTS_EXPORT void lastEncryptAndSignState(bool &lastEncrypt, bool &lastSign, const KMime::Message::Ptr &msg);

KMAILTESTS_TESTS_EXPORT void addQActionHelpText(QAction *action, const QString &text);

/**
 * Set an action's text, icon etc. as appropriate for whether a message is
 * in the trash folder (delete permanently) or any other (move to trash).
 */
KMAILTESTS_TESTS_EXPORT void setActionTrashOrDelete(QAction *action, bool isInTrashFolder);

void executeAccountWizard(QWidget *parentWidget);
}
}
