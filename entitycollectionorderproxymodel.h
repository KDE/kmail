/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

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

#ifndef ENTITYCOLLECTIONORDERPROXYMODEL_H
#define ENTITYCOLLECTIONORDERPROXYMODEL_H

#include <akonadi_next/entityorderproxymodel.h>

class EntityCollectionOrderProxyModel : public Akonadi::EntityOrderProxyModel
{
  Q_OBJECT
public:
  EntityCollectionOrderProxyModel( QObject * parent = 0 );

  virtual ~EntityCollectionOrderProxyModel();

  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

  void setManualSortingActive( bool active );
  bool isManualSortingActive() const;

private:
  class EntityCollectionOrderProxyModelPrivate;
  EntityCollectionOrderProxyModelPrivate * const d;

};

#endif /* ENTITYCOLLECTIONORDERPROXYMODEL_H */

