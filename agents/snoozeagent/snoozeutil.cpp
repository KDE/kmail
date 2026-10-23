/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeutil.h"
#include "snoozeagentsettings.h"
#include "snoozeinfo.h"
#include <Akonadi/ServerManager>
#include <QDBusInterface>
using namespace Qt::Literals::StringLiterals;
using namespace Snooze;

namespace
{
QString serviceName()
{
    return Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_snooze_agent"_s);
}

QString dbusPath()
{
    return u"/SnoozeAgent"_s;
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
    if (QDBusInterface interface(serviceName(), dbusPath()); interface.isValid()) {
        interface.call(u"reload"_s);
    }
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
    if (!info || !info->isValid()) {
        return;
    }
    KConfigGroup general = config->group(u"General"_s);
    // "Number" is the next free identifier: it only ever grows, so a removed
    // item can never hand its identifier to a new one still in use.
    int identifier = info->uniqueIdentifier();
    if (identifier == -1) {
        identifier = general.readEntry("Number", 0);
        general.writeEntry("Number", identifier + 1);
    }

    const QString groupName = u"SnoozeItem %1"_s.arg(identifier);
    config->deleteGroup(groupName);
    KConfigGroup group = config->group(groupName);
    info->writeConfig(group, identifier);

    config->sync();
    config->reparseConfiguration();
    if (forceReload) {
        reload();
    }
}

bool SnoozeUtil::removeSnoozeInfo(const KSharedConfig::Ptr &config, const QList<qint32> &listRemove, bool forceReload)
{
    if (listRemove.isEmpty()) {
        return false;
    }

    bool needSaveConfig = false;
    for (qint32 identifier : listRemove) {
        const QString groupName = u"SnoozeItem %1"_s.arg(identifier);
        if (config->hasGroup(groupName)) {
            config->deleteGroup(groupName);
            needSaveConfig = true;
        }
    }
    if (needSaveConfig) {
        config->sync();
        config->reparseConfiguration();
        if (forceReload) {
            reload();
        }
    }
    return needSaveConfig;
}

bool SnoozeUtil::compareSnoozeInfo(Snooze::SnoozeInfo *left, Snooze::SnoozeInfo *right)
{
    // Earliest wake-up first, so the manager only ever has to look at the head.
    return left->wakeUpDateTime() < right->wakeUpDateTime();
}

Akonadi::Collection::Id SnoozeUtil::snoozeCollectionId()
{
    return SnoozeAgentSettings::self()->snoozeCollectionId();
}

void SnoozeUtil::setSnoozeCollectionId(Akonadi::Collection::Id id)
{
    SnoozeAgentSettings::self()->setSnoozeCollectionId(id);
    SnoozeAgentSettings::self()->save();
}
