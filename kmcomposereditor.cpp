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

#include <kabc/addressee.h>
#include <kmime/kmime_codecs.h>
#include <akonadi/itemfetchjob.h>

#include <kio/jobuidelegate.h>

#include "messagecore/globalsettings.h"

#include <KPIMTextEdit/EMailQuoteHighlighter>

#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KMenu>
#include <KPushButton>
#include <KInputDialog>

#include "kmkernel.h"
#include "foldercollection.h"
#include <QTextCodec>
#include <QFileInfo>
#include <QtCore/QMimeData>

using namespace MailCommon;

KMComposerEditor::KMComposerEditor( KMComposeWin *win,QWidget *parent)
 : Message::KMeditor(parent, "kmail2rc" ),m_composerWin(win)
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
  connect( mPasteQuotation, SIGNAL(triggered(bool)), this, SLOT(slotPasteAsQuotation()) );

  mAddQuoteChars = new KAction( i18n("Add &Quote Characters"), this );
  actionCollection->addAction( "tools_quote", mAddQuoteChars );
  connect( mAddQuoteChars, SIGNAL(triggered(bool)), this, SLOT(slotAddQuotes()) );

  mRemQuoteChars = new KAction( i18n("Re&move Quote Characters"), this );
  actionCollection->addAction( "tools_unquote", mRemQuoteChars );
  connect (mRemQuoteChars, SIGNAL(triggered(bool)), this, SLOT(slotRemoveQuotes()) );
}

void KMComposerEditor::setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter * highlighter)
{
  // defaults from kmreaderwin.cpp. FIXME: centralize somewhere.
  QColor color1( 0x00, 0x80, 0x00 );
  QColor color2( 0x00, 0x70, 0x00 );
  QColor color3( 0x00, 0x60, 0x00 );
  QColor misspelled = Qt::red;

  if ( !MessageCore::GlobalSettings::self()->useDefaultColors() ) {
    color1 = MessageCore::GlobalSettings::self()->quotedText1();
    color2 = MessageCore::GlobalSettings::self()->quotedText2();
    color3 = MessageCore::GlobalSettings::self()->quotedText3();
    misspelled = MessageCore::GlobalSettings::self()->misspelledColor();
  }

  highlighter->setQuoteColor( Qt::black /* ignored anyway */,
                              color1, color2, color3, misspelled );
}

QString KMComposerEditor::smartQuote( const QString & msg )
{
  return m_composerWin->smartQuote( msg );
}

void KMComposerEditor::replaceUnknownChars( const QTextCodec *codec )
{
  QTextCursor cursor( document() );
  cursor.beginEditBlock();
  while ( !cursor.atEnd() ) {
    cursor.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor );
    const QChar cur = cursor.selectedText().at( 0 );
    if ( codec->canEncode( cur ) ) {
       cursor.clearSelection();
    } else {
       cursor.insertText( "?" );
    }
  }
  cursor.endEditBlock();
}

bool KMComposerEditor::canInsertFromMimeData( const QMimeData *source ) const
{
  if ( source->hasImage() && source->hasFormat( "image/png" ) )
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
    // Get the image data before showing the dialog, since that processes events which can delete
    // the QMimeData object behind our back
    const QByteArray imageData = source->data( "image/png" );
 	  
    if ( textMode() == KRichTextEdit::Rich && isEnableImageActions() ) {
      QImage image = qvariant_cast<QImage>( source->imageData() );
      QFileInfo fi( source->text() );

      KMenu menu;
      const QAction *addAsInlineImageAction = menu.addAction( i18n("Add as &Inline Image") );
      /*const QAction *addAsAttachmentAction = */menu.addAction( i18n("Add as &Attachment") );
      const QAction *selectedAction = menu.exec( QCursor::pos() );
      if ( selectedAction == addAsInlineImageAction ) {
        // Let the textedit from kdepimlibs handle inline images
        insertImage( image, fi );
        return;
      } else if( !selectedAction ) {
        return;
      } 
      // else fall through
    }
    m_composerWin->addImageAsAttachement(imageData);
    return;
  }


  if ( source->hasFormat( "text/x-kmail-textsnippet" ) ) {
    emit insertSnippet();
    return;
  }

  // If this is a URL list, add those files as attachments or text
  const KUrl::List urlList = KUrl::List::fromMimeData( source );
  if ( !urlList.isEmpty() ) {
    //Search if it's message items.
    Akonadi::Item::List items;
    bool allLocalURLs = true;

    foreach ( const KUrl &url, urlList ) {
      if ( !url.isLocalFile() ) {
        allLocalURLs = false;
      }
      const Akonadi::Item item = Akonadi::Item::fromUrl( url );
      if ( item.isValid() ) {
        items << item;
      }
    }

    if ( items.isEmpty() ) {
      if ( allLocalURLs ) {
        foreach( const KUrl &url, urlList ) {
          m_composerWin->addAttachment( url, "" );
        }
      } else {
        KMenu p;
        const QAction *addAsTextAction = p.addAction( i18np("Add URL into Message &Text", "Add URLs into Message &Text", urlList.size() ) );
        const QAction *addAsAttachmentAction = p.addAction( i18np("Add File as &Attachment", "Add Files as &Attachment", urlList.size() ) );
        const QAction *selectedAction = p.exec( QCursor::pos() );

        if ( selectedAction == addAsTextAction ) {
          foreach( const KUrl &url, urlList ) {
            textCursor().insertText(url.url() + '\n');
          }
        } else if ( selectedAction == addAsAttachmentAction ) {
          foreach( const KUrl &url, urlList ) {
            m_composerWin->addAttachment( url, "" );
          }
        }
      }
      return;
    } else {
      Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( items, this );
      itemFetchJob->fetchScope().fetchFullPayload( true );
      itemFetchJob->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
      connect( itemFetchJob, SIGNAL(result(KJob*)), m_composerWin, SLOT(slotFetchJob(KJob*)) );
      return;
    }
  }

  KPIMTextEdit::TextEdit::insertFromMimeData( source );
}

#include "kmcomposereditor.moc"
