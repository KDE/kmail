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

#include <QWidget>
#include <akonadi/item.h>
#include <Akonadi/Collection>
#include "mailcommon/folder/foldercollection.h"
class KUrl;

namespace KMail
{
/**
     * The Util namespace contains a collection of helper functions use in
     * various places.
     */
namespace Util {

/** Test if all required settings are set.
      Reports problems to user via dialogs and returns false.
      Returns true if everything is Ok. */
bool checkTransport( QWidget *w );

/**
     * Convert all sequences of "\r\n" (carriage return followed by a line
     * feed) to a single "\n" (line feed). The conversion happens in place.
     * Returns the length of the resulting string.
     * @param str The string to convert.
     * @param strLen The length of the string to convert.
     * @return The new length of the converted string.
     */
size_t crlf2lf( char* str, const size_t strLen );

/**
     * Returns any mailing list post addresses set on the
     *  parent collection (the mail folder) of the item.
     */
KMime::Types::Mailbox::List mailingListsFromMessage( const Akonadi::Item& item );

/**
     * Whether or not the mail item has the keep-reply-in-folder
     *  attribute set.
     */
Akonadi::Item::Id putRepliesInSameFolder( const Akonadi::Item& item );

void launchAccountWizard( QWidget * );

/**
     * Handles a clicked URL, but only in case the viewer didn't handle it.
     * Currently only support mailto.
     */
bool handleClickedURL( const KUrl &url, const QSharedPointer<MailCommon::FolderCollection> &folder = QSharedPointer<MailCommon::FolderCollection>() );


bool mailingListsHandleURL( const KUrl::List& lst,const QSharedPointer<MailCommon::FolderCollection> &folder );

bool mailingListPost( const QSharedPointer<MailCommon::FolderCollection> &fd );
bool mailingListSubscribe( const QSharedPointer<MailCommon::FolderCollection> &fd );
bool mailingListUnsubscribe( const QSharedPointer<MailCommon::FolderCollection> &fd );
bool mailingListArchives( const QSharedPointer<MailCommon::FolderCollection> &fd );
bool mailingListHelp( const QSharedPointer<MailCommon::FolderCollection> &fd );

void lastEncryptAndSignState(bool &lastEncrypt, bool &lastSign, const KMime::Message::Ptr& msg);

QColor misspelledColor();
QColor quoteL1Color();
QColor quoteL2Color();
QColor quoteL3Color();
void migrateFromKMail1();
}
}

#endif
