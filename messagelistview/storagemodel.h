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

#ifndef __KMAIL_MESSAGELISTVIEW_STORAGEMODEL_H__
#define __KMAIL_MESSAGELISTVIEW_STORAGEMODEL_H__

#include "messagelistview/core/storagemodelbase.h"

class KMFolder;
class KMMessage;
class KMMsgBase;

#include <QColor>

namespace KPIM
{
	class MessageStatus;
}

namespace KMail
{

namespace MessageListView
{

/**
 * The KMail specific implementation of the Core::StorageModel.
 *
 * Provides an interface over a KMFolder. In the future
 * it's expected to wrap Akonadi::MessageModel.
 */
class StorageModel : public Core::StorageModel
{
  Q_OBJECT

public:
  /**
   * Create a StorageModel wrapping the specified folder.
   */
  explicit StorageModel( KMFolder * folder, QObject * parent = 0 );
  ~StorageModel();

protected:
  // KMail specific stuff
  KMFolder * mFolder;            ///< Shallow pointer to the folder, KMail specific
  unsigned int mMessageCount;    ///< The count this model can see, actually. KMail specific
  QColor mColorNewMessage;
  QColor mColorUnreadMessage;
  QColor mColorImportantMessage;
  QColor mColorToDoMessage;
public:

  // When porting to Akonadi the stuff below will be simply killed as it serves
  // only for the "outside" world (not used in the MessageListView namespace).

  /**
   * Return the folder that this StorageModel is attached to.
   */
  KMFolder * folder() const
    { return mFolder; };

  /**
   * Return the KMMessage belonging to the specified row in the underlying folder
   */
  KMMessage * message( int row ) const;

  /**
   * Return the KMMessage belonging to the specified Core::MessageItem.
   */
  KMMessage * message( Core::MessageItem * mi ) const;

  /**
   * Return the KMMsgBase belonging to the specified row in the underlying folder
   * This is supposed to be less expensive than message() (?)
   */
  KMMsgBase * msgBase( int row ) const;

  /**
   * Return the KMMsgBase belonging to the specified Core::MessageItem.
   * This is supposed to be less expensive than message() (?)
   */
  KMMsgBase * msgBase( Core::MessageItem * mi ) const;

  /**
   * Returns the current row index of the specified msgBase or -1 if not found
   * This performs a _linear_ search: it's slow.
   */
  int msgBaseRow( KMMsgBase * msgBase );

  /**
   * Releases a previously fetched KMMessage object by row.
   * This is kinda ugly and provided only to emulate the old KMHeaders behaviour.
   * Will be killed sooner or later.
   */
  void releaseMessage( int row ) const;

  // When porting to Akonadi the stuff below needs to be implemented for MessageListView to work

  /**
   * Returns an unique id for this Storage collection.
   */
  virtual QString id() const;

  /**
   * Returns true if this StorageModel (folder) contains outbound messages and false otherwise.
   */
  virtual bool containsOutboundMessages() const;

  /**
   * Returns (a guess for) the number of unread messages just after the folder has been opened.
   */
  virtual int initialUnreadRowCountGuess() const;

  /**
   * This method uses the inner KMFolder to fill in the
   * data for the specified MessageItem from the underlying storage item at
   * the specified row index. Returns true if the data fetch was succesfull
   * and false otherwise. For base data we intend: subject, sender, receiver,
   * senderOrReceiver, size, date, signature state, encryption state, status and unique id.
   * If bUseReceiver is true then the senderOrReceiver
   * field will be set to the receiver, otherwise it will be set to the sender.
   */
  virtual bool initializeMessageItem( Core::MessageItem * mi, int row, bool bUseReceiver ) const;

  /**
   * This method uses the inner KMFolder to fill in the specified subset of
   * threading data for the specified MessageItem from the underlying storage item at
   * the specified row index. 
   */
  virtual void fillMessageItemThreadingData( Core::MessageItem * mi, int row, ThreadingDataSubset subset ) const;

  /**
   * This method uses the inner KMFolder to re-fill the date, the status,
   * the encryption state, the signature state and eventually updates the min/max dates
   * for the specified MessageItem from the underlying storage slot at the specified row index. 
   */
  virtual void updateMessageItemData( Core::MessageItem * mi, int row ) const;

  /**
   * This method uses the inner model implementation to associate the new status
   * to the specified message item. The new status is be stored in the folder but isn't
   * set as mi->status() itself as the caller is responsable for this.
   */
  virtual void setMessageItemStatus( Core::MessageItem * mi, const KPIM::MessageStatus &status );

  // When porting to Akonadi the stuff below should be already implemented by MessageModel

  /**
   * Mandatory QAbstractItemModel interface
   */
  virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;

  /**
   * Mandatory QAbstractItemModel interface
   */
  virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

  /**
   * Mandatory QAbstractItemModel interface
   */
  virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;

  /**
   * Mandatory QAbstractItemModel interface
   */
  virtual QModelIndex parent( const QModelIndex &index ) const;

  /**
   * Mandatory QAbstractItemModel interface
   */
  virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

private slots:

  /**
   * Internal handler of KMFolder signal.
   */
  void slotMessageAdded( KMFolder *folder, quint32 sernum );

  /**
   * Internal handler of KMFolder signal.
   */
  void slotMessageRemoved( int idx );

  /**
   * Internal handler of KMFolder signal.
   */
  void slotFolderCleared();

  /**
   * Internal handler of KMFolder signal.
   */
  void slotFolderClosed();

  /**
   * Internal handler of KMFolder signal.
   */
  void slotFolderExpunged();

  /**
   * Internal handler of KMFolder signal.
   */
  void slotFolderInvalidated();

  /**
   * Internal handler of KMFolder signal.
   */
  void slotFolderNameChanged();

  /**
   * Internal handler of KMFolder signal.
   */
  void slotMessageHeaderChanged( KMFolder *folder, int idx );

};

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_STORAGEMODEL_H__
