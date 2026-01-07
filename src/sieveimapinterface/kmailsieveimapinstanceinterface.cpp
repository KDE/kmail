/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailsieveimapinstanceinterface.h"
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <KSieveCore/SieveImapInstance>

KMailSieveImapInstanceInterface::KMailSieveImapInstanceInterface() = default;

QList<KSieveCore::SieveImapInstance> KMailSieveImapInstanceInterface::sieveImapInstances()
{
    QList<KSieveCore::SieveImapInstance> listInstance;

    const Akonadi::AgentInstance::List allInstances = Akonadi::AgentManager::self()->instances();
    listInstance.reserve(allInstances.count());
    for (const Akonadi::AgentInstance &instance : allInstances) {
        KSieveCore::SieveImapInstance sieveInstance;
        sieveInstance.setCapabilities(instance.type().capabilities());
        sieveInstance.setIdentifier(instance.identifier());
        sieveInstance.setMimeTypes(instance.type().mimeTypes());
        sieveInstance.setName(instance.name());
        switch (instance.status()) {
        case Akonadi::AgentInstance::Idle:
            sieveInstance.setStatus(KSieveCore::SieveImapInstance::Status::Idle);
            break;
        case Akonadi::AgentInstance::Running:
            sieveInstance.setStatus(KSieveCore::SieveImapInstance::Status::Running);
            break;
        case Akonadi::AgentInstance::Broken:
            sieveInstance.setStatus(KSieveCore::SieveImapInstance::Status::Broken);
            break;
        case Akonadi::AgentInstance::NotConfigured:
            sieveInstance.setStatus(KSieveCore::SieveImapInstance::Status::NotConfigured);
            break;
        }
        listInstance.append(sieveInstance);
    }
    return listInstance;
}
