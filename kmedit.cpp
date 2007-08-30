// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include <config.h>

#include "kmedit.h"
#include "kmlineeditspell.h"

#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H
#include "kmmsgdict.h"
#include "kmfolder.h"
#include "kmcommands.h"

#include <maillistdrag.h>
using KPIM::MailListDrag;

#include <libkdepim/kfileio.h>
#include <libemailfunctions/email.h>

#include <kcursor.h>
#include <kprocess.h>

#include <kpopupmenu.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurldrag.h>

#include <ktempfile.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdirwatch.h>
#include <kiconloader.h>

#include "globalsettings.h"
#include "replyphrases.h"

#include <kspell.h>
#include <kspelldlg.h>
#include <spellingfilter.h>
#include <ksyntaxhighlighter.h>

#include <qregexp.h>
#include <qbuffer.h>
#include <qevent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>


void KMEdit::contentsDragEnterEvent(QDragEnterEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept(true);
    else if (e->provides("image/png"))
        e->accept();
    else
        return KEdit::contentsDragEnterEvent(e);
}

void KMEdit::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept();
    else if (e->provides("image/png"))
        e->accept();
    else
        return KEdit::contentsDragMoveEvent(e);
}

void KMEdit::keyPressEvent( QKeyEvent* e )
{
    if( e->key() == Key_Return ) {
        int line, col;
        getCursorPosition( &line, &col );
        QString lineText = text( line );
        // returns line with additional trailing space (bug in Qt?), cut it off
        lineText.truncate( lineText.length() - 1 );
        // special treatment of quoted lines only if the cursor is neither at
        // the begin nor at the end of the line
        if( ( col > 0 ) && ( col < int( lineText.length() ) ) ) {
            bool isQuotedLine = false;
            uint bot = 0; // bot = begin of text after quote indicators
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
                unsigned int leadingWhiteSpaceCount = 0;
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

void KMEdit::contentsDropEvent(QDropEvent *e)
{
    if (e->provides(MailListDrag::format())) {
        // Decode the list of serial numbers stored as the drag data
        QByteArray serNums;
        MailListDrag::decode( e, serNums );
        QBuffer serNumBuffer(serNums);
        serNumBuffer.open(IO_ReadOnly);
        QDataStream serNumStream(&serNumBuffer);
        Q_UINT32 serNum;
        KMFolder *folder = 0;
        int idx;
        QPtrList<KMMsgBase> messageList;
        while (!serNumStream.atEnd()) {
            KMMsgBase *msgBase = 0;
            serNumStream >> serNum;
            KMMsgDict::instance()->getLocation(serNum, &folder, &idx);
            if (folder)
                msgBase = folder->getMsgBase(idx);
            if (msgBase)
                messageList.append( msgBase );
        }
        serNumBuffer.close();
        uint identity = folder ? folder->identity() : 0;
        KMCommand *command =
            new KMForwardAttachedCommand(mComposer, messageList,
                                         identity, mComposer);
        command->start();
    }
    else if( e->provides("image/png") ) {
        emit attachPNGImageData(e->encodedData("image/png"));
    }
    else if( KURLDrag::canDecode( e ) ) {
        KURL::List urlList;
        if( KURLDrag::decode( e, urlList ) ) {
            KPopupMenu p;
            p.insertItem( i18n("Add as Text"), 0 );
            p.insertItem( i18n("Add as Attachment"), 1 );
            int id = p.exec( mapToGlobal( e->pos() ) );
            switch ( id) {
              case 0:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  insert( (*it).url() );
                }
                break;
              case 1:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  mComposer->addAttach( *it );
                }
                break;
            }
        }
        else if ( QTextDrag::canDecode( e ) ) {
          QString s;
          if ( QTextDrag::decode( e, s ) )
            insert( s );
        }
        else
          kdDebug(5006) << "KMEdit::contentsDropEvent, unable to add dropped object" << endl;
    }
    else {
        return KEdit::contentsDropEvent(e);
    }
}

KMEdit::KMEdit(QWidget *parent, KMComposeWin* composer,
               KSpellConfig* autoSpellConfig,
               const char *name)
  : KEdit( parent, name ),
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
}


