/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef REMOVEDUPLICATEMAILJOB_H
#define REMOVEDUPLICATEMAILJOB_H

#include <QObject>
class QWidget;
class QItemSelectionModel;
namespace KPIM {
class ProgressItem;
}
class KJob;
class RemoveDuplicateMailJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveDuplicateMailJob(QItemSelectionModel *selectionModel, QWidget *widget, QObject *parent = nullptr);
    ~RemoveDuplicateMailJob();

    void start();

private:
    Q_DISABLE_COPY(RemoveDuplicateMailJob)
    void slotRemoveDuplicatesDone(KJob *job);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    QWidget *const mParent;
    QItemSelectionModel *const mSelectionModel;
};

#endif // REMOVEDUPLICATEMAILJOB_H
