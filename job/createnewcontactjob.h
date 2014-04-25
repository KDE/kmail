/*
  Copyright (c) 2012-2013 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifndef CREATENEWCONTACTJOB_H
#define CREATENEWCONTACTJOB_H

#include <AkonadiCore/Item>
#include <KJob>

/**
 * @brief The CreateNewContactJob class
 * The job will check if there is address book folder to store new contact to akonadi
 * otherise it will allow to create new one.
 */
class CreateNewContactJob : public KJob
{
    Q_OBJECT
public:
    /**
     * @brief CreateNewContactJob create a new contact job
     * @param parentWidget The widget that will be used as parent for dialog.
     * @param parent The parent object
     */
    explicit CreateNewContactJob(QWidget *parentWidget, QObject *parent = 0);

    /**
     * Destroys the new contact job
     */
    ~CreateNewContactJob();

    /**
     * @brief start the job
     */
    void start();

private Q_SLOTS:
    void slotCollectionsFetched(KJob*);
    void slotResourceCreationDone(KJob* job);
    void slotContactEditorError(const QString &error);
    void contactStored( const Akonadi::Item &item );

private:
    void createContact();

private:
    QWidget *mParentWidget;
};

#endif // CREATENEWCONTACTJOB_H
