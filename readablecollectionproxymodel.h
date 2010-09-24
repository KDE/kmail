/*
    Copyright (c) 2009, 2010 Laurent Montel <montel@kde.org>

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


#include "akonadi/entityrightsfiltermodel.h"
#include <akonadi/collection.h>

class MailCommon;

class ReadableCollectionProxyModel : public Akonadi::EntityRightsFilterModel
{
  Q_OBJECT

public:
  enum ReadableCollectionOption
  {
    None = 0,
    HideVirtualFolder = 1,
    HideSpecificFolder = 2,
    HideOutboxFolder = 4,
    HideImapFolder = 8
  };
  Q_DECLARE_FLAGS( ReadableCollectionOptions, ReadableCollectionOption )

  explicit ReadableCollectionProxyModel( MailCommon* mailCommon, QObject *parent = 0, ReadableCollectionOptions = ReadableCollectionProxyModel::None );

  virtual ~ReadableCollectionProxyModel();

  virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

  void setEnabledCheck( bool enable );
  bool enabledCheck() const;

  void setHideVirtualFolder( bool exclude );
  bool hideVirtualFolder() const;


  void setHideSpecificFolder( bool hide );
  bool hideSpecificFolder() const;


  void setHideOutboxFolder( bool hide );
  bool hideOutboxFolder() const;

  void setHideImapFolder( bool hide );
  bool hideImapFolder() const;

protected:
  virtual bool acceptRow( int sourceRow, const QModelIndex &sourceParent ) const;

private:
  class Private;
  Private* const d;
};

#endif
