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


void KMEdit::contentsDragEnterEvent( QDragEnterEvent *e )
{
  if ( KPIM::MailList::canDecode( e->mimeData() ) ) {
    e->setAccepted( true );
  } else if ( e->mimeData()->hasFormat( "image/png" ) ) {
    e->accept();
  } else {
    return KEdit::contentsDragEnterEvent( e );
  }
}

void KMEdit::contentsDragMoveEvent( QDragMoveEvent *e )
{
  if ( KPIM::MailList::canDecode( e->mimeData() ) ) {
    e->accept();
  } else if  ( e->mimeData()->hasFormat( "image/png" ) ) {
    e->accept();
  } else {
    return KEdit::contentsDragMoveEvent( e );
  }
}

void KMEdit::keyPressEvent( QKeyEvent *e )
{
  if( e->key() == Qt::Key_Return ) {
    int line, col;
    getCursorPosition( &line, &col );
    QString lineText = text( line );
    // returns line with additional trailing space (bug in Qt?), cut it off
    lineText.truncate( lineText.length() - 1 );
    // special treatment of quoted lines only if the cursor is neither at
    // the begin nor at the end of the line
    if( ( col > 0 ) && ( col < int( lineText.length() ) ) ) {
      bool isQuotedLine = false;
      int bot = 0; // bot = begin of text after quote indicators
      while( bot < lineText.length() ) {
        if( ( lineText[bot] == '>' ) || ( lineText[bot] == '|' ) ) {
          isQuotedLine = true;
          ++bot;
        }
        else if( lineText[bot].isSpace() ) {
          ++bot;
        }
        else {
          break;
        }
      }

      KEdit::keyPressEvent( e );

      // duplicate quote indicators of the previous line before the new
      // line if the line actually contained text (apart from the quote
      // indicators) and the cursor is behind the quote indicators
      if( isQuotedLine
          && ( bot != lineText.length() )
          && ( col >= int( bot ) ) ) {

        // The cursor position might have changed unpredictably if there was selected
        // text which got replaced by a new line, so we query it again:
        getCursorPosition( &line, &col );
        QString newLine = text( line );
        // remove leading white space from the new line and instead
        // add the quote indicators of the previous line
        int leadingWhiteSpaceCount = 0;
        while( ( leadingWhiteSpaceCount < newLine.length() )
               && newLine[leadingWhiteSpaceCount].isSpace() ) {
          ++leadingWhiteSpaceCount;
        }
        newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                   lineText.left( bot ) );
        removeParagraph( line );
        insertParagraph( newLine, line );
        // place the cursor at the begin of the new line since
        // we assume that the user split the quoted line in order
        // to add a comment to the first part of the quoted line
        setCursorPosition( line, 0 );
      }
    }
    else
      KEdit::keyPressEvent( e );
  }
  else
    KEdit::keyPressEvent( e );
}

