/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tagmonitormanager.h"
#include "kmail_debug.h"

#include <AkonadiCore/Monitor>
#include <AkonadiCore/TagAttribute>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>

TagMonitorManager::TagMonitorManager(QObject *parent)
    : QObject(parent)
    , mMonitor(new Akonadi::Monitor(this))
{
    mMonitor->setObjectName(QStringLiteral("TagActionManagerMonitor"));
    mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    mMonitor->tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(mMonitor, &Akonadi::Monitor::tagAdded, this, &TagMonitorManager::onTagAdded);
    connect(mMonitor, &Akonadi::Monitor::tagRemoved, this, &TagMonitorManager::onTagRemoved);
    connect(mMonitor, &Akonadi::Monitor::tagChanged, this, &TagMonitorManager::onTagChanged);
    createActions();
}

TagMonitorManager::~TagMonitorManager() = default;

TagMonitorManager *TagMonitorManager::self()
{
    static TagMonitorManager s_self;
    return &s_self;
}

void TagMonitorManager::createActions()
{
    if (mTags.isEmpty()) {
        auto fetchJob = new Akonadi::TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        connect(fetchJob, &Akonadi::TagFetchJob::result, this, &TagMonitorManager::finishedTagListing);
    }
}

void TagMonitorManager::finishedTagListing(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << job->errorString();
    }
    auto fetchJob = static_cast<Akonadi::TagFetchJob *>(job);
    const Akonadi::Tag::List lstTags = fetchJob->tags();
    for (const Akonadi::Tag &result : lstTags) {
        mTags.append(MailCommon::Tag::fromAkonadi(result));
    }
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    Q_EMIT fetchTagDone();
}

QVector<MailCommon::Tag::Ptr> TagMonitorManager::tags() const
{
    return mTags;
}

void TagMonitorManager::onTagAdded(const Akonadi::Tag &akonadiTag)
{
    mTags.append(MailCommon::Tag::fromAkonadi(akonadiTag));
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    Q_EMIT tagAdded();
}

void TagMonitorManager::onTagRemoved(const Akonadi::Tag &akonadiTag)
{
    for (const MailCommon::Tag::Ptr &tag : std::as_const(mTags)) {
        if (tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }
    Q_EMIT tagRemoved();
}

void TagMonitorManager::onTagChanged(const Akonadi::Tag &akonadiTag)
{
    for (const MailCommon::Tag::Ptr &tag : std::as_const(mTags)) {
        if (tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }
    mTags.append(MailCommon::Tag::fromAkonadi(akonadiTag));
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    Q_EMIT tagChanged();
}
