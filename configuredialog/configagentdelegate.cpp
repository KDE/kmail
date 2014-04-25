/*
    Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>
    Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#include "configagentdelegate.h"

#include <Akonadi/AgentInstanceModel>
#include <AkonadiCore/AgentInstance>

#include <KDebug>
#include <KIcon>
#include <KLocalizedString>
#include <KIconLoader>

#include <QUrl>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <QMouseEvent>

using Akonadi::AgentInstanceModel;
using Akonadi::AgentInstance;

static const int s_delegatePaddingSize = 7;

struct Icons {
    Icons()
        : readyPixmap ( KIcon ( QLatin1String ( "user-online" ) ).pixmap ( QSize ( 16, 16 ) ) )
        , syncPixmap ( KIcon ( QLatin1String ( "network-connect" ) ).pixmap ( QSize ( 16, 16 ) ) )
        , errorPixmap ( KIcon ( QLatin1String ( "dialog-error" ) ).pixmap ( QSize ( 16, 16 ) ) )
        , offlinePixmap ( KIcon ( QLatin1String ( "network-disconnect" ) ).pixmap ( QSize ( 16, 16 ) ) )
        , checkMailIcon ( KIcon ( QLatin1String ("mail-receive") ) ) {
    }
    QPixmap readyPixmap, syncPixmap, errorPixmap, offlinePixmap;
    KIcon checkMailIcon;
};

K_GLOBAL_STATIC ( Icons, s_icons )

ConfigAgentDelegate::ConfigAgentDelegate ( QObject *parent )
    : QStyledItemDelegate ( parent )
{
}

QTextDocument* ConfigAgentDelegate::document ( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return 0;

    const QString name = index.model()->data ( index, Qt::DisplayRole ).toString();
    int status = index.model()->data ( index, AgentInstanceModel::StatusRole ).toInt();
    uint progress = index.model()->data ( index, AgentInstanceModel::ProgressRole ).toUInt();
    const QString statusMessage = index.model()->data ( index, AgentInstanceModel::StatusMessageRole ).toString();

    QTextDocument *document = new QTextDocument ( 0 );

    const QSize decorationSize( KIconLoader::global()->currentSize( KIconLoader::Desktop ), KIconLoader::global()->currentSize( KIconLoader::Desktop ) );
    const QVariant data = index.model()->data ( index, Qt::DecorationRole );
    if ( data.isValid() && data.type() == QVariant::Icon ) {
        document->addResource ( QTextDocument::ImageResource, QUrl ( QLatin1String ( "agent_icon" ) ),
                                qvariant_cast<QIcon> ( data ).pixmap ( decorationSize ) );
    }

    if ( !index.data ( AgentInstanceModel::OnlineRole ).toBool() )
        document->addResource ( QTextDocument::ImageResource, QUrl ( QLatin1String ( "status_icon" ) ), s_icons->offlinePixmap );
    else if ( status == AgentInstance::Idle )
        document->addResource ( QTextDocument::ImageResource, QUrl ( QLatin1String ( "status_icon" ) ), s_icons->readyPixmap );
    else if ( status == AgentInstance::Running )
        document->addResource ( QTextDocument::ImageResource, QUrl ( QLatin1String ( "status_icon" ) ), s_icons->syncPixmap );
    else
        document->addResource ( QTextDocument::ImageResource, QUrl ( QLatin1String ( "status_icon" ) ), s_icons->errorPixmap );


    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if ( cg == QPalette::Normal && ! ( option.state & QStyle::State_Active ) )
        cg = QPalette::Inactive;

    QColor textColor;
    if ( option.state & QStyle::State_Selected ) {
        textColor = option.palette.color ( cg, QPalette::HighlightedText );
    } else {
        textColor = option.palette.color ( cg, QPalette::Text );
    }

    const QString content = QString::fromLatin1 (
                "<html style=\"color:%1\">"
                "<body>"
                "<table>"
                "<tr>"
                "<td rowspan=\"2\"><img src=\"agent_icon\">&nbsp;&nbsp;</td>"
                "<td><b>%2</b></td>"
                "</tr>" ).arg ( textColor.name().toUpper() ).arg ( name )
            + QString::fromLatin1 (
                "<tr>"
                "<td><img src=\"status_icon\"/> %1 %2</td>"
                "</tr>" ).arg ( statusMessage ).arg ( status == 1 ? QString::fromLatin1( "(%1%)" ).arg ( progress ) : QLatin1String ( "" ) )
            + QLatin1String ( "</table></body></html>" );

    document->setHtml ( content );

    return document;
}

void ConfigAgentDelegate::paint ( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return;

    QTextDocument *doc = document ( option, index );
    if ( !doc )
        return;

    QStyleOptionButton buttonOpt = buttonOption ( option );

    doc->setTextWidth( option.rect.width() - buttonOpt.rect.width() );
    painter->setRenderHint ( QPainter::Antialiasing );

    QPen pen = painter->pen();

    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if ( cg == QPalette::Normal && ! ( option.state & QStyle::State_Active ) )
        cg = QPalette::Inactive;

    QStyleOptionViewItemV4 opt ( option );
    opt.showDecorationSelected = true;
    QApplication::style()->drawPrimitive ( QStyle::PE_PanelItemViewItem, &opt, painter );
    painter->save();
    painter->translate ( option.rect.topLeft() );

    doc->drawContents ( painter );

    QApplication::style()->drawControl ( QStyle::CE_PushButton, &buttonOpt, painter );

    painter->restore();
    painter->setPen ( pen );

    drawFocus ( painter, option, option.rect );
    delete doc;
}

QSize ConfigAgentDelegate::sizeHint ( const QStyleOptionViewItem &option, const QModelIndex & ) const
{
    const int iconHeight = KIconLoader::global()->currentSize(KIconLoader::Desktop) + ( s_delegatePaddingSize*2 );  //icon height + padding either side
    const int textHeight = option.fontMetrics.height() + qMax( option.fontMetrics.height(), 16 ) + ( s_delegatePaddingSize*2 ); //height of text + icon/text + padding either side

    return QSize( 1,qMax( iconHeight, textHeight ) ); //any width,the view will give us the whole thing in list mode
}

QWidget  * ConfigAgentDelegate::createEditor ( QWidget *, const QStyleOptionViewItem  &, const QModelIndex & ) const
{
    return 0;
}

bool ConfigAgentDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED ( model );
    if ( !index.isValid() )
        return false;
    if ( ! ( ( event->type() == QEvent::MouseButtonRelease )
             || ( event->type() == QEvent::MouseButtonPress )
             || ( event->type() == QEvent::MouseMove ) ) )
        return false;


    QMouseEvent *me = static_cast<QMouseEvent*> ( event );
    const QPoint mousePos = me->pos() - option.rect.topLeft();

    QStyleOptionButton buttonOpt = buttonOption ( option );

    if ( buttonOpt.rect.contains ( mousePos ) ) {

        switch ( event->type() ) {
        case QEvent::MouseButtonPress:
            return false;
        case QEvent::MouseButtonRelease: {
            QPoint pos = buttonOpt.rect.bottomLeft() + option.rect.topLeft();
            const QString ident = index.data ( Akonadi::AgentInstanceModel::InstanceIdentifierRole ).toString();
            emit optionsClicked ( ident, pos );
            return true;
        }
        default:
            return false;
        }
    }
    return false;
}

void ConfigAgentDelegate::drawFocus ( QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect ) const
{
    if ( option.state & QStyle::State_HasFocus ) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator= ( option );
        o.rect = rect;
        o.state |= QStyle::State_KeyboardFocusChange;
        QPalette::ColorGroup cg = ( option.state & QStyle::State_Enabled ) ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color ( cg, ( option.state & QStyle::State_Selected )
                                                   ? QPalette::Highlight : QPalette::Background );
        QApplication::style()->drawPrimitive ( QStyle::PE_FrameFocusRect, &o, painter );
    }
}

QStyleOptionButton ConfigAgentDelegate::buttonOption ( const QStyleOptionViewItem& option ) const
{
    const QString label = i18n( "Retrieval Options" );
    QStyleOptionButton buttonOpt;
    QRect buttonRect = option.rect;
    int height = option.rect.height() / 2;
    int width = 22 + option.fontMetrics.width( label ) + 40; // icon size + label size + arrow and padding
    buttonRect.setTop ( 0/*( option.rect.height() /2  ) - height / 2)*/ ); // center the button vertically
    buttonRect.setHeight ( height );
    buttonRect.setLeft ( option.rect.right() - width );
    buttonRect.setWidth ( width );

    buttonOpt.rect = buttonRect;
    buttonOpt.state = option.state;
    buttonOpt.text = label;
    buttonOpt.palette = option.palette;
    buttonOpt.features = QStyleOptionButton::HasMenu;
    buttonOpt.icon = s_icons->checkMailIcon;
    buttonOpt.iconSize = QSize ( 22, 22 ); // FIXME don't hardcode this icon size

    return buttonOpt;
}

