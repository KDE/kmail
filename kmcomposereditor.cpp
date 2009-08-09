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
#include "kmcommands.h"
#include "kmmsgdict.h"
#include "kmfolder.h"
#include "maillistdrag.h"

#include <kmime/kmime_codecs.h>
#include <KPIMTextEdit/EMailQuoteHighlighter>

#include <KAction>
#include <KActionCollection>
#include <KFileDialog>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KPushButton>
#include <KInputDialog>

#include <QBuffer>
#include <QClipboard>
#include <QDropEvent>
#include <QFileInfo>

KMComposerEditor::KMComposerEditor( KMComposeWin *win,QWidget *parent)
 :KMeditor(parent),m_composerWin(win)
{
}

KMComposerEditor::~KMComposerEditor()
{
}

void KMComposerEditor::createActions( KActionCollection *actionCollection )
{
  KMeditor::createActions( actionCollection );

  mPasteQuotation = new KAction( i18n("Pa&ste as Quotation"), this );
  actionCollection->addAction("paste_quoted", mPasteQuotation );
  connect( mPasteQuotation, SIGNAL(triggered(bool) ), this, SLOT( slotPasteAsQuotation()) );

  mAddQuoteChars = new KAction( i18n("Add &Quote Characters"), this );
  actionCollection->addAction( "tools_quote", mAddQuoteChars );
  connect( mAddQuoteChars, SIGNAL(triggered(bool) ), this, SLOT(slotAddQuotes()) );

  mRemQuoteChars = new KAction( i18n("Re&move Quote Characters"), this );
  actionCollection->addAction( "tools_unquote", mRemQuoteChars );
  connect (mRemQuoteChars, SIGNAL(triggered(bool) ), this, SLOT(slotRemoveQuotes()) );
}

void KMComposerEditor::setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter * highlighter)
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

const QString KMComposerEditor::defaultQuoteSign() const
{
  if ( !m_quotePrefix.simplified().isEmpty() )
    return m_quotePrefix;
  else
    return KPIMTextEdit::TextEdit::defaultQuoteSign();
}

int KMComposerEditor::quoteLength( const QString& line ) const
{
  if ( !m_quotePrefix.simplified().isEmpty() ) {
    if ( line.startsWith( m_quotePrefix ) )
      return m_quotePrefix.length();
    else
      return 0;
  }
  else
    return KPIMTextEdit::TextEdit::quoteLength( line );
}

void KMComposerEditor::setQuotePrefixName( const QString &quotePrefix )
{
  m_quotePrefix = quotePrefix;
}

QString KMComposerEditor::quotePrefixName() const
{
  if ( !m_quotePrefix.simplified().isEmpty() )
    return m_quotePrefix;
  else
    return ">";
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

bool KMComposerEditor::canInsertFromMimeData( const QMimeData *source ) const
{
  if ( source->hasImage() && source->hasFormat( "image/png" ) )
    return true;
  if ( KPIM::MailList::canDecode( source ) )
    return true;
  if ( source->hasFormat( "text/x-kmail-textsnippet" ) )
    return true;
  if ( source->hasUrls() )
    return true;

  return KPIMTextEdit::TextEdit::canInsertFromMimeData( source );
}

void KMComposerEditor::insertFromMimeData( const QMimeData *source )
{
  // If this is a PNG image, either add it as an attachment or as an inline image
  if ( source->hasImage() && source->hasFormat( "image/png" ) ) {
    if ( textMode() == KRichTextEdit::Rich ) {
      KMenu menu;
      const QAction *addAsInlineImageAction = menu.addAction( i18n("Add as &Inline Image") );
      /*const QAction *addAsAttachmentAction = */menu.addAction( i18n("Add as &Attachment") );
      const QAction *selectedAction = menu.exec( QCursor::pos() );
      if ( selectedAction == addAsInlineImageAction ) {
        // Let the textedit from kdepimlibs handle inline images
        KPIMTextEdit::TextEdit::insertFromMimeData( source );
        return;
      }
      // else fall through
    }

    // Ok, when we reached this point, the user wants to add the image as an attachment.
    // Ask for the filename first.
    bool ok;
    const QString attName =
       KInputDialog::getText( "KMail", i18n( "Name of the attachment:" ), QString(), &ok, this );
    if ( !ok ) {
      return;
    }

    const QByteArray imageData = source->data( "image/png" );
    m_composerWin->addAttachment( attName, "base64", imageData, "image", "png", QByteArray(),
                                  QString(), QByteArray() );
    return;
  }

    // If this is a list of mails, attach those mails as forwards.
  if ( KPIM::MailList::canDecode( source ) ) {

    // Decode the list of serial numbers stored as the drag data
    QByteArray serNums = KPIM::MailList::serialsFromMimeData( source );
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
    return;
  }

  if ( source->hasFormat( "text/x-kmail-textsnippet" ) ) {
    emit insertSnippet();
    return;
  }

  // If this is a URL list, add those files as attachments or text
  const KUrl::List urlList = KUrl::List::fromMimeData( source );
  if ( !urlList.isEmpty() ) {
    KMenu p;
    const QAction *addAsTextAction = p.addAction( i18n("Add as &Text") );
    const QAction *addAsAttachmentAction = p.addAction( i18n("Add as &Attachment") );
    const QAction *selectedAction = p.exec( QCursor::pos() );
    if ( selectedAction == addAsTextAction ) {
      foreach( const KUrl &url, urlList ) {
        textCursor().insertText(url.url());
      }
    } else if ( selectedAction == addAsAttachmentAction ) {
      foreach( const KUrl &url, urlList ) {
        m_composerWin->addAttach( url );
      }
    }
    return;
  }

  KPIMTextEdit::TextEdit::insertFromMimeData( source );
}

#include "kmcomposereditor.moc"
