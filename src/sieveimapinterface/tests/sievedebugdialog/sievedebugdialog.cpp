/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "sieveimapinterface/kmsieveimappasswordprovider.h"
#include <KSieveCore/SieveImapInstanceInterfaceManager>
#include <KSieveUi/SieveDebugDialog>

#include <QApplication>
#include <QStandardPaths>
using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QApplication::setApplicationName(u"sievedebugdialog"_s);
    QApplication::setApplicationVersion(u"1.0"_s);
    QStandardPaths::setTestModeEnabled(true);

    KSieveCore::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    KMSieveImapPasswordProvider provider(nullptr);
    auto dlg = KSieveUi::SieveDebugDialog(&provider);
    dlg.exec();
    return 0;
}
