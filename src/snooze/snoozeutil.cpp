/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeutil.h"
#include "util.h"
#include <MailCommon/MailKernel>

using namespace Qt::Literals::StringLiterals;

QString KMail::SnoozeUtil::agentIdentifier()
{
    return u"akonadi_snooze_agent"_s;
}

bool KMail::SnoozeUtil::snoozeAgentAvailable()
{
    return KMail::Util::agentAvailable(agentIdentifier());
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
