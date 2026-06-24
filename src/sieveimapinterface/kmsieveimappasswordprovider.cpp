/*
    SPDX-FileCopyrightText: 2017 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmsieveimappasswordprovider.h"
#include "kmail_debug.h"
using namespace QKeychain;
KMSieveImapPasswordProvider::KMSieveImapPasswordProvider(QObject *parent)
    : KSieveCore::SieveImapPasswordProvider(parent)
{
}

KMSieveImapPasswordProvider::~KMSieveImapPasswordProvider() = default;

void KMSieveImapPasswordProvider::passwords(const QString &identifier)
{
    const uint requestId = ++mRequestId;

    auto readJob = new ReadPasswordJob(QStringLiteral("imap"), this);
    connect(readJob, &Job::finished, this, &KMSieveImapPasswordProvider::readSieveServerPasswordFinished);
    mJobRequestIds.insert(readJob, requestId);
    mJobIdentifiers.insert(readJob, identifier);
    readJob->setKey(identifier + QStringLiteral("rc"));
    readJob->start();
}

void KMSieveImapPasswordProvider::readSieveServerPasswordFinished(QKeychain::Job *baseJob)
{
    auto job = qobject_cast<ReadPasswordJob *>(baseJob);
    Q_ASSERT(job);
    const uint requestId = mJobRequestIds.take(job);
    const QString identifier = mJobIdentifiers.take(job);
    if (requestId != mRequestId) {
        return;
    }

    if (job->error()) {
        qCWarning(KMAIL_LOG) << "An error occurred while reading password: " << job->errorString();
    } else {
        mSievePassword = job->textData();
    }

    auto readJob = new ReadPasswordJob(QStringLiteral("imap"), this);
    connect(readJob, &Job::finished, this, &KMSieveImapPasswordProvider::readSieveServerCustomPasswordFinished);
    mJobRequestIds.insert(readJob, requestId);
    readJob->setKey(QStringLiteral("custom_sieve_") + identifier + QStringLiteral("rc"));
    readJob->start();
}

void KMSieveImapPasswordProvider::readSieveServerCustomPasswordFinished(QKeychain::Job *baseJob)
{
    auto job = qobject_cast<ReadPasswordJob *>(baseJob);
    Q_ASSERT(job);
    const uint requestId = mJobRequestIds.take(job);
    if (requestId != mRequestId) {
        return;
    }

    if (job->error()) {
        if (job->error() != QKeychain::EntryNotFound) {
            qCWarning(KMAIL_LOG) << "An error occurred while reading password: " << job->errorString();
        }
    } else {
        mSieveCustomPassword = job->textData();
    }
    Q_EMIT passwordsRequested(mSievePassword, mSieveCustomPassword);
    // Don't store it.
    mSievePassword.clear();
    mSieveCustomPassword.clear();
}

#include "moc_kmsieveimappasswordprovider.cpp"
