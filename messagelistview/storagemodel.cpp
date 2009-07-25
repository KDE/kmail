/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/storagemodel.h"

#include "messagelistview/core/messageitem.h"

#include <config-kmail.h>

#include <libkdepim/messagestatus.h>

#include <KDebug>
#include <KIconLoader>
#include <QList>

#include "kmfolder.h"
#include "kmfolderindex.h"
#include "kmmsgbase.h"
#include "kmmessagetag.h"
#include "kmkernel.h"
#include "kmmsgdict.h"

namespace KMail
{

namespace MessageListView
{

StorageModel::StorageModel( KMFolder * folder, QObject * parent )
  : Core::StorageModel( parent ), mFolder( folder )
{

#ifdef KDEPIM_FOLDEROPEN_PROFILE
  QTime openFolderTimer;
  openFolderTimer.start();
#endif

  mFolder->open( "MessageListView::StorageModel" );

#ifdef KDEPIM_FOLDEROPEN_PROFILE
  kDebug() << "==========================================================";
  kDebug() << "Opening the folder took" << openFolderTimer.elapsed() << "msecs.";
  QTime serialCacheTimer;
  serialCacheTimer.start();
#endif

  // Add all items of the folder to the serial cache. This makes initalizing
  // the message items much faster, since they no longer need to call the expensive
  // KMFolderIndex::find() when trying to get the serial number.
  if ( mFolder->storage() ) {
     KMFolderIndex *index = dynamic_cast<KMFolderIndex*>( mFolder->storage() );
     if ( index )
       index->addToSerialCache();
  }

#ifdef KDEPIM_FOLDEROPEN_PROFILE
  kDebug() << "Creating the serial cache took" << serialCacheTimer.elapsed() << "msecs.";
#endif

#if 0
  // This signal is unreliable, it's not emitted when quiet() is set on the folder...
  // It's a shame since with the other signal we need to lookup the index which
  // is actually known when it's emitted....
  connect( mFolder, SIGNAL( msgAdded( int ) ),
           SLOT( slotMessageAdded( int ) ) );
#endif

  connect( mFolder, SIGNAL( msgAdded( KMFolder *, quint32 ) ),
           SLOT( slotMessageAdded( KMFolder *, quint32 ) ) );

  connect( mFolder, SIGNAL( msgRemoved( int, const QString & ) ),
           SLOT( slotMessageRemoved( int ) ) );

  connect( mFolder, SIGNAL( cleared() ),
           SLOT( slotFolderCleared() ) );

  connect( mFolder, SIGNAL( compacted() ),
           SLOT( slotFolderCompacted() ) );

  connect( mFolder, SIGNAL( closed() ),
           SLOT( slotFolderClosed() ) );

  connect( mFolder, SIGNAL( expunged( KMFolder * ) ),
           SLOT( slotFolderExpunged() ) );

  connect( mFolder->storage(), SIGNAL( invalidated( KMFolder * ) ),
           SLOT( slotFolderInvalidated() ) );

  connect( mFolder, SIGNAL( nameChanged() ),
           SLOT( slotFolderNameChanged() ) );

  connect( mFolder, SIGNAL( msgHeaderChanged( KMFolder * , int ) ),
           SLOT( slotMessageHeaderChanged( KMFolder *, int ) ) );

  connect( mFolder, SIGNAL( viewConfigChanged() ),
           SLOT( slotViewConfigChanged() ) );

#if 0
  // FIXME: Do we need to handle these remaining signals ?

  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when the icon paths are set. */
  void iconsChanged();

  /** Emitted when the shortcut associated with this folder changes. */
  void shortcutChanged( KMFolder * );

  /** Emitted, when the status of a message is changed */
  void msgChanged(KMFolder*, quint32 sernum, int delta);

  /** Emmited to display a message somewhere in a status line. */
  void statusMsg(const QString&);

  /** Emitted when a folder was removed */
  void removed(KMFolder*, bool);

#endif

  mMessageCount = mFolder->count();

  // Custom/System colors
  KConfigGroup config( KMKernel::config(), "Reader" );

  mColorNewMessage = QColor( "red" );
  mColorUnreadMessage = QColor( "blue" );
  mColorImportantMessage = QColor( 0x00, 0x7f, 0x00 );
  mColorToDoMessage = QColor( 0x00, 0x98, 0x00 );

  if ( !config.readEntry( "defaultColors", true ) )
  {
    mColorNewMessage = config.readEntry( "NewMessage", mColorNewMessage );
    mColorUnreadMessage = config.readEntry( "UnreadMessage", mColorUnreadMessage );
    mColorImportantMessage = config.readEntry( "FlagMessage", mColorImportantMessage );
    mColorToDoMessage = config.readEntry( "TodoMessage", mColorToDoMessage );
  }

}

StorageModel::~StorageModel()
{
  // Disconnect anything that might trigger before we write index and close the folder...

#if 0
  disconnect(
      mFolder, SIGNAL( msgAdded( int ) ),
      this, SLOT( slotMessageAdded( int ) )
    );
#endif

  disconnect(
      mFolder, SIGNAL( msgAdded( KMFolder *, quint32 ) ),
      this, SLOT( slotMessageAdded( KMFolder *, quint32 ) )
    );

  disconnect(
      mFolder, SIGNAL( msgRemoved( int, const QString & ) ),
      this, SLOT( slotMessageRemoved( int ) )
    );

  disconnect(
      mFolder, SIGNAL( cleared() ),
      this, SLOT( slotFolderCleared() )
    );

  disconnect(
      mFolder, SIGNAL( closed() ),
      this, SLOT( slotFolderClosed() )
    );

  disconnect(
      mFolder, SIGNAL( expunged( KMFolder * ) ),
      this, SLOT( slotFolderExpunged() )
    );

  disconnect(
      mFolder, SIGNAL( nameChanged() ),
      this, SLOT( slotFolderNameChanged() )
    );

  disconnect(
      mFolder, SIGNAL( msgHeaderChanged( KMFolder * , int ) ),
      this, SLOT( slotMessageHeaderChanged( KMFolder *, int ) )
    );

  disconnect(
      mFolder, SIGNAL( viewConfigChanged() ),
      this, SLOT( slotViewConfigChanged() )
    );


  //mFolder->markNewAsUnread(); <-- do we REALLY need to do this ?.. couldn't we use a timed-expire instead ?
  if ( mFolder->dirty() )
    mFolder->writeIndex(); // this is straight from KMHeaders...

  mFolder->close( "MessageListView::StorageModel" );
}

void StorageModel::releaseMessage( int row ) const
{
  Q_ASSERT( row >= 0 );
  if ( row >= mFolder->count() ) {
    kWarning() << "Trying to release a message at row" << row << "that no longer exists in the folder";
  }

  KMMessage * msg = mFolder->getMsg( row );
  if ( !msg )
    return;

  if ( msg->transferInProgress() )
    return;

  mFolder->ignoreJobsForMessage( msg );
  mFolder->unGetMsg( row );
}

KMMessage * StorageModel::message( int row ) const
{
  Q_ASSERT( row >= 0 );
  Q_ASSERT( row < mFolder->count() );

  return mFolder->getMsg( row ); // this CAN fail!
}

KMMessage * StorageModel::message( Core::MessageItem * mi ) const
{
  if ( !mi->isValid() )
    return 0;
  // FIXME: KMHeaders seem to _attempt_ to apply an unGetMsg()
  //        on the previously queried message item.
  //        The fact is that NOT ALL of the KMMessages requested
  //        get a corresponding unGetMsg(). It's also very unclear
  //        when the unGetMsg() should happen since there seem
  //        to be (documented by comments) conditions in that
  //        the unGetMsg() causes crashes in other parts of KMail
  //        that still reference the KMMessage object. This is broken
  //        and should be substituted by a proper reference counting
  //        method. Anyone who wishes to keep the message for more
  //        than one Qt event processing step should ref() the message.
  //        The folder storage, then, should have a housekeeping
  //        mechanism (timer which sweeps away the unreferenced objects).
  return message( mi->currentModelIndexRow() );
}

KMMsgBase * StorageModel::msgBase( int row ) const
{
  Q_ASSERT( row >= 0 );
  Q_ASSERT( row < mFolder->count() );

  return mFolder->getMsgBase( row );
}

KMMsgBase * StorageModel::msgBase( Core::MessageItem * mi ) const
{
  if ( !mi->isValid() )
    return 0;
  return msgBase( mi->currentModelIndexRow() );
}

int StorageModel::msgBaseRow( KMMsgBase * msgBase )
{
  return mFolder->find( msgBase );
}

QString StorageModel::id() const
{
  return mFolder->idString();
}

bool StorageModel::containsOutboundMessages() const
{
  return mFolder->whoField().toLower() == "to";
}

/**
 * Uses the KMMsgBase to fill a list of tags. It also picks out
 * the colors the message should use.
 */
QList< Core::MessageItem::Tag * > * fillTagList( KMMsgBase * msg,
    QColor & textColor, QColor & backgroundColor )
{
  // FIXME: Tags should be sorted by priority!
  QList< Core::MessageItem::Tag * > * tagList = 0;

  if ( msg->tagList() )
  {
    if ( !msg->tagList()->isEmpty() )
    {
      int bestPriority = -0xfffff;

      tagList = new QList< Core::MessageItem::Tag * >();
      for ( KMMessageTagList::Iterator it = msg->tagList()->begin(); it != msg->tagList()->end(); ++it )
      {
        const KMMessageTagDescription * description = kmkernel->msgTagMgr()->find( *it );
        if ( description )
        {
          if ( ( bestPriority < description->priority() ) || ( !textColor.isValid() ) )
          {
            textColor = description->textColor();
            backgroundColor = description->backgroundColor();
            bestPriority = description->priority();
          }

          Core::MessageItem::Tag * tag;
          if ( description->toolbarIconName().isEmpty() )
            tag = new Core::MessageItem::Tag( SmallIcon( "feed-subscribe" ), description->name(), *it );
          else
            tag = new Core::MessageItem::Tag( SmallIcon( description->toolbarIconName() ), description->name(), *it );
          tagList->append( tag );
        }
      }
      if ( tagList->isEmpty() )
      {
        delete tagList;
        tagList = 0;
      }
    }
  }

  return tagList;
}

static void setMessageItemEncryptionState( Core::MessageItem * mi, KMMsgBase * msg )
{
  switch ( msg->encryptionState() )
  {
    case KMMsgFullyEncrypted:
      mi->setEncryptionState( Core::MessageItem::FullyEncrypted );
    break;
    case KMMsgPartiallyEncrypted:
      mi->setEncryptionState( Core::MessageItem::PartiallyEncrypted );
    break;
    case KMMsgEncryptionStateUnknown:
    case KMMsgEncryptionProblematic:
      mi->setEncryptionState( Core::MessageItem::EncryptionStateUnknown );
    break;
    default:
      mi->setEncryptionState( Core::MessageItem::NotEncrypted );
    break;
  }
}

static void setMessageItemSignatureState( Core::MessageItem * mi, KMMsgBase * msg )
{
  switch ( msg->signatureState() )
  {
    case KMMsgFullySigned:
      mi->setSignatureState( Core::MessageItem::FullySigned );
    break;
    case KMMsgPartiallySigned:
      mi->setSignatureState( Core::MessageItem::PartiallySigned );
    break;
    case KMMsgSignatureStateUnknown:
    case KMMsgSignatureProblematic:
      mi->setSignatureState( Core::MessageItem::SignatureStateUnknown );
    break;
    default:
      mi->setSignatureState( Core::MessageItem::NotSigned );
    break;
  }
}

void StorageModel::prepareForScan()
{
  Q_ASSERT( mFolder );

  if ( !mFolder->isOpened() )
  {
    // It happens... hard to say why :D
    kDebug() << "The folder was closed. Re-opening.";
    mFolder->open( "MessageListView::StorageModel" );
  }
}

bool StorageModel::initializeMessageItem( Core::MessageItem * mi, int row, bool bUseReceiver ) const
{
  KMMsgBase * msg = mFolder->getMsgBase( row );
  if ( !msg )
    return false;

  QString sender = msg->fromStrip();
  QString receiver = msg->toStrip();

  // Static for speed reasons
  static const QString noSubject = i18nc( "displayed as subject when the subject of a mail is empty", "No Subject" );
  static const QString unknown( i18nc( "displayed when a mail has unknown sender, receiver or date", "Unknown" ) );

  if ( sender.isEmpty() )
    sender = unknown;
  if ( receiver.isEmpty() )
    receiver = unknown;

  mi->initialSetup(
      msg->date(),
      mFolder->folderType() == KMFolderTypeImap ? msg->msgSizeServer() : msg->msgSize(),
      sender, receiver,
      bUseReceiver ? receiver : sender
    );

  mi->setUniqueId( msg->getMsgSerNum() );

  KPIM::MessageStatus stat = msg->messageStatus();

  QString subject = msg->subject();
  if ( subject.isEmpty() )
    subject = '(' + noSubject + ')';
  mi->setSubjectAndStatus(
      subject,
      stat
    );

  setMessageItemEncryptionState( mi, msg );
  setMessageItemSignatureState( mi, msg );

  QColor clr;
  QColor backClr;
  QList< Core::MessageItem::Tag * > * tagList;
  tagList = fillTagList( msg, clr, backClr );
  mi->setTagList( tagList );

  if ( !clr.isValid() )
  {
    if ( stat.isNew() )
      clr = mColorNewMessage;
    else if ( stat.isUnread() )
      clr = mColorUnreadMessage;
    else if ( stat.isImportant() )
      clr = mColorImportantMessage;
    else if ( stat.isToAct() )
      clr = mColorToDoMessage;
  }

  if ( clr.isValid() )
    mi->setTextColor( clr );
  if ( backClr.isValid() )
    mi->setBackgroundColor( backClr );

  return true;
}

void StorageModel::updateMessageItemData( Core::MessageItem * mi, int row ) const
{
  KMMsgBase * msg = mFolder->getMsgBase( row );
  Q_ASSERT( msg ); // We ASSUME that initializeMessageItem has been called successfully...

  bool dateDiffers = mi->date() != msg->date();

  if ( dateDiffers )
  {
    mi->setDate( msg->date() );
    mi->recomputeMaxDate();
  }

  KPIM::MessageStatus stat = msg->messageStatus();

  mi->setStatus( stat );

  setMessageItemEncryptionState( mi, msg );
  setMessageItemSignatureState( mi, msg );

  QColor clr;
  QColor backClr;
  QList< Core::MessageItem::Tag * > * tagList;
  tagList = fillTagList( msg, clr, backClr );
  mi->setTagList( tagList );

  if ( !clr.isValid() )
  {
    if ( stat.isNew() )
      clr = mColorNewMessage;
    else if ( stat.isUnread() )
      clr = mColorUnreadMessage;
    else if ( stat.isImportant() )
      clr = mColorImportantMessage;
    else if ( stat.isToAct() )
      clr = mColorToDoMessage;
  }

  mi->setTextColor( clr ); // set even if invalid (->default color)
  mi->setBackgroundColor( backClr );

  // FIXME: Handle MDN State ?
}

void StorageModel::fillMessageItemThreadingData( Core::MessageItem * mi, int row, ThreadingDataSubset subset ) const
{
  KMMsgBase * msg = mFolder->getMsgBase( row );
  Q_ASSERT( msg ); // We ASSUME that initializeMessageItem has been called successfully...

  switch ( subset )
  {
    case PerfectThreadingReferencesAndSubject:
      mi->setStrippedSubjectMD5( msg->strippedSubjectMD5() );
      if ( mi->strippedSubjectMD5().isEmpty() )
      {
        msg->initStrippedSubjectMD5();
        mi->setStrippedSubjectMD5( msg->strippedSubjectMD5() );
      }
      mi->setSubjectIsPrefixed( msg->subjectIsPrefixed() && !msg->subject().isEmpty() );
    // fall through
    case PerfectThreadingPlusReferences:
      mi->setReferencesIdMD5( msg->replyToAuxIdMD5() );
    // fall through
    case PerfectThreadingOnly:
      mi->setMessageIdMD5( msg->msgIdMD5() );
      mi->setInReplyToIdMD5( msg->replyToIdMD5() );
    break;
    default:
      Q_ASSERT( false ); // should never happen
    break;
  };  
}

void StorageModel::setMessageItemStatus( Core::MessageItem * mi, const KPIM::MessageStatus &status )
{
  KMMsgBase * msg = msgBase( mi );
  if ( !msg )
    return; // This can be called at a really later stage (with respect to the initial fill).
            // Assume that something wrong may be happened to the folder in the meantime...

  msg->setStatus( status );
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

QModelIndex StorageModel::parent( const QModelIndex & ) const
{
  return QModelIndex(); // this model is flat.
}

int StorageModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
    return mMessageCount;
  return 0; // this model is flat.
}

int StorageModel::initialUnreadRowCountGuess() const
{
  return mFolder->countUnread();
}


void StorageModel::slotFolderClosed()
{
  Q_ASSERT( mFolder );

  // Reopen the folder, then reset the model.
  mFolder->open( "MessageListView::StorageModel" );
  slotFolderCleared();
}

void StorageModel::slotFolderCleared()
{
  Q_ASSERT( mFolder );

  // Something is screwed in the MBOX compaction: the folder tends to be closed and the closed() signal isnt emitted
  if( !mFolder->isOpened() )
    mFolder->open( "MessageListView::StorageModel" );

  mMessageCount = mFolder->count();
  reset();
}

void StorageModel::slotFolderCompacted()
{
  slotFolderCleared();
}

void StorageModel::slotViewConfigChanged()
{
  reset();
}

void StorageModel::slotFolderExpunged()
{
  // ?
  slotFolderClosed();
}

void StorageModel::slotFolderInvalidated()
{
  slotFolderCleared();
}

void StorageModel::slotFolderNameChanged()
{
}

void StorageModel::slotMessageHeaderChanged( KMFolder *folder, int idx )
{
  Q_ASSERT( folder == mFolder );
  QModelIndex modelIndex = createIndex( idx, 0 );
  emit dataChanged( modelIndex, modelIndex );
}

void StorageModel::slotMessageAdded( KMFolder *folder, quint32 sernum )
{
  Q_ASSERT( folder == mFolder );

  KMFolder * retFolder;
  int idx;

  KMMsgDict::instance()->getLocation( sernum, &retFolder, &idx );

  // retFolder is 0 in case of search folders
  Q_ASSERT( retFolder == folder || !retFolder );
  Q_ASSERT( idx != -1 );

  beginInsertRows( QModelIndex(), idx, idx );
  mMessageCount++;
  endInsertRows();
}

void StorageModel::slotMessageRemoved( int idx )
{
  beginRemoveRows( QModelIndex(), idx, idx );
  mMessageCount--;
  endRemoveRows();
}

} // namespace MessageListView

} // namespace KMail

