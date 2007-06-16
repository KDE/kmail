/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef KMSTARTUP
#define KMSTARTUP

#include "kmail_export.h"

extern "C" {
  KMAIL_EXPORT void kmsetSignalHandler( void (*handler)(int) );
  KMAIL_EXPORT void kmsignalHandler( int sigId );
  KMAIL_EXPORT void kmcrashHandler( int sigId );
}

namespace KMail {
  KMAIL_EXPORT void checkConfigUpdates();
  KMAIL_EXPORT void lockOrDie();
  KMAIL_EXPORT void insertLibraryCataloguesAndIcons();
  KMAIL_EXPORT void cleanup();
}

#endif





