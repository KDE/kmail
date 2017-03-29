/*
   Copyright (C) 2009-2017 Montel Laurent <montel@kde.org>

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

#ifndef COLLECTIONMAINTENANCEPAGE_H
#define COLLECTIONMAINTENANCEPAGE_H
#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>

class QCheckBox;
class QLabel;
class QPushButton;
namespace Akonadi
{
class CollectionStatistics;
}

class CollectionMaintenancePage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionMaintenancePage(QWidget *parent = nullptr);

    void load(const Akonadi::Collection &col) Q_DECL_OVERRIDE;
    void save(Akonadi::Collection &col) Q_DECL_OVERRIDE;

protected:
    void init(const Akonadi::Collection &);

private:
    void updateCollectionStatistic(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    void slotReindexCollection();
    void updateLabel(qint64 nbMail, qint64 nbUnreadMail, qint64 size);
    Akonadi::Collection mCurrentCollection;
    bool mIsNotAVirtualCollection;
    QLabel *mFolderSizeLabel;
    QLabel *mCollectionCount;
    QLabel *mCollectionUnread;
    QCheckBox *mIndexingEnabled;
    QLabel *mLastIndexed;
    QPushButton *mReindexCollection;
    QLabel *mIndexedInfo;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMaintenancePageFactory, CollectionMaintenancePage)

#endif /* COLLECTIONMAINTENANCEPAGE_H */

