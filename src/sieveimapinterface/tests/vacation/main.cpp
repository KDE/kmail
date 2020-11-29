/*
  SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>

#include <KSieveUi/MultiImapVacationDialog>
#include <KSieveUi/MultiImapVacationManager>
#include <KSieveUi/SieveImapInstanceInterfaceManager>
#include "../../../sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "../../../sieveimapinterface/kmsieveimappasswordprovider.h"
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(app);

    app.setQuitOnLastWindowClosed(true);
    KSieveUi::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    KMSieveImapPasswordProvider provider(nullptr);
    KSieveUi::MultiImapVacationManager manager(&provider);
    KSieveUi::MultiImapVacationDialog dlg(&manager);
    QObject::connect(&dlg, &KSieveUi::MultiImapVacationDialog::okClicked, &app, &QApplication::quit);
    QObject::connect(&dlg, &KSieveUi::MultiImapVacationDialog::cancelClicked, &app, &QApplication::quit);

    dlg.show();

    return app.exec();
}
