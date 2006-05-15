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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#ifndef MAILSOURCEVIEWER_H
#define MAILSOURCEVIEWER_H

#include <ktextbrowser.h>
#include <qsyntaxhighlighter.h>


/**
 * A tiny little class to use for displaying raw messages, textual
 * attachments etc.
 *
 * Auto-deletes itself when closed.
 *
 * @author Carsten Pfeiffer <pfeiffer@kde.org>
 */
namespace KMail {

class MailSourceHighlighter : public QSyntaxHighlighter
{
public:
  MailSourceHighlighter( QTextEdit* edit )
    : QSyntaxHighlighter( edit )
  {}
  int highlightParagraph( const QString& text, int ); 
};


class MailSourceViewer : public KTextBrowser
{
public:
  MailSourceViewer( QWidget *parent = 0, const char *name = 0 );
  ~MailSourceViewer();
  void setText( const QString& text );
private:
  MailSourceHighlighter *mSourceHighLighter;
};

}

#endif // MAILSOURCEVIEWER_H
