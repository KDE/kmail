/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "unifiedmailbox.h"
#include "unifiedmailboxmanager.h"
#include "utils.h"

#include <KConfigGroup>


void UnifiedMailbox::load(const KConfigGroup &group)
{
    mId = group.name();
    mName = group.readEntry("name");
    mIcon = group.readEntry("icon", QStringLiteral("folder-mail"));
    mSources = listToSet(group.readEntry("sources", QList<qint64>{}));
    // This is not authoritative, we will do collection discovery anyway
    mCollectionId = group.readEntry("collectionId", -1ll);
}

void UnifiedMailbox::save(KConfigGroup& group) const
{
    group.writeEntry("name", name());
    group.writeEntry("icon", icon());
    group.writeEntry("sources", setToList(sourceCollections()));
    // just for cacheing, we will do collection discovery on next start anyway
    group.writeEntry("collectionId", collectionId());
}


bool UnifiedMailbox::isSpecial() const
{
    return mId == QLatin1String("inbox")
        || mId == QLatin1String("sent-mail")
        || mId == QLatin1String("drafts");
}

void UnifiedMailbox::setCollectionId(qint64 id)
{
    mCollectionId = id;
}

qint64 UnifiedMailbox::collectionId() const
{
    return mCollectionId;
}

void UnifiedMailbox::setId(const QString &id)
{
    mId = id;
}

QString UnifiedMailbox::id() const
{
    return mId;
}

void UnifiedMailbox::setName(const QString &name)
{
    mName = name;
}

QString UnifiedMailbox::name() const
{
    return mName;
}

void UnifiedMailbox::setIcon(const QString &icon)
{
    mIcon = icon;
}

QString UnifiedMailbox::icon() const
{
    return mIcon;
}

void UnifiedMailbox::addSourceCollection(qint64 source)
{
    mSources.insert(source);
    if (mManager) {
        mManager->mMonitor.setCollectionMonitored(Akonadi::Collection{source});
    }
}

void UnifiedMailbox::removeSourceCollection(qint64 source)
{
    mSources.remove(source);
    if (mManager) {
        mManager->mMonitor.setCollectionMonitored(Akonadi::Collection{source}, false);
    }
}

void UnifiedMailbox::setSourceCollections(const QSet<qint64> &sources)
{
    const auto updateMonitor = [this](bool monitor) {
        if (mManager) {
            for (const auto source : mSources) {
                mManager->mMonitor.setCollectionMonitored(Akonadi::Collection{source}, monitor);
            }
        }
    };

    updateMonitor(false);
    mSources = sources;
    updateMonitor(true);
}

QSet<qint64> UnifiedMailbox::sourceCollections() const
{
    return mSources;
}

void UnifiedMailbox::attachManager(UnifiedMailboxManager *manager)
{
    Q_ASSERT(manager);
    if (mManager != manager) {
        mManager = manager;
        // Force that we start monitoring all the collections
        setSourceCollections(mSources);
    }
}


