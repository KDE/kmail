/*
    kmgroupware.cpp

    This file is part of KMail.

    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
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

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "kmgroupware.h"

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "kmcomposewin.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include <libemailfunctions/email.h>
#include "kmkernel.h"

#include <kurl.h>
#include <kmessagebox.h>
#include <dcopclient.h>
#include <kdcopservicestarter.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <qregexp.h>

#include <mimelib/enum.h>

#include <assert.h>

#include "korganizeriface_stub.h"


// Handle KOrganizer connection
static bool connectToKOrganizer();
static KOrganizerIface_stub* mKOrganizerIfaceStub;


//-----------------------------------------------------------------------------
KMGroupware::KMGroupware( QObject* parent, const char* name )
  : QObject( parent, name ), mUseGroupware( false )
{
  // Make the connection to KOrganizer ready
  mKOrganizerIfaceStub = 0;
  kapp->dcopClient()->setNotifications( true );
  connect( kapp->dcopClient(), SIGNAL( applicationRemoved( const QCString& ) ),
           this, SLOT( unregisteredFromDCOP( const QCString& ) ) );

  // Listen to config changes
  connect( kmkernel, SIGNAL( configChanged() ), this, SLOT( readConfig() ) );
}

//-----------------------------------------------------------------------------
KMGroupware::~KMGroupware()
{
  kapp->dcopClient()->setNotifications( false );
  delete mKOrganizerIfaceStub;
}

void KMGroupware::readConfig()
{
  KConfigGroup options( KMKernel::config(), "Groupware" );
  mUseGroupware = options.readBoolEntry( "Enabled", false );
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
      if ( !connectToKOrganizer() )
        kdError() << "DCOP error during KMGroupware::vPartFoundAndDecoded()\n";
      else {
        s = mKOrganizerIfaceStub->msTNEFToVPart( msgPart.bodyDecodedBinary() );
        return !s.isEmpty();
      }
    } else {
      dwPart = msg->findDwBodyPart( DwMime::kTypeText, DwMime::kSubtypeVCal );
      if (dwPart) {
        KMMessagePart msgPart;
        KMMessage::bodyPart(dwPart, &msgPart);
        s = msgPart.body();
        return true;
      }
    }
  }else if( DwMime::kTypeMultipart == msg->type() &&
            DwMime::kSubtypeMixed  == msg->subtype() ) {
    // TODO: Something?
  }

  return false;
}

/*
 * Message part formatting
 */

QString KMGroupware::vPartToHTML( const QString& /*iCal*/ )
{
  return "<b>This is an iCalendar attachment handled by the wrong bodypart"
         "formatterplugin.</b><p/>";
#if 0
  QString html;

  if( mUseGroupware ) {
    // Call KOrganizer and make that format the mail
    if ( !connectToKOrganizer() ) {
      kdError() << "DCOP error during KMGroupware::processVCalRequest()\n";
    } else {
      html = mKOrganizerIfaceStub->formatICal( iCal );
      kdDebug(5006) << "KOrganizer call succeeded, html = " << html << endl;
    }
  }

  return html;
#endif
}

QString KMGroupware::msTNEFToHTML( const QByteArray& tnef )
{
  QString html;

  if( mUseGroupware ) {
    // Call KOrganizer and make that format the mail
    if ( !connectToKOrganizer() ) {
      kdError() << "DCOP error during KMGroupware::processVCalRequest()\n";
    } else {
      html = mKOrganizerIfaceStub->formatTNEF( tnef );
      kdDebug(5006) << "KOrganizer call succeeded, html = " << html << endl;
    }
  }

  return html;
}

/*
 * Groupware URL handling
 */

