/*
    Copyright (c) 2009 Laurent Montel <montel@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef READABLECOLLECTIONPROXYMODEL_H
#define READABLECOLLECTIONPROXYMODEL_H


#include <akonadi/entityrightsfiltermodel.h>
#include <akonadi/collection.h>
class ReadableCollectionProxyModel : public Akonadi::EntityRightsFilterModel
{
  Q_OBJECT

public:
  explicit ReadableCollectionProxyModel( QObject *parent = 0 );

  virtual ~ReadableCollectionProxyModel();

  virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

  void setEnabledCheck( bool enable );
  bool enabledCheck() const;

  void setNecessaryRight( Akonadi::Collection::Rights right );

private:
  class Private;
  Private* const d;
};

#endif
