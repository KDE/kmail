/*
    transportmanager.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "transportmanager.h"

#include "kmtransport.h"
#include "kmkernel.h"
#include <kconfig.h>

namespace KMail {

  QStringList TransportManager::transportNames()
  {
    KConfigGroup general( KMKernel::config(), "General");

    int numTransports = general.readNumEntry("transports", 0);

    QStringList transportNames;
    for ( int i = 1 ; i <= numTransports ; i++ ) {
      KMTransportInfo ti;
      ti.readConfig(i);
      transportNames << ti.name;
    }

    return transportNames;
  }

}; // namespace KMail
