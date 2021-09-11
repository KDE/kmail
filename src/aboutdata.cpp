/*
    aboutdata.cpp

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "aboutdata.h"

#include "kmail-version.h"

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
// top level folder as well.
// Testing mosifications. August 2021. Permission requested of M. L. Montel. @dcb
// Commit statistics below current as of June 30, 2021.

static const about_data authors[] = {
    {"Laurent Montel", I18N_NOOP("Maintainer"), "montel@kde.org", nullptr}, // Last commit June 30, 2021. 8,006 commits.
    {"Thomas McGuire", I18N_NOOP("Former maintainer"), "mcguire@kde.org", nullptr}, // Last commit Sep 10, 2011 (9/10/11). 2,344 commits.
    {"Stefan Taferner", I18N_NOOP("Original author"), "taferner@kde.org", nullptr}, // Last commit Feb 14, 2001. 189 commits.
    {"Michael H\303\244ckel", I18N_NOOP("Former maintainer"), "haeckel@kde.org", nullptr}, // Last commit Nov 12, 2002. 818 commits.
    {"Ingo Kl\303\266cker", I18N_NOOP("Former maintainer"), "kloecker@kde.org", nullptr}, // Last commit May 14, 2021. 1,017 commits. 
    {"Don Sanders", I18N_NOOP("Former co-maintainer"), "sanders@kde.org", nullptr}, // Last commit Aug 22, 2005. 648 commits.
    {"Till Adam", I18N_NOOP("Former core developer"), "adam@kde.org", nullptr}, //last commit Oct 13, 2012. 1,517 commits.
    {"Volker Krause", I18N_NOOP("Core developer"), "vkrause@kde.org", nullptr}, // still active (2021). 911 commits.
    {"Carsten Burghardt", I18N_NOOP("Former core developer"), "burghardt@kde.org", nullptr},// Last commit Oct 6, 2006. 415 commits.
    {"Marc Mutz", I18N_NOOP("Former core developer"), "mutz@kde.org", nullptr}, // Last commit Aug 19, 2010. 900 commits.
    {"Zack Rusin", I18N_NOOP("Former core developer"), "zack@kde.org", nullptr}, // Last commit Apr 13, 2004. 127 commits.
    {"Daniel Naber", I18N_NOOP("Documentation"), "daniel.naber@t-online.de", nullptr}, // Last commit Nov 10, 2003. 124 commits.
    {"Toyohiro Asukai", I18N_NOOP("Japanese language support"), "toyohiro@ksmplus.com", nullptr}, // 8 patches for Japanese language. Last patch June 25, 2004.
    {"Waldo Bastian", I18N_NOOP("Numerous technical corrections"), "bastian@kde.org", nullptr}, // Last commit Jun 16, 2004. 118 commits.
    {"Ryan Breen", I18N_NOOP("System tray notification"), "ryan@ryanbreen.com", nullptr}, // Last commit Feb 7, 2003. 10 commits.
    {"Steven Brown", I18N_NOOP("Patch for PGP"), "swbrown@ucsd.edu", nullptr}, // No commits. Mentioned once -- patch for PGP, Mar. 8, 2001.
    {"Matthias Kalle Dalheimer", I18N_NOOP("Crypto support"), "kalle@kde.org", nullptr}, // Kalle Dalheimer in logs. Last commit Aug 30, 2002. 33 commits.
    {"Matt Douhan", I18N_NOOP("Improved internal program documentation"), "matt@fruitsalad.org", nullptr}, // Last commit Jul 28, 2005. 50 commits.
    {"Cristi Dumitrescu", nullptr, "cristid@chip.ro", nullptr}, // No commits. One patch, Apr 5, 2001.
    {"David Faure", I18N_NOOP("Major contributor, still active"), "faure@kde.org", nullptr}, // Last Commit Oct 6, 2020. 195 commits.
    {"Philippe Fremy", nullptr, "pfremy@chez.com", nullptr}, // No commits. Mentioned twice in 2001.
    {"Kurt Granroth", I18N_NOOP("Added maildir support"), "granroth@kde.org", nullptr}, // Last commit Jun 26, 2002. 20 commits.
    {"Andreas Gungl", I18N_NOOP("PGP 6 support and further enhancements of the encryption support"), "a.gungl@gmx.de", nullptr}, // Last commit Mar 25, 2007. 247 commits.
    {"Steffen Hansen", nullptr, "hansen@kde.org", nullptr}, // Last commit Jun 11, 2004. 12 commits.
    {"Igor Janssen", nullptr, "rm@linux.ru.net", nullptr}, // One commit dated May 28, 2001.
    {"Matt Johnston", nullptr, "matt@caifex.org", nullptr}, // No commits. Two patches, Nov - Dec, 2000.
    {"Christer Kaivo-oja", nullptr, "whizkid@telia.com", nullptr}, // No commits. One patch, Mar 7,2001.
    {"Lars Knoll", I18N_NOOP("Original encryption support PGP 2 and PGP 5 support"), "knoll@kde.org", nullptr}, // Last commit Sep 17, 2002. 22 commits.
    {"J. Nick Koston", I18N_NOOP("GnuPG support"), "bdraco@darkorb.net", nullptr}, // No commits. One or two patches in 1999 (GnuPG).
    {"Stephan Kulow", I18N_NOOP("Major conntributor"), "coolo@kde.org", nullptr}, // Last commit Aug 20, 2008. 354 commits.
    {"Guillaume Laurent", nullptr, "glaurent@telegraph-road.org", nullptr}, // Last Apr 19, 2001. 2 commits, five patches.
    {"Sam Magnuson", nullptr, "sam@trolltech.com", nullptr}, // One commit, Mar 10, 2003. Two patches.
    {"Matt Newell", nullptr, "newellm@proaxis.com", nullptr}, // No commits. Three patches, the latest one dated Feb 23, 2001
    {"Denis Perchine", nullptr, "dyp@perchine.com", nullptr}, // Last commit Dec 22, 2000. 4 commits, 1 large patch.
    {"Samuel Penn", nullptr, "sam@bifrost.demon.co.uk", nullptr}, // No commits. Two patches -- most recent Jan 1, 2002.
    {"Carsten Pfeiffer", I18N_NOOP("Various and sundry bug fixes"), "pfeiffer@kde.org", nullptr}, // Last commit Oct 5, 2003. 43 commits.
    {"Sven Radej", I18N_NOOP("Improved asynchronous processes"), "radej@kde.org", nullptr}, // Last commit Mar 30, 2000. 25 commits.
    {"Mark Roberts", nullptr, "mark@taurine.demon.co.uk", nullptr}, // No commits. One patch dated Sep 19, 2000.
    {"Wolfgang Rohdewald", nullptr, "wolfgang@rohdewald.de", nullptr}, // One commit, Dec 10, 2008.
    {"Espen Sand", I18N_NOOP("Enhanced configuration dialogs"), "espen@kde.org", nullptr}, // Last commit Jul 4, 2000. 30 commits.
    {"Aaron J. Seigo", I18N_NOOP("Notes/calendar integration"), "aseigo@olympusproject.org", nullptr}, // Last commit Oct 6, 2014. 50 commits.
    {"George Staikos", I18N_NOOP("Improve efficiency and consistency of code"), "staikos@kde.org", nullptr}, // Last commit Apr 15, 2005. 113 commits.
    {"Szymon Stefanek", I18N_NOOP("New message list and new folder tree"), "pragma@kvirc.net", nullptr}, // Last commit Sep 2, 2009. 94 commits.
    {"Jason Stephenson", nullptr, "panda@mis.net", nullptr}, // Last commit Mar 15, 2001. 8 commits.
    {"Jacek Stolarczyk", nullptr, "jacek@mer.chemia.polsl.gliwice.pl", nullptr}, // Last commit Nov 9, 2000. 16 commits.
    {"Roberto S. Teixeira", nullptr, "maragato@kde.org", nullptr}, // Last commit Oct 23, 2003. 5 commits.
    {"Bo Thorsen", I18N_NOOP("Many valuable contributions") , "bo@sonofthor.dk", nullptr}, // Last commit Nov17, 2004. 183 commits.
    {"Ronen Tzur", nullptr, "rtzur@shani.net", nullptr}, // No commits. 10 patches, last one dated Jan 4, 2002.
    {"Mario Weilguni", nullptr, "mweilguni@sime.com", nullptr}, // Last commit May 29, 2000. 10 commits.
    {"Wynn Wilkes", nullptr, "wynnw@calderasystems.com", nullptr}, // One commit, Jan 11, 2002. Also one patch.
    {"Robert D. Williams", nullptr, "rwilliams@kde.org", nullptr}, // Last commit Jun 2, 2000. 26 commits.
    {"Markus Wübben", I18N_NOOP("Killed lots of bugs"), "markus.wuebben@kde.org", nullptr}, // Last commit Jan 16, 1999. 77 commits.
    {"Karl-Heinz Zimmer", I18N_NOOP("Major contributor"), "khz@kde.org", nullptr} // Last commit Nov 19, 2004. 203 commits.
};

static const about_data credits[] = {
    {"Sam Abed", I18N_NOOP("Smiley-->emoticons processing"), "earlgreykde@pop.netspace.net.au", nullptr}, // KConfigXT porting, smileys->emoticons replacement
    {"Joern Ahrens", I18N_NOOP("Improve 'Mark Message as' popup menu"), "joern.ahrens@kdemail.net", nullptr}, // implement wish 77182 (Add some separators to "Mark Message as" popup menu)
    {"Tom Albers", I18N_NOOP("Bugzilla maintenance"), "toma@kde.org", nullptr}, // small fixes, bugzilla maintenance
    {"Jaime Torres Amate", I18N_NOOP("Improved execution speed"), "jtamate@gmail.com", nullptr}, // improvements in speed of execution. Avoid unnecessary processes.
    {"Albert Cervera Areny", I18N_NOOP("Optionally compress attachments"), "albertca@hotpop.com", nullptr}, // implemented wish 88309 (optional compression of attachments)
    {"Jonathan Armond", nullptr, "jon.armond@gmail.com", nullptr},
    {"Patrick Audley", I18N_NOOP("Added spam status to fancy headers"), nullptr, nullptr}, // add optional graphical spam status to fancy headers
    {"Benjamin Azan", I18N_NOOP("Implemented todo status handling"), "b.azan@free.fr", nullptr}, // implemented todo status handling
    {"Davide Bettio", I18N_NOOP("Bug fixes, code cleanup"), "davide.bettio@kdemail.net", nullptr}, // Bug fixes, code cleanup
    {"Pradeepto Bhattacharya", I18N_NOOP("LDAP support, maintenance"), "pradeepto@kde.org", nullptr}, // 294 commits. Lots of "bump version number" commits. LDAP patches
    {"Bruno Bigras", nullptr, "bigras.bruno@gmail.com", nullptr},
    {"Bertjan Broeksema", I18N_NOOP("Major contributor to Akonadi"), "broeksema@kde.org", nullptr}, // 382 commits, mostly on Akonadi
    {"David Bryant", I18N_NOOP("Documentation, still active"), "davidbryant@gvtc.com", "https://davidcbryant.net"}, // Last commit August 29, 2021. 35 commits.
    {"Albert Astals Cid", I18N_NOOP("Substantial contributor, still active"), "aacid@kde.org", nullptr}, // fix for bug:95441 (folder tree context menu doesn't show shortcuts assigned to the actions) 172 commits
    {"Frederick Emmott", I18N_NOOP("Anti-virus support"), "fred87@users.sf.net", nullptr},
    {"Christophe Giboudeaux", I18N_NOOP("Major contributor, still actiive"), "christophe@krop.fr", nullptr}, // 1,021 commits, latest in July, 2021
    {"Sandro Giessl", I18N_NOOP("Fixed widget styles"), "sgiessl@gmail.com", nullptr}, // frame width fixes for widget styles
    {"Olivier Goffart", I18N_NOOP("Bug fixes"), "ogoffart@kde.org", nullptr}, // Bug fixes, for the most part
    {"Severin Greimel", I18N_NOOP("Several patches"), "greimel-kde@fs-pw.uni-muenchen.de", nullptr}, // several patches
    {"Shaheed Haque", I18N_NOOP("Bug fixes"), "shaheedhaque@gmail.com", nullptr}, // fix for bug:69744 (Resource folders: "Journals" should be "Journal")
    {"Ingo Heeskens", I18N_NOOP("Enable external reference loading for a whole folder"), "ingo@fivemile.org", nullptr}, // implemented wish 34857 (per folder option for loading external references)
    {"Kurt Hindenburg", I18N_NOOP("Delete an entire thread"), "kurt.hindenburg@gmail.com", nullptr}, // implemented wish 89003 (delete whole thread)
    {"Heiko Hund", I18N_NOOP("POP filters"), "heiko@ist.eigentlich.net", nullptr},
    {"Torsten Kasch", I18N_NOOP("Patch Solaris crash"), "tk@genetik.uni-bielefeld.de", nullptr}, // crash fix for Solaris (cf. bug:68801)
    {"Jason 'vanRijn' Kasper", I18N_NOOP("Choice of fonts for new/unread/importamt messages"), "vr@movingparts.net", nullptr}, // implemented wish 79938 (configurable font for new/unread/important messages)
    {"Martijn Klingens", I18N_NOOP("Fix Quick Search keyboard navigation"), "klingens@kde.org", nullptr}, // fix keyboard navigation in the Status combo of the quick search
    {"Christoph Klünter", I18N_NOOP("Fix drag and drop from address book to composer"), "chris@inferno.nadir.org", nullptr}, // fix for bug:88216 (drag&drop from KAddressBook to the To: field)
    {"Martin Koller", I18N_NOOP("Substantial contributor"), "kollix@aon.at", nullptr}, // optional columns in the message list
    {"Tobias König", I18N_NOOP("Major contributor"), "tokoe@kde.org", nullptr}, // edit recent addresses, store email<->OpenPGP key association in address book
    {"Nikolai Kosjar", nullptr, "nikolai.kosjar@qt.io", nullptr},
    {"Francois Kritzinger", I18N_NOOP("Improve configuration dialog"), nullptr, nullptr}, // fix bug in configuration dialog
    {"Danny Kukawka", I18N_NOOP("Improved message importing"), "danny.kukawka@bisect.de", nullptr}, // DCOP enhancements for better message importing
    {"Roger Larsson", I18N_NOOP("Improve status bar messages"), "roger.larsson@norran.net", nullptr}, // add name of checked account to status bar message
    {"Michael Leupold", nullptr, "lemma@confuego.org", nullptr},
    {"Thiago Macieira", nullptr, "thiago@kde.org", nullptr},
    {"Andras Mantia", nullptr, "amantia@kde.org", nullptr},
    {"Jonathan Marten", nullptr, "jjm@keelhaul.me.uk", nullptr},
    {"Sergio Luis Martins", nullptr, "iamsergio@gmail.com", nullptr},
    {"Jeffrey McGee", I18N_NOOP("Patch for window resizing problem"), "jeffreym@cs.tamu.edu", nullptr}, // fix for bug:64251
    {"Thomas Moenicke", nullptr, "tm@php-qt.org", nullptr},
    {"Dirk Müller", I18N_NOOP("Optimize qt-cast; fix QUrl()"), "mueller@kde.org", nullptr}, // QUrl() fixes and qt_cast optimizations
    {"Torgny Nyblom", I18N_NOOP("Substantial contributor"), "nyblom@kde.org", nullptr},
    {"OpenUsability", I18N_NOOP("Usability tests and improvements"), "contact@openusability.org", "https://www.openusability.org"},
    {"Mario Teijeiro Otero", nullptr, "emeteo@escomposlinux.org", nullptr}, // various vendor annotations fixes
    {"Kevin Ottens", nullptr, "ervin@kde.org", nullptr},
    {"Simon Perreault", I18N_NOOP("Patch composer configuration"), "nomis80@nomis80.org", nullptr}, // make the composer remember its "Use Fixed Font" setting (bug 49481)
    {"Jakob Petsovits", nullptr, "jpetso@gmx.at", nullptr},
    {"Romain Pokrzywka", nullptr, "romain@kdab.net", nullptr},
    {"Bernhard Reiter", I18N_NOOP("Ägypten and Kroupware project management"), "bernhard@intevation.de", nullptr},
    {"Darío Andrés Rodríguez", nullptr, "andresbajotierra@gmail.com", nullptr},
    {"Edwin Schepers", I18N_NOOP("Improved HTML support"), "yez@familieschepers.nl", nullptr}, // composition of HTML messages
    {"Jakob Schröter", I18N_NOOP("X-Face support"), "cvsci@camaya.net", nullptr}, // implemented wish 28319 (X-Face support)
    {"Cornelius Schumacher", I18N_NOOP("Automated recipient selection"), "schumacher@kde.org", nullptr}, // implemented the new recipients editor and picker
    {"Jan Simonson", I18N_NOOP("Beta testing of PGP 6 support"), "jan@simonson.pp.se", nullptr},
    {"Paul Sprakes", I18N_NOOP("Bug fixes, cleaned up context menus"), "kdecvs@sprakes.co.uk", nullptr}, // fix for bug:63619 (filter button in toolbar doesn't work), context menu clean up
    {"Jarosław Staniek", I18N_NOOP("Port to MS Windows®"), "staniek@kde.org", nullptr}, // MS Windows porting
    {"Will Stephenson", I18N_NOOP("IM status indicator"), "wstephenson@kde.org", nullptr}, // added IM status indicator
    {"Hasso Tepper", I18N_NOOP("Improve layout of recipients editor"), "hasso@kde.org", nullptr}, // improve layout of recipients editor
    {"Frank Thieme", I18N_NOOP("Bug fixes"), "frank@fthieme.net", nullptr}, // patched two bugsin 2009 / 2010 timeframe
    {"Patrick S. Vogt", I18N_NOOP("Timestamp for 'Transmission completed' status messages"), "patrick.vogt@unibas.ch", nullptr},
    {"Jan-Oliver Wagner", I18N_NOOP("Ägypten and Kroupware project management"), "jan@intevation.de", nullptr},
    {"Wolfgang Westphal", I18N_NOOP("Multiple encryption keys per address"), "wolfgang.westphal@gmx.de", nullptr},
    {"Allen Winter", I18N_NOOP("Major contributor, still active"), "winter@kde.org", nullptr}, // 2,061 commits as of August, 2021
    {"Urs Wolfer", I18N_NOOP("Improved icon/image processing"), "uwolfer@kde.org", nullptr},
    {"Thorsten Zachmann", I18N_NOOP("POP filters"), "t.zachmann@zagge.de", nullptr},
    {"Thomas Zander", I18N_NOOP("Fixed many IMAP bugs"), "zander@kde.org", nullptr}};

AboutData::AboutData()
    : KAboutData(QStringLiteral("kmail2"),
                 i18n("KMail"),
                 QStringLiteral(KDEPIM_VERSION),
                 i18n("KDE Email Client"),
                 KAboutLicense::GPL,
                 i18n("Copyright © 1997–2021, KMail authors"),
                 QString(),
                 QStringLiteral("https://userbase.kde.org/KMail"))
{
    using KMail::authors;
    using KMail::credits;
    const unsigned int numberAuthors(sizeof authors / sizeof *authors);
    for (unsigned int i = 0; i < numberAuthors; ++i) {
        addAuthor(i18n(authors[i].name), authors[i].desc ? i18n(authors[i].desc) : QString(), QLatin1String(authors[i].email), QLatin1String(authors[i].web));
    }

    const unsigned int numberCredits(sizeof credits / sizeof *credits);
    for (unsigned int i = 0; i < numberCredits; ++i) {
        addCredit(i18n(credits[i].name), credits[i].desc ? i18n(credits[i].desc) : QString(), QLatin1String(credits[i].email), QLatin1String(credits[i].web));
    }
}

AboutData::~AboutData() = default;
} // namespace KMail
