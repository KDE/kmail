/*  -*- c++ -*-
    vacation.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "vacation.h"

#include "vacationdialog.h"
#include "sievejob.h"
using KMail::SieveJob;
#include "sieveconfig.h"
#include "kmkernel.h"
#include "kmacctmgr.h"
#include "kmacctimap.h"

#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qregexp.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qtimer.h>

#include <cassert>

namespace KMail {

  Vacation::Vacation( QObject * parent, const char * name )
    : QObject( parent, name ), mSieveJob( 0 ), mDialog( 0 )
  {
    mUrl = findURL();
    kdDebug() << "Vacation: found url \"" << mUrl.url() << "\"" << endl;
    if ( mUrl.isEmpty() ) // nothing to do...
      return;
    mUrl.setFileName( "kmail-vacation.siv" );
    mSieveJob = SieveJob::get( mUrl );
    connect( mSieveJob, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
	     SLOT(slotGetResult(KMail::SieveJob*,bool,const QString&,bool)) );
  }

  Vacation::~Vacation() {
    if ( mSieveJob ) mSieveJob->kill();
    delete mDialog;
    kdDebug() << "~Vacation()" << endl;
  }

  QString Vacation::composeScript( const QDate & returnDate, int notificationInterval ) {
    QString script
      = QString::fromLatin1("require \"vacation\";\r\n"
			    "# keep next two lines:\r\n"
			    // if you change these lines, change parseScript, too!
			    "# return date: %1\r\n"
			    "# notification interval: %2\r\n"
			    "vacation ").arg( returnDate.toString( ISODate ) )
                                        .arg( notificationInterval );
    if ( notificationInterval > 0 )
      script += QString::fromLatin1(":days %1 ").arg( notificationInterval );
    script += QString::fromLatin1("text:\r\n");
    script += i18n("mutz@kde.org: If your translation results in a line "
		   "beginning with a dot (.), escape it by prepending another "
		   "dot. Keep the trailing line with the single dot!",
		   "This is to inform you that I'm out of office until %1.\r\n"
		   ".\r\n"
		   ";\r\n").arg( KGlobal::locale()->formatDate( returnDate ) );
    return script;
  }

  static KURL findUrlForAccount( const KMAcctImap * a ) {
    assert( a );
    SieveConfig sieve = a->sieveConfig();
    if ( !sieve.managesieveSupported() )
      return KURL();
    if ( sieve.reuseConfig() ) {
      // assemble Sieve url from the settings of the account:
      KURL u;
      u.setProtocol( "sieve" );
      u.setHost( a->host() );
      u.setUser( a->login() );
      u.setPass( a->passwd() );
      u.setPort( sieve.port() );
      return u;
    } else {
      return sieve.alternateURL();
    }
  }

  KURL Vacation::findURL() const {
    KMAcctMgr * am = kernel->acctMgr();
    assert( am );
    for ( KMAccount * a = am->first() ; a ; a = am->next() )
      if ( dynamic_cast<KMAcctImap*>(a) ) {
	KURL u = findUrlForAccount( static_cast<KMAcctImap*>(a) );
	if ( !u.isEmpty() )
	  return u;
      }
    return KURL();
  }

  bool Vacation::parseScript( const QString & script, QDate & returnDate,
			      int & notificationInterval ) {
    if ( script.isEmpty() ) {
      returnDate = defaultReturnDate();
      notificationInterval = defaultNotificationInterval();
      return true;
    }
    // extract the data from the comments we added in composeScript():
    QRegExp rx("#\\s*return\\s+date:\\s*(\\d{4}-\\d{2}-\\d{2}).*"
	       "#\\s*notification\\s+interval:\\s*(-?\\d+)", false );
    if ( rx.search( script, 0 ) != -1 ) {
      // found
      returnDate = QDate::fromString( rx.cap(1), ISODate );
      notificationInterval = rx.cap(2).toInt();
      return true;
    }
    return false;
  };

  QDate Vacation::defaultReturnDate() {
    return QDate::currentDate().addDays( 1 );
  }

  int Vacation::defaultNotificationInterval() {
    return 7; // days
  }

  void Vacation::slotGetResult( SieveJob * job, bool success,
				const QString & script, bool active ) {
    kdDebug() << "Vacation::slotGetResult( ??, " << success
	      << ", ?, " << active << " )" << endl
	      << "script:" << endl
	      << script << endl;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    if ( mUrl.protocol() == "sieve" && !job->sieveCapabilities().isEmpty() &&
	 !job->sieveCapabilities().contains("vacation") ) {
      KMessageBox::sorry( 0, i18n("Your server didn't list \"vacation\" in "
				  "it's list of supported Sieve extensions.\n"
				  "Without it, KMail cannot install out of "
				  "office replies for you.\n"
				  "Please contact you system administrator.") );
      emit result( false );
      return;
    }

    if ( !mDialog )
      mDialog = new VacationDialog( i18n("Configure Out Of Office Replies"), 0, 0, false );

    QDate returnDate = defaultReturnDate();
    int notificationInterval = defaultNotificationInterval();
    if ( !success ) active = false; // default to inactive

    if ( !success || !parseScript( script, returnDate, notificationInterval ) )
      KMessageBox::information( 0, i18n("Someone (probably you) changed the "
					"vacation script on the server.\n"
					"KMail is no longer able to determine "
					"the parameters for the autoreplies.\n"
					"Default values will be used." ) );

    mDialog->setActivateVacation( active );
    mDialog->setReturnDate( returnDate );
    mDialog->setNotificationInterval( notificationInterval );

    connect( mDialog, SIGNAL(okClicked()), SLOT(slotDialogOk()) );
    connect( mDialog, SIGNAL(cancelClicked()), SLOT(slotDialogCancel()) );

    mDialog->show();
  }

  void Vacation::slotDialogOk() {
    kdDebug() << "Vacation::slotDialogOk()" << endl;
    // compose a new script:
    QString script = composeScript( mDialog->returnDate(),
				    mDialog->notificationInterval() );
    bool active = mDialog->activateVacation();

    kdDebug() << "script:" << endl << script << endl;

    // and commit the dialog's settings to the server:
    mSieveJob = SieveJob::put( mUrl, script, active );
    connect( mSieveJob, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
	     SLOT(slotPutResult(KMail::SieveJob*,bool,const QString&,bool)) );

    // destroy the dialog:
    mDialog->delayedDestruct();
    mDialog = 0;
  }

  void Vacation::slotDialogCancel() {
    kdDebug() << "Vacation::slotDialogCancel()" << endl;
    mDialog->delayedDestruct();
    mDialog = 0;
    emit result( false );
  }

  void Vacation::slotPutResult( SieveJob *, bool success, const QString &, bool ) {
    if ( success )
      KMessageBox::information( 0, i18n("Out of Office reply installed successfully.") );
    kdDebug() << "Vacation::slotPutResult( ???, " << success << ", ?, ? )"
	      << endl;
    mSieveJob = 0; // job deletes itself after returning from this slot!
    emit result( success );
  }
  

}; // namespace KMail

#include "vacation.moc"