static void iCalRequest( const QString& receiver, const QString& iCal,
                         const QString& choice )
{
#if 0
  // FIXME: Reinstate Outlook workaround
  // If we are in legacy mode, and there is more than one receiver, we
  // need to ask the user which address to use
  KMMessage* msgOld = mMainWidget->headers()->currentMsg();
  KConfigGroup options( KMKernel::config(), "Groupware" );
  QString fromAddress; // this variable is only used in legacy mode
  if( options.readBoolEntry( "LegacyMangleFromToHeaders", false ) ) {
      QStringList toAddresses = KPIM::splitEmailAddrList( msgOld->to() );
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
              if( KPIM::getEmailAddr( *sit ) ==
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
                                                   i18n( "In order to let Outlook recognize you as the receiver, you need to indicate which one of the following addresses is your email address:" ),
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

  if ( !connectToKOrganizer() )
    kdError() << "DCOP error during KMGroupware::processVCalRequest()\n";
  else {
    bool rc = mKOrganizerIfaceStub->eventRequest( choice, receiver, iCal );
    kdDebug(5006) << "KOrganizer call succeeded, rc = " << rc << endl;

#if 0
    // TODO(bo): We need to delete the msg somehow
    if( rc && mMainWidget ) mMainWidget->slotTrashMsg();
#endif
  }
}

static void iCalReply( const QString& iCal )
{
  bool event;
  if( iCal.find( QRegExp( "BEGIN:\\s*VEVENT" ) ) != -1 )
    event = true;
  else if( iCal.find( QRegExp( "BEGIN:\\s*VTODO" ) ) != -1 )
    event = false;
  else {
    kdDebug(5006) << "processVCalReply called with something that is not a iCal\n";
    return;
  }

  // Step 1: call Organizer
  if ( !connectToKOrganizer() )
    kdError() << "DCOP error during KMGroupware::processVCalReply()\n";
  else {
    bool rc = mKOrganizerIfaceStub->eventReply( iCal );
    kdDebug(5006) << "KOrganizer call succeeded, rc = " << rc << endl;

    // step 2: inform user that Organizer was updated
    KMessageBox::information( kmkernel->mainWin(),
                              (event ?
                               i18n("The answer was registered in your calendar.") :
                               i18n("The answer was registered in your task list.")),
                              QString::null, "groupwareBox");

#if 0
    // An answer was saved, so trash the message
    if( mMainWidget ) mMainWidget->slotTrashMsg();
#endif
  }
}

static void iCalCancel( const QString& iCal )
{
  if ( !connectToKOrganizer() )
    kdError() << "DCOP error during KMGroupware::iCalCancel()\n";
  else {
    bool rc = mKOrganizerIfaceStub->cancelEvent( iCal );
    kdDebug(5006) << "KOrganizer call succeeded, rc = " << rc << endl;

#if 0
    // An answer was saved, so trash the message
    if( mMainWidget ) mMainWidget->slotTrashMsg();
#endif
  }
}

// Return the part up until the first '_' or '#', or all of it
// Chop off the used part of the string
static QString urlPart( QString& aUrl )
{
  QString result;
  int i = aUrl.find( '_' );
  if( i < 0 ) i = aUrl.find( '#' );
  if( i < 0 ) {
    // Just take the remaining part
    result = aUrl;
    aUrl.truncate( 0 );
  } else {
    result = aUrl.left( i );
    aUrl = aUrl.mid( i + 1 );
  }
  return result;
}

bool KMGroupware::handleLink( const KURL &url, KMMessage* msg )
{
  QString aUrl = url.path();
  if( urlPart( aUrl ) != "groupware" ) return false;
  QString gwAction = urlPart( aUrl );

  // Find the part of the message with the iCal
  QString iCal;
  if( !vPartFoundAndDecoded( msg, iCal ) ) {
    kdDebug(5006) << "Could not find the iCal for this link\n";
    return false;
  }

  if( gwAction == "request" ) {
    // Find the receiver if we can
    QString receiver;
    if( msg ) {
      KPIM::Identity ident =
        kmkernel->identityManager()->identityForAddress( msg->to() );
      if( ident != KPIM::Identity::null ) {
        receiver = ident.emailAddr();
      } else {
        QStringList addrs = KPIM::splitEmailAddrList( msg->to() );
        if( addrs.count() == 1 )
          // Don't ask the user to choose between 1 items
          receiver = addrs[0];
        else {
          bool ok;
          receiver = KInputDialog::
            getItem( i18n("Select Address"),
                     i18n("None of your identities match the receiver "
                          "of this message,<br> please choose which of "
                          "the following addresses is yours:"),
                     addrs, 0, FALSE, &ok, kmkernel->mainWin() );
          if( !ok ) return false;
        }
      }
    }
    iCalRequest( receiver, iCal, urlPart( aUrl ) );
  } else if( gwAction == "reply" )
    iCalReply( iCal );
  else if( gwAction == "cancel" )
    iCalCancel( iCal );
  else {
    // What what what?
    kdDebug(5006) << "Unknown groupware action " << gwAction << endl;
    return false;
  }

  return true;
}

/*
 * Handle connection to KOrganizer.
 */

static const QCString dcopObjectId = "KOrganizerIface";

static bool connectToKOrganizer()
{
  if ( !mKOrganizerIfaceStub ) {
    QString error;
    QCString dcopService;
    int result = KDCOPServiceStarter::self()->
      findServiceFor( "DCOP/Organizer", QString::null,
                      QString::null, &error, &dcopService );
    if ( result != 0 ) {
      kdDebug(5800) << "Could not connect to KOrganizer\n";
      // TODO: You might want to show "error" (if not empty) here,
      // using e.g. KMessageBox
      return false;
    }

    QCString dummy;
    // OK, so korganizer (or kontact) is running. Now ensure the object we want is available.
    if ( !kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dcopService, dummy ) ) {
      KDCOPServiceStarter::self()->startServiceFor( "DCOP/Organizer", QString::null,
                                                    QString::null, &error, &dcopService );
      if( !kapp->dcopClient()->findObject( dcopService, dcopObjectId, "", QByteArray(), dcopService, dummy ) )
        return false;
    }

    mKOrganizerIfaceStub = new KOrganizerIface_stub( kapp->dcopClient(),
                                                     dcopService,
                                                     dcopObjectId );
  }

  return ( mKOrganizerIfaceStub != 0 );
}

void KMGroupware::unregisteredFromDCOP( const QCString& appId )
{
  if ( mKOrganizerIfaceStub && mKOrganizerIfaceStub->app() == appId ) {
    // Delete the stub so that the next time we need the organizer,
    // we'll know that we need to start a new one.
    delete mKOrganizerIfaceStub;
    mKOrganizerIfaceStub = 0;
  }
}


#include "kmgroupware.moc"
