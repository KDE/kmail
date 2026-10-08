/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KJob>

class ShowMessageJob : public KJob
{
    Q_OBJECT
public:
    explicit ShowMessageJob(Akonadi::Item::Id id, QObject *parent = nullptr);
    ~ShowMessageJob() override;

    void start() override;

private:
    const Akonadi::Item::Id mId;
};
