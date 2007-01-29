/*
    transportmanager.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "transportmanager.h"

#include "kmtransport.h"
#include "kmkernel.h"
#include <kconfig.h>
//Added by qt3to4:
#include <QList>
#include <krandom.h>
#include <kconfiggroup.h>

namespace KMail {

  QStringList TransportManager::transportNames()
  {
    KConfigGroup general( KMKernel::config(), "General");

    int numTransports = general.readEntry( "transports", 0 );

    QStringList transportNames;
    for ( int i = 1 ; i <= numTransports ; i++ ) {
      KMTransportInfo ti;
      ti.readConfig(i);
      transportNames << ti.name;
    }

    return transportNames;
  }

  // more or less copied from AccountManager
  uint TransportManager::createId()
  {
    QList<unsigned int> usedIds;

    KConfigGroup general( KMKernel::config(), "General");
    int numTransports = general.readEntry( "transports", 0 );

    for ( int i = 1 ; i <= numTransports ; i++ ) {
      KMTransportInfo ti;
      ti.readConfig( i );
      usedIds << ti.id();
    }

    usedIds << 0; // 0 is default for unknown
    int newId;
    do
    {
      newId = KRandom::random();
    } while ( usedIds.contains(newId)  );

    return newId;
  }

} // namespace KMail
