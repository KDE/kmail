/*
   SPDX-FileCopyrightText: 2019-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "refreshsettingsassistant.h"

#include <Akonadi/ControlGui>

#include <KAboutData>
#include <QApplication>

#include <KDBusService>
#include <KLocalizedString>

#include <KCrash>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kmail-refresh-settings"));
    KCrash::initialize();
    KAboutData aboutData(QStringLiteral("kmail-refresh-settings"),
                         i18n("KMail Assistant for refreshing settings"),
                         QStringLiteral("0.1"),
                         i18n("KMail Assistant for refreshing settings"),
                         KAboutLicense::LGPL,
                         i18n("© 2019-2025 Laurent Montel <montel@kde.org>"));
    aboutData.addAuthor(i18nc("@info:credit", "Laurent Montel"), i18n("Author"), QStringLiteral("montel@kde.org"));

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
