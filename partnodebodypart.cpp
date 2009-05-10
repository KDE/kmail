/*  -*- mode: C++; c-file-style: "gnu" -*-
    partnodebodypart.cpp

    This file is part of KMail, the KDE mail client.
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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


#include "partnodebodypart.h"

#include "partNode.h"

#include <kurl.h>

#include <QString>

static int serial = 0;

KMail::PartNodeBodyPart::PartNodeBodyPart( partNode & n, const QTextCodec * codec  )
  : KMail::Interface::BodyPart(), mPartNode( n ), mCodec( codec ),
    mDefaultDisplay( KMail::Interface::BodyPart::None )
{}

QString KMail::PartNodeBodyPart::makeLink( const QString & path ) const {
  // FIXME: use a PRNG for the first arg, instead of a serial number
  return QString( "x-kmail:/bodypart/%1/%2/%3" )
    .arg( serial++ ).arg( mPartNode.nodeId() )
    .arg( QString::fromLatin1( KUrl::toPercentEncoding( path, "/" ) ) );
}

QString KMail::PartNodeBodyPart::asText() const {
  if ( mPartNode.type() != DwMime::kTypeText )
    return QString();
  return mPartNode.msgPart().bodyToUnicode( mCodec );
}

QByteArray KMail::PartNodeBodyPart::asBinary() const {
  return mPartNode.msgPart().bodyDecodedBinary();
}

QString KMail::PartNodeBodyPart::contentTypeParameter( const char * param ) const {
  return mPartNode.contentTypeParameter( param );
}

QString KMail::PartNodeBodyPart::contentDescription() const {
  return mPartNode.msgPart().contentDescription();
}

QString KMail::PartNodeBodyPart::contentDispositionParameter( const char * ) const {
  kWarning() << "Sorry, not yet implemented.";
  return QString();
}

bool KMail::PartNodeBodyPart::hasCompleteBody() const {
  kWarning() << "Sorry, not yet implemented.";
  return true;
}

KMail::Interface::BodyPartMemento * KMail::PartNodeBodyPart::memento() const {
  return mPartNode.bodyPartMemento();
}

void KMail::PartNodeBodyPart::setBodyPartMemento( Interface::BodyPartMemento * memento ) {
  mPartNode.setBodyPartMemento( memento );
}

KMail::Interface::BodyPart::Display KMail::PartNodeBodyPart::defaultDisplay() const {
  return mDefaultDisplay;
}

void KMail::PartNodeBodyPart::setDefaultDisplay( KMail::Interface::BodyPart::Display d ){
  mDefaultDisplay = d;
}
