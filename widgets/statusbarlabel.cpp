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

#include "statusbarlabel.h"

#include <kdebug.h>


using namespace KMail;

StatusBarLabel::StatusBarLabel(QWidget * parent) :
    QLabel( parent )
{
}

void StatusBarLabel::mouseReleaseEvent(QMouseEvent * event)
{
    emit clicked();
    QLabel::mouseReleaseEvent( event );
}

void StatusBarLabel::setBackgroundColor(const QColor & color)
{
    // changing the palette doesn't work, seems to be overwriten by the
    // statusbar again, stylesheets seems to work though
    setStyleSheet( QString::fromLatin1("background-color: %1;" ).arg( color.name() ) );
}

