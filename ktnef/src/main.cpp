/*
  This file is part of KTnef.

  Copyright (C) 2002 Michael Goffioul <kdeprint@swing.be>
  Copyright (c) 2012 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "ktnefmain.h"
#include "ktnef-version.h"

#include <Kdelibs4ConfigMigrator>
#include <kaboutdata.h>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QApplication>
#include <KDBusService>
#include <KCrash>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("ktnef");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    KCrash::initialize();
    Kdelibs4ConfigMigrator migrate(QStringLiteral("ktnef"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("ktnefrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("ktnefui.rc"));
    migrate.migrate();

    KAboutData aboutData(QStringLiteral("ktnef"),
                         i18n("KTnef"),
                         QStringLiteral(KTNEF_VERSION),
                         i18n("Viewer for mail attachments using TNEF format"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2000 Michael Goffioul\nCopyright 2012  Allen Winter"));

    aboutData.addAuthor(
        i18n("Michael Goffioul"),
        i18n("Author"),
        QStringLiteral("kdeprint@swing.be"));

    aboutData.addAuthor(
        i18n("Allen Winter"),
        i18n("Author, Ported to Qt4/KDE4"),
        QStringLiteral("winter@kde.org"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.setApplicationDescription(QApplication::applicationDisplayName());
    parser.addPositionalArgument(QStringLiteral("file"), i18n("An optional argument 'file' "), QStringLiteral("[file]"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    KTNEFMain *tnef = new KTNEFMain();
    tnef->show();
    const QStringList &args = parser.positionalArguments();

    if (!args.isEmpty()) {
        tnef->loadFile(args.constFirst());
    }

    return app.exec();
}
