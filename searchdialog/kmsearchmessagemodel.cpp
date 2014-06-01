/*
 * Copyright (c) 2011, 2012 Montel Laurent <montel@kde.org>
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
#include "mailcommon/util/mailutil.h"
#include "messagelist/messagelistutil.h"

#include "messagecore/utils/stringutil.h"

#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/monitor.h>
#include <AkonadiCore/session.h>

#include <Akonadi/KMime/MessageParts>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <QColor>
#include <QApplication>
#include <QPalette>
#include <QTextDocument>
#include <qdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <KLocale>

KMSearchMessageModel::KMSearchMessageModel( QObject *parent )
    : Akonadi::MessageModel( parent )
{
    fetchScope().fetchFullPayload();
    fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::All );
}

KMSearchMessageModel::~KMSearchMessageModel( )
{
}

QString toolTip( const Akonadi::Item& item )
{
    MessagePtr msg = item.payload<MessagePtr>();

    QColor bckColor = QApplication::palette().color( QPalette::ToolTipBase );
    QColor txtColor = QApplication::palette().color( QPalette::ToolTipText );

    const QString bckColorName = bckColor.name();
    const QString txtColorName = txtColor.name();
    const bool textIsLeftToRight = ( QApplication::layoutDirection() == Qt::LeftToRight );
    const QString textDirection =  textIsLeftToRight ? QLatin1String( "left" ) : QLatin1String( "right" );

    QString tip = QString::fromLatin1(
                "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
                );
    tip += QString::fromLatin1(
                "<tr>"                                                         \
                "<td bgcolor=\"%1\" align=\"%4\" valign=\"middle\">"           \
                "<div style=\"color: %2; font-weight: bold;\">"                \
                "%3"                                                           \
                "</div>"                                                       \
                "</td>"                                                        \
                "</tr>"
                ).arg( txtColorName ).arg( bckColorName ).arg( Qt::escape( msg->subject()->asUnicodeString() ) ).arg( textDirection );

    tip += QString::fromLatin1(
                "<tr>"                                                              \
                "<td align=\"center\" valign=\"middle\">"                           \
                "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
                );

    const QString htmlCodeForStandardRow = QString::fromLatin1(
                "<tr>"                                                       \
                "<td align=\"right\" valign=\"top\" width=\"45\">"           \
                "<div style=\"font-weight: bold;\"><nobr>"                   \
                "%1:"                                                        \
                "</nobr></div>"                                              \
                "</td>"                                                      \
                "<td align=\"left\" valign=\"top\">"                         \
                "%2"                                                         \
                "</td>"                                                      \
                "</tr>" );

    QString content = MessageList::Util::contentSummary( item );

    if ( textIsLeftToRight ) {
        tip += htmlCodeForStandardRow.arg( i18n( "From" ) ).arg( MessageCore::StringUtil::stripEmailAddr( msg->from()->asUnicodeString() ) );
        tip += htmlCodeForStandardRow.arg( i18nc( "Receiver of the email", "To" ) ).arg( MessageCore::StringUtil::stripEmailAddr( msg->to()->asUnicodeString() ) );
        tip += htmlCodeForStandardRow.arg( i18n( "Date" ) ).arg(  KLocale::global()->formatDateTime( msg->date()->dateTime().toLocalZone(), KLocale::FancyLongDate ) );
        if ( !content.isEmpty() ) {
            tip += htmlCodeForStandardRow.arg( i18n( "Preview" ) ).arg( content.replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) );
        }
    } else {
        tip += htmlCodeForStandardRow.arg(  MessageCore::StringUtil::stripEmailAddr( msg->from()->asUnicodeString() ) ).arg( i18n( "From" ) );
        tip += htmlCodeForStandardRow.arg(  MessageCore::StringUtil::stripEmailAddr( msg->to()->asUnicodeString() ) ).arg( i18nc( "Receiver of the email", "To" ) );
        tip += htmlCodeForStandardRow.arg(   KLocale::global()->formatDateTime( msg->date()->dateTime().toLocalZone(), KLocale::FancyLongDate ) ).arg( i18n( "Date" ) );
        if ( !content.isEmpty() ) {
            tip += htmlCodeForStandardRow.arg( content.replace( QLatin1Char( '\n' ), QLatin1String( "<br>" ) ) ).arg( i18n( "Preview" ) );
        }
    }
    tip += QString::fromLatin1(
                "</table"         \
                "</td>"           \
                "</tr>"
                );
    return tip;
}

int KMSearchMessageModel::columnCount( const QModelIndex & parent ) const
{
    if ( collection().isValid()
         && !collection().contentMimeTypes().contains( QLatin1String("message/rfc822") )
         && collection().contentMimeTypes() != QStringList( QLatin1String("inode/directory") ) )
        return 1;

    if ( !parent.isValid() )
        return 8; // keep in sync with the column type enum

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
            if ( item.storageCollectionId() >= 0 ) {
                return MailCommon::Util::fullCollectionPath( Akonadi::Collection( item.storageCollectionId() ) );
            }
            return MailCommon::Util::fullCollectionPath(item.parentCollection());
        case Subject:
            return msg->subject()->asUnicodeString();
        case Sender:
            return msg->from()->asUnicodeString();
        case Receiver:
            return msg->to()->asUnicodeString();
        case Date:
            return KLocale::global()->formatDateTime( msg->date()->dateTime().toLocalZone(), KLocale::FancyLongDate );
        case Size:
            if ( item.size() == 0 )
                return i18nc( "@label No size available", "-" );
            else
                return KLocale::global()->formatByteSize( item.size() );
        case SizeNotLocalized:
            return item.size();
        case DateNotTranslated:
            return msg->date()->dateTime().dateTime();
        default:
            return QVariant();
        }
    } else if ( role == Qt::EditRole ) {
        switch ( index.column() ) {
        case Collection:
            if ( item.storageCollectionId() >= 0 ) {
                return MailCommon::Util::fullCollectionPath( Akonadi::Collection( item.storageCollectionId() ) );
            }
            return MailCommon::Util::fullCollectionPath(item.parentCollection());
        case Subject:
            return msg->subject()->asUnicodeString();
        case Sender:
            return msg->from()->asUnicodeString();
        case Receiver:
            return msg->to()->asUnicodeString();
        case Date:
            return msg->date()->dateTime().dateTime();
        case SizeNotLocalized:
        case Size:
            return item.size();
        case DateNotTranslated:
            return msg->date()->dateTime().dateTime();
        default:
            return QVariant();
        }
    } else if( role == Qt::ToolTipRole ) {
        return toolTip( item );
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

