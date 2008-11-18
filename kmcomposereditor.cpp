/**
 * kmcomposereditor.cpp
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
}

KMComposerEditor::~KMComposerEditor()
{
}

void KMComposerEditor::changeHighlighterColors(KPIM::KEMailQuotingHighlighter * highlighter)
{
  // defaults from kmreaderwin.cpp. FIXME: centralize somewhere.
  QColor color1( 0x00, 0x80, 0x00 );
  QColor color2( 0x00, 0x70, 0x00 );
  QColor color3( 0x00, 0x60, 0x00 );
  QColor misspelled = Qt::red;

  if ( !GlobalSettings::self()->useDefaultColors() ) {
    KConfigGroup readerConfig( KMKernel::config(), "Reader" );
    color3 = readerConfig.readEntry( "QuotedText3", color3  );
    color2 = readerConfig.readEntry( "QuotedText2", color2  );
    color1 = readerConfig.readEntry( "QuotedText1", color1  );
    misspelled = readerConfig.readEntry( "MisspelledColor", misspelled );
  }

  highlighter->setQuoteColor( Qt::black /* ignored anyway */,
                              color3, color2, color1, misspelled );
}

QString KMComposerEditor::smartQuote( const QString & msg )
{
  return m_composerWin->smartQuote( msg );
}

QString KMComposerEditor::quotePrefixName() const
{
  if ( !m_quotePrefix.simplified().isEmpty() )
    return m_quotePrefix;
  else
    return ">";
}

void KMComposerEditor::setQuotePrefixName( const QString &quotePrefix )
{
  m_quotePrefix = quotePrefix;
}

void KMComposerEditor::replaceUnknownChars( const QTextCodec *codec )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();
  while ( !cursor.atEnd() ) {
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor );
    QChar cur = cursor.selectedText().at( 0 );
    if ( !codec->canEncode( cur ) ) {
      cursor.insertText( "?" );
    }
    else {
      cursor.clearSelection();
    }
  }
  cursor.endEditBlock();
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
      kDebug(5006) << "Unable to add dropped object";
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

#include "kmcomposereditor.moc"
