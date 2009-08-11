/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "ak_storagemodel.h"

#include <akonadi/attributefactory.h>
#include <akonadi/collection.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/selectionproxymodel.h>
#include <akonadi/item.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/kmime/messagefolderattribute.h>

#include <KDE/KCodecs>
#include <KDE/KLocale>

#include "messagelistview/akonadi/ak_storagemodel.h"
#include "messagelistview/core/messageitem.h"

#include <QtCore/QAbstractItemModel>
#include <QtGui/QItemSelectionModel>

using namespace Akonadi::MessageListView;
using namespace KMail::MessageListView;


StorageModel::StorageModel( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent )
  : Core::StorageModel( parent ), mModel( 0 ), mSelectionModel( selectionModel )
{
  AttributeFactory::registerAttribute<MessageFolderAttribute>();

  SelectionProxyModel *childrenFilter = new SelectionProxyModel( mSelectionModel, this );
  childrenFilter->setSourceModel( model );
  childrenFilter->setFilterBehavior( SelectionProxyModel::OnlySelectedChildren );

  EntityFilterProxyModel *itemFilter = new EntityFilterProxyModel( this );
  itemFilter->setSourceModel( childrenFilter );
  itemFilter->addMimeTypeExclusionFilter( Collection::mimeType() );
  itemFilter->addMimeTypeInclusionFilter( "message/rfc822" );
  itemFilter->setHeaderSet( EntityTreeModel::ItemListHeaders );

  mModel = itemFilter;

  // Custom/System colors
  KConfigGroup config( KSharedConfig::openConfig( "kmailrc" ), "Reader" );

  mColorNewMessage = QColor( "red" );
  mColorUnreadMessage = QColor( "blue" );
  mColorImportantMessage = QColor( 0x00, 0x7f, 0x00 );
  mColorToDoMessage = QColor( 0x00, 0x98, 0x00 );

  if ( !config.readEntry( "defaultColors", true ) ) {
    mColorNewMessage = config.readEntry( "NewMessage", mColorNewMessage );
    mColorUnreadMessage = config.readEntry( "UnreadMessage", mColorUnreadMessage );
    mColorImportantMessage = config.readEntry( "FlagMessage", mColorImportantMessage );
    mColorToDoMessage = config.readEntry( "TodoMessage", mColorToDoMessage );
  }

  connect( mModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
           this, SLOT(onSourceDataChanged(QModelIndex, QModelIndex)) );

  connect( mModel, SIGNAL(layoutAboutToBeChanged()),
           this, SIGNAL(layoutAboutToBeChanged()) );
  connect( mModel, SIGNAL(layoutChanged()),
           this, SIGNAL(layoutChanged()) );
  connect( mModel, SIGNAL(modelAboutToBeReset()),
           this, SIGNAL(modelAboutToBeReset()) );
  connect( mModel, SIGNAL(modelReset()),
           this, SIGNAL(modelReset()) );

  //Here we assume we'll always get QModelIndex() in the parameters
  connect( mModel, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)),
           this, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)) );
  connect( mModel, SIGNAL(rowsInserted(QModelIndex, int, int)),
           this, SIGNAL(rowsInserted(QModelIndex, int, int)) );
  connect( mModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
           this, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)) );
  connect( mModel, SIGNAL(rowsRemoved(QModelIndex, int, int)),
           this, SIGNAL(rowsRemoved(QModelIndex, int, int)) );

  connect( mSelectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           this, SLOT(onSelectionChanged()) );
}

StorageModel::~StorageModel()
{
}

QString StorageModel::id() const
{
  QStringList ids;
  QModelIndexList indexes = mSelectionModel->selectedRows();

  foreach ( const QModelIndex &index, indexes ) {
    Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
    if ( c.isValid() ) {
      ids << QString::number( c.id() );
    }
  }

  ids.sort();
  return ids.join(":");
}

bool StorageModel::containsOutboundMessages() const
{
  QModelIndexList indexes = mSelectionModel->selectedRows();

  foreach ( const QModelIndex &index, indexes ) {
    Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
    if ( c.isValid()
      && c.hasAttribute<MessageFolderAttribute>()
      && c.attribute<MessageFolderAttribute>()->isOutboundFolder() ) {
      return true;
    }
  }

  return false;
}

