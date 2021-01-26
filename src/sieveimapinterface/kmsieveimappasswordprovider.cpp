/*
    SPDX-FileCopyrightText: 2017 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmsieveimappasswordprovider.h"
#include "kmail_debug.h"
using namespace QKeychain;
KMSieveImapPasswordProvider::KMSieveImapPasswordProvider(QObject *parent)
    : KSieveUi::SieveImapPasswordProvider(parent)
{
}

KMSieveImapPasswordProvider::~KMSieveImapPasswordProvider()
{
}

void KMSieveImapPasswordProvider::passwords(const QString &identifier)
{
    mIdentifier = identifier;

    auto readJob = new ReadPasswordJob(QStringLiteral("imap"), this);
    connect(readJob, &Job::finished, this, &KMSieveImapPasswordProvider::readSieveServerPasswordFinished);
    readJob->setKey(mIdentifier + QStringLiteral("rc"));
    readJob->start();
}

void KMSieveImapPasswordProvider::readSieveServerPasswordFinished(QKeychain::Job *baseJob)
{
    auto job = qobject_cast<ReadPasswordJob *>(baseJob);
    Q_ASSERT(job);
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "An error occurred while reading password: " << job->errorString();
    } else {
        mSievePassword = job->textData();
    }

    auto readJob = new ReadPasswordJob(QStringLiteral("imap"), this);
    connect(readJob, &Job::finished, this, &KMSieveImapPasswordProvider::readSieveServerCustomPasswordFinished);
    readJob->setKey(QStringLiteral("custom_sieve_") + mIdentifier + QStringLiteral("rc"));
    readJob->start();
}

void KMSieveImapPasswordProvider::readSieveServerCustomPasswordFinished(QKeychain::Job *baseJob)
{
    auto job = qobject_cast<ReadPasswordJob *>(baseJob);
    Q_ASSERT(job);
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
