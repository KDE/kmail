/*  -*- mode: C++; c-file-style: "gnu" -*-
    aboutdata.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

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

#include "aboutdata.h"

#include "kdepim-version.h"

#include <KLocalizedString>

namespace KMail
{

struct about_data {
    const char *name;
    const char *desc;
    const char *email;
    const char *web;
};

// This file should not be changed by anybody other than the maintainer
// If you change the maintainer here, change it in the MAINTAINERS file in the
// top level kdepim folder as well.

static const about_data authors[] = {
    {
        "Laurent Montel", I18N_NOOP("Maintainer"),
        "montel@kde.org", 0
    },
    {
        "Thomas McGuire", I18N_NOOP("Former maintainer"),
        "mcguire@kde.org", 0
    },
    {
        "Stefan Taferner", I18N_NOOP("Original author"),
        "taferner@kde.org", 0
    },
    {
        "Michael H\303\244ckel", I18N_NOOP("Former maintainer"),
        "haeckel@kde.org", 0
    },
    {
        "Ingo Kl\303\266cker", I18N_NOOP("Former maintainer"),
        "kloecker@kde.org", 0
    },
    {
        "Don Sanders", I18N_NOOP("Former co-maintainer"),
        "sanders@kde.org", 0
    },

    {
        "Till Adam", I18N_NOOP("Core developer"),
        "adam@kde.org", 0
    },
    {
        "Volker Krause", I18N_NOOP("Core developer"),
        "vkrause@kde.org", 0
    },
    {
        "Carsten Burghardt", I18N_NOOP("Former core developer"),
        "burghardt@kde.org", 0
    },
    {
        "Marc Mutz", I18N_NOOP("Former core developer"),
        "mutz@kde.org", 0
    },
    {
        "Zack Rusin", I18N_NOOP("Former core developer"),
        "zack@kde.org", 0
    },
    {
        "Daniel Naber", I18N_NOOP("Documentation"),
        "daniel.naber@t-online.de", 0
    },

    {
        "Toyohiro Asukai", 0,
        "toyohiro@ksmplus.com", 0
    },
    {
        "Waldo Bastian", 0,
        "bastian@kde.org", 0
    },
    {
        "Ryan Breen", I18N_NOOP("System tray notification"),
        "ryan@ryanbreen.com", 0
    },
    {
        "Steven Brown", 0,
        "swbrown@ucsd.edu", 0
    },
    {
        "Matthias Kalle Dalheimer", 0,
        "kalle@kde.org", 0
    },
    {
        "Matt Douhan", 0,
        "matt@fruitsalad.org", 0
    },
    {
        "Cristi Dumitrescu", 0,
        "cristid@chip.ro", 0
    },
    {
        "David Faure", 0,
        "faure@kde.org", 0
    },
    {
        "Philippe Fremy", 0,
        "pfremy@chez.com", 0
    },
    {
        "Kurt Granroth", 0,
        "granroth@kde.org", 0
    },
    {
        "Andreas Gungl", I18N_NOOP("PGP 6 support and further enhancements of the encryption support"),
        "a.gungl@gmx.de", 0
    },
    {
        "Steffen Hansen", 0,
        "hansen@kde.org", 0
    },
    {
        "Igor Janssen", 0,
        "rm@linux.ru.net", 0
    },
    {
        "Matt Johnston", 0,
        "matt@caifex.org", 0
    },
    {
        "Christer Kaivo-oja", 0,
        "whizkid@telia.com", 0
    },
    {
        "Lars Knoll", I18N_NOOP("Original encryption support<br/>"
        "PGP 2 and PGP 5 support"),
        "knoll@kde.org", 0
    },
    {
        "J. Nick Koston", I18N_NOOP("GnuPG support"),
        "bdraco@darkorb.net", 0
    },
    {
        "Stephan Kulow", 0,
        "coolo@kde.org", 0
    },
    {
        "Guillaume Laurent", 0,
        "glaurent@telegraph-road.org", 0
    },
    {
        "Sam Magnuson", 0,
        "sam@trolltech.com", 0
    },
    {
        "Matt Newell", 0,
        "newellm@proaxis.com", 0
    },
    {
        "Denis Perchine", 0,
        "dyp@perchine.com", 0
    },
    {
        "Samuel Penn", 0,
        "sam@bifrost.demon.co.uk", 0
    },
    {
        "Carsten Pfeiffer", 0,
        "pfeiffer@kde.org", 0
    },
    {
        "Sven Radej", 0,
        "radej@kde.org", 0
    },
    {
        "Mark Roberts", 0,
        "mark@taurine.demon.co.uk", 0
    },
    {
        "Wolfgang Rohdewald", 0,
        "wolfgang@rohdewald.de", 0
    },
    {
        "Espen Sand", 0,
        "espen@kde.org", 0
    },
    {
        "Aaron J. Seigo", 0,
        "aseigo@olympusproject.org", 0
    },
    {
        "George Staikos", 0,
        "staikos@kde.org", 0
    },
    {
        "Szymon Stefanek", I18N_NOOP("New message list and new folder tree"),
        "pragma@kvirc.net", 0
    },
    {
        "Jason Stephenson", 0,
        "panda@mis.net", 0
    },
    {
        "Jacek Stolarczyk", 0,
        "jacek@mer.chemia.polsl.gliwice.pl", 0
    },
    {
        "Roberto S. Teixeira", 0,
        "maragato@kde.org", 0
    },
    {
        "Bo Thorsen", 0,
        "bo@sonofthor.dk", 0
    },
    {
        "Ronen Tzur", 0,
        "rtzur@shani.net", 0
    },
    {
        "Mario Weilguni", 0,
        "mweilguni@sime.com", 0
    },
    {
        "Wynn Wilkes", 0,
        "wynnw@calderasystems.com", 0
    },
    {
        "Robert D. Williams", 0,
        "rwilliams@kde.org", 0
    },
    {
        "Markus W\303\274bben", 0,
        "markus.wuebben@kde.org", 0
    },
    {
        "Karl-Heinz Zimmer", 0,
        "khz@kde.org", 0
    }
};

static const about_data credits[] = {
    { "Sam Abed", 0, 0, 0 }, // KConfigXT porting, smileys->emoticons replacement
    { "Joern Ahrens", 0, 0, 0 }, // implement wish 77182 (Add some separators to "Mark Message as" popup menu)
    { "Tom Albers", 0, 0, 0 }, // small fixes, bugzilla maintenance
    { "Jaime Torres Amate", 0, "jtamate@gmail.com", 0 },
    { "Albert Cervera Areny", 0, 0, 0 }, // implemented wish 88309 (optional compression of attachments)
    { "Jonathan Armond", 0, "jon.armond@gmail.com", 0 },
    { "Patrick Audley", 0, 0, 0 }, // add optional graphical spam status to fancy headers
    { "Benjamin Azan", 0, 0, 0 }, // implemented todo status handling
    { "Davide Bettio", 0, "davide.bettio@kdemail.net", 0 },
    { "Pradeepto Bhattacharya", 0, "pradeepto@kde.org", 0 },
    { "Bruno Bigras", 0, "bigras.bruno@gmail.com", 0 },
    { "Bertjan Broeksema", 0 , "broeksema@kde.org", 0 },
    { "Albert Astals Cid", 0, 0, 0 }, // fix for bug:95441 (folder tree context menu doesn't show shortcuts assigned to the actions)
    { "Cornelius Schumacher", 0, "schumacher@kde.org", 0 }, // implemented the new recipients editor and picker
    {
        "Frederick Emmott", I18N_NOOP("Anti-virus support"),
        "fred87@users.sf.net", 0
    },
    { "Christophe Giboudeaux", 0, "cgiboudeaux@gmail.com", 0 },
    { "Sandro Giessl", 0, 0, 0 }, // frame width fixes for widget styles
    { "Olivier Goffart", 0, "ogoffart@kde.org", 0 },
    { "Severin Greimel", 0, 0, 0 }, // several patches
    { "Shaheed Haque", 0, 0, 0 }, // fix for bug:69744 (Resource folders: "Journals" should be "Journal")
    { "Ingo Heeskens", 0, 0, 0 }, // implemented wish 34857 (per folder option for loading external references)
    { "Kurt Hindenburg", 0, 0, 0 }, // implemented wish 89003 (delete whole thread)
    {
        "Heiko Hund", I18N_NOOP("POP filters"),
        "heiko@ist.eigentlich.net", 0
    },
    { "Torsten Kasch", 0, 0, 0 }, // crash fix for Solaris (cf. bug:68801)
    { "Jason 'vanRijn' Kasper", 0, 0, 0 }, // implemented wish 79938 (configurable font for new/unread/important messages)
    { "Martijn Klingens", 0, 0, 0 }, // fix keyboard navigation in the Status combo of the quick search
    { "Christoph Kl\303\274nter", 0, 0, 0 }, // fix for bug:88216 (drag&drop from KAddressBook to the To: field)
    { "Martin Koller", 0, 0, 0 }, // optional columns in the message list
    { "Tobias K\303\266nig", 0, 0, 0 }, // edit recent addresses, store email<->OpenPGP key association in address book
    { "Nikolai Kosjar", 0, "klebezettel@gmx.net", 0 },
    { "Francois Kritzinger", 0, 0, 0 }, // fix bug in configuration dialog
    { "Danny Kukawka", 0, 0, 0 }, // DCOP enhancements for better message importing
    { "Roger Larsson", 0, 0, 0 }, // add name of checked account to status bar message
    { "Michael Leupold", 0, "lemma@confuego.org", 0 },
    { "Thiago Macieira", 0, "thiago@kde.org", 0 },
    { "Andras Mantia", 0, "amantia@kde.org", 0 },
    { "Jonathan Marten", 0, "jjm@keelhaul.me.uk", 0 },
    { "Sergio Luis Martins", 0, "iamsergio@gmail.com", 0 },
    { "Jeffrey McGee", 0, 0, 0 }, // fix for bug:64251
    { "Thomas Moenicke", 0, "tm@php-qt.org", 0 },
    { "Dirk M\303\274ller", 0, 0, 0 }, // KUrl() fixes and qt_cast optimizations
    { "Torgny Nyblom", 0, "nyblom@kde.org", 0 },
    { "OpenUsability", I18N_NOOP("Usability tests and improvements"), 0, "http://www.openusability.org" },
    { "Mario Teijeiro Otero", 0, 0, 0 }, // various vendor annotations fixes
    { "Kevin Ottens", 0, "ervin@kde.org", 0 },
    { "Simon Perreault", 0, 0, 0 }, // make the composer remember its "Use Fixed Font" setting (bug 49481)
    { "Jakob Petsovits", 0, "jpetso@gmx.at", 0 },
    { "Romain Pokrzywka", 0, "romain@kdab.net", 0 },
    {
        "Bernhard Reiter", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "bernhard@intevation.de", 0
    },
    { "Darío Andrés Rodríguez", 0, "andresbajotierra@gmail.com", 0 },
    { "Edwin Schepers", I18N_NOOP("Improved HTML support"), "yez@familieschepers.nl", 0 },   // composition of HTML messages
    { "Jakob Schr\303\266ter", 0, 0, 0 }, // implemented wish 28319 (X-Face support)
    {
        "Jan Simonson", I18N_NOOP("Beta testing of PGP 6 support"),
        "jan@simonson.pp.se", 0
    },
    { "Paul Sprakes", 0, 0, 0 }, // fix for bug:63619 (filter button in toolbar doesn't work), context menu clean up
    { "Jarosław Staniek", 0, "staniek@kde.org", 0 }, // MS Windows porting
    { "Will Stephenson", 0, 0, 0 }, // added IM status indicator
    { "Hasso Tepper", 0, 0, 0 }, // improve layout of recipients editor
    { "Frank Thieme", 0, "frank@fthieme.net", 0 },
    {
        "Patrick S. Vogt", I18N_NOOP("Timestamp for 'Transmission completed' status messages"),
        "patrick.vogt@unibas.ch", 0
    },
    {
        "Jan-Oliver Wagner", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "jan@intevation.de", 0
    },
    {
        "Wolfgang Westphal", I18N_NOOP("Multiple encryption keys per address"),
        "wolfgang.westphal@gmx.de", 0
    },
    { "Allen Winter", 0, "winter@kde.org", 0 },
    { "Urs Wolfer", 0, "uwolfer@kde.org", 0 },
    {
        "Thorsten Zachmann", I18N_NOOP("POP filters"),
        "t.zachmann@zagge.de", 0
    },
    { "Thomas Zander", 0, 0, 0 }
};

AboutData::AboutData()
    : K4AboutData("kmail2", "kmail", ki18n("KMail"), KDEPIM_VERSION,
                  ki18n("KDE Email Client"), License_GPL,
                  ki18n("Copyright © 1997–2014, KMail authors"), KLocalizedString(),
                  "http://userbase.kde.org/KMail")
{
    setProgramIconName(QLatin1String("kmail"));
    using KMail::authors;
    using KMail::credits;
    const unsigned int numberAuthors(sizeof authors / sizeof * authors);
    for (unsigned int i = 0 ; i < numberAuthors; ++i) {
        addAuthor(ki18n(authors[i].name), ki18n(authors[i].desc), authors[i].email, authors[i].web);
    }

    const unsigned int numberCredits(sizeof credits / sizeof * credits);
    for (unsigned int i = 0 ; i < numberCredits; ++i) {
        addCredit(ki18n(credits[i].name), ki18n(credits[i].desc), credits[i].email, credits[i].web);
    }
}

AboutData::~AboutData()
{

}

} // namespace KMail
