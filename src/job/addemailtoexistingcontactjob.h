/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ADDEMAILTOEXISTINGCONTACTJOB_H
#define ADDEMAILTOEXISTINGCONTACTJOB_H

#include <KJob>
#include <AkonadiCore/Item>

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

#endif // ADDEMAILTOEXISTINGCONTACTJOB_H
