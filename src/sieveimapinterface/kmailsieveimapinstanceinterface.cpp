/*
   Copyright (C) 2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kmailsieveimapinstanceinterface.h"
#include <KSieveUi/SieveImapInstance>
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>

KMailSieveImapInstanceInterface::KMailSieveImapInstanceInterface()
{
}

QVector<KSieveUi::SieveImapInstance> KMailSieveImapInstanceInterface::sieveImapInstances()
{
    QVector<KSieveUi::SieveImapInstance> listInstance;

    const Akonadi::AgentInstance::List allInstances = Akonadi::AgentManager::self()->instances();
    listInstance.reserve(allInstances.count());
    for (const Akonadi::AgentInstance &instance : allInstances) {
        KSieveUi::SieveImapInstance sieveInstance;
        sieveInstance.setCapabilities(instance.type().capabilities());
        sieveInstance.setIdentifier(instance.identifier());
        sieveInstance.setMimeTypes(instance.type().mimeTypes());
        sieveInstance.setName(instance.name());
        switch (instance.status()) {
        case Akonadi::AgentInstance::Idle:
            sieveInstance.setStatus(KSieveUi::SieveImapInstance::Idle);
            break;
        case Akonadi::AgentInstance::Running:
            sieveInstance.setStatus(KSieveUi::SieveImapInstance::Running);
            break;
        case Akonadi::AgentInstance::Broken:
            sieveInstance.setStatus(KSieveUi::SieveImapInstance::Broken);
            break;
        case Akonadi::AgentInstance::NotConfigured:
            sieveInstance.setStatus(KSieveUi::SieveImapInstance::NotConfigured);
            break;
        }
        listInstance.append(sieveInstance);
    }
    return listInstance;
}