void KMEdit::initializeAutoSpellChecking()
{
  if ( mSpellChecker )
    return; // already initialized
  QColor defaultColor1( 0x00, 0x80, 0x00 ); // defaults from kmreaderwin.cpp
  QColor defaultColor2( 0x00, 0x70, 0x00 );
  QColor defaultColor3( 0x00, 0x60, 0x00 );
  QColor defaultForeground( kapp->palette().active().text() );

  QColor c = Qt::red;
  KConfigGroup readerConfig( KMKernel::config(), "Reader" );
  QColor col1;
  if ( !readerConfig.readBoolEntry(  "defaultColors", true ) )
      col1 = readerConfig.readColorEntry( "ForegroundColor", &defaultForeground );
  else
      col1 = defaultForeground;
  QColor col2 = readerConfig.readColorEntry( "QuotedText3", &defaultColor3 );
  QColor col3 = readerConfig.readColorEntry( "QuotedText2", &defaultColor2 );
  QColor col4 = readerConfig.readColorEntry( "QuotedText1", &defaultColor1 );
  QColor misspelled = readerConfig.readColorEntry( "MisspelledColor", &c );
  mSpellChecker = new KDictSpellingHighlighter( this, /*active*/ true,
                                                /*autoEnabled*/ false,
                                                /*spellColor*/ misspelled,
                                                /*colorQuoting*/ true,
                                                col1, col2, col3, col4,
                                                mSpellConfig );

  connect( mSpellChecker, SIGNAL(newSuggestions(const QString&, const QStringList&, unsigned int)),
           this, SLOT(addSuggestion(const QString&, const QStringList&, unsigned int)) );
}


QPopupMenu *KMEdit::createPopupMenu( const QPoint& pos )
{
  enum { IdUndo, IdRedo, IdSep1, IdCut, IdCopy, IdPaste, IdClear, IdSep2, IdSelectAll };

  QPopupMenu *menu = KEdit::createPopupMenu( pos );
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
    lineBreakColumn = QMAX( lineBreakColumn, textLine( numlines ).length() );
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
      if (k->key() == Key_Up)
      {
        emit focusUp();
        return true;
      }

      // ignore modifier keys (cf. bug 48841)
      if ( (k->key() == Key_Shift) || (k->key() == Key_Control) ||
           (k->key() == Key_Meta) || (k->key() == Key_Alt) )
        return true;
      if (mExtEditorTempFile) return true;
      QString sysLine = mExtEditor;
      mExtEditorTempFile = new KTempFile();

      mExtEditorTempFile->setAutoDelete(true);

      (*mExtEditorTempFile->textStream()) << text();

      mExtEditorTempFile->close();
      // replace %f in the system line
      sysLine.replace( "%f", mExtEditorTempFile->name() );
      mExtEditorProcess = new KProcess();
      mExtEditorProcess->setUseShell( true );
      sysLine += " ";
      while (!sysLine.isEmpty())
      {
        *mExtEditorProcess << sysLine.left(sysLine.find(" ")).local8Bit();
        sysLine.remove(0, sysLine.find(" ") + 1);
      }
      connect(mExtEditorProcess, SIGNAL(processExited(KProcess*)),
              SLOT(slotExternalEditorDone(KProcess*)));
      if (!mExtEditorProcess->start())
      {
        KMessageBox::error( topLevelWidget(),
                            i18n("Unable to start external editor.") );
        killExternalEditor();
      } else {
        mExtEditorTempFileWatcher = new KDirWatch( this, "mExtEditorTempFileWatcher" );
        connect( mExtEditorTempFileWatcher, SIGNAL(dirty(const QString&)),
                 SLOT(slotExternalEditorTempFileChanged(const QString&)) );
        mExtEditorTempFileWatcher->addFile( mExtEditorTempFile->name() );
      }
      return true;
    } else {
    // ---sven's Arrow key navigation start ---
    // Key Up in first line takes you to Subject line.
    if (k->key() == Key_Up && k->state() != ShiftButton && currentLine() == 0
      && lineOfChar(0, currentColumn()) == 0)
    {
      deselect();
      emit focusUp();
      return true;
    }
    // ---sven's Arrow key navigation end ---

    if (k->key() == Key_Backtab && k->state() == ShiftButton)
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
      firstSpace = paraText.findRev( wordBoundary, charPos ) + 1;
      lastSpace = paraText.find( wordBoundary, charPos );
      if( lastSpace == -1 )
        lastSpace = paraText.length();
      QString word = paraText.mid( firstSpace, lastSpace - firstSpace );
      //Continue if this word was misspelled
      if( !word.isEmpty() && mReplacements.contains( word ) )
      {
        KPopupMenu p;
        p.insertTitle( i18n("Suggestions") );

        //Add the suggestions to the popup menu
        QStringList reps = mReplacements[word];
        if( reps.count() > 0 )
        {
          int listPos = 0;
          for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
            p.insertItem( *it, listPos );
            listPos++;
          }
        }
        else
        {
          p.insertItem( QString::fromLatin1("No Suggestions"), -2 );
        }

        //Execute the popup inline
        int id = p.exec( mapToGlobal( event->pos() ) );

        if( id > -1 )
        {
          //Save the cursor position
          int parIdx = 1, txtIdx = 1;
          getCursorPosition(&parIdx, &txtIdx);
          setSelection(para, firstSpace, para, lastSpace);
          insert(mReplacements[word][id]);
          // Restore the cursor position; if the cursor was behind the
          // misspelled word then adjust the cursor position
          if ( para == parIdx && txtIdx >= lastSpace )
            txtIdx += mReplacements[word][id].length() - word.length();
          setCursorPosition(parIdx, txtIdx);
        }
        //Cancel original event
        return true;
      }
    }
  } else if ( e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut ) {
    QFocusEvent *fe = static_cast<QFocusEvent*>(e);
    if(! (fe->reason() == QFocusEvent::ActiveWindow || fe->reason() == QFocusEvent::Popup) )
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
  if ( fileName != mExtEditorTempFile->name() )
    return;
  // read data back in from file
  setAutoUpdate(false);
  clear();

  insertLine(QString::fromLocal8Bit(KPIM::kFileToString( fileName, true, false )), -1);
  setAutoUpdate(true);
  repaint();
}

