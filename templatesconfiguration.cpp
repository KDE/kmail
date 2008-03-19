/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "templatesconfiguration.h"
#include "ui_templatesconfiguration_base.h"
#include "templatesconfiguration_kfg.h"
#include "globalsettings.h"
#include "replyphrases.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>

#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <QWhatsThis>
#include <qtoolbox.h>
#include <qfont.h>

TemplatesConfiguration::TemplatesConfiguration( QWidget *parent, const char *name )
  : QWidget( parent )
{
  setupUi(this);
  setObjectName(name);

  QFont f = KGlobalSettings::fixedFont();
  textEdit_new->setFont( f );
  textEdit_reply->setFont( f );
  textEdit_reply_all->setFont( f );
  textEdit_forward->setFont( f );

  setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
  sizeHint();

  connect( textEdit_new, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( textEdit_reply, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( textEdit_reply_all, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( textEdit_forward, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( lineEdit_quote, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotTextChanged( void ) ) );

  connect( mInsertCommand, SIGNAL( insertCommand(const QString&, int) ),
           this, SLOT( slotInsertCommand(const QString &, int) ) );

  mHelpString =
    i18n( "<qt>"
	  "<p>Here you can create and manage templates to use when "
	  "composing new messages, replies or forwarded messages.</p>"
	  "<p>The message templates support substitution commands, "
	  "either simply type them or select them from "
	  "the <i>Insert command</i> menu.</p>" );
  if ( QString( name ) == "folder-templates" ) {
    mHelpString +=
      i18n( "<p>Templates specified here are folder-specific. "
            "They override both global templates and per-identity "
            "templates.</p>" );
  } else if ( QString( name ) == "identity-templates" ) {
    mHelpString +=
      i18n( "<p>Templates specified here are identity-specific. "
            "They override global templates, but can be overridden by "
	    "per-folder templates if they are specified.</p>" );
  } else {
    mHelpString +=
      i18n( "<p>These are global (default) templates. They can be overridden "
            "by per-identity templates or per-folder templates "
            "if they are specified.</p>" );
  }

  mHelp->setText( i18n( "<a href=\"whatsthis\">How does this work?</a>" ) );
  connect( mHelp, SIGNAL( linkActivated ( const QString& ) ),
           this, SLOT( slotHelpLinkClicked( const QString& ) ) );
}


void TemplatesConfiguration::slotHelpLinkClicked( const QString& )
{
  QWhatsThis::showText( QCursor::pos(), mHelpString );
}

void TemplatesConfiguration::slotTextChanged()
{
  emit changed();
}

void TemplatesConfiguration::loadFromGlobal()
{
  if ( !GlobalSettings::self()->phrasesConverted() ) {
    kDebug(5006) <<"Phrases to templates conversion";
    importFromPhrases();
  }

  QString str;
  str = GlobalSettings::self()->templateNewMessage();
  if ( str.isEmpty() ) {
    textEdit_new->setText( defaultNewMessage() );
  } else {
    textEdit_new->setText(str);
  }
  str = GlobalSettings::self()->templateReply();
  if ( str.isEmpty() ) {
    textEdit_reply->setText( defaultReply() );
  } else {
    textEdit_reply->setText( str );
  }
  str = GlobalSettings::self()->templateReplyAll();
  if ( str.isEmpty() ) {
    textEdit_reply_all->setText( defaultReplyAll() );
  } else {
    textEdit_reply_all->setText( str );
  }
  str = GlobalSettings::self()->templateForward();
  if ( str.isEmpty() ) {
    textEdit_forward->setText( defaultForward() );
  } else {
    textEdit_forward->setText( str );
  }
  str = GlobalSettings::self()->quoteString();
  if ( str.isEmpty() ) {
    lineEdit_quote->setText( defaultQuoteString() );
  } else {
    lineEdit_quote->setText( str );
  }
}

void TemplatesConfiguration::saveToGlobal()
{
  GlobalSettings::self()->setTemplateNewMessage( strOrBlank( textEdit_new->toPlainText() ) );
  GlobalSettings::self()->setTemplateReply( strOrBlank( textEdit_reply->toPlainText() ) );
  GlobalSettings::self()->setTemplateReplyAll( strOrBlank( textEdit_reply_all->toPlainText() ) );
  GlobalSettings::self()->setTemplateForward( strOrBlank( textEdit_forward->toPlainText() ) );
  GlobalSettings::self()->setQuoteString( lineEdit_quote->text() );
  GlobalSettings::self()->setPhrasesConverted( true );
  GlobalSettings::self()->writeConfig();
}

void TemplatesConfiguration::loadFromIdentity( uint id )
{
  Templates t( QString("IDENTITY_%1").arg( id ) );

  QString str;

  str = t.templateNewMessage();
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateNewMessage();
  }
  if ( str.isEmpty() ) {
    str = defaultNewMessage();
  }
  textEdit_new->setText( str );

  str = t.templateReply();
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateReply();
  }
  if ( str.isEmpty() ) {
    str = defaultReply();
  }
  textEdit_reply->setText( str );

  str = t.templateReplyAll();
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateReplyAll();
  }
  if ( str.isEmpty() ) {
    str = defaultReplyAll();
  }
  textEdit_reply_all->setText( str );

  str = t.templateForward();
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateForward();
  }
  if ( str.isEmpty() ) {
    str = defaultForward();
  }
  textEdit_forward->setText( str );

  str = t.quoteString();
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->quoteString();
  }
  if ( str.isEmpty() ) {
    str = defaultQuoteString();
  }
  lineEdit_quote->setText( str );
}

