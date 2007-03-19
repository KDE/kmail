/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *                            2007 Mathias Soeken, msoeken@tzi.de
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _COLOR_LISTBOX_H_
#define _COLOR_LISTBOX_H_

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QTreeWidget>

class ColorListBox : public QTreeWidget
{
  Q_OBJECT

  public:
    ColorListBox( QWidget *parent=0 );
    void addColor( const QString& text, const QColor& color=Qt::black );
    void setColor( int index, const QColor &color );
    QColor color( int index ) const;
  signals:
    void changed();

  protected:
    void dragEnterEvent( QDragEnterEvent *e );
    void dragLeaveEvent( QDragLeaveEvent *e );
    void dragMoveEvent( QDragMoveEvent *e );
    void dropEvent( QDropEvent *e );

  private slots:
    void newColor( const QModelIndex& index );

  private:
    QTreeWidgetItem* mCurrentOnDragEnter;

};

#endif

