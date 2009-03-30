/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef KMAIL_STRINGUTIL_H
#define KMAIL_STRINGUTIL_H

#include <QString>

namespace KMail
{

/**
 * This namespace contain helper functions for string manipulation
 */
namespace StringUtil
{

  /**
   * Strips the signature from a message text. "-- " is considered as
   * a signature seperator.
   */
  QString stripSignature ( const QString & msg, bool clearSigned );
}

}

#endif