/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 */

#include "configuredialoglistview.h"

#include <KLocalizedString>
#include <QMenu>

ListView::ListView( QWidget *parent )
    : QTreeWidget( parent )
{
    setAllColumnsShowFocus( true );
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setRootIsDecorated( false );
    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotContextMenu(QPoint)) );
}


void ListView::resizeEvent( QResizeEvent *e )
{
    QTreeWidget::resizeEvent(e);
    resizeColums();
}

void ListView::showEvent( QShowEvent *e )
{
    QTreeWidget::showEvent(e);
    resizeColums();
}


void ListView::resizeColums()
{
    const int c = columnCount();
    if( c == 0 ) {
        return;
    }

    const int w1 = viewport()->width();
    const int w2 = w1 / c;
    const int w3 = w1 - (c-1)*w2;

    for ( int i=0; i<c-1; ++i ) {
        setColumnWidth( i, w2 );
    }
    setColumnWidth( c-1, w3 );
}

void ListView::slotContextMenu(const QPoint& pos)
{
    QMenu *menu = new QMenu( this );
    menu->addAction( i18n("Add"), this, SIGNAL(addHeader()));
    if (currentItem()) {
        menu->addAction( i18n("Remove"), this, SIGNAL(removeHeader()));
    }
    menu->exec( viewport()->mapToGlobal( pos ) );
    delete menu;
}

