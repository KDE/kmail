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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef KMSTARTUP
#define KMSTARTUP

#include <kdepimmacros.h>

extern "C" {

KDE_EXPORT void kmsetSignalHandler(void (*handler)(int));
KDE_EXPORT void kmsignalHandler(int sigId);
KDE_EXPORT void kmcrashHandler(int sigId);

}

namespace KMail
{
    KDE_EXPORT void checkConfigUpdates();
    KDE_EXPORT void lockOrDie();
    KDE_EXPORT void insertLibraryCataloguesAndIcons();
    KDE_EXPORT void cleanup();
}

#endif





