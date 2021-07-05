/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unifiedmailbox.h"
#include "common.h"
#include "unifiedmailboxmanager.h"
#include "utils.h"

#include <KConfigGroup>

bool UnifiedMailbox::operator==(const UnifiedMailbox &other) const
{
    return mId == other.mId;
}

void UnifiedMailbox::load(const KConfigGroup &group)
{
    mId = group.name();
    mName = group.readEntry("name");
    mIcon = group.readEntry("icon", QStringLiteral("folder-mail"));
    mSources = listToSet(group.readEntry("sources", QList<qint64>{}));
    // This is not authoritative, we will do collection discovery anyway
    mCollectionId = group.readEntry("collectionId", -1ll);
}

void UnifiedMailbox::save(KConfigGroup &group) const
{
    group.writeEntry("name", name());
    group.writeEntry("icon", icon());
    group.writeEntry("sources", setToList(sourceCollections()));
    // just for caching, we will do collection discovery on next start anyway
    group.writeEntry("collectionId", collectionId());
}

bool UnifiedMailbox::isSpecial() const
{
    return mId == Common::InboxBoxId || mId == Common::SentBoxId || mId == Common::DraftsBoxId;
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
        mManager->mSourceToBoxMap.insert({source, this});
    }
}

void UnifiedMailbox::removeSourceCollection(qint64 source)
{
    mSources.remove(source);
    if (mManager) {
        mManager->mMonitor.setCollectionMonitored(Akonadi::Collection{source}, false);
        mManager->mSourceToBoxMap.erase(source);
    }
}

void UnifiedMailbox::setSourceCollections(const QSet<qint64> &sources)
{
    while (!mSources.empty()) {
        removeSourceCollection(*mSources.begin());
    }
    for (auto source : sources) {
        addSourceCollection(source);
    }
}

QSet<qint64> UnifiedMailbox::sourceCollections() const
{
    return mSources;
}

void UnifiedMailbox::attachManager(UnifiedMailboxManager *manager)
{
    if (mManager != manager) {
        if (manager) {
            // Force that we start monitoring all the collections
            for (const auto source : std::as_const(mSources)) {
                manager->mMonitor.setCollectionMonitored(Akonadi::Collection{source});
                manager->mSourceToBoxMap.insert({source, this});
            }
        } else {
            for (const auto &source : std::as_const(mSources)) {
                mManager->mMonitor.setCollectionMonitored(Akonadi::Collection{source}, false);
                mManager->mSourceToBoxMap.erase(source);
            }
        }
        mManager = manager;
    }
}
