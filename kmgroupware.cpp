/*
    kmgroupware.cpp

    This file is part of KMail.

    Copyright (c) 2003 - 2004 Bo Thorsen <bo@sonofthor.dk>
    Copyright (c) 2002 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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

#include "kmgroupware.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "libkcal/incidenceformatter.h"
#include <kdebug.h>
#include <mimelib/enum.h>
#include <assert.h>


bool vPartFoundAndDecoded( KMMessage* msg, QString& s )
{
  assert( msg );

  if( ( DwMime::kTypeText == msg->type() && ( DwMime::kSubtypeVCal   == msg->subtype() ||
                                              DwMime::kSubtypeXVCard == msg->subtype() ) ) ||
      ( DwMime::kTypeApplication == msg->type() &&
        DwMime::kSubtypeOctetStream == msg->subtype() ) )
  {
    s = QString::fromUtf8( msg->bodyDecoded() );
    return true;
  } else if( DwMime::kTypeMultipart == msg->type() &&
             (DwMime::kSubtypeMixed  == msg->subtype() ) ||
             (DwMime::kSubtypeAlternative  == msg->subtype() ))
  {
    // kdDebug(5006) << "KMGroupware looking for TNEF data" << endl;
    DwBodyPart* dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
                                              DwMime::kSubtypeMsTNEF );
    if( !dwPart )
      dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
                                    DwMime::kSubtypeOctetStream );
    if( dwPart ){
      // kdDebug(5006) << "KMGroupware analyzing TNEF data" << endl;
      KMMessagePart msgPart;
      KMMessage::bodyPart(dwPart, &msgPart);
      s = KCal::IncidenceFormatter::msTNEFToVPart( msgPart.bodyDecodedBinary() );
      return !s.isEmpty();
    } else {
      dwPart = msg->findDwBodyPart( DwMime::kTypeText, DwMime::kSubtypeVCal );
      if (dwPart) {
        KMMessagePart msgPart;
        KMMessage::bodyPart(dwPart, &msgPart);
        s = msgPart.body();
        return true;
      }
    }
  }else if( DwMime::kTypeMultipart == msg->type() &&
            DwMime::kSubtypeMixed  == msg->subtype() ) {
    // TODO: Something?
  }

  return false;
}
