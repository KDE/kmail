/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeutil.h"
#include <Akonadi/AgentManager>
#include <KConfigGroup>
#include <KSharedConfig>
#include <MailCommon/MailKernel>

using namespace Qt::Literals::StringLiterals;

QString KMail::SnoozeUtil::agentIdentifier()
{
    return u"akonadi_snooze_agent"_s;
}

bool KMail::SnoozeUtil::snoozeAgentAvailable()
{
    if (!Akonadi::AgentManager::self()->instance(agentIdentifier()).isValid()) {
        return false;
    }
    // Written by the agent process: reparse to see its latest state.
    const KSharedConfig::Ptr config = KSharedConfig::openConfig(u"akonadi_snooze_agentrc"_s);
    config->reparseConfiguration();
    return config->group(u"General"_s).readEntry("enabled", true);
}

bool KMail::SnoozeUtil::canSnoozeInCollection(const Akonadi::Collection &collection)
{
    if (!collection.isValid()) {
        return true;
    }
    if (CommonKernel->folderIsDraftOrOutbox(collection) || CommonKernel->folderIsTemplates(collection)) {
        return false;
    }
    // The item only carries the collection id: resolve it to know its resource.
    const Akonadi::Collection col = collection.resource().isEmpty() ? CommonKernel->collectionFromId(collection.id()) : collection;
    return col.resource() != agentIdentifier();
}
