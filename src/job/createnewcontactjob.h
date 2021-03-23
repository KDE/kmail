/*
  SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

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
    explicit CreateNewContactJob(QWidget *parentWidget, QObject *parent = nullptr);

    /**
     * Destroys the new contact job
     */
    ~CreateNewContactJob() override;

    /**
     * @brief start the job
     */
    void start() override;

private:
    void slotCollectionsFetched(KJob *);
    void slotResourceCreationDone(KJob *job);
    void slotContactEditorError(const QString &error);
    void contactStored(const Akonadi::Item &item);

    void createContact();

    QWidget *const mParentWidget;
};

