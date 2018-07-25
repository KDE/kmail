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

#include "unifiedmailboxmanager.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>
#include <Akonadi/KMime/SpecialMailCollections>

#include <QList>

#include "utils.h"

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
}

void UnifiedMailbox::removeSourceCollection(qint64 source)
{
    mSources.remove(source);
}

void UnifiedMailbox::setSourceCollections(const QSet<qint64> &sources)
{
    mSources = sources;
}

QSet<qint64> UnifiedMailbox::sourceCollections() const
{
    return mSources;
}

UnifiedMailboxManager::UnifiedMailboxManager(KSharedConfigPtr config, QObject* parent)
    : QObject(parent), mConfig(std::move(config))
{
}

UnifiedMailboxManager::~UnifiedMailboxManager()
{
}

void UnifiedMailboxManager::loadBoxes(LoadCallback &&cb)
{
    auto general = mConfig->group("General");
    if (general.readEntry("DiscoverDefaultBoxes", true)) {
        discoverDefaultBoxes(std::move(cb));
        general.writeEntry("DiscoverDefaultBoxes", false);
    } else {
        loadBoxesFromConfig(std::move(cb));
    }
}

void UnifiedMailboxManager::loadBoxesFromConfig(LoadCallback &&cb)
{
    const auto group = mConfig->group("UnifiedMailboxes");
    const auto boxGroups = group.groupList();
    for (const auto &boxGroupName : boxGroups) {
        const auto boxGroup = group.group(boxGroupName);
        UnifiedMailbox box;
        box.setId(boxGroupName);
        box.setName(boxGroup.readEntry("name"));
        box.setIcon(boxGroup.readEntry("icon", QStringLiteral("folder-mail")));
        QList<qint64> sources = boxGroup.readEntry("sources", QList<qint64>{});
        for (auto source : sources) {
            box.addSourceCollection(source);
        }
        insertBox(std::move(box));
    }

    discoverBoxCollections(std::move(cb));
}

void UnifiedMailboxManager::saveBoxes()
{
    auto group = mConfig->group("UnifiedMailboxes");
    const auto currentGroups = group.groupList();
    for (const auto &groupName : currentGroups) {
        group.deleteGroup(groupName);
    }
    for (const auto &box : mBoxes) {
        auto boxGroup = group.group(box.id());
        boxGroup.writeEntry("name", box.name());
        boxGroup.writeEntry("icon", box.icon());
        boxGroup.writeEntry("sources", setToList(box.sourceCollections()));
    }
}

void UnifiedMailboxManager::insertBox(UnifiedMailbox box)
{
    mBoxes.insert(box.id(), box);
    Q_EMIT boxesChanged();
}

void UnifiedMailboxManager::removeBox(const QString &name)
{
    mBoxes.remove(name);
    Q_EMIT boxesChanged();
}

const UnifiedMailbox *UnifiedMailboxManager::unifiedMailboxForSource(qint64 source) const
{
    for (const auto &box : mBoxes) {
        if (box.mSources.contains(source)) {
            return &box;
        }
    }

    return nullptr;
}

qint64 UnifiedMailboxManager::collectionIdForUnifiedMailbox(const QString &id) const
{
    return mBoxId.value(id, -1);
}

void UnifiedMailboxManager::discoverDefaultBoxes(LoadCallback &&cb)
{
    // First build empty boxes
    UnifiedMailbox inbox;
    inbox.setId(QStringLiteral("inbox"));
    inbox.setName(i18n("Inbox"));
    inbox.setIcon(QStringLiteral("mail-folder-inbox"));
    insertBox(std::move(inbox));

    UnifiedMailbox sent;
    sent.setId(QStringLiteral("sent-mail"));
    sent.setName(i18n("Sent"));
    sent.setIcon(QStringLiteral("mail-folder-sent"));
    insertBox(std::move(sent));

    UnifiedMailbox drafts;
    drafts.setId(QStringLiteral("drafts"));
    drafts.setName(i18n("Drafts"));
    drafts.setIcon(QStringLiteral("document-properties"));
    insertBox(std::move(drafts));

    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    list->fetchScope().fetchAttribute<Akonadi::SpecialCollectionAttribute>();
    list->fetchScope().setContentMimeTypes({QStringLiteral("message/rfc822")});
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [this](const Akonadi::Collection::List &list) {
                for (const auto &col : list) {
                    if (col.resource() == QLatin1String("akonadi_unifiedmailbox_agent")) {
                        continue;
                    }

                    switch (Akonadi::SpecialMailCollections::self()->specialCollectionType(col)) {
                    case Akonadi::SpecialMailCollections::Inbox:
                        mBoxes.find(QStringLiteral("inbox"))->addSourceCollection(col.id());
                        break;
                    case Akonadi::SpecialMailCollections::SentMail:
                        mBoxes.find(QStringLiteral("sent-mail"))->addSourceCollection(col.id());
                        break;
                    case Akonadi::SpecialMailCollections::Drafts:
                        mBoxes.find(QStringLiteral("drafts"))->addSourceCollection(col.id());
                        break;
                    default:
                        continue;
                    }
                }
            });
    connect(list, &Akonadi::CollectionFetchJob::finished,
            this, [this, cb = std::move(cb)]() {
                saveBoxes();
                if (cb) {
                    cb();
                }
            });
}

void UnifiedMailboxManager::discoverBoxCollections(LoadCallback &&cb)
{
    auto list = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    list->fetchScope().setResource(QStringLiteral("akonadi_unifiedmailbox_agent"));
    connect(list, &Akonadi::CollectionFetchJob::collectionsReceived,
            this, [this](const Akonadi::Collection::List &list) {
                for (const auto &col : list) {
                    mBoxId.insert(col.name(), col.id());
                }
            });
    connect(list, &Akonadi::CollectionFetchJob::finished,
            this, [this, cb = std::move(cb)]() {
                if (cb) {
                    cb();
                }
            });
}
