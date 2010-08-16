/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Montel Laurent <montel@kde.org>

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

#ifndef KMCOLLECTIONPROPERTIESDIALOG_H
#define KMCOLLECTIONPROPERTIESDIALOG_H

#include <KDialog>
#include <KJob>
#include <Akonadi/Collection>
#include <Akonadi/CollectionPropertiesPage>

class KTabWidget;

class KMCollectionPropertiesDialog : public KDialog
{
  Q_OBJECT
public:
    explicit KMCollectionPropertiesDialog( const Akonadi::Collection &collection, QWidget *parent = 0 );

protected:
  void insertPage( Akonadi::CollectionPropertiesPage * page );

protected slots:
  void save();
  void saveResult( KJob *job );

private:
  Akonadi::Collection mCollection;
  KTabWidget* mTabWidget;

};


#endif /* KMCOLLECTIONPROPERTIESDIALOG_H */

