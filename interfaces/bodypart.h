/*  -*- mode: C++; c-file-style: "gnu" -*-
    bodypart.h

    This file is part of KMail's plugin interface.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>,
                       Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KMAIL_INTERFACES_BODYPART_H__
#define __KMAIL_INTERFACES_BODYPART_H__

template <typename T> class QMemArray;
typedef QMemArray<char> QByteArray;
class QString;

namespace KMail {
  namespace Interface {

    class Observer;
    class Observable;

    /**
       @short interface of classes that implement status for BodyPartFormatters.
    */
    class BodyPartMemento {
    public:
      virtual ~BodyPartMemento() {}

      /** If your BodyPartMemento implementation also implements the
	  KMail::Observer interface, simply implement these as
	  <code>return this;</code>, else as <code>return
	  0;</code>. This is needed to avoid forcing a dependency of
	  plugins on internal KMail classes.
      */
      virtual Observer * asObserver() = 0;

      /** If your BodyPartMemento implementation also implements the
	  KMail::Observable interface, simply implement these as
	  <code>return this;</code>, else as <code>return
	  0;</code>. This is needed to avoid forcing a dependency of
	  plugins on internal KMail classes.
      */
      virtual Observable * asObservable() = 0;
    };

    /**
       @short interface of message body parts.
    */
    class BodyPart {
    public:
      virtual ~BodyPart() {}

      /**
	 @return a string respresentation of an URL that can be used
	 to invoke a BodyPartURLHandler for this body part.
      */
      virtual QString makeLink( const QString & path ) const = 0;

      /**
	 @return the decoded (CTE, canonicalisation, and charset
	 encoding undone) text contained in the body part, or
	 QString::null, it the body part is not of type "text".
      */
      virtual QString asText() const = 0;

      /**
	 @return the decoded (CTE undone) content of the body part, or
	 a null array if this body part instance is of type text.
      */
      virtual QByteArray asBinary() const = 0;

      /**
	 @return the value of the content-type header field parameter
	 with name \a parameter, or QString::null, if that that
	 parameter is not present in the body's content-type header
	 field. RFC 2231 encoding is removed first.

	 Note that this method will suppress queries to certain
	 standard parameters (most notably "charset") to keep plugins
	 decent.

	 Note2 that this method preserves the case of the parameter
	 value returned. So, if the parameter you want to use defines
	 the value to be case-insensitive (such as the smime-type
	 parameter), you need to make sure you do the casemap yourself
	 before comparing to a reference value.
      */
      virtual QString contentTypeParameter( const char * parameter ) const = 0;

      /**
	 @return the content of the content-description header field,
	 or QString::null if that header is not present in this body
	 part. RFC 2047 encoding is decoded first.
      */
      virtual QString contentDescription() const = 0;

      //virtual int contentDisposition() const = 0;
      /**
	 @return the value of the content-disposition header field
	 parameter with name \a parameter, or QString::null if that
	 parameter is not present in the body's content-disposition
	 header field. RFC 2231 encoding is removed first.

	 The notes made for contentTypeParameter() above apply here as
	 well.
      */
      virtual QString contentDispositionParameter( const char * parameter ) const = 0;

      /**
	 @return whether this part already has it's complete body
	 fetched e.g. from an IMAP server.
      */
      virtual bool hasCompleteBody() const = 0;

      /**
	 @return the BodyPartMemento set for this part, or null, if
	 none is set.
      */
      virtual BodyPartMemento * memento() const = 0;

      /**
	 @return register an implementation of the BodyPartMemento
	 interface as a status object with this part.
      */
      virtual void setBodyPartMemento( BodyPartMemento * ) = 0;

      enum Display { None, AsIcon, Inline };
      /**
        @return whether this body part should be displayed iconic or inline
       */
      virtual Display defaultDisplay() const = 0;
    };

  } // namespace Interface

} // namespace KMail

#endif // __KMAIL_INTERFACES_BODYPART_H__