void TemplatesConfiguration::saveToIdentity( uint id )
{
  Templates t( QString("IDENTITY_%1").arg( id ) );

  t.setTemplateNewMessage( strOrBlank( textEdit_new->toPlainText() ) );
  t.setTemplateReply( strOrBlank( textEdit_reply->toPlainText() ) );
  t.setTemplateReplyAll( strOrBlank( textEdit_reply_all->toPlainText() ) );
  t.setTemplateForward( strOrBlank( textEdit_forward->toPlainText() ) );
  t.setQuoteString( lineEdit_quote->text() );
  t.writeConfig();
}

void TemplatesConfiguration::loadFromFolder( const QString &id, uint identity )
{
  Templates t( id );
  Templates* tid = 0;

  if ( identity ) {
    tid = new Templates( QString("IDENTITY_%1").arg( identity ) );
  }

  QString str;

  str = t.templateNewMessage();
  if ( str.isEmpty() && tid ) {
    str = tid->templateNewMessage();
  }
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateNewMessage();
  }
  if ( str.isEmpty() ) {
    str = defaultNewMessage();
  }
  textEdit_new->setText( str );

  str = t.templateReply();
  if ( str.isEmpty() && tid ) {
    str = tid->templateReply();
  }
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateReply();
  }
  if ( str.isEmpty() ) {
    str = defaultReply();
  }
  textEdit_reply->setText( str );

  str = t.templateReplyAll();
  if ( str.isEmpty() && tid ) {
    str = tid->templateReplyAll();
  }
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateReplyAll();
  }
  if ( str.isEmpty() ) {
    str = defaultReplyAll();
  }
  textEdit_reply_all->setText( str );

  str = t.templateForward();
  if ( str.isEmpty() && tid ) {
    str = tid->templateForward();
  }
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->templateForward();
  }
  if ( str.isEmpty() ) {
    str = defaultForward();
  }
  textEdit_forward->setText( str );

  str = t.quoteString();
  if ( str.isEmpty() && tid ) {
    str = tid->quoteString();
  }
  if ( str.isEmpty() ) {
    str = GlobalSettings::self()->quoteString();
  }
  if ( str.isEmpty() ) {
      str = defaultQuoteString();
  }
  lineEdit_quote->setText( str );

  delete(tid);
}

