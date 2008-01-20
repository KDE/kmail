/**
 * kmeditor.cpp
 *
 * Copyright (C)  2007, 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kmcomposereditor.h"
#include "kmcomposewin.h"
#include "kemailquotinghighter.h"
#include "kmcommands.h"
#include "kmmsgdict.h"
#include "kmfolder.h"
#include "maillistdrag.h"

#include <klocale.h>
#include <kmenu.h>

#include <QBuffer>

KMComposerEditor::KMComposerEditor( KMComposeWin *win,QWidget *parent)
 :KMeditor(parent),m_composerWin(win)
{
  switchTextMode( false );
  mHtmlMode = false;
}

KMComposerEditor::~KMComposerEditor()
{
}

void KMComposerEditor::changeHighlighterColors(KPIM::KEMailQuotingHighlighter * highlighter)
{
  QColor defaultColor1( 0x00, 0x80, 0x00 ); // defaults from kmreaderwin.cpp
  QColor defaultColor2( 0x00, 0x70, 0x00 );
  QColor defaultColor3( 0x00, 0x60, 0x00 );
  QColor defaultForeground( qApp->palette().color( QPalette::Text ) );

  //TODO look at background color
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
  highlighter->setQuoteColor(col1, col2, col3, col4);

}

void KMComposerEditor::slotDictionaryChanged( const QString & dict )
{
  if ( highlighter() )
  {
    kDebug()<<" language bezfore: "<<highlighter()->currentLanguage();
    highlighter()->setCurrentLanguage( dict );
    kDebug()<<" language after :"<<highlighter()->currentLanguage();
  }
  setSpellCheckingLanguage(dict);
}

QString KMComposerEditor::smartQuote( const QString & msg )
{
  return m_composerWin->smartQuote(msg);
}

QString KMComposerEditor::quotePrefixName() const
{
   return m_composerWin->quotePrefixName();
}

void KMComposerEditor::dropEvent( QDropEvent *e )
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
    KMCommand *command = new KMForwardAttachedCommand( m_composerWin, messageList,
                                                       identity, m_composerWin );
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
          textCursor().insertText( (*it).url() );
        }
      } else if ( selectedAction == addAsAtmAction ) {
        for ( KUrl::List::Iterator it = urlList.begin();
              it != urlList.end(); ++it ) {
          m_composerWin->addAttach( *it );
        }
      }
    } else if ( md->hasText() ) {
      textCursor().insertText( md->text() );
      e->accept();
    } else {
      kDebug(5006) <<"KMComposerEditor::dropEvent, unable to add dropped object";
      return KMeditor::dropEvent( e );
    }
  }
}

bool KMComposerEditor::canInsertFromMimeData( const QMimeData *source ) const
{
  if ( source->hasFormat( "text/x-kmail-textsnippet" ) )
    return true;
  return KMeditor::canInsertFromMimeData( source );
}

void KMComposerEditor::insertFromMimeData( const QMimeData *source )
{
  if ( source->hasFormat( "text/x-kmail-textsnippet" ) )
    emit insertSnippet();
  KMeditor::insertFromMimeData( source );
}

QString KMComposerEditor::brokenText() const
{
  QString temp;
  QTextDocument* doc = document();
  QTextBlock block = doc->begin();
  while ( block.isValid() ) {
    QTextLayout* layout = block.layout();
    for ( int i = 0; i < layout->lineCount(); i++ ) {
      QTextLine line = layout->lineAt( i );
      temp += block.text().mid( line.textStart(), line.textLength() ) + '\n';
    }
    block = block.next();
  }
  return temp;
}

void KMComposerEditor::setHtmlMode( bool mode )
{
  if ( mode ) {
    if ( ! mHtmlMode ) {
      mHtmlMode = true;

      // set all highlighted text caused by spelling back to black
      //int paraFrom, indexFrom, paraTo, indexTo;
      // set all highlighted text caused by spelling back to black
      // for the case we're in textmode, the user selects some text and decides to format this selected text
      //int startpos = textCursor().selectionStart();
      //int endpos = textCursor().selectionEnd();
      //selectAll();
      //setTextColor(QColor(0,0,0));

      //Laurent fix me
      //mEditor->setSelection ( paraFrom, indexFrom, paraTo, indexTo );
      switchTextMode( true );
    }
  }
  else {
    mHtmlMode = false;
    // like the next 2 lines, or should we selectAll and apply the default font?

    selectAll();
    setTextColor(QColor(0,0,0));
    switchTextMode( false );
  }
  document()->setModified( true );
}

bool KMComposerEditor::htmlMode()
{
  return mHtmlMode;
}

QString KMComposerEditor::text()
{
  if ( mHtmlMode )
    return toHtml();
  else
    return toPlainText();
}

#include "kmcomposereditor.moc"
