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
#include "mailsourceviewer.h"
#include <QApplication>
#include <QIcon>
#include <kwindowsystem.h>

#include <QRegExp>
#include <QShortcut>
#include <kiconloader.h>

namespace KMail {

void MailSourceHighlighter::highlightBlock ( const QString & text ) {
  // all visible ascii except space and :
  const QRegExp regexp( "^([\\x21-9;-\\x7E]+:\\s)" );
  const int headersState = -1; // Also the initial State
  const int bodyState = 0;

  // keep the previous state
  setCurrentBlockState( previousBlockState() );
  // If a header is found
  if( regexp.indexIn( text ) != -1 )
  {
    // Content- header starts a new mime part, and therefore new headers
    // If a Content-* header is found, change State to headers until a blank line is found.
    if ( text.startsWith( "Content-" ) )
    {
      setCurrentBlockState( headersState );
    }
    // highligth it if in headers state
    if( ( currentBlockState() == headersState ) )
    {
      QFont font = document()->defaultFont ();
      font.setBold( true );
      setFormat( 0, regexp.matchedLength(), font );
    }
  }
  // Change to body state
  else if ( text.isEmpty() )
  {
    setCurrentBlockState( bodyState );
  }
}

MailSourceViewer::MailSourceViewer( QWidget *parent )
  : KTextBrowser( parent ), mSourceHighLighter( 0 )
{
  setAttribute( Qt::WA_DeleteOnClose );
  setLineWrapMode( QTextEdit::NoWrap );
  setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );

  // combining the shortcuts in one qkeysequence() did not work...
  QShortcut* shortcut = new QShortcut( this );
  shortcut->setKey( Qt::Key_Escape );
  connect( shortcut, SIGNAL( activated() ), SLOT( close() ) );

  shortcut = new QShortcut( this );
  shortcut->setKey( Qt::Key_W+Qt::CTRL );
  connect( shortcut, SIGNAL( activated() ), SLOT( close() ) );
  KWindowSystem::setIcons( winId(),
                  qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ),
                  IconSize( KIconLoader::Desktop ) ),
                  qApp->windowIcon().pixmap( IconSize( KIconLoader::Small ),
                  IconSize( KIconLoader::Small ) ) );
  mSourceHighLighter = new MailSourceHighlighter( this );
}

MailSourceViewer::~MailSourceViewer()
{
  delete mSourceHighLighter; mSourceHighLighter = 0;
}

}
