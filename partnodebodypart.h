/*  -*- mode: C++; c-file-style: "gnu" -*-
    partnodebodypart.h

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

#ifndef __KMAIL_BODYPART_H_
#define __KMAIL_BODYPART_H_

#include "interfaces/bodypart.h"

class partNode;

class QTextCodec;

namespace KMail {

  /**
     @short an implemenation of the BodyPart interface using partNodes
  */
  class PartNodeBodyPart : public Interface::BodyPart {
  public:
    PartNodeBodyPart( partNode & n, const QTextCodec * codec=0 );

    QString makeLink( const QString & path ) const;
    QString asText() const;
    QByteArray asBinary() const;
    QString contentTypeParameter( const char * param ) const;
    QString contentDescription() const;
    //int contentDisposition() const;
    QString contentDispositionParameter( const char * param ) const;
    bool hasCompleteBody() const;

    Interface::BodyPartMemento * memento() const;
    void setBodyPartMemento( Interface::BodyPartMemento * memento );
    BodyPart::Display defaultDisplay() const;
    void setDefaultDisplay( BodyPart::Display );

  private:
    partNode & mPartNode;
    const QTextCodec * mCodec;
    BodyPart::Display mDefaultDisplay;
  };

} // namespace KMail

#endif // __KMAIL_BODYPART_H_
