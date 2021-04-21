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
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("ktnef");
    KCrash::initialize();
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("ktnef"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("ktnefrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("ktnefui.rc"));
    migrate.migrate();
#endif

    KAboutData aboutData(QStringLiteral("ktnef"),
                         i18n("KTnef"),
                         QStringLiteral(KTNEF_VERSION),
                         i18n("Viewer for mail attachments using TNEF format"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2000 Michael Goffioul \nCopyright 2012  Allen Winter"));

    aboutData.addAuthor(i18n("Michael Goffioul"), i18n("Author"), QStringLiteral("kdeprint@swing.be"));

    aboutData.addAuthor(i18n("Allen Winter"), i18n("Author, Ported to Qt4/KDE4"), QStringLiteral("winter@kde.org"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.setApplicationDescription(QApplication::applicationDisplayName());
    parser.addPositionalArgument(QStringLiteral("file"), i18n("An optional argument 'file' "), QStringLiteral("[file]"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    auto tnef = new KTNEFMain();
    tnef->show();
    const QStringList &args = parser.positionalArguments();

    if (!args.isEmpty()) {
        tnef->loadFile(args.constFirst());
    }

    return app.exec();
}
