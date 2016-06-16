/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

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

#ifndef CHECKINDEXINGJOB_H
#define CHECKINDEXINGJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
class KJob;
class CheckIndexingJob : public QObject
{
    Q_OBJECT
public:
    explicit CheckIndexingJob(QObject *parent = Q_NULLPTR);
    ~CheckIndexingJob();

    void setCollection(const Akonadi::Collection &col);

    void start();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void slotCollectionPropertiesFinished(KJob *job);
    void indexerStatsFetchFinished(KJob *job);
private:
    void askForNextCheck();
    Akonadi::Collection mCollection;
};

#endif // CHECKINDEXINGJOB_H
