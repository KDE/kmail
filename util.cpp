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

#include <stdlib.h>
#include <q3cstring.h>

size_t KMail::Util::crlf2lf( char* str, const size_t strLen )
{
    if ( !str || strLen == 0 )
        return 0;

    const char* source = str;
    const char* sourceEnd = source + strLen;

    // search the first occurrence of "\r\n"
    for ( ; source < sourceEnd - 1; ++source ) {
        if ( *source == '\r' && *( source + 1 ) == '\n' )
            break;
    }

    if ( source == sourceEnd - 1 ) {
        // no "\r\n" found
        return strLen;
    }

    // replace all occurrences of "\r\n" with "\n" (in place)
    char* target = const_cast<char*>( source ); // target points to '\r'
    ++source; // source points to '\n'
    for ( ; source < sourceEnd; ++source ) {
        if ( *source != '\r' || *( source + 1 ) != '\n' )
            * target++ = *source;
    }
    *target = '\0'; // terminate result
    return target - str;
}

Q3CString KMail::Util::lf2crlf( const Q3CString & src )
{
    Q3CString result( 1 + 2*src.length() );  // maximal possible length

    Q3CString::ConstIterator s = src.begin();
    Q3CString::Iterator d = result.begin();
  // we use cPrev to make sure we insert '\r' only there where it is missing
    char cPrev = '?';
    while ( *s ) {
        if ( ('\n' == *s) && ('\r' != cPrev) )
            *d++ = '\r';
        cPrev = *s;
        *d++ = *s++;
    }
    result.truncate( d - result.begin() ); // adds trailing NUL
    return result;
}