void TemplatesConfiguration::saveToFolder( const QString &id )
{
  Templates t( id );

  t.setTemplateNewMessage( strOrBlank( textEdit_new->toPlainText() ) );
  t.setTemplateReply( strOrBlank( textEdit_reply->toPlainText() ) );
  t.setTemplateReplyAll( strOrBlank( textEdit_reply_all->toPlainText() ) );
  t.setTemplateForward( strOrBlank( textEdit_forward->toPlainText() ) );
  t.setQuoteString( lineEdit_quote->text() );
  t.writeConfig();
}

void TemplatesConfiguration::loadFromPhrases()
{
  int currentNr = GlobalSettings::self()->replyCurrentLanguage();

  ReplyPhrases replyPhrases( QString::number( currentNr ) );

  textEdit_new->setText( defaultNewMessage() );

  QString str;

  str = replyPhrases.phraseReplySender();
  if ( !str.isEmpty() ) {
    textEdit_reply->setText( convertPhrases( str ) + "\n%QUOTE\n%CURSOR\n" );
  }
  else {
    textEdit_reply->setText( defaultReply() );
  }

  str = replyPhrases.phraseReplyAll();
  if ( !str.isEmpty() ) {
    textEdit_reply_all->setText( convertPhrases( str ) + "\n%QUOTE\n%CURSOR\n" );
  }
  else {
    textEdit_reply_all->setText( defaultReplyAll() );
  }

  str = replyPhrases.phraseForward();
  if ( !str.isEmpty() ) {
    textEdit_forward->setText( i18n(
    "%REM=\"Default forward template\"%-\n"
    "----------  %1  ----------\n"
    "%TEXT\n"
    "-------------------------------------------------------\n",
    convertPhrases( str ) ) );
  }
  else {
    textEdit_forward->setText( defaultForward() );
  }

  str = replyPhrases.indentPrefix();
  if ( !str.isEmpty() ) {
    // no need to convert indentPrefix() because it is passed to KMMessage::asQuotedString() as is
    lineEdit_quote->setText( str );
  }
  else {
    lineEdit_quote->setText( defaultQuoteString() );
  }
}

void TemplatesConfiguration::importFromPhrases()
{
  kDebug(5006);

  int currentNr = GlobalSettings::self()->replyCurrentLanguage();

  ReplyPhrases replyPhrases( QString::number( currentNr ) );

  QString str;

  str = replyPhrases.phraseReplySender();
  if ( !str.isEmpty() ) {
    GlobalSettings::self()->setTemplateReply( convertPhrases( str ) + "\n%QUOTE\n%CURSOR\n" );
  }
  else {
    GlobalSettings::self()->setTemplateReply( defaultReply() );
  }

  str = replyPhrases.phraseReplyAll();
  if ( !str.isEmpty() ) {
    GlobalSettings::self()->setTemplateReplyAll( convertPhrases( str ) + "\n%QUOTE\n%CURSOR\n" );
  }
  else {
    GlobalSettings::self()->setTemplateReplyAll( defaultReplyAll() );
  }

  str = replyPhrases.phraseForward();
  if ( !str.isEmpty() ) {
    GlobalSettings::self()->setTemplateForward( i18n(
    "%REM=\"Default forward template\"%-\n"
    "\n"
    "----------  %1  ----------\n"
    "\n"
    "Subject: %OFULLSUBJECT\n"
    "Date: %ODATE\n"
    "From: %OFROMADDR\n"
    "To: %OTOADDR\n"
    "\n"
    "%TEXT\n"
    "-------------------------------------------------------\n",
    convertPhrases( str ) ) );
  }
  else {
    GlobalSettings::self()->setTemplateForward( defaultForward() );
  }

  str = replyPhrases.indentPrefix();
  if ( !str.isEmpty() ) {
    // no need to convert indentPrefix() because it is passed to KMMessage::asQuotedString() as is
    GlobalSettings::self()->setQuoteString( str );
  }
  else {
    GlobalSettings::self()->setQuoteString( defaultQuoteString() );
  }

  GlobalSettings::self()->setPhrasesConverted( true );
  GlobalSettings::self()->writeConfig();
}

