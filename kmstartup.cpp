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

#include <klocale.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <K4AboutData>
#include <kiconloader.h>
#include <KLocale>

#undef Status // stupid X headers

namespace KMail {

void checkConfigUpdates() {
    static const char * const updates[] = {
        "9",
        "3.1-update-identities",
        "3.1-use-identity-uoids",
        "3.1-new-mail-notification",
        "3.2-update-loop-on-goto-unread-settings",
        "3.1.4-dont-use-UOID-0-for-any-identity",  //krazy:exclude=spelling
        "3.2-misc",
        "3.2-moves",
        "3.3-use-ID-for-accounts",
        "3.3-update-filter-rules",
        "3.3-move-identities-to-own-file",
        "3.3-aegypten-kpgprc-to-kmailrc",
        "3.3-aegypten-kpgprc-to-libkleopatrarc",
        "3.3-aegypten-emailidentities-split-sign-encr-keys",
        "3.3-misc",
        "3.3b1-misc",
        "3.4-misc",
        "3.4a",
        "3.4b",
        "3.4.1",
        "3.5.4",
        "3.5.7-imap-flag-migration",
        "4.0-misc",
        "4.2",
        "4.4-akonadi",
        "4.12",
        "4.13",
        "4.13.2"
    };
    static const int numUpdates = sizeof updates / sizeof *updates;
    // Warning: do not remove entries in the above array, or the update-level check below will break

    KSharedConfig::Ptr config = KMKernel::self()->config();
    const int configUpdateLevel = GlobalSettings::self()->updateLevel();
    if ( configUpdateLevel == numUpdates ) // Optimize for the common case that everything is OK
        return;

    for ( int i = configUpdateLevel ; i < numUpdates ; ++i ) {
        config->checkUpdate( QLatin1String(updates[i]), QLatin1String("kmail.upd") );
    }
    GlobalSettings::self()->setUpdateLevel( numUpdates );
}

void insertLibraryCataloguesAndIcons() {
    static const char * const catalogs[] = {
        "libkdepim",
        "libksieve",
        "libkleopatra",
        "libkpgp",
        "libkmime",
        "libmessagelist",
        "libmessageviewer",
        "libmessagecore",
        "libmessagecomposer",
        "libpimcommon",
        "libmailcommon",
        "libtemplateparser",
        "libakonadi_next",
        "libakonadi-kde",
        "libakonadi-kmime",
        "libkpimtextedit",
        "libkpimutils",
        "akonadicontact",
        "kabc",
        "akonadi_sendlater_agent",
        "akonadi_folderagent_agent",
        "libkgapi"
    };

    KLocale * l = KLocale::global();
    KIconLoader * il = KIconLoader::global();
    for ( unsigned int i = 0 ; i < sizeof catalogs / sizeof *catalogs ; ++i ) {
        il->addAppDir( QLatin1String(catalogs[i]) );
    }

}

}
