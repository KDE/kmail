/*
    kmgroupware.cpp

    This file is part of KMail.

    Copyright (c) 2003 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
    Copyright (c) 2002 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "kmgroupware.h"

#include "kfileio.h"
#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmfoldermgr.h"
#include "kmfoldertree.h"
#include "kmcomposewin.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "kmacctmgr.h"
#include "kmgroupwarefuncs.h"
#include "kmcommands.h"
#include "kmfolderindex.h"
#include "kmkernel.h"
#include "objecttreeparser.h"
#include "kmailicalifaceimpl.h"

using KMail::ObjectTreeParser;

#include <libkcal/icalformat.h>
#include <libkcal/calendarlocal.h>
#include <libkcal/event.h>

using namespace KCal;

#include <kabc/addressee.h>
#include <kabc/address.h>
#include <kabc/phonenumber.h>
#include <kabc/vcardconverter.h>

using namespace KABC;

#include <ktnef/ktnefparser.h>
#include <ktnef/ktnefmessage.h>
#include <ktnef/ktnefdefs.h>

#include <kurl.h>
#include <kmessagebox.h>
#include <klibloader.h>
#include <dcopclient.h>
#include <kparts/part.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kinputdialog.h>

#include <kdebug.h>

#include <qregexp.h>
#include <qbuffer.h>
#include <qfile.h>

#include <mimelib/enum.h>
#include <mimelib/headers.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>
#include <mimelib/text.h>

#include <assert.h>

//-----------------------------------------------------------------------------
KMGroupware::KMGroupware( QObject* parent, const char* name )
  : QObject( parent, name ), mUseGroupware( false )
{
}

//-----------------------------------------------------------------------------
KMGroupware::~KMGroupware()
{
}

void KMGroupware::readConfig()
{
  KConfigGroup options( KMKernel::config(), "Groupware" );

  // Do not read the config for this, if it's not setup at all
  if( options.readEntry( "Enabled", "notset" ) == "notset" )
    return;

  mUseGroupware = options.readBoolEntry( "Enabled", true );
}

bool KMGroupware::vPartFoundAndDecoded( KMMessage* msg, QString& s )
{
  assert( msg );

  if( ( DwMime::kTypeText == msg->type() && ( DwMime::kSubtypeVCal   == msg->subtype() ||
					      DwMime::kSubtypeXVCard == msg->subtype() ) ) ||
      ( DwMime::kTypeApplication == msg->type() &&
	DwMime::kSubtypeOctetStream == msg->subtype() ) )
  {
    s = QString::fromUtf8( msg->bodyDecoded() );
    return true;
  } else if( DwMime::kTypeMultipart == msg->type() &&
	    (DwMime::kSubtypeMixed  == msg->subtype() ) ||
	    (DwMime::kSubtypeAlternative  == msg->subtype() ))
  {
    // kdDebug(5006) << "KMGroupware looking for TNEF data" << endl;
    DwBodyPart* dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
					      DwMime::kSubtypeMsTNEF );
    if( !dwPart )
      dwPart = msg->findDwBodyPart( DwMime::kTypeApplication,
				    DwMime::kSubtypeOctetStream );
    if( dwPart ){
      // kdDebug(5006) << "KMGroupware analyzing TNEF data" << endl;
      KMMessagePart msgPart;
      KMMessage::bodyPart(dwPart, &msgPart);
      return KMGroupware::msTNEFToVPart( msgPart.bodyDecodedBinary(), s );
    }
    else {
	    dwPart = msg->findDwBodyPart( DwMime::kTypeText,
			    DwMime::kSubtypeVCal );
	    if (dwPart) {
      		KMMessagePart msgPart;
		KMMessage::bodyPart(dwPart, &msgPart);
		s = msgPart.body();    
		return true;
	    }
    }
  }else if( DwMime::kTypeMultipart == msg->type() &&
	    DwMime::kSubtypeMixed  == msg->subtype() ){
  }

  return false;
}


void KMGroupware::slotInvalidateIMAPFolders()
{
  QString str = i18n("Are you sure you want to refresh the IMAP cache?\n"
		     "This will remove all changes you have done locally to your folders");
  QString s1 = i18n("Refresh IMAP Cache");
  QString s2 = i18n("&Refresh");
  if( KMessageBox::warningContinueCancel(kmkernel->mainWin(), str, s1, s2 ) == KMessageBox::Continue)
    kmkernel->acctMgr()->invalidateIMAPFolders();
}

//-----------------------------------------------------------------------------
void KMGroupware::setupActions()
{
  static bool actionsSetup = false;

  if( !actionsSetup && kmkernel->mainWin() ) {
    actionsSetup = true;

    // file menu: some entries
    new KAction( i18n("refresh local imap cache", "Refresh Local IMAP Cache"), 0,
		 this, SLOT(slotInvalidateIMAPFolders()), kmkernel->mainWin()->actionCollection(),
		 "invalidate_imap_cache" );
  }

  emit signalMenusChanged();
}

//-----------------------------------------------------------------------------
//   Special Contacts methods called by KMKernel's DCOP functions
//-----------------------------------------------------------------------------
void KMGroupware::requestAddresses( QString fname )
{
  QFile file( fname );
  if( file.open( IO_WriteOnly ) ) {
    QTextStream ts( &file );
    ts.setEncoding( QTextStream::UnicodeUTF8 );

    KMFolder* contacts = kmkernel->iCalIface().folderFromType( "Contact" );
    if( contacts ) {
      QString s;
      for( int i=0; i<contacts->count(); ++i ) {
        bool unget = !contacts->isMessage(i);
        if( KMGroupware::vPartFoundAndDecoded( contacts->getMsg( i ), s ) ) {
          ts << s;
          s.replace('\n', "\\n");
          s.truncate(65);
        }
        if( unget ) contacts->unGetMsg(i);
      }
    }else {
      kdDebug(5006) << "+++KMGroupware::requestAddresses(): Contacts folder does not exist" << endl;
    }
    file.close();
  }else {
    kdDebug(5006) << "+++KMGroupware::requestAddresses(): could not open file" << endl;
  }
}

//--------------
bool KMGroupware::storeAddresses( QString fname, QStringList delUIDs )
{
  KMFolder* contacts = kmkernel->iCalIface().folderFromType( "Contact" );
  if( !contacts ) {
    kdDebug(5006) << "KMGroupware::storeAddresses(): Contacts folder does not exist" << endl;
    return false;
  }

  QFile file( fname );
  QStringList vCards;
  if( file.open( IO_ReadOnly ) ) {
    QTextStream ts( &file );
    ts.setEncoding( QTextStream::UnicodeUTF8 );
    QString currentVCard;
    while( !ts.eof() ) {
      QString line;
      line = ts.readLine();
      if( line.isEmpty() ) {
	// New vCard
	vCards << currentVCard;
	currentVCard = "";
      } else {
	// Continue current vCard
	currentVCard += line + "\r\n";
      }
    }
    file.close();
  }else{
    kdDebug(5006) << "KMGroupware::storeAddresses(): could not open file" << endl;
    return false;
  }

  for( QStringList::iterator it = delUIDs.begin(); it != delUIDs.end(); ++it ) {
    KMMessage* msg = kmkernel->iCalIface().findMessageByUID( *it, contacts );
    if( msg )
      kmkernel->iCalIface().deleteMsg( msg );
    else
      kdDebug(5006) << "vCard not found, cannot remove: " << *it << endl;
  }

  for( QStringList::iterator it2 = vCards.begin(); it2 != vCards.end(); ++it2 ) {
    QCString vCard( (*it2).utf8() );
    QString uid( "UID" );
    QString name( "NAME" );
    vPartMicroParser( vCard, uid, name );
    KMMessage* msg = kmkernel->iCalIface().findMessageByUID( uid, contacts );
    if( !msg ) {
      // This is a new vCard, make a message to store it in
      msg = new KMMessage(); // makes a "Content-Type=text/plain" message
      msg->initHeader();
      msg->setType( DwMime::kTypeText );
      msg->setSubtype( DwMime::kSubtypeXVCard );
      msg->setHeaderField( "Content-Type", "Text/X-VCard; charset=\"utf-8\"" );
      msg->setSubject( "Contact" );
      msg->setTo( name );

      // add missing headers/content:
      msg->setBodyEncoded( vCard );

      // mark the message as read and store it in our Contacts folder
      msg->touch();
      contacts->addMsg( msg );
    } else {
      // Figure out if the contact have been changed
      QString s;
      if( vPartFoundAndDecoded( msg, s ) && s.utf8() != vCard ) {
	msg->setBodyEncoded( vCard );
	msg->setTo( name );
      }
    }
  }
  return true;
}


KMGroupware::VCalType KMGroupware::getVCalType( const QString &vCal )
{
  // This is ugly: We can't even use vPartMicroParser() here, because
  // we are actually looking for the _second_ BEGIN: line.
  // PENDING(kalle) We might need to look for even more things here,
  // like journals.
  if( vCal.find( QRegExp( "BEGIN:\\s*VEVENT" ) ) != -1 )
    return vCalEvent;
  else if( vCal.find( QRegExp( "BEGIN:\\s*VTODO" ) ) != -1 )
    return vCalTodo;
  return vCalUnknown;
}

//-----------------------------------------------------------------------------
void KMGroupware::processVCalRequest( const QCString& receiver,
				      const QString& vCalIn,
                                      QString& choice )
{
#if 0
  // FIXME: Reinstate Outlook workaround
  // If we are in legacy mode, and there is more than one receiver, we
  // need to ask the user which address to use
  KMMessage* msgOld = mMainWidget->headers()->currentMsg();
  KConfigGroup options( KMKernel::config(), "Groupware" );
  QString fromAddress; // this variable is only used in legacy mode
  if( options.readBoolEntry( "LegacyMangleFromToHeaders", false ) ) {
      QStringList toAddresses = KMMessage::splitEmailAddrList( msgOld->to() );
      if( toAddresses.count() <= 1 )
          // only one address: no problem, we can spare the user the dialog
          // and just take the from address
          fromAddress = msgOld->to();
      else {
          // We have more than one To: address and are in legacy mode. Next
          // try is to search the identities for one of the email addresses
          // in the toAddresses list.
          for( QStringList::Iterator sit = toAddresses.begin();
               sit != toAddresses.end(); ++sit ) {
              if( KMMessage::getEmailAddr( *sit ) ==
                  kmkernel->identityManager()->defaultIdentity().emailAddr().local8Bit() ) {
                  // our default identity was contained in the To: list,
                  // copy that from To: to From:
                  fromAddress = *sit;
                  break; // We are done
              }
          }

          // If we still haven't found anything, we have to ask the user
          // what to do.
          if( fromAddress.isEmpty() ) {
              bool bOk;
              fromAddress = KInputDialog::getItem( i18n( "Select Address" ),
                                                   i18n( "In order to let Outlook(tm) recognize you as the receiver, you need to indicate which one of the following addresses is your email address" ),
                                                   toAddresses, 0, false, &bOk,
                                                   kmkernel->mainWin() );
              if( !bOk )
                  // If the user didn't select anything, just take the
                  // first one so that we have something at all.
                  fromAddress = toAddresses.first();
          }
      }
  }
#endif

  QByteArray data, replyData;
  QCString replyType;
  QDataStream arg( data, IO_WriteOnly );
  arg << choice << receiver << vCalIn;
  if( kapp->dcopClient()->call( "korganizer", "KOrganizerIface",
				"eventRequest(QString,QCString,QString)",
				data, replyType, replyData )
      && replyType == "bool" )
  {
    bool rc;
    QDataStream replyStream( replyData, IO_ReadOnly );
    replyStream >> rc;
    kdDebug(5006) << "KOrganizer call succeeded, rc = " << rc << endl;

    if( rc )
      mMainWidget->slotTrashMsg();
  } else
    kdDebug(5006) << "KOrganizer call failed";
}


//-----------------------------------------------------------------------------
void KMGroupware::processVCalReply( const QCString& sender,
				    const QString& vCal,
                                    const QString& choice )
{
  VCalType type = getVCalType( vCal );
  if( type == vCalUnknown ) {
    kdDebug(5006) << "processVCalReply called with something that is not a vCal\n";
    return;
  }

  if( choice == "enter" ) {
    // step 1: call Organizer
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg( data, IO_WriteOnly );
    arg << sender << vCal;
    if( kapp->dcopClient()->call( "korganizer", "KOrganizerIface",
				  "eventReply(QCString,QString)",
				  data, replyType, replyData )
	&& replyType == "bool" )
    {
      bool rc;
      QDataStream replyStream( replyData, IO_ReadOnly );
      replyStream >> rc;
      kdDebug(5006) << "KOrganizer call succeeded, rc = " << rc << endl;

      if( rc )
	kdDebug(5006) << "KOrganizer call succeeded\n";
    } else
      kdDebug(5006) << "KOrganizer call failed\n";

    // step 2: inform user that Organizer was updated
    KMessageBox::information( kmkernel->mainWin(), (type == vCalEvent ?
					 i18n("The answer was registered in your calendar.") :
					 i18n("The answer was registered in your task list.")),
			      QString::null, "groupwareBox");
  } else if( choice == "cancel" ) {
#if 0
    // TODO: Implement this with DCOP
    QString uid( "UID" );
    QString descr("DESCRIPTION");
    QString summary("SUMMARY");

    vPartMicroParser( vCal.utf8(), uid, descr, summary );
    if( type == vCalEvent ) {
      emit signalEventDeleted( uid );
      KMessageBox::information( kmkernel->mainWin(), i18n("<qt>The event <b>%1</b> was deleted from your calendar.</qt>")
				.arg( descr) );
    } else if( type == vCalTodo ) {
      emit signalTaskDeleted( uid );
      KMessageBox::information( kmkernel->mainWin(), i18n("The task was deleted from your tasks")
				.arg( summary ) );
    }
#endif
  } else {
    // Don't know what to do, so better not delete the mail
    return;
  }

  // An answer was saved, so trash the message
  mMainWidget->slotTrashMsg();
}


//-----------------------------------------------------------------------------
bool KMGroupware::vPartToHTML( int /*aUpdateCounter*/, const QString& vCal,
			       QString fname, QString& prefix,
			       QString& postfix ) const
{
  VCalType type = getVCalType( vCal );
  if( type == vCalUnknown ) {
    kdDebug(5006) << "Unknown incidence!\n";
    return false;
  }

  CalendarLocal cl;
  ICalFormat format;
  format.fromString(&cl, vCal);
  // make a shallow copy of the event list
  Event::List eventList = cl.events();
  // the events will be deleted automatically when cl is destroyed
  eventList.setAutoDelete(false);

  Q_ASSERT(eventList.count() != 0);

  // parse the first event out of the vcal
  // ### is it legal to have several events per mail?
  Event* event = eventList.first();
  QString sLocation = event->location();
  QString sDtEnd = event->dtEndTimeStr();
  QString sDtStart = event->dtStartTimeStr();
  QString sDescr = event->description().simplifyWhiteSpace();
  QString sMethod; // = event->method(); //###TODO actually the scheduler needs to do that

  QString sAttendee;
  Attendee::List attendees = event->attendees();
  if( attendees.count() == 0 ) {
    kdDebug(5006) << "No attendees in the iCal!\n";
    return false;
  }
  if( attendees.count() != 1 )
    kdDebug(5006) << "Warning: attendeecount in the reply should be 1 but is "
		  << attendees.count() << endl;
  Attendee* attendee = *attendees.begin();

  // FIXME: This is a temporary workaround to get the method
  sMethod = "METHOD";
  vPartMicroParser( vCal.utf8(), sMethod );

  QString sSummary = event->summary();

  kdDebug(5006) << "Event stuff: " << sLocation << ", " << sDtEnd << ", "
		<< sDtStart << ", " << sDescr << ", " << sMethod << ", "
		<< sAttendee << endl;

  string2HTML( sLocation );
  string2HTML( sDescr );

  sMethod = sMethod.lower();

  QString typeString;
  if( type == vCalEvent )
    typeString = i18n("calendar");
  else
    typeString = i18n("tasks");

  if( sMethod == "request" ) {
    if( type == vCalEvent ) {
      prefix = i18n("You have been invited to a meeting");
      prefix += "<br>";
      if( !sLocation.isEmpty() )
	prefix.append( i18n( "The meeting will take place in %1 from %2 to %3" )
		       .arg( sLocation ).arg( sDtStart ).arg( sDtEnd ) );
      else
	prefix.append( i18n( "The meeting will take place from %1 to %2" )
		       .arg( sDtStart )
		       .arg( sDtEnd ) );
    } else {
      prefix = i18n( "You have been assigned a task:<br>%1" ).arg( sSummary );
    }
  } else if( sMethod == "reply" ) {
    switch( attendee->status() ) {
    case Attendee::Accepted:
      if( type == vCalEvent )
	prefix = i18n("Sender <b>accepts</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( type == vCalTodo )
	prefix = i18n( "Sender <b>accepts</b> the task <b>%1</b>." ).arg(sSummary );
      break;

    case Attendee::Tentative:
      if( type == vCalEvent )
	prefix = i18n("Sender <b>tentatively accepts</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( type == vCalTodo )
	prefix = i18n( "Sender <b>tentatively accepts</b> the task <b>%1</b>." ).
	  arg(sSummary );
      break;

    case Attendee::Declined:
      if( type == vCalEvent )
	prefix = i18n("Sender <b>declines</b> the invitation to meet in %1<br>from %2 to %3.")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      else if( vCalTodo )
	prefix = i18n( "Sender <b>declines</b> the task %1." ).arg( sSummary );
      break;

    default:
      if( type == vCalEvent ) {
	prefix = i18n("This is an unknown reply to the event in %1 from %2 to %3")
	  .arg( sLocation ).arg( sDtStart ).arg( sDtEnd );
      } else if( type == vCalTodo ) {
	prefix = i18n("This is an unknown reply to the task %1").arg(sSummary);
      }
    }
  } else if( sMethod == "cancel" ) {
    if( type == vCalEvent ) {
      prefix = i18n("The event %1 was canceled").arg(sSummary);
    } else if( type == vCalTodo ) {
      prefix = i18n("The task %1 was canceled").arg(sSummary);
    }
  }

  // show the 'buttons' (only if in groupware mode)
  if( mUseGroupware ) {
    prefix.append( "<br>&nbsp;<br>&nbsp;<br><table border=\"0\" cellspacing=\"0\"><tr><td>&nbsp;</td><td>" );
    if( sMethod == "request" || sMethod == "update" ) {
      // Accept
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_accept#%1\"><b>")
		     .arg(fname) );
      prefix.append( i18n("[Accept]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>") );
      // Accept conditionally
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_accept conditionally#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Accept&nbsp;cond.]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>") );
      // Decline
      prefix.append( QString("<a href=\"kmail:groupware_vCal_request_decline#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Decline]") );
      prefix.append( QString("</b></a></td><td> &nbsp; </td><td>" ) );
      if( type == vCalEvent ) {
	// Check my calendar...
	prefix.append(QString("<a href=\"kmail:groupware_vCal_request_check#%1\"><b>")
		      .arg(fname));
	prefix.append(i18n("[Check&nbsp;my&nbsp;calendar...]"));
	prefix.append(QString("</b></a>"));
      }
    } else if( sMethod == "reply" ) {
      // Enter this into my calendar
      prefix.append(QString("<a href=\"kmail:groupware_vCal_reply_enter#%1\"><b>")
		    .arg(fname));
      if( type == vCalEvent )
	prefix.append(i18n("[Enter&nbsp;this&nbsp;into&nbsp;my&nbsp;calendar]"));
      else
	prefix.append(i18n("[Enter&nbsp;this&nbsp;into&nbsp;my&nbsp;tasks]"));
      prefix.append(QString("</b></a>"));
    } else if( sMethod == "cancel" ) {
      // Cancel event from my calendar
      prefix.append( QString("<a href=\"kmail:groupware_vCal_cancel_enter#%1\"><b>")
		     .arg( fname ) );
      prefix.append( i18n("[Remove&nbsp;this&nbsp;from&nbsp;my&nbsp;calendar]"));
      prefix.append(QString("</b></a>"));
    }
    prefix.append( "</td></tr></table>" );
  }

  if( sMethod == "request" || sMethod == "cancel" ) {
    sDescr.prepend( "<br>&nbsp;<br>&nbsp;<br><u>" + i18n("Description:")
		    + "</u><br><table border=\"0\"><tr><td>&nbsp;</td><td>" );
    sDescr.append( "</td></tr></table>" );
    prefix.append( sDescr );
  }
  prefix.append("&nbsp;<br>&nbsp;<br><u>");
  prefix.append(i18n("Original message:"));
  prefix.append("</u><br><table border=\"0\"><tr><td>&nbsp;</td><td>");
  // postfix:
  postfix = "</td></tr></table>";

  return true;
}


//-----------------------------------------------------------------------------

QString stringProp( KTNEFMessage* tnefMsg, const Q_UINT32& key,
                    const QString& fallback = QString::null)
{
  return tnefMsg->findProp( key < 0x10000 ? key & 0xFFFF : key >> 16, fallback );
}

QString sNamedProp( KTNEFMessage* tnefMsg, const QString& name,
                    const QString& fallback = QString::null)
{
  return tnefMsg->findNamedProp( name, fallback );
}

//-----------------------------------------------------------------------------

bool KMGroupware::msTNEFToVPart( const QByteArray& tnef, QString& vPart )
{
  // Note: vPart is not erased but
  //       keeps it's initial data if it cannot be decoded
  bool bOk = false;

  KTNEFParser parser;
  QBuffer buf( tnef );
  CalendarLocal cal;
  Addressee addressee;
  VCardConverter cardConv;
  ICalFormat calFormat;
  Event* event = new Event();

  if( parser.openDevice( &buf ) )
  {
    KTNEFMessage* tnefMsg = parser.message();
    //QMap<int,KTNEFProperty*> props = parser.message()->properties();

    // everything depends from property PR_MESSAGE_CLASS
    // (this is added by KTNEFParser):
    QString msgClass = tnefMsg->findProp(0x001A, QString::null, true).upper();
    if( !msgClass.isEmpty() ){
      // Match the old class names that might be used by Outlook for
      // compatibility with Microsoft Mail for Windows for Workgroups 3.1.
      bool bCompatClassAppointment   = false;
      bool bCompatMethodRequest      = false;
      bool bCompatMethodCancled      = false;
      bool bCompatMethodAccepted     = false;
      bool bCompatMethodAcceptedCond = false;
      bool bCompatMethodDeclined     = false;
      if( msgClass.startsWith( "IPM.MICROSOFT SCHEDULE." ) ){
        bCompatClassAppointment = true;
        if( msgClass.endsWith( ".MTGREQ" ) )
          bCompatMethodRequest = true;
        if( msgClass.endsWith( ".MTGCNCL" ) )
          bCompatMethodCancled = true;
        if( msgClass.endsWith( ".MTGRESPP" ) )
          bCompatMethodAccepted = true;
        if( msgClass.endsWith( ".MTGRESPA" ) )
          bCompatMethodAcceptedCond = true;
        if( msgClass.endsWith( ".MTGRESPN" ) )
          bCompatMethodDeclined = true;
      }
      bool bCompatClassNote = (msgClass == "IPM.MICROSOFT MAIL.NOTE");


      if( bCompatClassAppointment || "IPM.APPOINTMENT" == msgClass ){

        // compose a vCal
        bool bIsReply = false;
        QString prodID;
        prodID += "-//Microsoft Corporation//Outlook ";
        prodID += tnefMsg->findNamedProp("0x8554", "9.0");
        prodID += "MIMEDIR/EN\n";
        prodID += "VERSION:2.0\n";
        calFormat.setApplication("Outlook", prodID);

        Scheduler::Method method;

        if( bCompatMethodRequest )
          method  = Scheduler::Request;
        else if ( bCompatMethodCancled )
          method = Scheduler::Cancel;
        else if ( bCompatMethodAccepted ||
                  bCompatMethodAcceptedCond ||
                  bCompatMethodDeclined ){
          method = Scheduler::Reply;
          bIsReply = true;
        }
        else{
          // pending(khz): verify whether "0x0c17" is the right tag ???
          //
          // at the moment we think there are REQUESTS and UPDATES
          //
          // but WHAT ABOUT REPLIES ???
          //
          //

          if( tnefMsg->findProp(0x0c17) == "1" )
            bIsReply = true;
          method = Scheduler::Request;
        }

        /// ###  FIXME Need to get this attribute written
        ScheduleMessage schedMsg(event, method, ScheduleMessage::Unknown /*???*/);
		
        QString sSenderSearchKeyEmail( tnefMsg->findProp(0x0C1D) );

        if( !sSenderSearchKeyEmail.isEmpty() ){
          int colon = sSenderSearchKeyEmail.find(':');
          if( sSenderSearchKeyEmail.find(':') == -1 ) // may be e.g. "SMTP:KHZ@KDE.ORG"
            sSenderSearchKeyEmail.remove(0, colon+1);
        }

        QString s( tnefMsg->findProp(0x0e04) );
        QStringList attendees = QStringList::split(';', s);
        if( attendees.count() ){
          for ( QStringList::Iterator it = attendees.begin(); it != attendees.end(); ++it ) {
            // skip all entries that have no '@' since these are no mail addresses
            if( (*it).find('@') == -1 ){
              s = (*it).stripWhiteSpace();

              Attendee *attendee = new Attendee(s,s, true);
              if (bIsReply) {
                if (bCompatMethodAccepted)
                  attendee->setStatus(Attendee::Accepted);
                if (bCompatMethodAcceptedCond)
                  attendee->setStatus(Attendee::Declined);
                if (bCompatMethodDeclined)
                  attendee->setStatus(Attendee::Tentative);
              }
              else {
                attendee->setStatus(Attendee::NeedsAction);
                attendee->setRole(Attendee::ReqParticipant);
              }
              event->addAttendee(attendee);
            }
         }
       }
       else
       {
          // Oops, no attendees?
          // This must be old style, let us use the PR_SENDER_SEARCH_KEY.
          s = sSenderSearchKeyEmail;
          if( !s.isEmpty() ){
            Attendee *attendee = new Attendee(QString::null,QString::null, true);
            if (bIsReply) {
              if (bCompatMethodAccepted)
                attendee->setStatus(Attendee::Accepted);
              if (bCompatMethodAcceptedCond)
                attendee->setStatus(Attendee::Declined);
              if (bCompatMethodDeclined)
                attendee->setStatus(Attendee::Tentative);
            }
            else {
              attendee->setStatus(Attendee::NeedsAction);
              attendee->setRole(Attendee::ReqParticipant);
            }
            event->addAttendee(attendee);
          }
        }
        s = tnefMsg->findProp(0x0c1f); // look for organizer property
        if( s.isEmpty() && !bIsReply )
          s = sSenderSearchKeyEmail;
        if( !s.isEmpty() )
          event->setOrganizer(s);

        s = tnefMsg->findProp(0x8516)
              .replace(QChar('-'), QString::null)
              .replace(QChar(':'), QString::null);
	    event->setDtStart(QDateTime::fromString(s)); // ## Format??

        s = tnefMsg->findProp(0x8517)
              .replace(QChar('-'), QString::null)
              .replace(QChar(':'), QString::null);

	    event->setDtEnd(QDateTime::fromString(s));

        s = tnefMsg->findProp(0x8208);
        event->setLocation(s);

        // is it OK to set this to OPAQUE always ??
        //vPart += "TRANSP:OPAQUE\n"; ###FIXME, portme!
        //vPart += "SEQUENCE:0\n";

        // is "0x0023" OK  -  or should we look for "0x0003" ??
        s = tnefMsg->findProp(0x0023);
        event->setUid(s);

        // pending(khz): is this value in local timezone ??   must it be adjusted ??
        // most likely this is a bug in the server or in Outlook - we ignore it for now.
        s = tnefMsg->findProp(0x8202)
              .replace(QChar('-'), QString::null)
              .replace(QChar(':'), QString::null);
        // event->setDtStamp(QDateTime::fromString(s)); // ### libkcal always uses currentDateTime()


		
        s = tnefMsg->findNamedProp("Keywords");
		event->setCategories(s);

        s = tnefMsg->findProp(0x1000);
        event->setDescription(s);

        s = tnefMsg->findProp(0x0070);
        event->setSummary(s);


        s = tnefMsg->findProp(0x0026);
        event->setPriority(s.toInt());

        // is reminder flag set ?
        if(!tnefMsg->findProp(0x8503).isEmpty()) {

		  Alarm *alarm = new Alarm(event);

          QDateTime highNoonTime(
                      pureISOToLocalQDateTime( tnefMsg->findProp(0x8502)
                                                    .replace(QChar('-'), "")
                                                    .replace(QChar(':'), "") ) );
          QDateTime wakeMeUpTime(
                      pureISOToLocalQDateTime( tnefMsg->findProp(0x8560, "")
                                                    .replace(QChar('-'), "")
                                                    .replace(QChar(':'), "") ) );

           alarm->setTime(wakeMeUpTime);

           if( highNoonTime.isValid() && wakeMeUpTime.isValid() )
             alarm->setStartOffset(Duration(highNoonTime, wakeMeUpTime));
           else
             // default: wake them up 15 minutes before the appointment
             alarm->setStartOffset(Duration(15*60));
			 alarm->setDisplayAlarm(i18n("Reminder"));

          // sorry: the different action types are not known (yet)
          //        so we always set 'DISPLAY' (no sounds, no images...)
		  event->addAlarm(alarm);
        }
		cal.addEvent(event);
        bOk = true;
        // we finished composing a vCal

      }else if( bCompatClassNote || "IPM.CONTACT" == msgClass ){

        addressee.setUid(stringProp(tnefMsg, attMSGID));
        addressee.setFormattedName(stringProp(tnefMsg, MAPI_TAG_PR_DISPLAY_NAME));
        addressee.insertEmail(sNamedProp(tnefMsg, MAPI_TAG_CONTACT_EMAIL1EMAILADDRESS),true);
        addressee.insertEmail(sNamedProp(tnefMsg, MAPI_TAG_CONTACT_EMAIL2EMAILADDRESS),false);
        addressee.insertEmail(sNamedProp(tnefMsg, MAPI_TAG_CONTACT_EMAIL3EMAILADDRESS),false);
        addressee.insertCustom("KADDRESSBOOK", "X-IMAddress",
          sNamedProp(tnefMsg, MAPI_TAG_CONTACT_IMADDRESS));
        addressee.insertCustom("KADDRESSBOOK", "X-SpousesName",
          stringProp(tnefMsg, MAPI_TAG_PR_SPOUSE_NAME));
        addressee.insertCustom("KADDRESSBOOK", "X-ManagersName",
          stringProp(tnefMsg, MAPI_TAG_PR_MANAGER_NAME));
        addressee.insertCustom("KADDRESSBOOK", "X-AssistantsName",
          stringProp(tnefMsg, MAPI_TAG_PR_ASSISTANT));
        addressee.insertCustom("KADDRESSBOOK", "X-Department",
          stringProp(tnefMsg, MAPI_TAG_PR_DEPARTMENT_NAME));
        addressee.insertCustom("KADDRESSBOOK", "X-Office",
          stringProp(tnefMsg, MAPI_TAG_PR_OFFICE_LOCATION));
        addressee.insertCustom("KADDRESSBOOK", "X-Profession",
          stringProp(tnefMsg, MAPI_TAG_PR_PROFESSION));

        QString s = tnefMsg->findProp( MAPI_TAG_PR_WEDDING_ANNIVERSARY)
                      .replace(QChar('-'), QString::null)
                      .replace(QChar(':'), QString::null);

        if( !s.isEmpty() )
          addressee.insertCustom("KADDRESSBOOK", "X-Anniversary", s);

        addressee.setUrl(sNamedProp(tnefMsg, MAPI_TAG_CONTACT_WEBPAGE));

        // collect parts of Name entry
        addressee.setFamilyName(stringProp(tnefMsg, MAPI_TAG_PR_SURNAME));
        addressee.setGivenName(stringProp(tnefMsg, MAPI_TAG_PR_GIVEN_NAME));
        addressee.setAdditionalName(stringProp(tnefMsg, MAPI_TAG_PR_MIDDLE_NAME));
        addressee.setPrefix(stringProp(tnefMsg, MAPI_TAG_PR_DISPLAY_NAME_PREFIX));
        addressee.setSuffix(stringProp(tnefMsg, MAPI_TAG_PR_GENERATION));

        addressee.setNickName(stringProp(tnefMsg, MAPI_TAG_PR_NICKNAME));
        addressee.setRole(stringProp(tnefMsg, MAPI_TAG_PR_TITLE));
        addressee.setOrganization(stringProp(tnefMsg, MAPI_TAG_PR_COMPANY_NAME));
        /*
        the MAPI property ID of this (multiline) )field is unknown:
        vPart += stringProp(tnefMsg, "\n","NOTE", ... , "" );
        */

        Address adr;

        adr.setPostOfficeBox(stringProp(tnefMsg, MAPI_TAG_PR_HOME_ADDRESS_PO_BOX));
        adr.setStreet(stringProp(tnefMsg,        MAPI_TAG_PR_HOME_ADDRESS_STREET));
        adr.setLocality(stringProp(tnefMsg,      MAPI_TAG_PR_HOME_ADDRESS_CITY));
        adr.setRegion(stringProp(tnefMsg,        MAPI_TAG_PR_HOME_ADDRESS_STATE_OR_PROVINCE));
        adr.setPostalCode(stringProp(tnefMsg,    MAPI_TAG_PR_HOME_ADDRESS_POSTAL_CODE));
        adr.setCountry(stringProp(tnefMsg,       MAPI_TAG_PR_HOME_ADDRESS_COUNTRY));
        adr.setType(Address::Home);

        addressee.insertAddress(adr);

        adr.setPostOfficeBox(sNamedProp(tnefMsg, MAPI_TAG_CONTACT_BUSINESSADDRESSPOBOX));
        adr.setStreet(sNamedProp(tnefMsg,        MAPI_TAG_CONTACT_BUSINESSADDRESSSTREET));
        adr.setLocality(sNamedProp(tnefMsg,      MAPI_TAG_CONTACT_BUSINESSADDRESSCITY));
        adr.setRegion(sNamedProp(tnefMsg,        MAPI_TAG_CONTACT_BUSINESSADDRESSSTATE));
        adr.setPostalCode(sNamedProp(tnefMsg,    MAPI_TAG_CONTACT_BUSINESSADDRESSPOSTALCODE));
        adr.setCountry(sNamedProp(tnefMsg,       MAPI_TAG_CONTACT_BUSINESSADDRESSCOUNTRY));
        adr.setType(Address::Work);

        addressee.insertAddress(adr);

        adr.setPostOfficeBox(stringProp(tnefMsg, MAPI_TAG_PR_OTHER_ADDRESS_PO_BOX));
        adr.setStreet(stringProp(tnefMsg,        MAPI_TAG_PR_OTHER_ADDRESS_STREET));
        adr.setLocality(stringProp(tnefMsg,      MAPI_TAG_PR_OTHER_ADDRESS_CITY));
        adr.setRegion(stringProp(tnefMsg,        MAPI_TAG_PR_OTHER_ADDRESS_STATE_OR_PROVINCE));
        adr.setPostalCode(stringProp(tnefMsg,    MAPI_TAG_PR_OTHER_ADDRESS_POSTAL_CODE));
        adr.setCountry(stringProp(tnefMsg,       MAPI_TAG_PR_OTHER_ADDRESS_COUNTRY));
        adr.setType(Address::Dom);

        addressee.insertAddress(adr);

        // problem: the 'other' address was stored by KOrganizer in
        //          a line looking like the following one:
        // vPart += "\nADR;TYPE=dom;TYPE=intl;TYPE=parcel;TYPE=postal;TYPE=work;TYPE=home:other_pobox;;other_str1\nother_str2;other_loc;other_region;other_pocode;other_country

        QString nr;
        nr = stringProp(tnefMsg, MAPI_TAG_PR_HOME_TELEPHONE_NUMBER);
        addressee.insertPhoneNumber(KABC::PhoneNumber(nr,PhoneNumber::Home));
        nr = stringProp(tnefMsg, MAPI_TAG_PR_BUSINESS_TELEPHONE_NUMBER);
        addressee.insertPhoneNumber(KABC::PhoneNumber(nr,PhoneNumber::Work));
        nr = stringProp(tnefMsg, MAPI_TAG_PR_MOBILE_TELEPHONE_NUMBER);
        addressee.insertPhoneNumber(KABC::PhoneNumber(nr,PhoneNumber::Cell));
        nr = stringProp(tnefMsg, MAPI_TAG_PR_HOME_FAX_NUMBER);
        addressee.insertPhoneNumber(KABC::PhoneNumber(nr,PhoneNumber::Fax|PhoneNumber::Home));
        nr = stringProp(tnefMsg, MAPI_TAG_PR_BUSINESS_FAX_NUMBER);
        addressee.insertPhoneNumber(KABC::PhoneNumber(nr,PhoneNumber::Fax|PhoneNumber::Work));

        s = tnefMsg->findProp( MAPI_TAG_PR_BIRTHDAY)
                      .replace(QChar('-'), QString::null)
                      .replace(QChar(':'), QString::null);
        if( !s.isEmpty() )
          addressee.setBirthday(QDateTime::fromString(s));

      bOk = (!addressee.isEmpty());

      }else if( "IPM.NOTE" == msgClass ){

      } // else if ... and so on ...
    }
  }

  // compose return string

  QString s;
  vPart  = calFormat.toString(&cal);
  if (cardConv.addresseeToVCard(addressee, s, VCardConverter::v3_0))
    vPart += s;

  return bOk;
}


//-----------------------------------------------------------------------------
bool KMGroupware::msTNEFToHTML( KMReaderWin* reader, QString& vPart, QString fname,
                                QString& prefix, QString& postfix ) const
{
  QByteArray tnef( kFileToBytes( fname, false ) );
  if( tnef.count() ) {
    int updateCounter = 0;
    if( msTNEFToVPart( tnef, vPart ) ){
      QByteArray theBody( vPart.utf8() );
      QString fname2( ObjectTreeParser::byteArrayToTempFile( reader,
                                                        "groupware",
                                                        "vPart_decoded.raw",
                                                        theBody ) );
      if( !fname2.isEmpty() )
        return vPartToHTML( updateCounter, vPart, fname2, prefix, postfix );
    }
  }else{
    KMessageBox::error(0, i18n("<qt>Unable to open file <b>%1</b>.</qt>").arg(fname));
  }
  return false;
}


//-----------------------------------------------------------------------------
bool KMGroupware::foundGroupwareLink( const QString aUrl, QString& gwType, QString& gwAction,
                                      QString& gwAction2, QString& gwData )
{
  static QString gwPrefix("groupware_");
  gwType    = "";
  gwAction  = "";
  gwAction2 = "";
  gwData    = "";

  int i1 = aUrl.find( gwPrefix );
  if( -1 < i1 ) {
    i1 += gwPrefix.length();

    int i2 = aUrl.find("_", i1);
    if( i1 <= i2 )
    {
      // retrieve gwType
      gwType = aUrl.mid( i1, i2-i1 );
      i1 = i2+1;
      i2 = aUrl.find("_", i1);
      if( i1 <= i2 )
      {
        // retrieve gwAction
        gwAction = aUrl.mid( i1, i2-i1 );
        i1 = i2+1;
        i2 = aUrl.find("#", i1);
        if( i1 <= i2 )
        {
          // retrieve gwAction2
          gwAction2 = aUrl.mid( i1, i2-i1 );
          i2 += 1;
          // retrieve gwData
          gwData = aUrl.mid( i2 );
        }
      }
    }
  }
  return !gwType.isEmpty();
}


bool KMGroupware::handleLink( const KURL &aUrl, KMMessage* msg )
{
  QString gwType, gwAction, gwAction2, gwData;

  if( !aUrl.hasRef() || !foundGroupwareLink( aUrl.path()+"#"+aUrl.ref(), gwType,
					     gwAction, gwAction2, gwData ) )
    // No groupware link to handle here
    return false;

  if( gwType != "vCal" || gwData.isEmpty()
      || ( "request" != gwAction && "reply" != gwAction
	   && "cancel" != gwAction ) ) {
    // Then we can't handle it. But it is a groupware link, so we return true
    kdDebug(5006) << "Unhandled groupware link\n";
    return true;
  }

  // Read the vCal
  QFile file( gwData );
  if( !file.open( IO_ReadOnly ) ) {
    kdDebug(5006) << "Could not open file " << gwData << endl;
    return true;
  }
  QTextStream ts( &file );
  ts.setEncoding( QTextStream::UnicodeUTF8 );
  QString vCal = ts.read();
  file.close();

  // Find the receiver if we can
  QString receiver;
  if( msg ) {
    KMIdentity ident = kmkernel->identityManager()->identityForAddress( msg->to() );
    if( ident != KMIdentity::null ) {
      receiver = ident.emailAddr();
    } else {
      QStringList addrs = KMMessage::splitEmailAddrList( msg->to() );
      bool ok;
      receiver = KInputDialog::getItem( i18n("Select Address"),
					i18n("None of your identities match the receiver "
					     "of this message,<br> please choose which of "
					     "the following addresses is yours:"),
					addrs, 0, FALSE, &ok, kmkernel->mainWin() );
      if( !ok ) return false;
    }
  }

  // Find the sender if we can
  QCString sender = KMMessage::getEmailAddr( msg->from() );

  if( "request" == gwAction )
    processVCalRequest( receiver.utf8(), vCal, gwAction2 );
  else if( "reply" == gwAction )
    processVCalReply( sender, vCal, gwAction2 );
  else if( "cancel" == gwAction )
    /* Note, we pass gwAction here, not gwAction2 */
    processVCalReply( sender, vCal, gwAction );

  return true;
}


/*!
  This method handles incoming resource requests. It sends them off to
  KOrganizer for answering, records the result and sends an answer
  back.
*/
bool KMGroupware::incomingResourceMessage( KMAccount* /*acct*/, KMMessage* /*msg*/ )
{
#if 0
  // TODO: Reimplement with DCOP

  if( !mUseGroupware)
    return false;

  QString vCalIn;
  if( vPartFoundAndDecoded( msg, vCalIn ) )
    return false;

  bool vCalInOK, vCalOutOK, isFree;
  QString vCalOut;
  QDateTime start, end;
  emit( signalResourceRequest( acct->intervals(), KMMessage::getEmailAddr( msg->to() ),
			       vCalIn, vCalInOK, vCalOut, vCalOutOK, isFree, start, end ) );
  if( !vCalInOK || !vCalOutOK )
    return false; // parsing or generation error somewhere

  // Check whether we are supposed to answer automatically at all
  KConfigGroup options( KMKernel::config(), "Groupware" );
  if( isFree && options.readBoolEntry( "AutoAccept", false ) )
    return false;
  if( !isFree && options.readBoolEntry( "AutoDeclConflict", false ) )
    return false;

  // Everything went fine so far, now attach the answer
  KMMessage* msgNew = 0;
  if( msg ){
    msgNew = msg->createReply( false, false, vCalOut, false, true, TRUE );
    msgNew->setType( DwMime::kTypeText );
    msgNew->setSubtype( DwMime::kSubtypeVCal );
    msgNew->setHeaderField("Content-Type", "text/calendar; method=REPLY; charset=\"utf-8\"");
    internal_directlySendMessage( msgNew );
  }

  // And also record in the account.
  acct->addInterval( qMakePair( start, end ) );
#endif

  return true;
}


void KMGroupware::reloadFolderTree() const
{
  // Make the folder tree show the icons or not
  if( mMainWidget->folderTree() )
    mMainWidget->folderTree()->reload();
}

#include "kmgroupware.moc"
