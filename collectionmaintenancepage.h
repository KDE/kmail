/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

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
#include <akonadi/collectionpropertiespage.h>
class QLabel;
class QPushButton;
class KLineEdit;

class CollectionMaintenancePage : public Akonadi::CollectionPropertiesPage
{
  Q_OBJECT
public:
  explicit CollectionMaintenancePage( QWidget *parent = 0 );

  void load( const Akonadi::Collection & col );
  void save( Akonadi::Collection & col );
private slots:
  void slotCompactNow();
  void slotRebuildIndex();
  void slotRebuildImap();
#if 0
  void updateFolderIndexSizes();
#endif
protected:
  void init();
private:
  QLabel *mFolderSizeLabel;
  QLabel *mIndexSizeLabel;
  QPushButton *mRebuildIndexButton;
  QPushButton *mRebuildImapButton;
  QLabel *mCompactStatusLabel;
  QPushButton *mCompactNowButton;
  KLineEdit *mCollectionLocation;
  QLabel *mCollectionCount;
  QLabel *mCollectionUnread;
};



#endif /* COLLECTIONMAINTENANCEPAGE_H */

