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
#include <qcstring.h>
#include <mimelib/string.h>

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

QCString KMail::Util::lf2crlf( const QCString & src )
{
    QCString result( 1 + 2*src.size() );  // maximal possible length

    QCString::ConstIterator s = src.begin();
    QCString::Iterator d = result.begin();
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

QByteArray KMail::Util::lf2crlf( const QByteArray & src )
{
    const char* s = src.data();
    if ( !s )
      return QByteArray();

    QByteArray result( 2 * src.size() );  // maximal possible length
    QByteArray::Iterator d = result.begin();
    // we use cPrev to make sure we insert '\r' only there where it is missing
    char cPrev = '?';
    const char* end = src.end();
    while ( s != end ) {
        if ( ('\n' == *s) && ('\r' != cPrev) )
            *d++ = '\r';
        cPrev = *s;
        *d++ = *s++;
    }
    result.truncate( d - result.begin() ); // does not add trailing NUL, as expected
    return result;
}

QCString KMail::Util::CString( const DwString& str )
{
  const int strLen = str.size();
  QCString cstr( strLen + 1 );
  memcpy( cstr.data(), str.data(), strLen );
  cstr[ strLen ] = 0;
  return cstr;
}

QByteArray KMail::Util::ByteArray( const DwString& str )
{
  const int strLen = str.size();
  QByteArray arr( strLen );
  memcpy( arr.data(), str.data(), strLen );
  return arr;
}

DwString KMail::Util::dwString( const QCString& str )
{
  if ( !str.data() ) // DwString doesn't like char*=0
    return DwString();
  return DwString( str.data(), str.size() - 1 );
}

DwString KMail::Util::dwString( const QByteArray& str )
{
  if ( !str.data() ) // DwString doesn't like char*=0
    return DwString();
  return DwString( str.data(), str.size() );
}

void KMail::Util::append( QByteArray& that, const QByteArray& str )
{
  that.detach();
  uint len1 = that.size();
  uint len2 = str.size();
  if ( that.resize( len1 + len2, QByteArray::SpeedOptim ) )
    memcpy( that.data() + len1, str.data(), len2 );
}

void KMail::Util::append( QByteArray& that, const char* str )
{
  if ( !str )
    return; // nothing to append
  that.detach();
  uint len1 = that.size();
  uint len2 = qstrlen(str);
  if ( that.resize( len1 + len2, QByteArray::SpeedOptim ) )
    memcpy( that.data() + len1, str, len2 );
}

void KMail::Util::append( QByteArray& that, const QCString& str )
{
  that.detach();
  uint len1 = that.size();
  uint len2 = str.size() - 1;
  if ( that.resize( len1 + len2, QByteArray::SpeedOptim ) )
    memcpy( that.data() + len1, str.data(), len2 );
}

// Code taken from QCString::insert, but trailing nul removed
void KMail::Util::insert( QByteArray& that, uint index, const char* s )
{
  int len = qstrlen(s);
  if ( len == 0 )
    return;
  uint olen = that.size();
  int nlen = olen + len;
  if ( index >= olen ) {                      // insert after end of string
    that.detach();
    if ( that.resize(nlen+index-olen, QByteArray::SpeedOptim ) ) {
      memset( that.data()+olen, ' ', index-olen );
      memcpy( that.data()+index, s, len );
    }
  } else {
    that.detach();
    if ( that.resize(nlen, QByteArray::SpeedOptim ) ) {    // normal insert
      memmove( that.data()+index+len, that.data()+index, olen-index );
      memcpy( that.data()+index, s, len );
    }
  }
}
