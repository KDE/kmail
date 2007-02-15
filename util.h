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
#include <qobject.h>
#include <qcstring.h>
class DwString;

namespace KMail
{
    /**
     * The Util namespace contains a collection of helper functions use in
     * various places.
     */
namespace Util {
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
     * Convert "\n" line endings to "\r\n".
     * @param src The source string to convert.
     * @return The result string.
     */
    QCString lf2crlf( const QCString & src );
    /**
     * Convert "\n" line endings to "\r\n".
     * @param src The source string to convert. NOT null-terminated.
     * @return The result string. NOT null-terminated.
     */
    QByteArray lf2crlf( const QByteArray & src );

    /**
     * Construct a QCString from a DwString
     */
    QCString CString( const DwString& str );

    /**
     * Construct a QByteArray from a DwString
     */
    QByteArray ByteArray( const DwString& str );

    /**
     * Construct a DwString from a QCString
     */
    DwString dwString( const QCString& str );

    /**
     * Construct a DwString from a QByteArray
     */
    DwString dwString( const QByteArray& str );

    /**
     * Fills a QByteArray from a QCString - removing the trailing null.
     */
    void setFromQCString( QByteArray& arr, const QCString& cstr );

    inline void setFromQCString( QByteArray& arr, const QCString& cstr )
    {
      if ( cstr.size() )
        arr.duplicate( cstr.data(), cstr.size()-1 );
      else
        arr.resize(0);
    }

    /**
     * Creates a QByteArray from a QCString without detaching (duplicating the data).
     * Fast, but be careful, the QCString gets modified by this; this is only good for
     * the case where the QCString is going to be thrown away afterwards anyway.
     */
    QByteArray byteArrayFromQCStringNoDetach( QCString& cstr );
    inline QByteArray byteArrayFromQCStringNoDetach( QCString& cstr )
    {
      QByteArray arr = cstr;
      if ( arr.size() )
        arr.resize( arr.size() - 1 );
      return arr;
    }

    /**
     * Restore the QCString after byteArrayFromQCStringNoDetach modified it
     */
    void restoreQCString( QCString& str );
    inline void restoreQCString( QCString& str )
    {
      if ( str.data() )
        str.resize( str.size() + 1 );
    }

    /**
     * Fills a QCString from a QByteArray - adding the trailing null.
     */
    void setFromByteArray( QCString& cstr, const QByteArray& arr );

    inline void setFromByteArray( QCString& result, const QByteArray& arr )
    {
      const int len = arr.size();
      result.resize( len + 1 /* trailing NUL */ );
      memcpy(result.data(), arr.data(), len);
      result[len] = 0;
    }

    /**
     * Append a bytearray to a bytearray. No trailing nuls anywhere.
     */
    void append( QByteArray& that, const QByteArray& str );

    /**
     * Append a char* to a bytearray. Trailing nul not copied.
     */
    void append( QByteArray& that, const char* str );

    /**
     * Append a QCString to a bytearray. Trailing nul not copied.
     */
    void append( QByteArray& that, const QCString& str );

    void insert( QByteArray& that, uint index, const char* s );

    /**
     * A LaterDeleter is intended to be used with the RAII ( Resource
     * Acquisition is Initialization ) paradigm. When an instance of it
     * goes out of scope it deletes the associated object  It can be 
     * disabled, in case the deletion needs to be avoided for some 
     * reason, since going out-of-scope cannot be avoided.
     */
    class LaterDeleter
    {
      public:
      LaterDeleter( QObject *o)
        :m_object( o ), m_disabled( false )
      {
      }
      virtual ~LaterDeleter()
      {
        if ( !m_disabled ) {
          m_object->deleteLater();
        }
      }
      void setDisabled( bool v )
      {
        m_disabled = v;
      }
      protected:
      QObject *m_object;
      bool m_disabled;
    };
}
}

#endif
