/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef SAVEDRAFTJOB_H
#define SAVEDRAFTJOB_H

#include <KJob>
#include <AkonadiCore/Collection>
#include <KMime/KMimeMessage>

class SaveDraftJob : public KJob
{
    Q_OBJECT
public:
    explicit SaveDraftJob(const KMime::Message::Ptr &msg, const Akonadi::Collection &col, QObject *parent = nullptr);
    ~SaveDraftJob() override;

    void start() override;

private:
    void slotStoreDone(KJob *job);
    KMime::Message::Ptr mMsg = nullptr;
    const Akonadi::Collection mCollection;
};

#endif // SAVEDRAFTJOB_H