void KMEdit::contentsDropEvent( QDropEvent *e )
{
  const QMimeData *md = e->mimeData();
  if ( KPIM::MailList::canDecode( md ) ) {
    e->accept();
    // Decode the list of serial numbers stored as the drag data
    QByteArray serNums = KPIM::MailList::serialsFromMimeData( md );
    QBuffer serNumBuffer( &serNums );
    serNumBuffer.open( QIODevice::ReadOnly );
    QDataStream serNumStream( &serNumBuffer );
    quint32 serNum;
    KMFolder *folder = 0;
    int idx;
    QList<KMMsgBase*> messageList;
    while ( !serNumStream.atEnd() ) {
      KMMsgBase *msgBase = 0;
      serNumStream >> serNum;
      KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
      if ( folder ) {
        msgBase = folder->getMsgBase( idx );
      }
      if ( msgBase ) {
        messageList.append( msgBase );
      }
    }
    serNumBuffer.close();
    uint identity = folder ? folder->identity() : 0;
    KMCommand *command = new KMForwardAttachedCommand( mComposer, messageList,
                                                       identity, mComposer );
    command->start();
  } else if ( md->hasFormat( "image/png" ) ) {
    emit attachPNGImageData( e->encodedData( "image/png" ) );
  } else {
    KUrl::List urlList = KUrl::List::fromMimeData( md );
    if ( !urlList.isEmpty() ) {
      e->accept();
      KMenu p;
      const QAction *addAsTextAction = p.addAction( i18n("Add as Text") );
      const QAction *addAsAtmAction = p.addAction( i18n("Add as Attachment") );
      const QAction *selectedAction = p.exec( mapToGlobal( e->pos() ) );
      if ( selectedAction == addAsTextAction ) {
        for ( KUrl::List::Iterator it = urlList.begin();
              it != urlList.end(); ++it ) {
          insert( (*it).url() );
        }
      } else if ( selectedAction == addAsAtmAction ) {
        for ( KUrl::List::Iterator it = urlList.begin();
              it != urlList.end(); ++it ) {
          mComposer->addAttach( *it );
        }
      }
    } else if ( md->hasText() ) {
      insert( md->text() );
      e->accept();
    } else {
      kDebug(5006) << "KMEdit::contentsDropEvent, unable to add dropped object" << endl;
      return KEdit::contentsDropEvent( e );
    }
  }
}

KMEdit::KMEdit( QWidget *parent, KMComposeWin *composer,
                K3SpellConfig *autoSpellConfig,
                const char *name )
  : KEdit( parent ),
    mComposer( composer ),
    mKSpell( 0 ),
    mSpellConfig( autoSpellConfig ),
    mSpellingFilter( 0 ),
    mExtEditorTempFile( 0 ),
    mExtEditorTempFileWatcher( 0 ),
    mExtEditorProcess( 0 ),
    mUseExtEditor( false ),
    mWasModifiedBeforeSpellCheck( false ),
    mSpellChecker( 0 ),
    mSpellLineEdit( false )
{
  installEventFilter(this);
  KCursor::setAutoHideCursor( this, true, true );
  setOverwriteEnabled( true );

  QTimer::singleShot( 0, this, SLOT( initializeAutoSpellChecking() ) );
}


void KMEdit::initializeAutoSpellChecking()
{
  if ( mSpellChecker )
    return; // already initialized
  QColor defaultColor1( 0x00, 0x80, 0x00 ); // defaults from kmreaderwin.cpp
  QColor defaultColor2( 0x00, 0x70, 0x00 );
  QColor defaultColor3( 0x00, 0x60, 0x00 );
  QColor defaultForeground( qApp->palette().color( QPalette::Text ) );

  QColor c = Qt::red;
  KConfigGroup readerConfig( KMKernel::config(), "Reader" );
  QColor col1;
  if ( !readerConfig.readEntry(  "defaultColors", true ) )
      col1 = readerConfig.readEntry( "ForegroundColor", defaultForeground );
  else
      col1 = defaultForeground;
  QColor col2 = readerConfig.readEntry( "QuotedText3", defaultColor3  );
  QColor col3 = readerConfig.readEntry( "QuotedText2", defaultColor2  );
  QColor col4 = readerConfig.readEntry( "QuotedText1", defaultColor1  );
  QColor misspelled = readerConfig.readEntry( "MisspelledColor", c  );
  mSpellChecker = new K3DictSpellingHighlighter( this, /*active*/ true,
                                                /*autoEnabled*/ false,
                                                /*spellColor*/ misspelled,
                                                /*colorQuoting*/ true,
                                                col1, col2, col3, col4,
                                                mSpellConfig );

  connect( mSpellChecker, SIGNAL(newSuggestions(const QString&, const QStringList&, unsigned int)),
           this, SLOT(addSuggestion(const QString&, const QStringList&, unsigned int)) );
}


