/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "markallmessagesasreadinfolderandsubfolderjob.h"
#include "kmail_debug.h"

#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/CollectionFetchScope>

MarkAllMessagesAsReadInFolderAndSubFolderJob::MarkAllMessagesAsReadInFolderAndSubFolderJob(QObject *parent)
    : QObject(parent)
{
}

MarkAllMessagesAsReadInFolderAndSubFolderJob::~MarkAllMessagesAsReadInFolderAndSubFolderJob() = default;

void MarkAllMessagesAsReadInFolderAndSubFolderJob::setTopLevelCollection(const Akonadi::Collection &topLevelCollection)
{
    mTopLevelCollection = topLevelCollection;
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::start()
{
    if (mTopLevelCollection.isValid()) {
        auto fetchJob = new Akonadi::CollectionFetchJob(mTopLevelCollection, Akonadi::CollectionFetchJob::Recursive, this);
        fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
        connect(fetchJob, &Akonadi::CollectionFetchJob::finished, this, [this](KJob *job) {
            if (job->error()) {
                qCWarning(KMAIL_LOG) << job->errorString();
                slotFetchCollectionFailed();
            } else {
                auto fetch = static_cast<Akonadi::CollectionFetchJob *>(job);
                slotFetchCollectionDone(fetch->collections());
            }
        });
    } else {
        qCDebug(KMAIL_LOG()) << "Invalid toplevel collection";
        deleteLater();
    }
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionFailed()
{
    qCDebug(KMAIL_LOG()) << "Fetch toplevel collection failed";
    deleteLater();
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionDone(const Akonadi::Collection::List &list)
{
    Akonadi::MessageStatus messageStatus;
    messageStatus.setRead(true);
    auto markAsReadAllJob = new Akonadi::MarkAsCommand(messageStatus, list);
    connect(markAsReadAllJob, &Akonadi::MarkAsCommand::result, this, &MarkAllMessagesAsReadInFolderAndSubFolderJob::slotMarkAsResult);
    markAsReadAllJob->execute();
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotMarkAsResult(Akonadi::MarkAsCommand::Result result)
{
    switch (result) {
    case Akonadi::MarkAsCommand::Undefined:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFolderJob undefined result";
        break;
    case Akonadi::MarkAsCommand::OK:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob Done";
        break;
    case Akonadi::MarkAsCommand::Canceled:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob was canceled";
        break;
    case Akonadi::MarkAsCommand::Failed:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob was failed";
        break;
    }
    deleteLater();
}