QString TemplatesConfiguration::convertPhrases( const QString &str )
{
  QString result;
  QChar ch;

  unsigned int strLength( str.length() );
  for ( uint i = 0; i < strLength; ) {
    ch = str[i++];
    if ( ch == '%' ) {
      ch = str[i++];
      switch ( ch.toAscii() ) {
      case 'D':
        result += "%ODATE";
        break;
      case 'e':
        result += "%OFROMADDR";
        break;
      case 'F':
        result += "%OFROMNAME";
        break;
      case 'f':
        // is this used for something like FIDO quotes, like "AB>" ?
        // not supported right now
        break;
      case 'T':
        result += "%OTONAME";
        break;
      case 't':
        result += "%OTOADDR";
        break;
      case 'C':
        result += "%OCCNAME";
        break;
      case 'c':
        result += "%OCCADDR";
        break;
      case 'S':
        result += "%OFULLSUBJECT";
        break;
      case '_':
        result += ' ';
        break;
      case 'L':
        result += '\n';
        break;
      case '%':
        result += "%%";
        break;
      default:
        result += '%';
        result += ch;
        break;
      }
    } else
      result += ch;
  }
  return result;
}

void TemplatesConfiguration::slotInsertCommand( const QString &cmd, int adjustCursor )
{
  QTextEdit* edit;

  if( toolBox1->widget( toolBox1->currentIndex() ) == page_new ) {
    edit = textEdit_new;
  } else if( toolBox1->widget( toolBox1->currentIndex() ) == page_reply ) {
    edit = textEdit_reply;
  } else if( toolBox1->widget( toolBox1->currentIndex() ) == page_reply_all ) {
    edit = textEdit_reply_all;
  } else if( toolBox1->widget( toolBox1->currentIndex() ) == page_forward ) {
    edit = textEdit_forward;
  } else {
    kDebug(5006) <<"Unknown current page in TemplatesConfiguration!";
    return;
  }

  // kDebug(5006) <<"Insert command:" << cmd;
  QTextCursor cursor = edit->textCursor();
  cursor.insertText( cmd );
  cursor.setPosition( cursor.position() + adjustCursor );
  edit->setTextCursor( cursor );
  edit->setFocus();
}

QString TemplatesConfiguration::defaultNewMessage() {
  return i18n(
    "%REM=\"Default new message template\"%-\n"
    "%BLANK"
    );
}

QString TemplatesConfiguration::defaultReply() {
  return i18n(
    "%REM=\"Default reply template\"%-\n"
    "On %ODATEEN %OTIMELONGEN you wrote:\n"
    "%QUOTE\n"
    "%CURSOR\n"
    );
}

QString TemplatesConfiguration::defaultReplyAll() {
  return i18n(
    "%REM=\"Default reply all template\"%-\n"
    "On %ODATEEN %OTIMELONGEN %OFROMNAME wrote:\n"
    "%QUOTE\n"
    "%CURSOR\n"
    );
}

QString TemplatesConfiguration::defaultForward() {
  return i18n(
    "%REM=\"Default forward template\"%-\n"
    "\n"
    "----------  Forwarded Message  ----------\n"
    "\n"
    "Subject: %OFULLSUBJECT\n"
    "Date: %ODATE\n"
    "From: %OFROMADDR\n"
    "To: %OTOADDR\n"
    "\n"
    "%TEXT\n"
    "-------------------------------------------------------\n"
    );
}

QString TemplatesConfiguration::defaultQuoteString() {
  return "> ";
}

QString TemplatesConfiguration::strOrBlank( const QString &str ) {
  if ( str.trimmed().isEmpty() ) {
    return QString( "%BLANK" );
  }
  return str;
}

#include "templatesconfiguration.moc"
