/*
 * Copyright (c) 2011 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#include "kmsubjectlineedit.h"

#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QStyle>
#include <QApplication>
#include <QMenu>
#include <QStyleOptionFrameV2>

#include <KLocale>

KMSubjectLineEdit::KMSubjectLineEdit(QWidget* parent)
  :KTextEdit( parent )
{
  enableFindReplace(false);
  setAcceptRichText(false);
  // widget may not be resized vertically
  setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
  setLineWrapMode(NoWrap);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setCheckSpellingEnabledInternal( true );
  document()->adjustSize();
  
  document()->setDocumentMargin(2);
}

KMSubjectLineEdit::~KMSubjectLineEdit()
{
}

void KMSubjectLineEdit::createHighlighter()
{
  Sonnet::Highlighter *highlighter = new Sonnet::Highlighter(this, "kmail2rc");
  highlighter->setAutomatic( false );
  
  KTextEdit::setHighlighter(highlighter);
}

void KMSubjectLineEdit::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Enter ||
      e->key() == Qt::Key_Return ||
      e->key() == Qt::Key_Down)
  {
    emit focusDown();
    return;
  }
  else if (e->key() == Qt::Key_Up)
  {
    emit focusUp();
    return;
  }
  KTextEdit::keyPressEvent(e);
}

QSize KMSubjectLineEdit::sizeHint() const
{
    QFontMetrics fm(font());

    int h = document()->size().toSize().height() - fm.descent() + 2 * frameWidth();

    QStyleOptionFrameV2 opt;
    opt.initFrom(this);
    opt.rect = QRect(0, 0, 100, h);
    opt.lineWidth = lineWidth();
    opt.midLineWidth = 0;
    opt.state |= QStyle::State_Sunken;

    QSize s = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(100, h).expandedTo(QApplication::globalStrut()), this);

    return s;
}

QSize KMSubjectLineEdit::minimumSizeHint() const
{
   return sizeHint();
}

void KMSubjectLineEdit::insertFromMimeData(const QMimeData * source)
{
    if(!source)
        return;

    setFocus();

    // Copy text from the clipboard (paste)
    QString pasteText = source->text();

    // is there any text in the clipboard?
    if(!pasteText.isEmpty())
    {
        // replace \r with \n to make xterm pastes happy
        pasteText.replace('\r','\n');
        // remove blank lines
        while(pasteText.contains("\n\n"))
            pasteText.replace("\n\n","\n");

        QRegExp reTopSpace("^ *\n");
        while(pasteText.contains(reTopSpace))
            pasteText.remove(reTopSpace);

        QRegExp reBottomSpace("\n *$");
        while(pasteText.contains(reBottomSpace))
            pasteText.remove(reBottomSpace);

        // does the text contain at least one newline character?
        if(pasteText.contains('\n'))
          pasteText.remove('\n');
        
        insertPlainText(pasteText);
        ensureCursorVisible();
        return;
    }
    else {
      KTextEdit::insertFromMimeData(source);
    }
}
#include "kmsubjectlineedit.moc"
