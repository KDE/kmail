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

#include "kmail-version.h"

#include <KLocalizedString>

namespace KMail {
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
        "montel@kde.org", nullptr
    },
    {
        "Thomas McGuire", I18N_NOOP("Former maintainer"),
        "mcguire@kde.org", nullptr
    },
    {
        "Stefan Taferner", I18N_NOOP("Original author"),
        "taferner@kde.org", nullptr
    },
    {
        "Michael H\303\244ckel", I18N_NOOP("Former maintainer"),
        "haeckel@kde.org", nullptr
    },
    {
        "Ingo Kl\303\266cker", I18N_NOOP("Former maintainer"),
        "kloecker@kde.org", nullptr
    },
    {
        "Don Sanders", I18N_NOOP("Former co-maintainer"),
        "sanders@kde.org", nullptr
    },

    {
        "Till Adam", I18N_NOOP("Core developer"),
        "adam@kde.org", nullptr
    },
    {
        "Volker Krause", I18N_NOOP("Core developer"),
        "vkrause@kde.org", nullptr
    },
    {
        "Carsten Burghardt", I18N_NOOP("Former core developer"),
        "burghardt@kde.org", nullptr
    },
    {
        "Marc Mutz", I18N_NOOP("Former core developer"),
        "mutz@kde.org", nullptr
    },
    {
        "Zack Rusin", I18N_NOOP("Former core developer"),
        "zack@kde.org", nullptr
    },
    {
        "Daniel Naber", I18N_NOOP("Documentation"),
        "daniel.naber@t-online.de", nullptr
    },

    {
        "Toyohiro Asukai", nullptr,
        "toyohiro@ksmplus.com", nullptr
    },
    {
        "Waldo Bastian", nullptr,
        "bastian@kde.org", nullptr
    },
    {
        "Ryan Breen", I18N_NOOP("System tray notification"),
        "ryan@ryanbreen.com", nullptr
    },
    {
        "Steven Brown", nullptr,
        "swbrown@ucsd.edu", nullptr
    },
    {
        "Matthias Kalle Dalheimer", nullptr,
        "kalle@kde.org", nullptr
    },
    {
        "Matt Douhan", nullptr,
        "matt@fruitsalad.org", nullptr
    },
    {
        "Cristi Dumitrescu", nullptr,
        "cristid@chip.ro", nullptr
    },
    {
        "David Faure", nullptr,
        "faure@kde.org", nullptr
    },
    {
        "Philippe Fremy", nullptr,
        "pfremy@chez.com", nullptr
    },
    {
        "Kurt Granroth", nullptr,
        "granroth@kde.org", nullptr
    },
    {
        "Andreas Gungl", I18N_NOOP("PGP 6 support and further enhancements of the encryption support"),
        "a.gungl@gmx.de", nullptr
    },
    {
        "Steffen Hansen", nullptr,
        "hansen@kde.org", nullptr
    },
    {
        "Igor Janssen", nullptr,
        "rm@linux.ru.net", nullptr
    },
    {
        "Matt Johnston", nullptr,
        "matt@caifex.org", nullptr
    },
    {
        "Christer Kaivo-oja", nullptr,
        "whizkid@telia.com", nullptr
    },
    {
        "Lars Knoll", I18N_NOOP("Original encryption support PGP 2 and PGP 5 support"),
        "knoll@kde.org", nullptr
    },
    {
        "J. Nick Koston", I18N_NOOP("GnuPG support"),
        "bdraco@darkorb.net", nullptr
    },
    {
        "Stephan Kulow", nullptr,
        "coolo@kde.org", nullptr
    },
    {
        "Guillaume Laurent", nullptr,
        "glaurent@telegraph-road.org", nullptr
    },
    {
        "Sam Magnuson", nullptr,
        "sam@trolltech.com", nullptr
    },
    {
        "Matt Newell", nullptr,
        "newellm@proaxis.com", nullptr
    },
    {
        "Denis Perchine", nullptr,
        "dyp@perchine.com", nullptr
    },
    {
        "Samuel Penn", nullptr,
        "sam@bifrost.demon.co.uk", nullptr
    },
    {
        "Carsten Pfeiffer", nullptr,
        "pfeiffer@kde.org", nullptr
    },
    {
        "Sven Radej", nullptr,
        "radej@kde.org", nullptr
    },
    {
        "Mark Roberts", nullptr,
        "mark@taurine.demon.co.uk", nullptr
    },
    {
        "Wolfgang Rohdewald", nullptr,
        "wolfgang@rohdewald.de", nullptr
    },
    {
        "Espen Sand", nullptr,
        "espen@kde.org", nullptr
    },
    {
        "Aaron J. Seigo", nullptr,
        "aseigo@olympusproject.org", nullptr
    },
    {
        "George Staikos", nullptr,
        "staikos@kde.org", nullptr
    },
    {
        "Szymon Stefanek", I18N_NOOP("New message list and new folder tree"),
        "pragma@kvirc.net", nullptr
    },
    {
        "Jason Stephenson", nullptr,
        "panda@mis.net", nullptr
    },
    {
        "Jacek Stolarczyk", nullptr,
        "jacek@mer.chemia.polsl.gliwice.pl", nullptr
    },
    {
        "Roberto S. Teixeira", nullptr,
        "maragato@kde.org", nullptr
    },
    {
        "Bo Thorsen", nullptr,
        "bo@sonofthor.dk", nullptr
    },
    {
        "Ronen Tzur", nullptr,
        "rtzur@shani.net", nullptr
    },
    {
        "Mario Weilguni", nullptr,
        "mweilguni@sime.com", nullptr
    },
    {
        "Wynn Wilkes", nullptr,
        "wynnw@calderasystems.com", nullptr
    },
    {
        "Robert D. Williams", nullptr,
        "rwilliams@kde.org", nullptr
    },
    {
        "Markus W\303\274bben", nullptr,
        "markus.wuebben@kde.org", nullptr
    },
    {
        "Karl-Heinz Zimmer", nullptr,
        "khz@kde.org", nullptr
    }
};

