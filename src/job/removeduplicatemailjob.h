/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
    void slotRemoveDuplicatesDone(KJob *job);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    QWidget *mParent = nullptr;
    QItemSelectionModel *mSelectionModel = nullptr;
};

#endif // REMOVEDUPLICATEMAILJOB_H
