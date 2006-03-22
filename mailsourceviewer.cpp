/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *
 *  Copyright (c) 2002-2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2003      Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mailsourceviewer.h"
#include <QIcon>
#include <kapplication.h>
#include <kwin.h>

#include <qregexp.h>
#include <q3accel.h>
#include <kiconloader.h>

namespace KMail {

void MailSourceHighlighter::highlightBlock ( const QString & text ) {
  QRegExp regexp( "^([\\w-]+:\\s)" );
  if( regexp.search( text ) != -1 ) {
    QFont font = document()->defaultFont ();
    font.setBold( true );
    setFormat( 0, regexp.matchedLength(), font );
  }
}

MailSourceViewer::MailSourceViewer( QWidget *parent, const char *name )
  : KTextBrowser( parent, name ), mSourceHighLighter( 0 )
{
  setAttribute( Qt::WA_DeleteOnClose );
  Q3Accel *accel = new Q3Accel( this, "browser close-accel" );
  accel->connectItem( accel->insertItem( Qt::Key_Escape ), this , SLOT( close() ));
  accel->connectItem( accel->insertItem( Qt::Key_W+Qt::CTRL ), this , SLOT( close() ));
#warning "kde4 port setWordWrap\n";
  //setWordWrap( KTextBrowser::NoWrap );
  KWin::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( K3Icon::Desktop ), IconSize( K3Icon::Desktop ) ),
                  qApp->windowIcon().pixmap( IconSize( K3Icon::Small ), IconSize( K3Icon::Small ) ) );
}

MailSourceViewer::~MailSourceViewer()
{
  delete mSourceHighLighter; mSourceHighLighter = 0;
}

void MailSourceViewer::setText( const QString& text )
{
  delete mSourceHighLighter; mSourceHighLighter = 0;
  if ( text.length() > 500000 ) {
    setTextFormat( Qt::LogText );
  } else {
    setTextFormat( Qt::PlainText );
    mSourceHighLighter = new MailSourceHighlighter( this );
  }
  KTextBrowser::setText( text );
}

}
