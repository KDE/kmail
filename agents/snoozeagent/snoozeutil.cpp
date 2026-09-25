/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeutil.h"
#include "snoozeagentsettings.h"
#include "snoozeinfo.h"
#include <Akonadi/ServerManager>
#include <KConfigGroup>
#include <QDBusInterface>
using namespace Qt::Literals::StringLiterals;
using namespace Snooze;

namespace
{
QString serviceName()
{
    // TODO verify
    return Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_snooze_agent"_s);
}

QString dbusPath()
{
    // TODO verify
    return u"/Snooze"_s;
}
}

bool SnoozeUtil::snoozeAgentWasRegistered()
{
    QDBusInterface interface(serviceName(), dbusPath());
    return interface.isValid();
}

bool SnoozeUtil::snoozeAgentEnabled()
{
    return SnoozeAgentSettings::self()->enabled();
}

void SnoozeUtil::reload()
{
    // TODO call reload() on the agent over D-Bus.
}

void SnoozeUtil::forceReparseConfiguration()
{
    defaultConfig()->reparseConfiguration();
}

KSharedConfig::Ptr SnoozeUtil::defaultConfig()
{
    return KSharedConfig::openConfig(u"akonadi_snooze_agentrc"_s);
}

QString SnoozeUtil::snoozePattern()
{
    return u"SnoozeItem \\d+"_s;
}

SnoozeInfo *SnoozeUtil::readSnoozeInfo(const KConfigGroup &config)
{
    return new SnoozeInfo(config);
}

void SnoozeUtil::writeSnoozeInfo(const KSharedConfig::Ptr &config, Snooze::SnoozeInfo *info, bool forceReload)
{
    // TODO allocate a unique identifier, write the group, sync and optionally reload.
    Q_UNUSED(config)
    Q_UNUSED(info)
    Q_UNUSED(forceReload)
}

bool SnoozeUtil::removeSnoozeInfo(const KSharedConfig::Ptr &config, const QList<qint32> &listRemove, bool forceReload)
{
    // TODO delete the matching groups.
    Q_UNUSED(config)
    Q_UNUSED(listRemove)
    Q_UNUSED(forceReload)
    return false;
}

bool SnoozeUtil::compareSnoozeInfo(Snooze::SnoozeInfo *left, Snooze::SnoozeInfo *right)
{
    // Earliest wake-up first, so the manager only ever has to look at the head.
    return left->wakeUpDateTime() < right->wakeUpDateTime();
}

Akonadi::Collection::Id SnoozeUtil::snoozeCollectionId()
{
    // TODO read it back from the agent settings.
    return -1;
}

void SnoozeUtil::setSnoozeCollectionId(Akonadi::Collection::Id id)
{
    // TODO store it in the agent settings.
    Q_UNUSED(id)
}
