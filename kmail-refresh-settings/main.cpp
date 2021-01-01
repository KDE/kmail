/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingsassistant.h"

#include <controlgui.h>

#include <KAboutData>
#include <QApplication>

#include <KDBusService>
#include <KLocalizedString>

#include <KCrash/KCrash>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kmail-refresh-settings");
    KCrash::initialize();
    KAboutData aboutData(QStringLiteral("kmail-refresh-settings"),
                         i18n("KMail Assistant for refreshing settings"),
                         QStringLiteral("0.1"),
                         i18n("KMail Assistant for refreshing settings"),
                         KAboutLicense::LGPL,
                         i18n("(c) 2019-2021 Laurent Montel <montel@kde.org>"));
    aboutData.addAuthor(i18n("Laurent Montel"), i18n("Author"), QStringLiteral("montel@kde.org"));

    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kontact")));
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service(KDBusService::Unique);

    Akonadi::ControlGui::start(nullptr);

    RefreshSettingsAssistant dlg(nullptr);
    dlg.show();
    return app.exec();
}
