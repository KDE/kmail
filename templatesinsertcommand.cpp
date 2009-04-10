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

#include "templatesinsertcommand.h"

#include <kdebug.h>
#include <kaction.h>
#include <kmenu.h>
#include <kactionmenu.h>
#include <klocale.h>

#include <qsignalmapper.h>
#include <qpushbutton.h>

TemplatesInsertCommand::TemplatesInsertCommand( QWidget *parent, const char *name )
  : QPushButton( parent )
{
  setObjectName( name );
  setText( i18n( "&Insert Command" ) );

  KAction *action;
  KActionMenu *menu;

  QSignalMapper *mapper = new QSignalMapper( this );
  connect( mapper, SIGNAL( mapped(int) ),
           this, SLOT( slotMapped(int) ) );

  mMenu = new KActionMenu( i18n( "Insert Command" ), this );

  // ******************************************************
  menu = new KActionMenu( i18n( "Original Message" ), mMenu );
  mMenu->addAction( menu );

  action = new KAction( i18n("Quoted Message"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CQuote );
  menu->addAction( action );

  action = new KAction( i18n("Message Text as Is"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CText );
  menu->addAction( action );

  action = new KAction( i18n("Message Id"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COMsgId );
  menu->addAction( action );

  action = new KAction( i18n("Date"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CODate );
  menu->addAction( action );

  action = new KAction( i18n("Date in Short Format"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CODateShort );
  menu->addAction( action );

  action = new KAction( i18n("Date in C Locale"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CODateEn );
  menu->addAction( action );

  action = new KAction( i18n("Day of Week"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CODow );
  menu->addAction( action );

  action = new KAction( i18n("Time"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COTime );
  menu->addAction( action );

  action = new KAction( i18n("Time in Long Format"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COTimeLong );
  menu->addAction( action );

  action = new KAction( i18n("Time in C Locale"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COTimeLongEn );
  menu->addAction( action );

  action = new KAction( i18n("To Field Address"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COToAddr );
  menu->addAction( action );

  action = new KAction( i18n("To Field Name"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COToName );
  menu->addAction( action );

  action = new KAction( i18n("To Field First Name"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COToFName );
  menu->addAction( action );

  action = new KAction( i18n("To Field Last Name"), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COToLName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Address" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COCCAddr );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COCCName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field First Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COCCFName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Last Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COCCLName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field Address" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COFromAddr );
  menu->addAction( action );

  action = new KAction( i18n( "From Field Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COFromName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field First Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COFromFName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field Last Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COFromLName );
  menu->addAction( action );

  action = new KAction( i18nc( "Template value for subject of the message", "Subject" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COFullSubject );
  menu->addAction( action );

  action = new KAction( i18n( "Quoted Headers" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CQHeaders );
  menu->addAction( action );

  action = new KAction( i18n( "Headers as Is" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CHeaders );
  menu->addAction( action );

  action = new KAction( i18n( "Header Content" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, COHeader );
  menu->addAction( action );

  // ******************************************************
  menu = new KActionMenu( i18n( "Current Message" ), mMenu );
  mMenu->addAction( menu );

  action = new KAction( i18n( "Message Id" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CMsgId );
  menu->addAction( action );

  action = new KAction( i18n( "Date" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDate );
  menu->addAction( action );

  action = new KAction( i18n( "Date in Short Format" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDateShort );
  menu->addAction( action );

  action = new KAction( i18n( "Date in C Locale" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDateEn );
  menu->addAction( action );

  action = new KAction( i18n( "Day of Week" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDow );
  menu->addAction( action );

  action = new KAction( i18n( "Time" ), menu );
  mapper->setMapping( action, CTime );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  menu->addAction( action );

  action = new KAction( i18n( "Time in Long Format" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CTimeLong );
  menu->addAction( action );

  action = new KAction( i18n( "Time in C Locale" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CTimeLongEn );
  menu->addAction( action );

  action = new KAction( i18n( "To Field Address" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CToAddr );
  menu->addAction( action );

  action = new KAction( i18n( "To Field Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CToName );
  menu->addAction( action );

  action = new KAction( i18n( "To Field First Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CToFName );
  menu->addAction( action );

  action = new KAction( i18n( "To Field Last Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CToLName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Address" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CCCAddr );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CCCName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field First Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CCCFName );
  menu->addAction( action );

  action = new KAction( i18n( "CC Field Last Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CCCLName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field Address" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CFromAddr );
  menu->addAction( action );

  action = new KAction( i18n( "From field Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CFromName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field First Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CFromFName );
  menu->addAction( action );

  action = new KAction( i18n( "From Field Last Name" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CFromLName );
  menu->addAction( action );

  action = new KAction( i18nc( "Template subject command.", "Subject" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CFullSubject );
  menu->addAction( action );

  action = new KAction( i18n( "Header Content" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CHeader );
  menu->addAction( action );

  // ******************************************************
  menu = new KActionMenu( i18n( "Process with External Programs" ), mMenu );
  mMenu->addAction( menu );

  action = new KAction( i18n( "Insert Result of Command" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CSystem );
  menu->addAction( action );

  action = new KAction( i18n( "Pipe Original Message Body and Insert Result as Quoted Text" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CQuotePipe );
  menu->addAction( action );

  action = new KAction( i18n( "Pipe Original Message Body and Insert Result as Is" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CTextPipe );
  menu->addAction( action );

  action = new KAction( i18n( "Pipe Original Message with Headers and Insert Result as Is" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CMsgPipe );
  menu->addAction( action );

  action = new KAction( i18n( "Pipe Current Message Body and Insert Result as Is" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CBodyPipe );
  menu->addAction( action );

  action = new KAction( i18n( "Pipe Current Message Body and Replace with Result" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CClearPipe );
  menu->addAction( action );

  // ******************************************************
  menu = new KActionMenu( i18nc( "Miscellaneous template commands menu", "Miscellaneous" ), mMenu );
  mMenu->addAction( menu );

  action = new KAction( i18n( "Set Cursor Position" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CCursor );
  menu->addAction( action );

  action = new KAction( i18n( "Insert File Content" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CInsert );
  menu->addAction( action );

  action = new KAction( i18nc( "All characters, up to and including the next newline, are discarded without performing any macro expansion", "Discard to Next Line" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDnl );
  menu->addAction( action );

  action = new KAction( i18n( "Template Comment" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CRem );
  menu->addAction( action );

  action = new KAction( i18n( "No Operation" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CNop );
  menu->addAction( action );

  action = new KAction( i18n( "Clear Generated Message" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CClear );
  menu->addAction( action );

  action = new KAction( i18n( "Turn Debug On" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDebug );
  menu->addAction( action );

  action = new KAction( i18n( "Turn Debug Off" ), menu );
  connect(action,SIGNAL(triggered(bool)),mapper,SLOT(map()));
  mapper->setMapping( action, CDebugOff );
  menu->addAction( action );

  setMenu( mMenu->menu() );
}

TemplatesInsertCommand::~TemplatesInsertCommand()
{
}

void TemplatesInsertCommand::slotMapped( int cmd )
{
  emit insertCommand( static_cast<Command>( cmd ) );

  switch( cmd ) {
  case TemplatesInsertCommand::CQuote: emit insertCommand("%QUOTE"); break;
  case TemplatesInsertCommand::CText: emit insertCommand("%TEXT"); break;
  case TemplatesInsertCommand::COMsgId: emit insertCommand("%OMSGID"); break;
  case TemplatesInsertCommand::CODate: emit insertCommand("%ODATE"); break;
  case TemplatesInsertCommand::CODateShort: emit insertCommand("%ODATESHORT"); break;
  case TemplatesInsertCommand::CODateEn: emit insertCommand("%ODATEEN"); break;
  case TemplatesInsertCommand::CODow: emit insertCommand("%ODOW"); break;
  case TemplatesInsertCommand::COTime: emit insertCommand("%OTIME"); break;
  case TemplatesInsertCommand::COTimeLong: emit insertCommand("%OTIMELONG"); break;
  case TemplatesInsertCommand::COTimeLongEn: emit insertCommand("%OTIMELONGEN"); break;
  case TemplatesInsertCommand::COToAddr: emit insertCommand("%OTOADDR"); break;
  case TemplatesInsertCommand::COToName: emit insertCommand("%OTONAME"); break;
  case TemplatesInsertCommand::COToFName: emit insertCommand("%OTOFNAME"); break;
  case TemplatesInsertCommand::COToLName: emit insertCommand("%OTOLNAME"); break;
  case TemplatesInsertCommand::COCCAddr: emit insertCommand("%OCCADDR"); break;
  case TemplatesInsertCommand::COCCName: emit insertCommand("%OCCNAME"); break;
  case TemplatesInsertCommand::COCCFName: emit insertCommand("%OCCFNAME"); break;
  case TemplatesInsertCommand::COCCLName: emit insertCommand("%OCCLNAME"); break;
  case TemplatesInsertCommand::COFromAddr: emit insertCommand("%OFROMADDR"); break;
  case TemplatesInsertCommand::COFromName: emit insertCommand("%OFROMNAME"); break;
  case TemplatesInsertCommand::COFromFName: emit insertCommand("%OFROMFNAME"); break;
  case TemplatesInsertCommand::COFromLName: emit insertCommand("%OFROMLNAME"); break;
  case TemplatesInsertCommand::COFullSubject: emit insertCommand("%OFULLSUBJECT"); break;
  case TemplatesInsertCommand::CQHeaders: emit insertCommand("%QHEADERS"); break;
  case TemplatesInsertCommand::CHeaders: emit insertCommand("%HEADERS"); break;
  case TemplatesInsertCommand::COHeader: emit insertCommand("%OHEADER=\"\"", -1); break;
  case TemplatesInsertCommand::CMsgId: emit insertCommand("%MSGID"); break;
  case TemplatesInsertCommand::CDate: emit insertCommand("%DATE"); break;
  case TemplatesInsertCommand::CDateShort: emit insertCommand("%DATESHORT"); break;
  case TemplatesInsertCommand::CDateEn: emit insertCommand("%DATEEN"); break;
  case TemplatesInsertCommand::CDow: emit insertCommand("%DOW"); break;
  case TemplatesInsertCommand::CTime: emit insertCommand("%TIME"); break;
  case TemplatesInsertCommand::CTimeLong: emit insertCommand("%TIMELONG"); break;
  case TemplatesInsertCommand::CTimeLongEn: emit insertCommand("%TIMELONGEN"); break;
  case TemplatesInsertCommand::CToAddr: emit insertCommand("%TOADDR"); break;
  case TemplatesInsertCommand::CToName: emit insertCommand("%TONAME"); break;
  case TemplatesInsertCommand::CToFName: emit insertCommand("%TOFNAME"); break;
  case TemplatesInsertCommand::CToLName: emit insertCommand("%TOLNAME"); break;
  case TemplatesInsertCommand::CCCAddr: emit insertCommand("%CCADDR"); break;
  case TemplatesInsertCommand::CCCName: emit insertCommand("%CCNAME"); break;
  case TemplatesInsertCommand::CCCFName: emit insertCommand("%CCFNAME"); break;
  case TemplatesInsertCommand::CCCLName: emit insertCommand("%CCLNAME"); break;
  case TemplatesInsertCommand::CFromAddr: emit insertCommand("%FROMADDR"); break;
  case TemplatesInsertCommand::CFromName: emit insertCommand("%FROMNAME"); break;
  case TemplatesInsertCommand::CFromFName: emit insertCommand("%FROMFNAME"); break;
  case TemplatesInsertCommand::CFromLName: emit insertCommand("%FROMLNAME"); break;
  case TemplatesInsertCommand::CFullSubject: emit insertCommand("%FULLSUBJECT"); break;
  case TemplatesInsertCommand::CHeader: emit insertCommand("%HEADER=\"\"", -1); break;
  case TemplatesInsertCommand::CSystem: emit insertCommand("%SYSTEM=\"\"", -1); break;
  case TemplatesInsertCommand::CQuotePipe: emit insertCommand("%QUOTEPIPE=\"\"", -1); break;
  case TemplatesInsertCommand::CTextPipe: emit insertCommand("%TEXTPIPE=\"\"", -1); break;
  case TemplatesInsertCommand::CMsgPipe: emit insertCommand("%MSGPIPE=\"\"", -1); break;
  case TemplatesInsertCommand::CBodyPipe: emit insertCommand("%BODYPIPE=\"\"", -1); break;
  case TemplatesInsertCommand::CClearPipe: emit insertCommand("%CLEARPIPE=\"\"", -1); break;
  case TemplatesInsertCommand::CCursor: emit insertCommand("%CURSOR"); break;
  case TemplatesInsertCommand::CInsert: emit insertCommand("%INSERT=\"\"", -1); break;
  case TemplatesInsertCommand::CDnl: emit insertCommand("%-"); break;
  case TemplatesInsertCommand::CRem: emit insertCommand("%REM=\"\"", -1); break;
  case TemplatesInsertCommand::CNop: emit insertCommand("%NOP"); break;
  case TemplatesInsertCommand::CClear: emit insertCommand("%CLEAR"); break;
  case TemplatesInsertCommand::CDebug: emit insertCommand("%DEBUG"); break;
  case TemplatesInsertCommand::CDebugOff: emit insertCommand("%DEBUGOFF"); break;
  default:
    kDebug(5006) <<"Unknown template command index:" << cmd;
      break;
  }
}

#include "templatesinsertcommand.moc"
