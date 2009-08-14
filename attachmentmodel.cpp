/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "attachmentmodel.h"

#include <boost/shared_ptr.hpp>

#include <KDebug>
#include <KGlobal>
#include <KLocale>

#include <kmime/kmime_util.h>

using KPIM::AttachmentPart;

using namespace KMail;

static Qt::CheckState boolToCheckState( bool checked ) // local
{
  if( checked ) {
    return Qt::Checked;
  } else {
    return Qt::Unchecked;
  }
}

class KMail::AttachmentModel::Private
{
  public:
    Private( AttachmentModel *qq );
    ~Private();

    AttachmentModel *const q;
    AttachmentPart::List parts;
    bool modified;
    bool encryptEnabled;
    bool signEnabled;
    bool encryptSelected;
    bool signSelected;
};

AttachmentModel::Private::Private( AttachmentModel *qq )
  : q( qq )
  , modified( false )
  , encryptEnabled( false )
  , signEnabled( false )
  , encryptSelected( false )
  , signSelected( false )
{
}

AttachmentModel::Private::~Private()
{
}



AttachmentModel::AttachmentModel( QObject *parent )
  : QAbstractItemModel( parent )
  , d( new Private( this ) )
{
}

AttachmentModel::~AttachmentModel()
{
  delete d;
}

bool AttachmentModel::isModified() const
{
  return d->modified; // TODO actually set modified=true sometime...
}

void AttachmentModel::setModified( bool modified )
{
  d->modified = modified;
}

bool AttachmentModel::isEncryptEnabled() const
{
  return d->encryptEnabled;
}

void AttachmentModel::setEncryptEnabled( bool enabled )
{
  d->encryptEnabled = enabled;
  emit encryptEnabled( enabled );
}

bool AttachmentModel::isSignEnabled() const
{
  return d->signEnabled;
}

void AttachmentModel::setSignEnabled( bool enabled )
{
  d->signEnabled = enabled;
  emit signEnabled( enabled );
}

bool AttachmentModel::isEncryptSelected() const
{
  return d->encryptSelected;
}

void AttachmentModel::setEncryptSelected( bool selected )
{
  d->encryptSelected = selected;
  foreach( AttachmentPart::Ptr part, d->parts ) {
    part->setEncrypted( selected );
  }
  emit dataChanged( index( 0, EncryptColumn ), index( rowCount() - 1, EncryptColumn ) );
}

bool AttachmentModel::isSignSelected() const
{
  return d->signSelected;
}

void AttachmentModel::setSignSelected( bool selected )
{
  d->signSelected = selected;
  foreach( AttachmentPart::Ptr part, d->parts ) {
    part->setSigned( selected );
  }
  emit dataChanged( index( 0, SignColumn ), index( rowCount() - 1, SignColumn ) );
}

QVariant AttachmentModel::data( const QModelIndex &index, int role ) const
{
  if( !index.isValid() ) {
    return QVariant();
  }

  AttachmentPart::Ptr part = d->parts[ index.row() ];

  if( role == Qt::DisplayRole ) {
    switch( index.column() ) {
      case NameColumn:
        return QVariant::fromValue( part->fileName() );
      case SizeColumn:
        return QVariant::fromValue( KGlobal::locale()->formatByteSize( part->size() ) );
      case EncodingColumn:
        return QVariant::fromValue( KMime::nameForEncoding( part->encoding() ) );
      case MimeTypeColumn:
        return QVariant::fromValue( part->mimeType() );
      default:
        return QVariant();
    };
  } else if( role == Qt::CheckStateRole ) {
    switch( index.column() ) {
      case CompressColumn:
        return QVariant::fromValue( int( boolToCheckState( part->isCompressed() ) ) );
      case EncryptColumn:
        return QVariant::fromValue( int( boolToCheckState( part->isEncrypted() ) ) );
      case SignColumn:
        return QVariant::fromValue( int( boolToCheckState( part->isSigned() ) ) );
      default:
        return QVariant();
    }
  } else {
    return QVariant();
  }
}

bool AttachmentModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  bool emitDataChanged = true;
  AttachmentPart::Ptr part = d->parts[ index.row() ];

  if( role == Qt::CheckStateRole ) {
    switch( index.column() ) {
      case CompressColumn:
        {
          bool toZip = value.toBool();
          if( toZip != part->isCompressed() ) {
            emit attachmentCompressRequested( part, toZip );
            emitDataChanged = false; // Will emit when the part is updated.
          }
          break;
        }
      case EncryptColumn:
        part->setEncrypted( value.toBool() );
        break;
      case SignColumn:
        part->setSigned( value.toBool() );
        break;
      default:
        break; // Do nothing.
    };
  } else {
    return false;
  }

  if( emitDataChanged ) {
    emit dataChanged( index, index );
  }
  return true;
}