Q3PopupMenu *KMEdit::createPopupMenu( const QPoint& pos )
{
  enum { IdUndo, IdRedo, IdSep1, IdCut, IdCopy, IdPaste, IdClear, IdSep2, IdSelectAll };

  Q3PopupMenu *menu = KEdit::createPopupMenu( pos );
  if ( !QApplication::clipboard()->image().isNull() ) {
    int id = menu->idAt(0);
    menu->setItemEnabled( id - IdPaste, true);
  }

  return menu;
}

void KMEdit::deleteAutoSpellChecking()
{ // because the highlighter doesn't support RichText, delete its instance.
  delete mSpellChecker;
  mSpellChecker =0;
}

void KMEdit::addSuggestion(const QString& text, const QStringList& lst, unsigned int )
{
  mReplacements[text] = lst;
}

void KMEdit::setSpellCheckingActive(bool spellCheckingActive)
{
  if ( mSpellChecker ) {
    mSpellChecker->setActive(spellCheckingActive);
  }
}


KMEdit::~KMEdit()
{
  removeEventFilter(this);

  delete mKSpell;
  delete mSpellChecker;
  mSpellChecker = 0;

}



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

    if( !paraText.at(charPos).isSpace() )
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


int KMEdit::autoSpellChecking( bool on )
{
  if ( textFormat() == Qt::RichText ) {
     // syntax highlighter doesn't support extended text properties
     if ( on )
       KMessageBox::sorry(this, i18n("Automatic spellchecking is not possible on text with markup."));
     return -1;
  }
  if ( mSpellChecker ) {
    // don't autoEnable spell checking if the user turned spell checking off
    mSpellChecker->setAutomatic( on );
    mSpellChecker->setActive( on );
  }
  return 1;
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

void KMEdit::killExternalEditor() {
  delete mExtEditorTempFileWatcher; mExtEditorTempFileWatcher = 0;
  delete mExtEditorTempFile; mExtEditorTempFile = 0;
  delete mExtEditorProcess; mExtEditorProcess = 0;
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

void KMEdit::spellcheck()
{
  if ( mKSpell )
    return;
  mWasModifiedBeforeSpellCheck = isModified();
  mSpellLineEdit = !mSpellLineEdit;
//  maybe for later, for now plaintext is given to KSpell
//  if (textFormat() == Qt::RichText ) {
//    kDebug(5006) << "KMEdit::spellcheck, spellchecking for RichText" << endl;
//    mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
//                    SLOT(slotSpellcheck2(KSpell*)),0,true,false,KSpell::HTML);
//  }
//  else {
    mKSpell = new K3Spell(this, i18n("Spellcheck - KMail"), this,
                      SLOT(slotSpellcheck2(K3Spell*)));
//  }

  QStringList l = K3SpellingHighlighter::personalWords();
  for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
      mKSpell->addPersonal( *it );
  }
  connect (mKSpell, SIGNAL( death()),
          this, SLOT (slotSpellDone()));
  connect (mKSpell, SIGNAL (misspelling (const QString &, const QStringList &, unsigned int)),
          this, SLOT (slotMisspelling (const QString &, const QStringList &, unsigned int)));
  connect (mKSpell, SIGNAL (corrected (const QString &, const QString &, unsigned int)),
          this, SLOT (slotCorrected (const QString &, const QString &, unsigned int)));
  connect (mKSpell, SIGNAL (done(const QString &)),
          this, SLOT (slotSpellResult (const QString&)));
}

void KMEdit::cut()
{
  KEdit::cut();
  if ( textFormat() != Qt::RichText && mSpellChecker )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::clear()
{
  KEdit::clear();
  if ( textFormat() != Qt::RichText && mSpellChecker )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::del()
{
  KEdit::del();
  if ( textFormat() != Qt::RichText && mSpellChecker )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::paste()
{
  if ( ! QApplication::clipboard()->image().isNull() )  {
    emit pasteImage();
  }
  else
    KEdit::paste();
}

void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos)
{
    kDebug(5006)<<"void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos) : "<<text <<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerMisspelling( text, lst, pos);
    else
        misspelling(text, lst, pos);
}

void KMEdit::slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos)
{
    kDebug(5006)<<"slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos) : "<<oldWord<<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerCorrected( oldWord, newWord, pos);
    else {
        unsigned int l = 0;
        unsigned int cnt = 0;
        bool _bold,_underline,_italic;
        QColor _color;
        QFont _font;
        posToRowCol (pos, l, cnt);
        setCursorPosition(l, cnt+1); // the new word will get the same markup now as the first character of the word
        _bold = bold();
        _underline = underline();
        _italic = italic();
        _color = color();
        _font = currentFont();
        corrected(oldWord, newWord, pos);
        setSelection (l, cnt, l, cnt+newWord.length());
        setBold(_bold);
        setItalic(_italic);
        setUnderline(_underline);
        setColor(_color);
        setCurrentFont(_font);
    }

}

void KMEdit::slotSpellcheck2(K3Spell*)
{
    if( !mSpellLineEdit)
    {
        spellcheck_start();

        QString quotePrefix;
        if(mComposer && mComposer->msg())
        {
            int languageNr = GlobalSettings::self()->replyCurrentLanguage();
            ReplyPhrases replyPhrases( QString::number(languageNr) );
            replyPhrases.readConfig();

            quotePrefix = mComposer->msg()->formatString(
                 replyPhrases.indentPrefix() );
        }

        kDebug(5006) << "spelling: new SpellingFilter with prefix=\"" << quotePrefix << "\"" << endl;
        QTextEdit plaintext;
        plaintext.setText(text());
        plaintext.setAcceptRichText(false);
        mSpellingFilter = new SpellingFilter(plaintext.text(), quotePrefix, SpellingFilter::FilterUrls,
                                             SpellingFilter::FilterEmailAddresses);

        mKSpell->check(mSpellingFilter->filteredText());
    }
    else if( mComposer )
        mKSpell->check( mComposer->sujectLineWidget()->text());
}

void KMEdit::slotSpellResult(const QString &s)
{
    if( !mSpellLineEdit)
        spellcheck_stop();

  int dlgResult = mKSpell->dlgResult();
  if ( dlgResult == KS_CANCEL )
  {
      if( mSpellLineEdit)
      {
          //stop spell check
          mSpellLineEdit = false;
          QString tmpText( s );
          tmpText =  tmpText.remove('\n');

          if( tmpText != mComposer->sujectLineWidget()->text() )
              mComposer->sujectLineWidget()->setText( tmpText );
      }
      else
      {
          setModified(true);
      }
  }
  mKSpell->cleanUp();
  K3DictSpellingHighlighter::dictionaryChanged();

  emit spellcheck_done( dlgResult );
}

void KMEdit::slotSpellDone()
{
  kDebug(5006)<<" void KMEdit::slotSpellDone()\n";
  K3Spell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;

  kDebug(5006) << "spelling: delete SpellingFilter" << endl;
  delete mSpellingFilter;
  mSpellingFilter = 0;
  mComposer->sujectLineWidget()->deselect();
  if (status == K3Spell::Error)
  {
     KMessageBox::sorry( topLevelWidget(),
                         i18n("ISpell/Aspell could not be started. Please "
                              "make sure you have ISpell or Aspell properly "
                              "configured and in your PATH.") );
     emit spellcheck_done( KS_CANCEL );
  }
  else if (status == K3Spell::Crashed)
  {
     spellcheck_stop();
     KMessageBox::sorry( topLevelWidget(),
                         i18n("ISpell/Aspell seems to have crashed.") );
     emit spellcheck_done( KS_CANCEL );
  }
  else
  {
      if( mSpellLineEdit )
          spellcheck();
      else if( !mComposer->subjectTextWasSpellChecked() && status == K3Spell::FinishedNoMisspellingsEncountered )
          KMessageBox::information( topLevelWidget(),
                                    i18n("No misspellings encountered.") );
  }
}

#include "kmedit.moc"
