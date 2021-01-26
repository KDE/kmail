/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailsieveimapinstanceinterface.h"
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <KSieveUi/SieveImapInstance>

KMailSieveImapInstanceInterface::KMailSieveImapInstanceInterface() = default;

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
