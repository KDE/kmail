/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../../../sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "../../../sieveimapinterface/kmsieveimappasswordprovider.h"
#include <KSieveUi/ManageSieveScriptsDialog>
#include <KSieveUi/SieveImapInstanceInterfaceManager>
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("managersievescriptsdialogtest"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    KSieveUi::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    KMSieveImapPasswordProvider provider(nullptr);
    auto dlg = new KSieveUi::ManageSieveScriptsDialog(&provider);
    dlg->show();
    return app.exec();
}
