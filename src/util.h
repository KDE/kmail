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
#ifndef KMAILUTIL_H
#define KMAILUTIL_H

#include <AkonadiCore/item.h>
#include <AkonadiCore/Collection>
#include <MailCommon/FolderSettings>

namespace KMail {
/**
     * The Util namespace contains a collection of helper functions use in
     * various places.
     */
namespace Util {
/**
     * Returns any mailing list post addresses set on the
     *  parent collection (the mail folder) of the item.
     */
KMime::Types::Mailbox::List mailingListsFromMessage(const Akonadi::Item &item);

/**
     * Whether or not the mail item has the keep-reply-in-folder
     *  attribute set.
     */
Akonadi::Item::Id putRepliesInSameFolder(const Akonadi::Item &item);

/**
     * Handles a clicked URL, but only in case the viewer didn't handle it.
     * Currently only support mailto.
     */
Q_REQUIRED_RESULT bool handleClickedURL(const QUrl &url, const QSharedPointer<MailCommon::FolderSettings> &folder = QSharedPointer<MailCommon::FolderSettings>(), const Akonadi::Collection &collection = Akonadi::Collection());

Q_REQUIRED_RESULT bool mailingListsHandleURL(const QList<QUrl> &lst, const QSharedPointer<MailCommon::FolderSettings> &folder, const Akonadi::Collection &collection);

Q_REQUIRED_RESULT bool mailingListPost(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
Q_REQUIRED_RESULT bool mailingListSubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
Q_REQUIRED_RESULT bool mailingListUnsubscribe(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
Q_REQUIRED_RESULT bool mailingListArchives(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);
Q_REQUIRED_RESULT bool mailingListHelp(const QSharedPointer<MailCommon::FolderSettings> &fd, const Akonadi::Collection &col);

void lastEncryptAndSignState(bool &lastEncrypt, bool &lastSign, const KMime::Message::Ptr &msg);

void addQActionHelpText(QAction *action, const QString &text);

/**
     * Set an action's text, icon etc. as appropriate for whether a message is
     * in the trash folder (delete permanently) or any other (move to trash).
     */
void setActionTrashOrDelete(QAction *action, bool isInTrashFolder);
}
}

#endif