static const about_data credits[] = {
    { "Sam Abed", nullptr, nullptr, nullptr }, // KConfigXT porting, smileys->emoticons replacement
    { "Joern Ahrens", nullptr, nullptr, nullptr }, // implement wish 77182 (Add some separators to "Mark Message as" popup menu)
    { "Tom Albers", nullptr, nullptr, nullptr }, // small fixes, bugzilla maintenance
    { "Jaime Torres Amate", nullptr, "jtamate@gmail.com", nullptr },
    { "Albert Cervera Areny", nullptr, nullptr, nullptr }, // implemented wish 88309 (optional compression of attachments)
    { "Jonathan Armond", nullptr, "jon.armond@gmail.com", nullptr },
    { "Patrick Audley", nullptr, nullptr, nullptr }, // add optional graphical spam status to fancy headers
    { "Benjamin Azan", nullptr, nullptr, nullptr }, // implemented todo status handling
    { "Davide Bettio", nullptr, "davide.bettio@kdemail.net", nullptr },
    { "Pradeepto Bhattacharya", nullptr, "pradeepto@kde.org", nullptr },
    { "Bruno Bigras", nullptr, "bigras.bruno@gmail.com", nullptr },
    { "Bertjan Broeksema", nullptr, "broeksema@kde.org", nullptr },
    { "Albert Astals Cid", nullptr, nullptr, nullptr }, // fix for bug:95441 (folder tree context menu doesn't show shortcuts assigned to the actions)
    { "Cornelius Schumacher", nullptr, "schumacher@kde.org", nullptr }, // implemented the new recipients editor and picker
    {
        "Frederick Emmott", I18N_NOOP("Anti-virus support"),
        "fred87@users.sf.net", nullptr
    },
    { "Christophe Giboudeaux", nullptr, "cgiboudeaux@gmail.com", nullptr },
    { "Sandro Giessl", nullptr, nullptr, nullptr }, // frame width fixes for widget styles
    { "Olivier Goffart", nullptr, "ogoffart@kde.org", nullptr },
    { "Severin Greimel", nullptr, nullptr, nullptr }, // several patches
    { "Shaheed Haque", nullptr, nullptr, nullptr }, // fix for bug:69744 (Resource folders: "Journals" should be "Journal")
    { "Ingo Heeskens", nullptr, nullptr, nullptr }, // implemented wish 34857 (per folder option for loading external references)
    { "Kurt Hindenburg", nullptr, nullptr, nullptr }, // implemented wish 89003 (delete whole thread)
    {
        "Heiko Hund", I18N_NOOP("POP filters"),
        "heiko@ist.eigentlich.net", nullptr
    },
    { "Torsten Kasch", nullptr, nullptr, nullptr }, // crash fix for Solaris (cf. bug:68801)
    { "Jason 'vanRijn' Kasper", nullptr, nullptr, nullptr }, // implemented wish 79938 (configurable font for new/unread/important messages)
    { "Martijn Klingens", nullptr, nullptr, nullptr }, // fix keyboard navigation in the Status combo of the quick search
    { "Christoph Kl\303\274nter", nullptr, nullptr, nullptr }, // fix for bug:88216 (drag&drop from KAddressBook to the To: field)
    { "Martin Koller", nullptr, nullptr, nullptr }, // optional columns in the message list
    { "Tobias K\303\266nig", nullptr, nullptr, nullptr }, // edit recent addresses, store email<->OpenPGP key association in address book
    { "Nikolai Kosjar", nullptr, "klebezettel@gmx.net", nullptr },
    { "Francois Kritzinger", nullptr, nullptr, nullptr }, // fix bug in configuration dialog
    { "Danny Kukawka", nullptr, nullptr, nullptr }, // DCOP enhancements for better message importing
    { "Roger Larsson", nullptr, nullptr, nullptr }, // add name of checked account to status bar message
    { "Michael Leupold", nullptr, "lemma@confuego.org", nullptr },
    { "Thiago Macieira", nullptr, "thiago@kde.org", nullptr },
    { "Andras Mantia", nullptr, "amantia@kde.org", nullptr },
    { "Jonathan Marten", nullptr, "jjm@keelhaul.me.uk", nullptr },
    { "Sergio Luis Martins", nullptr, "iamsergio@gmail.com", nullptr },
    { "Jeffrey McGee", nullptr, nullptr, nullptr }, // fix for bug:64251
    { "Thomas Moenicke", nullptr, "tm@php-qt.org", nullptr },
    { "Dirk M\303\274ller", nullptr, nullptr, nullptr }, // QUrl() fixes and qt_cast optimizations
    { "Torgny Nyblom", nullptr, "nyblom@kde.org", nullptr },
    { "OpenUsability", I18N_NOOP("Usability tests and improvements"), nullptr, "http://www.openusability.org" },
    { "Mario Teijeiro Otero", nullptr, nullptr, nullptr }, // various vendor annotations fixes
    { "Kevin Ottens", nullptr, "ervin@kde.org", nullptr },
    { "Simon Perreault", nullptr, nullptr, nullptr }, // make the composer remember its "Use Fixed Font" setting (bug 49481)
    { "Jakob Petsovits", nullptr, "jpetso@gmx.at", nullptr },
    { "Romain Pokrzywka", nullptr, "romain@kdab.net", nullptr },
    {
        "Bernhard Reiter", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "bernhard@intevation.de", nullptr
    },
    { "Darío Andrés Rodríguez", nullptr, "andresbajotierra@gmail.com", nullptr },
    { "Edwin Schepers", I18N_NOOP("Improved HTML support"), "yez@familieschepers.nl", nullptr },   // composition of HTML messages
    { "Jakob Schr\303\266ter", nullptr, nullptr, nullptr }, // implemented wish 28319 (X-Face support)
    {
        "Jan Simonson", I18N_NOOP("Beta testing of PGP 6 support"),
        "jan@simonson.pp.se", nullptr
    },
    { "Paul Sprakes", nullptr, nullptr, nullptr }, // fix for bug:63619 (filter button in toolbar doesn't work), context menu clean up
    { "Jarosław Staniek", nullptr, "staniek@kde.org", nullptr }, // MS Windows porting
    { "Will Stephenson", nullptr, nullptr, nullptr }, // added IM status indicator
    { "Hasso Tepper", nullptr, nullptr, nullptr }, // improve layout of recipients editor
    { "Frank Thieme", nullptr, "frank@fthieme.net", nullptr },
    {
        "Patrick S. Vogt", I18N_NOOP("Timestamp for 'Transmission completed' status messages"),
        "patrick.vogt@unibas.ch", nullptr
    },
    {
        "Jan-Oliver Wagner", I18N_NOOP("\xC3\x84gypten and Kroupware project management"),
        "jan@intevation.de", nullptr
    },
    {
        "Wolfgang Westphal", I18N_NOOP("Multiple encryption keys per address"),
        "wolfgang.westphal@gmx.de", nullptr
    },
    { "Allen Winter", nullptr, "winter@kde.org", nullptr },
    { "Urs Wolfer", nullptr, "uwolfer@kde.org", nullptr },
    {
        "Thorsten Zachmann", I18N_NOOP("POP filters"),
        "t.zachmann@zagge.de", nullptr
    },
    { "Thomas Zander", nullptr, nullptr, nullptr }
};

AboutData::AboutData()
    : KAboutData(QStringLiteral("kmail2"),
                 i18n("KMail"),
                 QStringLiteral(KDEPIM_VERSION),
                 i18n("KDE Email Client"),
                 KAboutLicense::GPL,
                 i18n("Copyright © 1997–2017, KMail authors"),
                 QString(),
                 QStringLiteral("http://userbase.kde.org/KMail"))
{
    using KMail::authors;
    using KMail::credits;
    const unsigned int numberAuthors(sizeof authors / sizeof *authors);
    for (unsigned int i = 0; i < numberAuthors; ++i) {
        addAuthor(i18n(authors[i].name), authors[i].desc ? i18n(authors[i].desc) : QString(),
                  QLatin1String(authors[i].email), QLatin1String(authors[i].web));
    }

    const unsigned int numberCredits(sizeof credits / sizeof *credits);
    for (unsigned int i = 0; i < numberCredits; ++i) {
        addCredit(i18n(credits[i].name), credits[i].desc ? i18n(credits[i].desc) : QString(),
                  QLatin1String(credits[i].email), QLatin1String(credits[i].web));
    }
}

AboutData::~AboutData()
{
}
} // namespace KMail
