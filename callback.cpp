/*  -*- c++ -*-
    callback.cpp

    This file is used by KMail's plugin interface.
    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

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

#include "callback.h"
#include "kmidentity.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include "identitymanager.h"
#include "kmmainwin.h"

#include <kinputdialog.h>
#include <klocale.h>
#include <kdebug.h>

using namespace KMail;


Callback::Callback( KMMessage* msg ) : mMsg( msg ),  mReceiverSet( false )
{
}

bool Callback::mail() const
{
  kdDebug(5006) << "bool Callback::mail() const: NYI\n";
  return false;
}

QString Callback::receiver() const
{
  if ( mReceiverSet )
    // Already figured this out
    return mReceiver;

  mReceiverSet = true;

  KMIdentity ident =
    kmkernel->identityManager()->identityForAddress( mMsg->to() );

  if( ident != KMIdentity::null ) {
    // That was easy
    mReceiver = ident.emailAddr();
  } else {
    QStringList addrs = KMMessage::splitEmailAddrList( mMsg->to() );
    if( addrs.count() == 1 )
      // Don't ask the user to choose between 1 items
      mReceiver = addrs[0];
    else {
      bool ok;
      mReceiver =
        KInputDialog::getItem( i18n( "Select Address" ),
                               i18n( "<qt>None of your identities match the "
                                     "receiver of this message,<br>please "
                                     "choose which of the following addresses "
                                     "is yours:" ),
                               addrs, 0, FALSE, &ok, kmkernel->mainWin() );
      if( !ok )
        mReceiver = QString::null;
    }
  }

  return mReceiver;
}
