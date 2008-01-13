/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "kmedit.h"
#include "kmlineeditspell.h"

#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H
#include "kmmsgdict.h"
#include "kmfolder.h"
#include "kmcommands.h"

#include <maillistdrag.h>
#include <QFocusEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QDropEvent>
#include <QContextMenuEvent>
#include <Q3PopupMenu>
#include <QDragEnterEvent>
#include <QTextStream>
#include <QTextEdit>

#include <kpimutils/kfileio.h>
#include <kpimutils/email.h>
#include <kpimutils/spellingfilter.h>

#include <kcursor.h>
#include <k3process.h>
#include <kconfiggroup.h>

#include <kmenu.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <ktemporaryfile.h>
#include <klocale.h>
#include <kdirwatch.h>
#include <kiconloader.h>

#include "globalsettings.h"
#include "replyphrases.h"

#include <k3spell.h>
#include <k3spelldlg.h>
#include <k3sconfig.h>
#include <k3syntaxhighlighter.h>

#include <QRegExp>
#include <QBuffer>
#include <QEvent>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>



QString KMEdit::brokenText()
{
  QString temp, line;

  int num_lines = numLines();
  for (int i = 0; i < num_lines; ++i)
  {
    int lastLine = 0;
    line = textLine(i);
    for (int j = 0; j < (int)line.length(); ++j)
    {
      if (lineOfChar(i, j) > lastLine)
      {
        lastLine = lineOfChar(i, j);
        temp += '\n';
      }
      temp += line[j];
    }
    if (i + 1 < num_lines) temp += '\n';
  }

  return temp;
}


unsigned int KMEdit::lineBreakColumn() const
{
  unsigned int lineBreakColumn = 0;
  unsigned int numlines = numLines();
  while ( numlines-- ) {
    lineBreakColumn = qMax( lineBreakColumn, (unsigned int)textLine( numlines ).length() );
  }
  return lineBreakColumn;
}