int StorageModel::initialUnreadRowCountGuess() const
{
  QModelIndexList indexes = mSelectionModel->selectedRows();

  int unreadCount = 0;

  foreach ( const QModelIndex &index, indexes ) {
    Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
    if ( c.isValid() ) {
      unreadCount+= c.statistics().unreadCount();
    }
  }

  return unreadCount;
}

bool StorageModel::initializeMessageItem( KMail::MessageListView::Core::MessageItem *mi,
                                          int row, bool bUseReceiver ) const
{
  Item item = mModel->data( mModel->index( row, 0 ), EntityTreeModel::ItemRole ).value<Item>();
  const MessagePtr mail = messageForRow( row );
  if ( !mail ) return false;

  QString sender = mail->from()->asUnicodeString();
  QString receiver = mail->to()->asUnicodeString();

  // Static for speed reasons
  static const QString noSubject = i18nc( "displayed as subject when the subject of a mail is empty", "No Subject" );
  static const QString unknown( i18nc( "displayed when a mail has unknown sender, receiver or date", "Unknown" ) );

  if ( sender.isEmpty() ) {
    sender = unknown;
  }
  if ( receiver.isEmpty() ) {
    receiver = unknown;
  }

  mi->initialSetup( mail->date()->dateTime().toTime_t(),
                    item.size(),
                    sender, receiver,
                    bUseReceiver ? receiver : sender );

  mi->setUniqueId( item.id() );

  QString subject = mail->subject()->asUnicodeString();
  if ( subject.isEmpty() ) {
    subject = '(' + noSubject + ')';
  }

  mi->setSubject( subject );

  updateMessageItemData( mi, row );

  return true;
}

static QString md5Encode( const QString &str )
{
  if ( str.trimmed().isEmpty() ) return QString();

  KMD5 md5( str.trimmed().toUtf8() );
  static const int Base64EncodedMD5Len = 22;
  return md5.base64Digest().left( Base64EncodedMD5Len );
}

void StorageModel::fillMessageItemThreadingData( KMail::MessageListView::Core::MessageItem *mi,
                                                 int row, ThreadingDataSubset subset ) const
{
  const MessagePtr mail = messageForRow( row );
  Q_ASSERT( mail ); // We ASSUME that initializeMessageItem has been called successfully...

  switch ( subset ) {
  case PerfectThreadingReferencesAndSubject:
    mi->setStrippedSubjectMD5( md5Encode( mail->subject()->stripOffPrefixes() ) );
    mi->setSubjectIsPrefixed( mail->subject()->asUnicodeString()!=mail->subject()->stripOffPrefixes() );
    // fall through
  case PerfectThreadingPlusReferences:
    if ( !mail->references()->identifiers().isEmpty() ) {
      mi->setReferencesIdMD5( md5Encode( mail->references()->identifiers().first() ) );
    }
    // fall through
  case PerfectThreadingOnly:
    mi->setMessageIdMD5( md5Encode( mail->messageID()->asUnicodeString() ) );
    if ( !mail->inReplyTo()->identifiers().isEmpty() ) {
      mi->setInReplyToIdMD5( md5Encode( mail->inReplyTo()->identifiers().first() ) );
    }
    break;
  default:
    Q_ASSERT( false ); // should never happen
    break;
  }
}

