/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#ifndef REMOVEDUPLICATEMAILJOB_H
#define REMOVEDUPLICATEMAILJOB_H

#include <QObject>
//fix it. Remove this include.
#include "kmmainwidget.h"
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
    explicit RemoveDuplicateMailJob(QItemSelectionModel *selectionModel, QWidget *widget, QObject *parent = Q_NULLPTR);
    ~RemoveDuplicateMailJob();

    void start();

private Q_SLOTS:
    void slotRemoveDuplicatesDone(KJob *job);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);

private:
    QWidget *mParent;
    QItemSelectionModel *mSelectionModel;
};

#endif // REMOVEDUPLICATEMAILJOB_H
