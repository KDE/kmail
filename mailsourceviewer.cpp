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
#include <QApplication>
#include <QIcon>
#include <kwm.h>

#include <QRegExp>
#include <QShortcut>
#include <kiconloader.h>

namespace KMail {

void MailSourceHighlighter::highlightBlock ( const QString & text ) {
  QRegExp regexp( "^([\\w-]+:\\s)" );
  if( regexp.indexIn( text ) != -1 ) {
    QFont font = document()->defaultFont ();
    font.setBold( true );
    setFormat( 0, regexp.matchedLength(), font );
  }
}

MailSourceViewer::MailSourceViewer( QWidget *parent, const char *name )
  : KTextBrowser( parent, name ), mSourceHighLighter( 0 )
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
#ifdef Q_OS_UNIX
  KWM::setIcons( winId(),
                  qApp->windowIcon().pixmap( IconSize( K3Icon::Desktop ),
                  IconSize( K3Icon::Desktop ) ),
                  qApp->windowIcon().pixmap( IconSize( K3Icon::Small ),
                  IconSize( K3Icon::Small ) ) );
#endif  
  mSourceHighLighter = new MailSourceHighlighter( this );
}

MailSourceViewer::~MailSourceViewer()
{
  delete mSourceHighLighter; mSourceHighLighter = 0;
}

}
