/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
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

#include <klistbox.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class ColorListBox : public KListBox
{
  Q_OBJECT

  public:
    ColorListBox( QWidget *parent=0, const char * name=0, Qt::WFlags f=0 );
    void setColor( uint index, const QColor &color );
    QColor color( uint index ) const;
signals:
    void changed();

  public slots:
    virtual void setEnabled( bool state );

  protected:
    void dragEnterEvent( QDragEnterEvent *e );
    void dragLeaveEvent( QDragLeaveEvent *e );
    void dragMoveEvent( QDragMoveEvent *e );
    void dropEvent( QDropEvent *e );

  private slots:
    void newColor( int index );

  private:
    int mCurrentOnDragEnter;

};


class ColorListItem : public Q3ListBoxItem
{
  public:
    ColorListItem( const QString &text, const QColor &color=Qt::black );
    const QColor &color( void );
    void  setColor( const QColor &color );

  protected:
    virtual void paint( QPainter * );
    virtual int height( const Q3ListBox * ) const;
    virtual int width( const Q3ListBox * ) const;

  private:
    QColor mColor;
    int mBoxWidth;
};

#endif

