/* 
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009-2015 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COLLECTIONMAINTENANCEPAGE_H
#define COLLECTIONMAINTENANCEPAGE_H
#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>

class QCheckBox;
class QLabel;
namespace Akonadi
{
class CollectionStatistics;
}

class CollectionMaintenancePage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionMaintenancePage(QWidget *parent = Q_NULLPTR);

    void load(const Akonadi::Collection &col);
    void save(Akonadi::Collection &col);

protected:
    void init(const Akonadi::Collection &);

private Q_SLOTS:
    void updateCollectionStatistic(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);
    void slotReindexing();
    void onIndexedItemsReceived(qint64 num);

private:
    void updateLabel(qint64 nbMail, qint64 nbUnreadMail, qint64 size);

private:
    Akonadi::Collection mCurrentCollection;
    bool mIsNotAVirtualCollection;
    QLabel *mFolderSizeLabel;
    QLabel *mCollectionCount;
    QLabel *mCollectionUnread;
    QCheckBox *mIndexingEnabled;
    QLabel *mLastIndexed;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMaintenancePageFactory, CollectionMaintenancePage)

#endif /* COLLECTIONMAINTENANCEPAGE_H */

