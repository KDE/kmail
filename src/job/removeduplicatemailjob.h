/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
class QWidget;
class QItemSelectionModel;
namespace KPIM
{
class ProgressItem;
}
class KJob;
class RemoveDuplicateMailJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveDuplicateMailJob(QItemSelectionModel *selectionModel, QWidget *widget, QObject *parent = nullptr);
    ~RemoveDuplicateMailJob() override;

    void start();

private:
    Q_DISABLE_COPY(RemoveDuplicateMailJob)
    void slotRemoveDuplicatesDone(KJob *job);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    QWidget *const mParent;
    QItemSelectionModel *const mSelectionModel;
};

