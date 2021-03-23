/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <AkonadiCore/Collection>
#include <KJob>
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

