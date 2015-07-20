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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmstartup.h"

#include "kmkernel.h" //control center

#include <KLocalizedString>

#include <kglobal.h>
#include <kiconloader.h>

#undef Status // stupid X headers

namespace KMail
{


void insertLibraryCataloguesAndIcons()
{
    static const char *const catalogs[] = {
        "libkdepim",
        "libkleopatra",
        "libkpgp",
        "libmessagelist",
        "libmessageviewer",
        "libmessagecore",
        "libmessagecomposer",
        "libpimcommon",
        "libmailcommon",
        "libkpimtextedit"
    };

    KIconLoader *il = KIconLoader::global();
    unsigned int catalogSize = (sizeof catalogs / sizeof * catalogs);
    for (unsigned int i = 0 ; i < catalogSize ; ++i) {
        il->addAppDir(QLatin1String(catalogs[i]));
    }

}

}
