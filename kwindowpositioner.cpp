/*
    This file is part of KDE.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
    
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "kwindowpositioner.h"

#include <kdebug.h>

#include <qwidget.h>

KWindowPositioner::KWindowPositioner( QWidget *master, QWidget *slave,
  Mode mode )
  : QObject( master ), mMaster( master ), mSlave( slave ), mMode( mode )
{
  master->topLevelWidget()->installEventFilter( this );
}

bool KWindowPositioner::eventFilter( QObject *, QEvent *e )
{
  if ( e->type() == QEvent::Move ) {
    reposition();
  }

  return false;
}

void KWindowPositioner::reposition()
{
  QPoint relativePos;
  if ( mMode == Right ) {
    relativePos = QPoint( mMaster->width(), -100 );
  } else if ( mMode == Bottom ) {
    relativePos = QPoint( 100 - mSlave->width() + mMaster->width(),
      mMaster->height() );
  } else {
    kdError() << "KWindowPositioner: Illegal mode" << endl;
  }
  QPoint pos = mMaster->mapToGlobal( relativePos );
  mSlave->move( pos );
  mSlave->raise();
}

#include "kwindowpositioner.moc"
