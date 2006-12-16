/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#include <qpushbutton.h>
#include <kaction.h>
#include <kactionclasses.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <qsignalmapper.h>
#include <kdebug.h>

#include "templatesinsertcommand.h"

TemplatesInsertCommand::TemplatesInsertCommand( QWidget *parent,
                                                const char *name )
  : QPushButton( parent, name )
{
  setText( i18n( "&Insert command..." ) );
  connect( this, SIGNAL( clicked() ),
           this, SLOT( slotClicked() ) );

  KAction *action;
  KActionMenu *menu;

  QSignalMapper *mapper = new QSignalMapper( this );
  connect( mapper, SIGNAL( mapped(int) ),
           this, SLOT( slotMapped(int) ) );

  mMenu = new KActionMenu( i18n( "Insert command" ), this );

  // ******************************************************
  menu = new KActionMenu( i18n( "Original message" ), mMenu );
  mMenu->insert( menu );

  action = new KAction( i18n("Quoted message"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CQuote );
  menu->insert( action );
  action = new KAction( i18n("Message text as is"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CText );
  menu->insert( action );
  action = new KAction( i18n("Message id"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COMsgId );
  menu->insert( action );
  action = new KAction( i18n("Date"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CODate );
  menu->insert( action );
  action = new KAction( i18n("Date in short format"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CODateShort );
  menu->insert( action );
  action = new KAction( i18n("Date in C locale"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CODateEn );
  menu->insert( action );
  action = new KAction( i18n("Day of week"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CODow );
  menu->insert( action );
  action = new KAction( i18n("Time"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COTime );
  menu->insert( action );
  action = new KAction( i18n("Time in long format"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COTimeLong );
  menu->insert( action );
  action = new KAction( i18n("Time in C locale"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COTimeLongEn );
  menu->insert( action );
  action = new KAction( i18n("To field address"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COToAddr );
  menu->insert( action );
  action = new KAction( i18n("To field name"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COToName );
  menu->insert( action );
  action = new KAction( i18n("To field first name"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COToFName );
  menu->insert( action );
  action = new KAction( i18n("To field last name"),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COToLName );
  menu->insert( action );
  action = new KAction( i18n( "CC field address" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COCCAddr );
  menu->insert( action );
  action = new KAction( i18n( "CC field name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COCCName );
  menu->insert( action );
  action = new KAction( i18n( "CC field first name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COCCFName );
  menu->insert( action );
  action = new KAction( i18n( "CC field last name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COCCLName );
  menu->insert( action );
  action = new KAction( i18n( "From field address" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COFromAddr );
  menu->insert( action );
  action = new KAction( i18n( "From field name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COFromName );
  menu->insert( action );
  action = new KAction( i18n( "From field first name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COFromFName );
  menu->insert( action );
  action = new KAction( i18n( "From field last name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COFromLName );
  menu->insert( action );
  action = new KAction( i18n( "Subject" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COFullSubject );
  menu->insert( action );
  action = new KAction( i18n( "Quoted headers" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CQHeaders );
  menu->insert( action );
  action = new KAction( i18n( "Headers as is" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CHeaders );
  menu->insert( action );
  action = new KAction( i18n( "Header content" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, COHeader );
  menu->insert( action );

  // ******************************************************
  menu = new KActionMenu( i18n( "Current message" ), mMenu );
  mMenu->insert( menu );

  action = new KAction( i18n( "Message id" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CMsgId );
  menu->insert( action );
  action = new KAction( i18n( "Date" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDate );
  menu->insert( action );
  action = new KAction( i18n( "Date in short format" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDateShort );
  menu->insert( action );
  action = new KAction( i18n( "Date in C locale" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDateEn );
  menu->insert( action );
  action = new KAction( i18n( "Day of week" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDow );
  menu->insert( action );
  action = new KAction( i18n( "Time" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CTime );
  menu->insert( action );
  action = new KAction( i18n( "Time in long format" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CTimeLong );
  menu->insert( action );
  action = new KAction( i18n( "Time in C locale" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CTimeLongEn );
  menu->insert( action );
  action = new KAction( i18n( "To field address" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CToAddr );
  menu->insert( action );
  action = new KAction( i18n( "To field name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CToName );
  menu->insert( action );
  action = new KAction( i18n( "To field first name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CToFName );
  menu->insert( action );
  action = new KAction( i18n( "To field last name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CToLName );
  menu->insert( action );
  action = new KAction( i18n( "CC field address" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CCCAddr );
  menu->insert( action );
  action = new KAction( i18n( "CC field name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CCCName );
  menu->insert( action );
  action = new KAction( i18n( "CC field first name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CCCFName );
  menu->insert( action );
  action = new KAction( i18n( "CC field last name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CCCLName );
  menu->insert( action );
  action = new KAction( i18n( "From field address" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CFromAddr );
  menu->insert( action );
  action = new KAction( i18n( "From field name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CFromName );
  menu->insert( action );
  action = new KAction( i18n( "From field first name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CFromFName );
  menu->insert( action );
  action = new KAction( i18n( "From field last name" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CFromLName );
  menu->insert( action );
  action = new KAction( i18n( "Subject" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CFullSubject );
  menu->insert( action );
  action = new KAction( i18n( "Header content" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CHeader );
  menu->insert( action );

  // ******************************************************
  menu = new KActionMenu( i18n( "Process with external programs" ), mMenu );
  mMenu->insert( menu );

  action = new KAction( i18n( "Insert result of command" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CSystem );
  menu->insert( action );
  action = new KAction( i18n( "Pipe original message body and insert result as quoted text" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CQuotePipe );
  menu->insert( action );
  action = new KAction( i18n( "Pipe original message body and insert result as is" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CTextPipe );
  menu->insert( action );
  action = new KAction( i18n( "Pipe original message with headers and insert result as is" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CMsgPipe );
  menu->insert( action );
  action = new KAction( i18n( "Pipe current message body and insert result as is" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CBodyPipe );
  menu->insert( action );
  action = new KAction( i18n( "Pipe current message body and replace with result" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CClearPipe );
  menu->insert( action );

  // ******************************************************
  menu = new KActionMenu( i18n( "Miscellaneous" ), mMenu );
  mMenu->insert( menu );

  action = new KAction( i18n( "Set cursor position" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CCursor );
  menu->insert( action );
  action = new KAction( i18n( "Insert file content" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CInsert );
  menu->insert( action );
  action = new KAction( i18n( "DNL" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDnl );
  menu->insert( action );
  action = new KAction( i18n( "Template comment" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CRem );
  menu->insert( action );
  action = new KAction( i18n( "No operation" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CNop );
  menu->insert( action );
  action = new KAction( i18n( "Clear generated message" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CClear );
  menu->insert( action );
  action = new KAction( i18n( "Turn debug on" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDebug );
  menu->insert( action );
  action = new KAction( i18n( "Turn debug off" ),
                        0, mapper, SLOT( map() ), menu );
  mapper->setMapping( action, CDebugOff );
  menu->insert( action );
}

TemplatesInsertCommand::~TemplatesInsertCommand()
{
}

void TemplatesInsertCommand::slotClicked()
{
  QSize ps = mMenu->popupMenu()->sizeHint();
  mMenu->popup( mapToGlobal( QPoint( 0, -(ps.height()) ) ) );
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
      kdDebug() << "Unknown template command index: " << cmd << endl;
      break;
  }
}

#include "templatesinsertcommand.moc"
