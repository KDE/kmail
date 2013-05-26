/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef KMAIL_STATUSBARLABEL_H
#define KMAIL_STATUSBARLABEL_H

#include <QLabel>

namespace KMail {

/**
  Slightly extended QLabel for embedding into the status bar, providing
  mouse click signals and support for changing the background color.
*/
class StatusBarLabel : public QLabel
{
    Q_OBJECT
public:
    explicit StatusBarLabel( QWidget *parent = 0 );

    void setBackgroundColor( const QColor &color );

signals:
    void clicked();

protected:
    void mouseReleaseEvent( QMouseEvent *event );
};

}

#endif
