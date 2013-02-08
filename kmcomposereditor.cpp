/**
 * kmcomposereditor.cpp
 *
 * Copyright (C)  2007, 2008, 2012 Laurent Montel <montel@kde.org>
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
#include "util.h"

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
#include <QtCore/QMimeData>

using namespace MailCommon;

KMComposerEditor::KMComposerEditor( KMComposeWin *win,QWidget *parent)
 : Message::KMeditor(parent, "kmail2rc" ),mComposerWin(win)
{
  setAutocorrection(KMKernel::self()->composerAutoCorrection());
}

KMComposerEditor::~KMComposerEditor()
{
}

void KMComposerEditor::createActions( KActionCollection *actionCollection )
{
  KMeditor::createActions( actionCollection );

  KAction *pasteQuotation = new KAction( i18n("Pa&ste as Quotation"), this );
  actionCollection->addAction("paste_quoted", pasteQuotation );
  pasteQuotation->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_O));
  connect( pasteQuotation, SIGNAL(triggered(bool)), this, SLOT(slotPasteAsQuotation()) );

  KAction *addQuoteChars = new KAction( i18n("Add &Quote Characters"), this );
  actionCollection->addAction( "tools_quote", addQuoteChars );
  connect( addQuoteChars, SIGNAL(triggered(bool)), this, SLOT(slotAddQuotes()) );

  KAction *remQuoteChars = new KAction( i18n("Re&move Quote Characters"), this );
  actionCollection->addAction( "tools_unquote", remQuoteChars );
  connect (remQuoteChars, SIGNAL(triggered(bool)), this, SLOT(slotRemoveQuotes()) );

  KAction *pasteWithoutFormatting = new KAction( i18n("Paste Without Formatting"), this );
  actionCollection->addAction( "paste_without_formatting", pasteWithoutFormatting );
  pasteWithoutFormatting->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_V));
  connect (pasteWithoutFormatting, SIGNAL(triggered(bool)), this, SLOT(slotPasteWithoutFormatting()) );
}

void KMComposerEditor::setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter * highlighter)
{
  QColor color1 = KMail::Util::quoteL1Color();
  QColor color2 = KMail::Util::quoteL2Color();
  QColor color3 = KMail::Util::quoteL3Color();
  QColor misspelled = KMail::Util::misspelledColor();

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
  return mComposerWin->smartQuote( msg );
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
  if ( source->hasFormat( "text/x-kmail-textsnippet" ) ) {
    emit insertSnippet();
    return;
  }
  
  if ( !mComposerWin->insertFromMimeData( source, false ) )
    KPIMTextEdit::TextEdit::insertFromMimeData( source );
}

#include "kmcomposereditor.moc"
