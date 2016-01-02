/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ADDEMAILTOEXISTINGCONTACTJOB_H
#define ADDEMAILTOEXISTINGCONTACTJOB_H

#include <KJob>
#include <AkonadiCore/Item>

class AddEmailToExistingContactJob : public KJob
{
    Q_OBJECT
public:
    explicit AddEmailToExistingContactJob(const Akonadi::Item &item, const QString &email, QObject *parent = Q_NULLPTR);
    ~AddEmailToExistingContactJob();

    void start() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotAddEmailDone(KJob *job);

private:
    QString mEmail;
    Akonadi::Item mItem;
};

#endif // ADDEMAILTOEXISTINGCONTACTJOB_H
