/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tagmonitormanager.h"
#include <AkonadiCore/Monitor>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>
#include <AkonadiCore/TagAttribute>

TagMonitorManager::TagMonitorManager(QObject *parent)
    : QObject(parent)
    , mMonitor(new Akonadi::Monitor(this))
{
    mMonitor->setObjectName(QStringLiteral("TagActionManagerMonitor"));
    mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    mMonitor->tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(mMonitor, &Akonadi::Monitor::tagAdded, this, &TagMonitorManager::tagAdded);
    connect(mMonitor, &Akonadi::Monitor::tagRemoved, this, &TagMonitorManager::tagRemoved);
    connect(mMonitor, &Akonadi::Monitor::tagChanged, this, &TagMonitorManager::tagChanged);
}

TagMonitorManager::~TagMonitorManager()
{

}

TagMonitorManager *TagMonitorManager::self()
{
    static TagMonitorManager s_self;
    return &s_self;
}