bool AttachmentModel::addAttachment( AttachmentPart::Ptr part )
{
  Q_ASSERT( !d->parts.contains( part ) );

  beginInsertRows( QModelIndex(), rowCount(), rowCount() );
  d->parts.append( part );
  endInsertRows();
  return true;
}

bool AttachmentModel::updateAttachment( AttachmentPart::Ptr part )
{
  int idx = d->parts.indexOf( part );
  if( idx == -1 ) {
    kWarning() << "Tried to update non-existent part.";
    return false;
  }
  // Emit dataChanged() for the whole row.
  emit dataChanged( index( idx, 0 ), index( idx, LastColumn - 1 ) );
  return true;
}

bool AttachmentModel::replaceAttachment( AttachmentPart::Ptr oldPart, AttachmentPart::Ptr newPart )
{
  Q_ASSERT( oldPart != newPart );

  int idx = d->parts.indexOf( oldPart );
  if( idx == -1 ) {
    kWarning() << "Tried to replace non-existent part.";
    return false;
  }
  d->parts[ idx ] = newPart;
  // Emit dataChanged() for the whole row.
  emit dataChanged( index( idx, 0 ), index( idx, LastColumn - 1 ) );
  return true;
}

bool AttachmentModel::removeAttachment( const QModelIndex &index )
{
  if( !index.isValid() ) {
    kWarning() << "Invalid index.";
    return false;
  }

  beginRemoveRows( QModelIndex(), index.row(), index.row() );
  AttachmentPart::Ptr part = d->parts.takeAt( index.row() );
  emit attachmentRemoved( part );
  endRemoveRows();
  return true;
}

bool AttachmentModel::removeAttachment( AttachmentPart::Ptr part )
{
  int idx = d->parts.indexOf( part );
  return removeAttachment( index( idx, 0 ) );
}

AttachmentPart::Ptr AttachmentModel::attachment( const QModelIndex &index )
{
  Q_ASSERT( index.isValid() );
  return d->parts[ index.row() ];
}

AttachmentPart::List AttachmentModel::attachments()
{
  return d->parts;
}

Qt::ItemFlags AttachmentModel::flags( const QModelIndex &index ) const
{
  if( !index.isValid() ) {
    return Qt::ItemIsEnabled;
  }

  if( index.column() == CompressColumn ||
      index.column() == EncryptColumn ||
      index.column() == SignColumn ) {
    return QAbstractItemModel::flags( index ) | Qt::ItemIsUserCheckable;
  } else {
    return QAbstractItemModel::flags( index );
  }
}

QVariant AttachmentModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if( orientation != Qt::Horizontal || role != Qt::DisplayRole ) {
    return QVariant();
  }

  switch( section ) {
    case NameColumn:
      return i18nc( "Name column", "Name" );
    case SizeColumn:
      return i18nc( "Size column", "Size" );
    case EncodingColumn:
      return i18nc( "Encoding column", "Encoding" );
    case MimeTypeColumn:
      return i18nc( "Type column", "Type" );
    case CompressColumn:
      return i18nc( "Whether to compress column", "Compress" );
    case EncryptColumn:
      return i18nc( "Whether to encrypt column", "Encrypt" );
    case SignColumn:
      return i18nc( "Whether to sign column", "Sign" );
    default:
      kWarning() << "Bad column" << section;
      return QVariant();
  };
}

QModelIndex AttachmentModel::index( int row, int column, const QModelIndex &parent ) const
{
  if( !hasIndex( row, column, parent ) ) {
    return QModelIndex();
  }
  Q_ASSERT( row >= 0 && row < rowCount() );

  if( parent.isValid() ) {
    kWarning() << "Called with weird parent.";
    return QModelIndex();
  }

  return createIndex( row, column );
}

QModelIndex AttachmentModel::parent( const QModelIndex &index ) const
{
  Q_UNUSED( index );
  return QModelIndex(); // No parent.
}

int AttachmentModel::rowCount( const QModelIndex &parent ) const
{
  if( parent.isValid() ) {
    return 0; // Items have no children.
  }
  Q_UNUSED( parent );
  return d->parts.count();
}

int AttachmentModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return LastColumn;
}

#include "attachmentmodel.moc"
