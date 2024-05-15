/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "ktnef-version.h"
#include "ktnefmain.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("ktnef"));
    KCrash::initialize();

    KAboutData aboutData(QStringLiteral("ktnef"),
                         i18n("KTnef"),
                         QStringLiteral(KTNEF_VERSION),
                         i18n("Viewer for mail attachments using TNEF format"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2000 Michael Goffioul \nCopyright 2012  Allen Winter"));

    aboutData.addAuthor(i18nc("@info:credit", "Michael Goffioul"), i18n("Author"), QStringLiteral("kdeprint@swing.be"));

    aboutData.addAuthor(i18nc("@info:credit", "Allen Winter"), i18n("Author, Ported to Qt4/KDE4"), QStringLiteral("winter@kde.org"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.setApplicationDescription(QApplication::applicationDisplayName());
    parser.addPositionalArgument(QStringLiteral("file"), i18n("An optional argument 'file' "), QStringLiteral("[file]"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));

    KDBusService service;

    auto tnef = new KTNEFMain();
    tnef->show();
    const QStringList &args = parser.positionalArguments();

    if (!args.isEmpty()) {
        tnef->loadFile(args.constFirst());
    }

    return app.exec();
}
