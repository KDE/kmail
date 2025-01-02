/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KJob>

class AddEmailToExistingContactJob : public KJob
{
    Q_OBJECT
public:
    explicit AddEmailToExistingContactJob(const Akonadi::Item &item, const QString &email, QObject *parent = nullptr);
    ~AddEmailToExistingContactJob() override;

    void start() override;

private:
    void slotAddEmailDone(KJob *job);
    QString mEmail;
    Akonadi::Item mItem;
};
