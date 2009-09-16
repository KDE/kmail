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

#include <QMimeData>
#include <QUrl>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KTempDir>

#include <kmime/kmime_util.h>

#include <kpimutils/kfileio.h>

#include <libkdepim/maillistdrag.h>

using namespace KMail;
using namespace KPIM;

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
    QList<KTempDir*> tempDirs;
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
  // There should be an automatic way to manage the lifetime of these...
  qDeleteAll( tempDirs );
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

bool AttachmentModel::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                    int row, int column, const QModelIndex &parent )
{
  Q_UNUSED( row );
  Q_UNUSED( column );
  Q_UNUSED( parent );

  kDebug() << "data has formats" << data->formats() << "urls" << data->urls() << "action" << action;

  if( action == Qt::IgnoreAction ) {
    return true;
  //} else if( action != Qt::CopyAction ) {
  //  return false;
  }

  if( KPIM::MailList::canDecode( data ) ) {
    // The dropped data is a list of messages.
    kDebug() << "Port me to Akonadi..."; // TODO
#if 0
    // Decode the list of serial numbers stored as the drag data
    QByteArray serNums = KPIM::MailList::serialsFromMimeData( md );
    QBuffer serNumBuffer( &serNums );
    serNumBuffer.open( QIODevice::ReadOnly );
    QDataStream serNumStream( &serNumBuffer );
    quint32 serNum;
    KMFolder *folder = 0;
    int idx;
    QList<KMMsgBase*> messageList;
    while( !serNumStream.atEnd() ) {
      KMMsgBase *msgBase = 0;
      serNumStream >> serNum;
      KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
      if( folder )
        msgBase = folder->getMsgBase( idx );
      if( msgBase )
        messageList.append( msgBase );
    }
    serNumBuffer.close();
    uint identity = folder ? folder->identity() : 0;
    KMCommand *command = new KMForwardAttachedCommand( mComposer, messageList,
                                                       identity, mComposer );
    command->start();
#endif
    return true;
  } else {
    // The dropped data is a list of URLs.
    KUrl::List urls = KUrl::List::fromMimeData( data );
    if( !urls.isEmpty() ) {
      emit attachUrlsRequested( urls );
      return true;
    } else {
      return false;
    }
  }
}

QMimeData *AttachmentModel::mimeData( const QModelIndexList &indexes ) const
{
  kDebug();
  QList<QUrl> urls;
  foreach( const QModelIndex &index, indexes ) {
    if( index.column() != 0 ) {
      // Avoid processing the same attachment more than once, since the entire
      // row is selected.
      kWarning() << "column != 0. Possibly duplicate rows passed to mimeData().";
      continue;
    }

    const AttachmentPart::Ptr part = d->parts[ index.row() ];
    QString attachmentName = part->fileName();
    if( attachmentName.isEmpty() ) {
      attachmentName = part->name();
    }
    if( attachmentName.isEmpty() ) {
      attachmentName = i18n( "unnamed attachment" );
    }

    KTempDir *tempDir = new KTempDir; // Will remove the directory on destruction.
    d->tempDirs.append( tempDir );
    const QString fileName = tempDir->name() + attachmentName;
    KPIMUtils::kByteArrayToFile( part->data(),
                                 fileName,
                                 false, false, false );
    QUrl url;
    url.setScheme( "file" );
    url.setPath( fileName );
    kDebug() << url;
    urls.append( url );
  }

  QMimeData *mimeData = new QMimeData;
  mimeData->setUrls( urls );
  return mimeData;
}

QStringList AttachmentModel::mimeTypes() const
{
  QStringList types;
  types << QString::fromLatin1( "text/uri-list" );
  return types;
}

Qt::DropActions AttachmentModel::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
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
  } else if( role == AttachmentPartRole ) {
    if( index.column() == 0 ) {
      return QVariant::fromValue( part );
    } else {
      kWarning() << "AttachmentPartRole and column != 0.";
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

bool AttachmentModel::removeAttachment( AttachmentPart::Ptr part )
{
  int idx = d->parts.indexOf( part );
  if( idx < 0 ) {
    kWarning() << "Attachment not found.";
    return false;
  }

  beginRemoveRows( QModelIndex(), idx, idx );
  d->parts.removeAt( idx );
  endRemoveRows();
  emit attachmentRemoved( part );
  return true;
}

AttachmentPart::List AttachmentModel::attachments() const
{
  return d->parts;
}

Qt::ItemFlags AttachmentModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

  if( !index.isValid() ) {
    return Qt::ItemIsDropEnabled | defaultFlags;
  }

  if( index.column() == CompressColumn ||
      index.column() == EncryptColumn ||
      index.column() == SignColumn ) {
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | defaultFlags;
  } else {
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  }
}

QVariant AttachmentModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if( orientation != Qt::Horizontal || role != Qt::DisplayRole ) {
    return QVariant();
  }

  switch( section ) {
    case NameColumn:
      return i18nc( "@title column attachment name.", "Name" );
    case SizeColumn:
      return i18nc( "@title column attachment size.", "Size" );
    case EncodingColumn:
      return i18nc( "@title column attachment encoding.", "Encoding" );
    case MimeTypeColumn:
      return i18nc( "@title column attachment type.", "Type" );
    case CompressColumn:
      return i18nc( "@title column attachment compression checkbox.", "Compress" );
    case EncryptColumn:
      return i18nc( "@title column attachment encryption checkbox.", "Encrypt" );
    case SignColumn:
      return i18nc( "@title column attachment signed checkbox.", "Sign" );
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