void KMEdit::slotExternalEditorDone( KProcess * proc ) {
  assert(proc == mExtEditorProcess);
  // make sure, we update even when KDirWatcher is too slow:
  slotExternalEditorTempFileChanged( mExtEditorTempFile->name() );
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
           i18n("Abort Editor"), i18n("Leave Editor Open") ) ) {
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
//    kdDebug(5006) << "KMEdit::spellcheck, spellchecking for RichText" << endl;
//    mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
//                    SLOT(slotSpellcheck2(KSpell*)),0,true,false,KSpell::HTML);
//  }
//  else {
    mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
                      SLOT(slotSpellcheck2(KSpell*)));
//  }

  QStringList l = KSpellingHighlighter::personalWords();
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
    mComposer->slotPaste();
}

void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos)
{
    kdDebug(5006)<<"void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos) : "<<text <<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerMisspelling( text, lst, pos);
    else
        misspelling(text, lst, pos);
}

void KMEdit::slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos)
{
    kdDebug(5006)<<"slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos) : "<<oldWord<<endl;
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

void KMEdit::slotSpellcheck2(KSpell*)
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

        kdDebug(5006) << "spelling: new SpellingFilter with prefix=\"" << quotePrefix << "\"" << endl;
        QTextEdit plaintext;
        plaintext.setText(text());
        plaintext.setTextFormat(Qt::PlainText);
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
  KDictSpellingHighlighter::dictionaryChanged();

  emit spellcheck_done( dlgResult );
}

void KMEdit::slotSpellDone()
{
  kdDebug(5006)<<" void KMEdit::slotSpellDone()\n";
  KSpell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;

  kdDebug(5006) << "spelling: delete SpellingFilter" << endl;
  delete mSpellingFilter;
  mSpellingFilter = 0;
  mComposer->sujectLineWidget()->deselect();
  if (status == KSpell::Error)
  {
     KMessageBox::sorry( topLevelWidget(),
                         i18n("ISpell/Aspell could not be started. Please "
                              "make sure you have ISpell or Aspell properly "
                              "configured and in your PATH.") );
     emit spellcheck_done( KS_CANCEL );
  }
  else if (status == KSpell::Crashed)
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
      else if( !mComposer->subjectTextWasSpellChecked() && status == KSpell::FinishedNoMisspellingsEncountered )
          KMessageBox::information( topLevelWidget(),
                                    i18n("No misspellings encountered.") );
  }
}

void KMEdit::setCursorPositionFromStart( unsigned int pos ) {
  unsigned int l = 0;
  unsigned int c = 0;
  posToRowCol( pos, l, c );
  // kdDebug() << "Num lines: " << numLines() << endl;
  // kdDebug() << "Position " << pos << " converted to " << l << ":" << c << endl;
  setCursorPosition( l, c );
  ensureCursorVisible();
}

#include "kmedit.moc"
