/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sieveimapinterface/kmailsieveimapinstanceinterface.h"
#include "sieveimapinterface/kmsieveimappasswordprovider.h"
#include <KSieveCore/SieveImapInstanceInterfaceManager>
#include <KSieveUi/ManageSieveScriptsDialog>
#include <QApplication>
using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(u"managersievescriptsdialogtest"_s);
    QApplication::setApplicationVersion(u"1.0"_s);

    KSieveCore::SieveImapInstanceInterfaceManager::self()->setSieveImapInstanceInterface(new KMailSieveImapInstanceInterface);
    KMSieveImapPasswordProvider provider(nullptr);
    auto dlg = KSieveUi::ManageSieveScriptsDialog(&provider);
    dlg.show();
    return app.exec();
}
