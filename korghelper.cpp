/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "korghelper.h"

#include <dcopclient.h>
#include <dcopref.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdcopservicestarter.h>

void KMail::KorgHelper::ensureRunning()
{
  QString error;
  QCString dcopService;
  int result = KDCOPServiceStarter::self()->findServiceFor( "DCOP/Organizer", QString::null, QString::null, &error, &dcopService );
  if ( result == 0 ) {
    // OK, so korganizer (or kontact) is running. Now ensure the object we want is available
    // [that's not the case when kontact was already running, but korganizer not loaded into it...]
    static const char* const dcopObjectId = "KOrganizerIface";
    QCString dummy;
    if ( !kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dummy, dummy ) ) {
      DCOPRef ref( dcopService, dcopService ); // talk to the KUniqueApplication or its kontact wrapper
      DCOPReply reply = ref.call( "load()" );
      if ( reply.isValid() && (bool)reply ) {
        kdDebug() << "Loaded " << dcopService << " successfully" << endl;
        Q_ASSERT( kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dummy, dummy ) );
      } else
        kdWarning() << "Error loading " << dcopService << endl;
    }

    // We don't do anything with it, we just need it to be running so that it handles
    // the incoming directory.
  }
  else
    kdWarning() << "Couldn't start DCOP/Organizer: " << dcopService << " " << error << endl;
}
