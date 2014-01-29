/*
 *   This file is part of kmail
 *
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2007 Mathias Soeken, msoeken@tzi.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "colorlistbox.h"

#include <KColorDialog>
#include <KColorMimeData>

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>

ColorListBox::ColorListBox( QWidget *parent )
    : QTreeWidget( parent ), mCurrentOnDragEnter( 0L )
{
    setColumnCount( 1 );
    setRootIsDecorated( false );
    header()->hide();

    connect( this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(newColor(QModelIndex)) );
    setAcceptDrops( true );
}

void ColorListBox::addColor( const QString& text, const QColor& color )
{
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << text );
    item->setData( 0, Qt::DecorationRole, color );
    addTopLevelItem( item );
}

void ColorListBox::setColorSilently( int index, const QColor &color )
{
    if (index < model()->rowCount()) {
        topLevelItem( index )->setData( 0, Qt::DecorationRole, color );
    }
}

void ColorListBox::setColor( int index, const QColor &color )
{
    if (index < model()->rowCount()) {
        topLevelItem( index )->setData( 0, Qt::DecorationRole, color );
        emit changed();
    }
}

QColor ColorListBox::color( int index ) const
{
    if (index < model()->rowCount()) {
        return topLevelItem( index )->data( 0, Qt::DecorationRole ).value<QColor>();
    } else {
        return Qt::black;
    }
}

void ColorListBox::newColor( const QModelIndex& index )
{
    if ( !isEnabled() ) {
        return;
    }

    if (index.isValid()) {
        QColor c = color( index.row() );
        if (KColorDialog::getColor(c, this) != QDialog::Rejected) {
            setColor( index.row(), c );
        }
    }
}

void ColorListBox::dragEnterEvent( QDragEnterEvent *e )
{
    if (KColorMimeData::canDecode( e->mimeData() ) && isEnabled()) {
        mCurrentOnDragEnter = currentItem();
        e->setAccepted( true );
    } else {
        mCurrentOnDragEnter = 0L;
        e->setAccepted( false );
    }
}


void ColorListBox::dragLeaveEvent( QDragLeaveEvent * )
{
    if (mCurrentOnDragEnter) {
        setCurrentItem( mCurrentOnDragEnter );
        mCurrentOnDragEnter = 0L;
    }
}


void ColorListBox::dragMoveEvent( QDragMoveEvent *e )
{
    if (KColorMimeData::canDecode( e->mimeData() ) && isEnabled()) {
        QTreeWidgetItem *item = itemAt( e->pos() );
        if ( item ) {
            setCurrentItem( item );
        }
    }
}


void ColorListBox::dropEvent( QDropEvent *e )
{
    QColor color = KColorMimeData::fromMimeData( e->mimeData() );
    if (color.isValid()) {
        QTreeWidgetItem *item = currentItem();
        if ( item ) {
            item->setData( 0, Qt::DecorationRole, color );
            emit changed();
        }
        mCurrentOnDragEnter = 0L;
    }
}


#include "moc_colorlistbox.cpp"
