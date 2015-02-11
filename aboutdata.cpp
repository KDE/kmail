/*
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
        "montel@kde.org", Q_NULLPTR
    },
    {
        "Thomas McGuire", I18N_NOOP("Former maintainer"),
        "mcguire@kde.org", Q_NULLPTR
    },
    {
        "Stefan Taferner", I18N_NOOP("Original author"),
        "taferner@kde.org", Q_NULLPTR
    },
    {
        "Michael H\303\244ckel", I18N_NOOP("Former maintainer"),
        "haeckel@kde.org", Q_NULLPTR
    },
    {
        "Ingo Kl\303\266cker", I18N_NOOP("Former maintainer"),
        "kloecker@kde.org", Q_NULLPTR
    },
    {
        "Don Sanders", I18N_NOOP("Former co-maintainer"),
        "sanders@kde.org", Q_NULLPTR
    },

    {
        "Till Adam", I18N_NOOP("Core developer"),
        "adam@kde.org", Q_NULLPTR
    },
    {
        "Volker Krause", I18N_NOOP("Core developer"),
        "vkrause@kde.org", Q_NULLPTR
    },
    {
        "Carsten Burghardt", I18N_NOOP("Former core developer"),
        "burghardt@kde.org", Q_NULLPTR
    },
    {
        "Marc Mutz", I18N_NOOP("Former core developer"),
        "mutz@kde.org", Q_NULLPTR
    },
    {
        "Zack Rusin", I18N_NOOP("Former core developer"),
        "zack@kde.org", Q_NULLPTR
    },
    {
        "Daniel Naber", I18N_NOOP("Documentation"),
        "daniel.naber@t-online.de", Q_NULLPTR
    },

    {
        "Toyohiro Asukai", Q_NULLPTR,
        "toyohiro@ksmplus.com", Q_NULLPTR
    },
    {
        "Waldo Bastian", Q_NULLPTR,
        "bastian@kde.org", Q_NULLPTR
    },
    {
        "Ryan Breen", I18N_NOOP("System tray notification"),
        "ryan@ryanbreen.com", Q_NULLPTR
    },
    {
        "Steven Brown", Q_NULLPTR,
        "swbrown@ucsd.edu", Q_NULLPTR
    },
    {
        "Matthias Kalle Dalheimer", Q_NULLPTR,
        "kalle@kde.org", Q_NULLPTR
    },
    {
        "Matt Douhan", Q_NULLPTR,
        "matt@fruitsalad.org", Q_NULLPTR
    },
    {
        "Cristi Dumitrescu", Q_NULLPTR,
        "cristid@chip.ro", Q_NULLPTR
    },
    {
        "David Faure", Q_NULLPTR,
        "faure@kde.org", Q_NULLPTR
    },
    {
        "Philippe Fremy", Q_NULLPTR,
        "pfremy@chez.com", Q_NULLPTR
    },
    {
        "Kurt Granroth", Q_NULLPTR,
        "granroth@kde.org", Q_NULLPTR
    },
    {
        "Andreas Gungl", I18N_NOOP("PGP 6 support and further enhancements of the encryption support"),
        "a.gungl@gmx.de", Q_NULLPTR
    },
    {
        "Steffen Hansen", Q_NULLPTR,
        "hansen@kde.org", Q_NULLPTR
    },
    {
        "Igor Janssen", Q_NULLPTR,
        "rm@linux.ru.net", Q_NULLPTR
    },
    {
        "Matt Johnston", Q_NULLPTR,
        "matt@caifex.org", Q_NULLPTR
    },
    {
        "Christer Kaivo-oja", Q_NULLPTR,
        "whizkid@telia.com", Q_NULLPTR
    },
    {
        "Lars Knoll", I18N_NOOP("Original encryption support<br/>"
        "PGP 2 and PGP 5 support"),
        "knoll@kde.org", Q_NULLPTR
    },
    {
        "J. Nick Koston", I18N_NOOP("GnuPG support"),
        "bdraco@darkorb.net", Q_NULLPTR
    },
    {
        "Stephan Kulow", Q_NULLPTR,
        "coolo@kde.org", Q_NULLPTR
    },
    {
        "Guillaume Laurent", Q_NULLPTR,
        "glaurent@telegraph-road.org", Q_NULLPTR
    },
    {
        "Sam Magnuson", Q_NULLPTR,
        "sam@trolltech.com", Q_NULLPTR
    },
    {
        "Matt Newell", Q_NULLPTR,
        "newellm@proaxis.com", Q_NULLPTR
    },
    {
        "Denis Perchine", Q_NULLPTR,
        "dyp@perchine.com", Q_NULLPTR
    },
    {
        "Samuel Penn", Q_NULLPTR,
        "sam@bifrost.demon.co.uk", Q_NULLPTR
    },
    {
        "Carsten Pfeiffer", Q_NULLPTR,
        "pfeiffer@kde.org", Q_NULLPTR
    },
    {
        "Sven Radej", Q_NULLPTR,
        "radej@kde.org", Q_NULLPTR
    },
    {
        "Mark Roberts", Q_NULLPTR,
        "mark@taurine.demon.co.uk", Q_NULLPTR
    },
    {
        "Wolfgang Rohdewald", Q_NULLPTR,
        "wolfgang@rohdewald.de", Q_NULLPTR
    },
    {
        "Espen Sand", Q_NULLPTR,
        "espen@kde.org", Q_NULLPTR
    },
    {
        "Aaron J. Seigo", Q_NULLPTR,
        "aseigo@olympusproject.org", Q_NULLPTR
    },
    {
        "George Staikos", Q_NULLPTR,
        "staikos@kde.org", Q_NULLPTR
    },
    {
        "Szymon Stefanek", I18N_NOOP("New message list and new folder tree"),
        "pragma@kvirc.net", Q_NULLPTR
    },
    {
        "Jason Stephenson", Q_NULLPTR,
        "panda@mis.net", Q_NULLPTR
    },
    {
        "Jacek Stolarczyk", Q_NULLPTR,
        "jacek@mer.chemia.polsl.gliwice.pl", Q_NULLPTR
    },
    {
        "Roberto S. Teixeira", Q_NULLPTR,
        "maragato@kde.org", Q_NULLPTR
    },
    {
        "Bo Thorsen", Q_NULLPTR,
        "bo@sonofthor.dk", Q_NULLPTR
    },
    {
        "Ronen Tzur", Q_NULLPTR,
        "rtzur@shani.net", Q_NULLPTR
    },
    {
        "Mario Weilguni", Q_NULLPTR,
        "mweilguni@sime.com", Q_NULLPTR
    },
    {
        "Wynn Wilkes", Q_NULLPTR,
        "wynnw@calderasystems.com", Q_NULLPTR
    },
    {
        "Robert D. Williams", Q_NULLPTR,
        "rwilliams@kde.org", Q_NULLPTR
    },
    {
        "Markus W\303\274bben", Q_NULLPTR,
        "markus.wuebben@kde.org", Q_NULLPTR
    },
    {
        "Karl-Heinz Zimmer", Q_NULLPTR,
        "khz@kde.org", Q_NULLPTR
    }
};

static const about_data credits[] = {
    { "Sam Abed", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // KConfigXT porting, smileys->emoticons replacement
    { "Joern Ahrens", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implement wish 77182 (Add some separators to "Mark Message as" popup menu)
    { "Tom Albers", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // small fixes, bugzilla maintenance
    { "Jaime Torres Amate", Q_NULLPTR, "jtamate@gmail.com", Q_NULLPTR },
    { "Albert Cervera Areny", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented wish 88309 (optional compression of attachments)
    { "Jonathan Armond", Q_NULLPTR, "jon.armond@gmail.com", Q_NULLPTR },
    { "Patrick Audley", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // add optional graphical spam status to fancy headers
    { "Benjamin Azan", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented todo status handling
    { "Davide Bettio", Q_NULLPTR, "davide.bettio@kdemail.net", Q_NULLPTR },
    { "Pradeepto Bhattacharya", Q_NULLPTR, "pradeepto@kde.org", Q_NULLPTR },
    { "Bruno Bigras", Q_NULLPTR, "bigras.bruno@gmail.com", Q_NULLPTR },
    { "Bertjan Broeksema", Q_NULLPTR , "broeksema@kde.org", Q_NULLPTR },
    { "Albert Astals Cid", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix for bug:95441 (folder tree context menu doesn't show shortcuts assigned to the actions)
    { "Cornelius Schumacher", Q_NULLPTR, "schumacher@kde.org", Q_NULLPTR }, // implemented the new recipients editor and picker
    {
        "Frederick Emmott", I18N_NOOP("Anti-virus support"),
        "fred87@users.sf.net", Q_NULLPTR
    },
    { "Christophe Giboudeaux", Q_NULLPTR, "cgiboudeaux@gmail.com", Q_NULLPTR },
    { "Sandro Giessl", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // frame width fixes for widget styles
    { "Olivier Goffart", Q_NULLPTR, "ogoffart@kde.org", Q_NULLPTR },
    { "Severin Greimel", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // several patches
    { "Shaheed Haque", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix for bug:69744 (Resource folders: "Journals" should be "Journal")
    { "Ingo Heeskens", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented wish 34857 (per folder option for loading external references)
    { "Kurt Hindenburg", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented wish 89003 (delete whole thread)
    {
        "Heiko Hund", I18N_NOOP("POP filters"),
        "heiko@ist.eigentlich.net", Q_NULLPTR
    },
    { "Torsten Kasch", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // crash fix for Solaris (cf. bug:68801)
    { "Jason 'vanRijn' Kasper", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented wish 79938 (configurable font for new/unread/important messages)
    { "Martijn Klingens", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix keyboard navigation in the Status combo of the quick search
    { "Christoph Kl\303\274nter", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix for bug:88216 (drag&drop from KAddressBook to the To: field)
    { "Martin Koller", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // optional columns in the message list
    { "Tobias K\303\266nig", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // edit recent addresses, store email<->OpenPGP key association in address book
    { "Nikolai Kosjar", Q_NULLPTR, "klebezettel@gmx.net", Q_NULLPTR },
    { "Francois Kritzinger", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix bug in configuration dialog
    { "Danny Kukawka", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // DCOP enhancements for better message importing
    { "Roger Larsson", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // add name of checked account to status bar message
    { "Michael Leupold", Q_NULLPTR, "lemma@confuego.org", Q_NULLPTR },
    { "Thiago Macieira", Q_NULLPTR, "thiago@kde.org", Q_NULLPTR },
    { "Andras Mantia", Q_NULLPTR, "amantia@kde.org", Q_NULLPTR },
    { "Jonathan Marten", Q_NULLPTR, "jjm@keelhaul.me.uk", Q_NULLPTR },
    { "Sergio Luis Martins", Q_NULLPTR, "iamsergio@gmail.com", Q_NULLPTR },
    { "Jeffrey McGee", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix for bug:64251
    { "Thomas Moenicke", Q_NULLPTR, "tm@php-qt.org", Q_NULLPTR },
    { "Dirk M\303\274ller", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // QUrl() fixes and qt_cast optimizations
    { "Torgny Nyblom", Q_NULLPTR, "nyblom@kde.org", Q_NULLPTR },
    { "OpenUsability", I18N_NOOP("Usability tests and improvements"), Q_NULLPTR, "http://www.openusability.org" },
    { "Mario Teijeiro Otero", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // various vendor annotations fixes
    { "Kevin Ottens", Q_NULLPTR, "ervin@kde.org", Q_NULLPTR },
    { "Simon Perreault", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // make the composer remember its "Use Fixed Font" setting (bug 49481)
    { "Jakob Petsovits", Q_NULLPTR, "jpetso@gmx.at", Q_NULLPTR },
    { "Romain Pokrzywka", Q_NULLPTR, "romain@kdab.net", Q_NULLPTR },
    {
        "Bernhard Reiter", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "bernhard@intevation.de", Q_NULLPTR
    },
    { "Darío Andrés Rodríguez", Q_NULLPTR, "andresbajotierra@gmail.com", Q_NULLPTR },
    { "Edwin Schepers", I18N_NOOP("Improved HTML support"), "yez@familieschepers.nl", Q_NULLPTR },   // composition of HTML messages
    { "Jakob Schr\303\266ter", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // implemented wish 28319 (X-Face support)
    {
        "Jan Simonson", I18N_NOOP("Beta testing of PGP 6 support"),
        "jan@simonson.pp.se", Q_NULLPTR
    },
    { "Paul Sprakes", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // fix for bug:63619 (filter button in toolbar doesn't work), context menu clean up
    { "Jarosław Staniek", Q_NULLPTR, "staniek@kde.org", Q_NULLPTR }, // MS Windows porting
    { "Will Stephenson", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // added IM status indicator
    { "Hasso Tepper", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }, // improve layout of recipients editor
    { "Frank Thieme", Q_NULLPTR, "frank@fthieme.net", Q_NULLPTR },
    {
        "Patrick S. Vogt", I18N_NOOP("Timestamp for 'Transmission completed' status messages"),
        "patrick.vogt@unibas.ch", Q_NULLPTR
    },
    {
        "Jan-Oliver Wagner", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "jan@intevation.de", Q_NULLPTR
    },
    {
        "Wolfgang Westphal", I18N_NOOP("Multiple encryption keys per address"),
        "wolfgang.westphal@gmx.de", Q_NULLPTR
    },
    { "Allen Winter", Q_NULLPTR, "winter@kde.org", Q_NULLPTR },
    { "Urs Wolfer", Q_NULLPTR, "uwolfer@kde.org", Q_NULLPTR },
    {
        "Thorsten Zachmann", I18N_NOOP("POP filters"),
        "t.zachmann@zagge.de", Q_NULLPTR
    },
    { "Thomas Zander", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR }
};

AboutData::AboutData()
    : K4AboutData("kmail2", "kmail", ki18n("KMail"), KDEPIM_VERSION,
                  ki18n("KDE Email Client"), License_GPL,
                  ki18n("Copyright © 1997–2015, KMail authors"), KLocalizedString(),
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