bool KMEdit::eventFilter(QObject*o, QEvent* e)
{
  if (o == this)
    KCursor::autoHideEventFilter(o, e);

  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent *k = (QKeyEvent*)e;

    if (mUseExtEditor) {
      if (k->key() == Qt::Key_Up)
      {
        emit focusUp();
        return true;
      }

      // ignore modifier keys (cf. bug 48841)
      if ( (k->key() == Qt::Key_Shift) || (k->key() == Qt::Key_Control) ||
           (k->key() == Qt::Key_Meta) || (k->key() == Qt::Key_Alt) )
        return true;
      if (mExtEditorTempFile) return true;
      QString sysLine = mExtEditor;
      mExtEditorTempFile = new KTemporaryFile();

      mExtEditorTempFile->open();

      QTextStream stream ( mExtEditorTempFile );
      stream << text();
      stream.flush();

      mExtEditorTempFile->close();
      // replace %f in the system line
      sysLine.replace( "%f", mExtEditorTempFile->fileName() );
      mExtEditorProcess = new K3Process();
      mExtEditorProcess->setUseShell( true );
      sysLine += ' ';
      while (!sysLine.isEmpty())
      {
        *mExtEditorProcess << sysLine.left(sysLine.indexOf(" ")).toLocal8Bit();
        sysLine.remove(0, sysLine.indexOf(" ") + 1);
      }
      connect(mExtEditorProcess, SIGNAL(processExited(K3Process*)),
              SLOT(slotExternalEditorDone(K3Process*)));
      if (!mExtEditorProcess->start())
      {
        KMessageBox::error( topLevelWidget(),
                            i18n("Unable to start external editor.") );
        killExternalEditor();
      } else {
        mExtEditorTempFileWatcher = new KDirWatch( this );
        mExtEditorTempFileWatcher->setObjectName( "mExtEditorTempFileWatcher" );
        connect( mExtEditorTempFileWatcher, SIGNAL(dirty(const QString&)),
                 SLOT(slotExternalEditorTempFileChanged(const QString&)) );
        mExtEditorTempFileWatcher->addFile( mExtEditorTempFile->fileName() );
      }
      return true;
    } else {
    // ---sven's Arrow key navigation start ---
    // Key Up in first line takes you to Subject line.
    if (k->key() == Qt::Key_Up && k->modifiers() != Qt::ShiftModifier && currentLine() == 0
      && lineOfChar(0, currentColumn()) == 0)
    {
      deselect();
      emit focusUp();
      return true;
    }
    // ---sven's Arrow key navigation end ---

    if (k->key() == Qt::Key_Backtab && k->modifiers() == Qt::ShiftModifier)
    {
      deselect();
      emit focusUp();
      return true;
    }

    }
  } else if ( e->type() == QEvent::ContextMenu ) {
    QContextMenuEvent *event = (QContextMenuEvent*) e;

    int para = 1, charPos, firstSpace, lastSpace;

    //Get the character at the position of the click
    charPos = charAt( viewportToContents(event->pos()), &para );
    QString paraText = text( para );

    if( charPos < paraText.length() && !paraText.at(charPos).isSpace() )
    {
      //Get word right clicked on
      const QRegExp wordBoundary( "[\\s\\W]" );
      firstSpace = paraText.lastIndexOf( wordBoundary, charPos ) + 1;
      lastSpace = paraText.indexOf( wordBoundary, charPos );
      if( lastSpace == -1 )
        lastSpace = paraText.length();
      QString word = paraText.mid( firstSpace, lastSpace - firstSpace );
      //Continue if this word was misspelled
      if( !word.isEmpty() && mReplacements.contains( word ) )
      {
        KMenu p;
        p.addTitle( i18n("Suggestions") );

        //Add the suggestions to the popup menu
        QStringList reps = mReplacements[word];
        if ( reps.count() > 0 ) {
          for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
            p.addAction( *it );
          }
        }
        else {
          p.addAction( i18n("No Suggestions") );
        }

        //Execute the popup inline
        const QAction *selectedAction = p.exec( mapToGlobal( event->pos() ) );

        if ( selectedAction && ( reps.count() > 0 ) ) {
          //Save the cursor position
          int parIdx = 1, txtIdx = 1;
          getCursorPosition(&parIdx, &txtIdx);
          setSelection(para, firstSpace, para, lastSpace);
          const QString replacement = selectedAction->text();
          insert( replacement );
          // Restore the cursor position; if the cursor was behind the
          // misspelled word then adjust the cursor position
          if ( para == parIdx && txtIdx >= lastSpace )
            txtIdx += replacement.length() - word.length();
          setCursorPosition(parIdx, txtIdx);
        }
        //Cancel original event
        return true;
      }
    }
  } else if ( e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut ) {
    QFocusEvent *fe = static_cast<QFocusEvent*>(e);
    if ( ! ( fe->reason() == Qt::ActiveWindowFocusReason || fe->reason() == Qt::PopupFocusReason ) )
      emit focusChanged( fe->gotFocus() );
  }

  return KEdit::eventFilter(o, e);
}

void KMEdit::slotExternalEditorTempFileChanged( const QString & fileName ) {
  if ( !mExtEditorTempFile )
    return;
  if ( fileName != mExtEditorTempFile->fileName() )
    return;
  // read data back in from file
  setAutoUpdate(false);
  clear();

  QByteArray ba = KPIMUtils::kFileToByteArray( fileName, true, false );
  insertLine( QString::fromLocal8Bit( ba.data(), ba.size() ), -1 );
  setAutoUpdate(true);
  repaint();
}

void KMEdit::slotExternalEditorDone( K3Process * proc ) {
  assert(proc == mExtEditorProcess);
  // make sure, we update even when KDirWatcher is too slow:
  slotExternalEditorTempFileChanged( mExtEditorTempFile->fileName() );
  killExternalEditor();
}
bool KMEdit::checkExternalEditorFinished() {
  if ( !mExtEditorProcess )
    return true;
  switch ( KMessageBox::warningYesNoCancel( topLevelWidget(),
           i18n("The external editor is still running.\n"
                "Abort the external editor or leave it open?"),
           i18n("External Editor"),
           KGuiItem(i18n("Abort Editor")), KGuiItem(i18n("Leave Editor Open")) ) ) {
  case KMessageBox::Yes:
    killExternalEditor();
    return true;
  case KMessageBox::No:
    return true;
  default:
    return false;
  }
}
