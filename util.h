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

#include <stdlib.h>
#include <QObject>
#include <QByteArray>
#include <QWidget>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <akonadi/item.h>
#include "kmkernel.h"
#include <kmime/kmime_message.h>
class KUrl;

class OrgKdeAkonadiImapSettingsInterface;

#define IMAP_RESOURCE_IDENTIFIER "akonadi_imap_resource"

namespace KMail
{
    /**
     * The Util namespace contains a collection of helper functions use in
     * various places.
     */
namespace Util {

    /**
     * First disconnect then reconnect a sender/signal-receiver/slot pair.
     * This improves readability of code a bit by reducing two (maybe long and complex) lines
     * to one and often helps in avoiding a pair of brackets.
     */
    void reconnectSignalSlotPair( QObject *src, const char *signal, QObject *dst, const char *slot );

    // return true if we should proceed, false if we should abort
    bool checkOverwrite( const KUrl &url, QWidget *w );

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
     * Delegates opening a URL to the Max OSX mechanisms for that.
     * Returns false if it did nothing (such as on other platforms.
     */
    bool handleUrlOnMac( const KUrl& url );

    /**
     * Convert "\n" line endings to "\r\n".
     * @param src The source string to convert.
     * @return The result string.
     */
    QByteArray lf2crlf( const QByteArray & src );

    /**
     * Validates a list of email addresses.
     * @return true if all addresses are valid.
     * @return false if one or several addresses are invalid.
     */
    bool validateAddresses( QWidget *parent, const QString &addresses );

    KMime::Message::Ptr message( const Akonadi::Item & item );

    /** Convert all non-ascii characters to question marks
     * If ok is non-null, *ok will be set to true if all characters
     * where ascii, *ok will be set to false otherwise */
    QByteArray toUsAscii(const QString& _str, bool *ok=0);


    /** Return a QTextCodec for the specified charset.
     * This function is a bit more tolerant, than QTextCodec::codecForName */
    const QTextCodec* codecForName(const QByteArray& _str);


    /**
     * Find out preferred charset for 'text'.
     * First @p encoding is tried and if that one is not suitable,
     * the encodings in @p encodingList are tried.
     */
     QByteArray autoDetectCharset(const QByteArray &encoding, const QStringList &encodingList, const QString &text);

     KUrl findSieveUrlForAccount( OrgKdeAkonadiImapSettingsInterface *a,  const QString &ident);
     OrgKdeAkonadiImapSettingsInterface *createImapSettingsInterface( const QString &ident );
}
}

#endif
