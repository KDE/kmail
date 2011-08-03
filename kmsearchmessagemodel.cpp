/*
 * Copyright (c) 2011 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#include "kmsearchmessagemodel.h"
#include "mailcommon/mailutil.h"
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

KMSearchMessageModel::KMSearchMessageModel( QObject *parent ) :
  Akonadi::MessageModel( parent )
{
  fetchScope().fetchPayloadPart( Akonadi::MessagePart::Envelope );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::All );
}

KMSearchMessageModel::~KMSearchMessageModel( )
{
}


int KMSearchMessageModel::columnCount( const QModelIndex & parent ) const
{
  if ( collection().isValid()
          && !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") )
          && collection().contentMimeTypes() != QStringList( QLatin1String("inode/directory") ) )
    return 1;

  if ( !parent.isValid() )
    return 6; // keep in sync with the column type enum

  return 0;
}

QVariant KMSearchMessageModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= rowCount() )
    return QVariant();

  if ( !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") ) ) {
     if ( role == Qt::DisplayRole )
       return i18nc( "@label", "This model can only handle email folders. The current collection holds mimetypes: %1",
                       collection().contentMimeTypes().join( QLatin1String(",") ) );
     else
       return QVariant();
  }

  Akonadi::Item item = itemForIndex( index );
  if ( !item.hasPayload<MessagePtr>() )
    return QVariant();
  MessagePtr msg = item.payload<MessagePtr>();
  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Collection:
        return MailCommon::Util::fullCollectionPath(item.parentCollection());
      case Subject:
        return msg->subject()->asUnicodeString();
      case Sender:
        return msg->from()->asUnicodeString();
      case Receiver:
        return msg->to()->asUnicodeString();
      case Date:
        return KGlobal::locale()->formatDateTime( msg->date()->dateTime().toLocalZone(), KLocale::FancyLongDate );
      case Size:
        if ( item.size() == 0 )
          return i18nc( "@label No size available", "-" );
        else
          return KGlobal::locale()->formatByteSize( item.size() );
      default:
        return QVariant();
    }
  } else if ( role == Qt::EditRole ) {
    switch ( index.column() ) {
      case Collection:
        return MailCommon::Util::fullCollectionPath(item.parentCollection());
      case Subject:
        return msg->subject()->asUnicodeString();
      case Sender:
        return msg->from()->asUnicodeString();
      case Receiver:
        return msg->to()->asUnicodeString();
      case Date:
        return msg->date()->dateTime().dateTime();
      case Size:
        return item.size();
      default:
        return QVariant();
    }
  }
  return ItemModel::data( index, role );
}

QVariant KMSearchMessageModel::headerData( int section, Qt::Orientation orientation, int role ) const
{

  if ( collection().isValid()
          && !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") )
          && collection().contentMimeTypes() != QStringList( QLatin1String("inode/directory") ) )
    return QVariant();

  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
    case Collection:
      return i18nc( "@title:column, folder (e.g. email)", "Folder" );
    default:
      return Akonadi::MessageModel::headerData( ( section-1 ), orientation, role );
    }
  }
  return Akonadi::MessageModel::headerData( ( section-1 ), orientation, role );
}

#include "kmsearchmessagemodel.moc"
