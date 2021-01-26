/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../../../sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "../../../sieveimapinterface/kmsieveimappasswordprovider.h"
#include <KSieveUi/SieveDebugDialog>
#include <KSieveUi/SieveImapInstanceInterfaceManager>

#include <QApplication>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QApplication::setApplicationName(QStringLiteral("sievedebugdialog"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));
    QStandardPaths::setTestModeEnabled(true);

    KSieveUi::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    KMSieveImapPasswordProvider provider(nullptr);
    auto dlg = new KSieveUi::SieveDebugDialog(&provider);
    dlg->exec();
    delete dlg;
    return 0;
}
