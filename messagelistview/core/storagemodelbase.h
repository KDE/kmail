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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_STORAGEMODEL_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_STORAGEMODEL_H__

#include <QAbstractItemModel>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class MessageItem;

/**
 * The QAbstractItemModel based interface that you need
 * to provide for your storage to work with MessageListView.
 */
class StorageModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  StorageModel( QObject * parent = 0 );
  ~StorageModel();

public:

  /**
   * Returns an unique id for this Storage collection.
   * FIXME: This could be embedded in "name()" ?
   */
  virtual QString id() const = 0;

  /**
   * Returns true if this StorageModel (folder) contains outbound messages and false otherwise.
   */
  virtual bool containsOutboundMessages() const = 0;

  /**
   * Returns (a guess for) the number of unread messages: must be pessimistic (i.e. if you
   * have no idea just return rowCount(), which is also what the default implementation does).
   * This must be (and is) queried ONLY when the folder is first opened. It doesn't actually need
   * to keep the number of messages in sync as they later arrive to the storage.
   */
  virtual int initialUnreadRowCountGuess() const;

  /**
   * This method should use the inner model implementation to fill in the
   * base data for the specified MessageItem from the underlying storage slot at
   * the specified row index. Must return true if the data fetch was succesfull
   * and false otherwise. For base data we intend: subject, sender, receiver,
   * senderOrReceiver, size, date, encryption state, signature state and status.
   * If bUseReceiver is true then the "senderOrReceiver"
   * field must be set to the receiver, otherwise it must be set to the sender.
   */
  virtual bool initializeMessageItem( MessageItem * it, int row, bool bUseReceiver ) const = 0;

  enum ThreadingDataSubset
  {
    PerfectThreadingOnly,                 ///< Only the data for messageIdMD5 and inReplyToMD5 is needed
    PerfectThreadingPlusReferences,       ///< messageIdMD5, inReplyToMD5, referencesIdMD5
    PerfectThreadingReferencesAndSubject  ///< All of the above plus subject stuff
  };

  /**
   * This method should use the inner model implementation to fill in the specified subset of
   * threading data for the specified MessageItem from the underlying storage slot at
   * the specified row index. 
   */
  virtual void fillMessageItemThreadingData( MessageItem * mi, int row, ThreadingDataSubset subset ) const  = 0;

  /**
   * This method should use the inner model implementation to re-fill the date, the status,
   * the encryption state, the signature state and eventually update the min/max dates
   * for the specified MessageItem from the underlying storage slot at the specified row index. 
   */
  virtual void updateMessageItemData( MessageItem * mi, int row ) const = 0;

};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_STORAGEMODEL_H__
