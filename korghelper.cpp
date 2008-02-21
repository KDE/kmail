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

#include <kdebug.h>
#include <kdbusservicestarter.h>

void KMail::KorgHelper::ensureRunning()
{
  QString error;
  QString dbusService;
  int result = KDBusServiceStarter::self()->findServiceFor( "DBUS/Organizer",
                                         QString(), &error, &dbusService );
  if ( result == 0 ) {
#ifdef __GNUC__
#warning Port me!
#endif
#if 0
    // OK, so korganizer (or kontact) is running. Now ensure the object we want is available
    // [that's not the case when kontact was already running, but korganizer not loaded into it...]
    static const char* const dcopObjectId = "KOrganizerIface";
    QCString dummy;
    if ( !kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dummy, dummy ) ) {
      DCOPRef ref( dcopService, dcopService ); // talk to the KUniqueApplication or its kontact wrapper
      DCOPReply reply = ref.call( "load()" );
      if ( reply.isValid() && (bool)reply ) {
        kDebug() <<"Loaded" << dcopService <<" successfully";
        Q_ASSERT( kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dummy, dummy ) );
      } else
        kWarning() << "Error loading" << dcopService;
    }
#endif
  }
  else
    kWarning() << "Couldn't start DBUS/Organizer: " << dbusService << " " << error;
}
