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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#include "kwindowpositioner.h"

#include <kdebug.h>

#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>

KWindowPositioner::KWindowPositioner( QWidget *master, QWidget *slave,
  Mode mode )
  : QObject( master ), mMaster( master ), mSlave( slave ), mMode( mode )
{
}

void KWindowPositioner::reposition()
{
  QPoint relativePos;
  if ( mMode == Right ) {
    relativePos = QPoint( mMaster->width(), 0 );
  } else if ( mMode == Bottom ) {
    relativePos = QPoint( mMaster->width() - mSlave->frameGeometry().width(),
      mMaster->height() );
  } else {
    kError() <<"KWindowPositioner: Illegal mode";
  }
  QPoint pos = mMaster->mapToGlobal( relativePos );
  
  // fix position to avoid hiding parts of the window (needed especially when not using KWin)
  const QRect desktopRect( qApp->desktop()->availableGeometry( mMaster ) );
  if ( ( pos.x() + mSlave->frameGeometry().width() ) > desktopRect.width() )
    pos.setX( desktopRect.width() - mSlave->frameGeometry().width() );
  if ( ( pos.y() + mSlave->frameGeometry().height() ) > desktopRect.height() )
    pos.setY( desktopRect.height() - mSlave->frameGeometry().height() - mMaster->height() );
  kDebug() << mMaster->pos() << mMaster->mapToGlobal(mMaster->pos()) << pos.y() << (mMaster->pos().y() - pos.y()) << mSlave->frameGeometry().height();
  if ( mMode == Bottom && mMaster->mapToGlobal(mMaster->pos()).y() > pos.y() && (mMaster->pos().y() - pos.y()) < mSlave->frameGeometry().height() ) {
    pos.setY( mMaster->mapToGlobal(  QPoint( 0, -mSlave->frameGeometry().height() ) ).y() );
  }
  if ( pos.x() < desktopRect.left() )
    pos.setX( desktopRect.left() );
  if ( pos.y() < desktopRect.top() )
    pos.setY( desktopRect.top() );

  mSlave->move( pos );
  mSlave->raise();
}

#include "kwindowpositioner.moc"