void StorageModel::updateMessageItemData( KMail::MessageListView::Core::MessageItem *mi,
                                          int row ) const
{
  Item item = mModel->data( mModel->index( row, 0 ), EntityTreeModel::ItemRole ).value<Item>();
  const MessagePtr mail = messageForRow( row );
  Q_ASSERT( mail );

  KPIM::MessageStatus stat;
  stat.setStatusFromFlags( item.flags() );

  mi->setStatus( stat );

  mi->setEncryptionState( Core::MessageItem::EncryptionStateUnknown );
  if ( mail->contentType()->isSubtype( "encrypted" )
    || mail->contentType()->isSubtype( "pgp-encrypted" )
    || mail->contentType()->isSubtype( "pkcs7-mime" ) ) {
      mi->setEncryptionState( Core::MessageItem::FullyEncrypted );
  } else if ( mail->mainBodyPart( "multipart/encrypted" )
           || mail->mainBodyPart( "application/pgp-encrypted" )
           || mail->mainBodyPart( "application/pkcs7-mime" ) ) {
    mi->setEncryptionState( Core::MessageItem::PartiallyEncrypted );
  }

  mi->setSignatureState( Core::MessageItem::SignatureStateUnknown );
  if ( mail->contentType()->isSubtype( "signed" )
    || mail->contentType()->isSubtype( "pgp-signature" )
    || mail->contentType()->isSubtype( "pkcs7-signature" )
    || mail->contentType()->isSubtype( "x-pkcs7-signature" ) ) {
      mi->setSignatureState( Core::MessageItem::FullySigned );
  } else if ( mail->mainBodyPart( "multipart/signed" )
           || mail->mainBodyPart( "application/pgp-signature" )
           || mail->mainBodyPart( "application/pkcs7-signature" )
           || mail->mainBodyPart( "application/x-pkcs7-signature" ) ) {
    mi->setSignatureState( Core::MessageItem::PartiallySigned );
  }

  QColor clr;
  QColor backClr;
#if 0
  //FIXME: Missing tag handling, nepomuk based?
  QList< Core::MessageItem::Tag * > * tagList;
  tagList = fillTagList( msg, clr, backClr );
  mi->setTagList( tagList );
#endif

  if ( !clr.isValid() ) {
    if ( stat.isNew() ) {
      clr = mColorNewMessage;
    } else if ( stat.isUnread() ) {
      clr = mColorUnreadMessage;
    } else if ( stat.isImportant() ) {
      clr = mColorImportantMessage;
    } else if ( stat.isToAct() ) {
      clr = mColorToDoMessage;
    }
  }

  if ( clr.isValid() ) {
    mi->setTextColor( clr );
  }
  if ( backClr.isValid() ) {
    mi->setBackgroundColor( backClr );
  }
}

void StorageModel::setMessageItemStatus( KMail::MessageListView::Core::MessageItem *mi,
                                         int row, const KPIM::MessageStatus &status )
{
  Item item = mModel->data( mModel->index( row, 0 ), EntityTreeModel::ItemRole ).value<Item>();
  item.setFlags( status.getStatusFlags() );
  new Akonadi::ItemModifyJob( item, this );
}

QVariant StorageModel::data( const QModelIndex &index, int role ) const
{
  // We don't provide an implementation for data() in No-Akonadi-KMail.
  // This is because StorageModel must be a wrapper anyway (because columns
  // must be re-mapped and functions not available in a QAbstractItemModel
  // are strictly needed. So when porting to Akonadi this class will
  // either wrap or subclass the MessageModel and implement initializeMessageItem()
  // with appropriate calls to data(). And for No-Akonadi-KMail we still have
  // a somewhat efficient implementation.

  Q_UNUSED( index );
  Q_UNUSED( role );

  return QVariant();
}

int StorageModel::columnCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return 1;
  return 0; // this model is flat.
}

QModelIndex StorageModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return createIndex( row, column, 0 );

  return QModelIndex(); // this model is flat.
}

QModelIndex StorageModel::parent( const QModelIndex &index ) const
{
  Q_UNUSED( index );
  return QModelIndex(); // this model is flat.
}

int StorageModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return mModel->rowCount();
  return 0; // this model is flat.
}

void StorageModel::prepareForScan()
{

}

void StorageModel::onSourceDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
  emit dataChanged( index( topLeft.row(), 0 ),
                    index( bottomRight.row(), 0 ) );
}

void StorageModel::onSelectionChanged()
{
  emit headerDataChanged( Qt::Horizontal, 0, columnCount()-1 );
}

MessagePtr StorageModel::messageForRow( int row ) const
{
  Item item = mModel->data( mModel->index( row, 0 ), EntityTreeModel::ItemRole ).value<Item>();

  if ( !item.hasPayload<MessagePtr>() ) {
    kWarning() << "Not a message" << item.id() << item.remoteId() << item.mimeType();
    return MessagePtr();
  }

  return item.payload<MessagePtr>();
}
