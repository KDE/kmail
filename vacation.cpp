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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vacation.h"
#include <limits.h>

#include "vacationdialog.h"
#include "sievejob.h"
using KMail::SieveJob;
#include "kmkernel.h"
#include "kmacctmgr.h"
#include "kmacctimap.h"
#include "kmmessage.h"
#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identity.h>

#include <kmime_header_parsing.h>
using KMime::Types::AddrSpecList;

#include <ksieve/parser.h>
#include <ksieve/scriptbuilder.h>
#include <ksieve/error.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qdatetime.h>

#include <cassert>

namespace {

  class VacationDataExtractor : public KSieve::ScriptBuilder {
    enum Context {
      None = 0,
      // command itself:
      VacationCommand,
      // tagged args:
      Days, Addresses
    };
  public:
    VacationDataExtractor()
      : KSieve::ScriptBuilder(),
	mContext( None ), mNotificationInterval( 0 )
    {
      kdDebug(5006) << "VacationDataExtractor instantiated" << endl;
    }
    virtual ~VacationDataExtractor() {}

    int notificationInterval() const { return mNotificationInterval; }
    const QString & messageText() const { return mMessageText; }
    const QStringList & aliases() const { return mAliases; }

  private:
    void commandStart( const QString & identifier ) {
      kdDebug( 5006 ) << "VacationDataExtractor::commandStart( \"" << identifier << "\" )" << endl;
      if ( identifier != "vacation" )
	return;
      reset();
      mContext = VacationCommand;
    }

    void commandEnd() {
      kdDebug( 5006 ) << "VacationDataExtractor::commandEnd()" << endl;
      mContext = None;
    }

    void testStart( const QString & ) {}
    void testEnd() {}
    void testListStart() {}
    void testListEnd() {}
    void blockStart() {}
    void blockEnd() {}
    void hashComment( const QString & ) {}
    void bracketComment( const QString & ) {}
    void lineFeed() {}
    void error( const KSieve::Error & e ) {
      kdDebug( 5006 ) << "VacationDataExtractor::error() ### "
		      << e.asString() << " @ " << e.line() << "," << e.column()
		      << endl;
    }
    void finished() {}

    void taggedArgument( const QString & tag ) {
      kdDebug( 5006 ) << "VacationDataExtractor::taggedArgument( \"" << tag << "\" )" << endl;
      if ( mContext != VacationCommand )
	return;
      if ( tag == "days" )
	mContext = Days;
      else if ( tag == "addresses" )
	mContext = Addresses;
    }

    void stringArgument( const QString & string, bool, const QString & ) {
      kdDebug( 5006 ) << "VacationDataExtractor::stringArgument( \"" << string << "\" )" << endl;
      if ( mContext == Addresses ) {
	mAliases.push_back( string );
	mContext = VacationCommand;
      } else if ( mContext == VacationCommand ) {
	mMessageText = string;
	mContext = VacationCommand;
      }
    }

    void numberArgument( unsigned long number, char ) {
      kdDebug( 5006 ) << "VacationDataExtractor::numberArgument( \"" << number << "\" )" << endl;
      if ( mContext != Days )
	return;
      if ( number > INT_MAX )
	mNotificationInterval = INT_MAX;
      else
	mNotificationInterval = number;
      mContext = VacationCommand;
    }

    void stringListArgumentStart() {}
    void stringListEntry( const QString & string, bool, const QString & ) {
      kdDebug( 5006 ) << "VacationDataExtractor::stringListEntry( \"" << string << "\" )" << endl;
      if ( mContext != Addresses )
	return;
      mAliases.push_back( string );
    }
    void stringListArgumentEnd() {
      kdDebug( 5006 ) << "VacationDataExtractor::stringListArgumentEnd()" << endl;
      if ( mContext != Addresses )
	return;
      mContext = VacationCommand;
    }

  private:
    Context mContext;
    int mNotificationInterval;
    QString mMessageText;
    QStringList mAliases;

    void reset() {
      kdDebug(5006) << "VacationDataExtractor::reset()" << endl;
      mContext = None;
      mNotificationInterval = 0;
      mAliases.clear();
      mMessageText = QString::null;
    }
  };

}

namespace KMail {

  Vacation::Vacation( QObject * parent, const char * name )
    : QObject( parent, name ), mSieveJob( 0 ), mDialog( 0 ), mWasActive( false )
  {
    mUrl = findURL();
    kdDebug(5006) << "Vacation: found url \"" << mUrl.prettyURL() << "\"" << endl;
    if ( mUrl.isEmpty() ) // nothing to do...
      return;
    mSieveJob = SieveJob::get( mUrl );
    connect( mSieveJob, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
	     SLOT(slotGetResult(KMail::SieveJob*,bool,const QString&,bool)) );
  }

  Vacation::~Vacation() {
    if ( mSieveJob ) mSieveJob->kill(); mSieveJob = 0;
    delete mDialog; mDialog = 0;
    kdDebug(5006) << "~Vacation()" << endl;
  }

  static inline QString dotstuff( QString s ) {
    if ( s.startsWith( "." ) )
      return '.' + s.replace( "\n.", "\n.." );
    else
      return s.replace( "\n.", "\n.." );
  }

  QString Vacation::composeScript( const QString & messageText,
				   int notificationInterval,
				   const AddrSpecList & addrSpecs )
  {
    QString addressesArgument;
    QStringList aliases;
    if ( !addrSpecs.empty() ) {
      addressesArgument += ":addresses [ ";
      QStringList sl;
      for ( AddrSpecList::const_iterator it = addrSpecs.begin() ; it != addrSpecs.end() ; ++it ) {
	sl.push_back( '"' + (*it).asString().replace( '\\', "\\\\" ).replace( '"', "\\\"" ) + '"' );
	aliases.push_back( (*it).asString() );
      }
      addressesArgument += sl.join( ", " ) + " ] ";
    }
    QString script = QString::fromLatin1("require \"vacation\";\n"
					 "\n"
					 "vacation ");
    script += addressesArgument;
    if ( notificationInterval > 0 )
      script += QString::fromLatin1(":days %1 ").arg( notificationInterval );
    script += QString::fromLatin1("text:\n");
    script += dotstuff( messageText.isEmpty() ? defaultMessageText() : messageText );
    script += QString::fromLatin1( "\n.\n;\n" );
    return script;
  }

  static KURL findUrlForAccount( const KMail::ImapAccountBase * a ) {
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
      u.setQuery( "x-mech=" + (a->auth() == "*" ? "PLAIN" : a->auth()) ); //translate IMAP LOGIN to PLAIN
      u.setFileName( sieve.vacationFileName() );
      return u;
    } else {
      KURL u = sieve.alternateURL();
      u.setFileName( sieve.vacationFileName() );
      return u;
    }
  }

  KURL Vacation::findURL() const {
    KMAcctMgr * am = kmkernel->acctMgr();
    assert( am );
    for ( KMAccount * a = am->first() ; a ; a = am->next() )
      if ( KMail::ImapAccountBase * iab = dynamic_cast<KMail::ImapAccountBase*>( a ) ) {
        KURL u = findUrlForAccount( iab );
	if ( !u.isEmpty() )
	  return u;
      }
    return KURL();
  }

  bool Vacation::parseScript( const QString & script, QString & messageText,
			      int & notificationInterval, QStringList & aliases ) {
    if ( script.stripWhiteSpace().isEmpty() ) {
      messageText = defaultMessageText();
      notificationInterval = defaultNotificationInterval();
      aliases = defaultMailAliases();
      return true;
    }

    // The stripWhiteSpace() call below prevents parsing errors. The
    // slave somehow omits the last \n, which results in a lone \r at
    // the end, leading to a parse error.
    const QCString scriptUTF8 = script.stripWhiteSpace().utf8();
    kdDebug(5006) << "scriptUtf8 = \"" + scriptUTF8 + "\"" << endl;
    KSieve::Parser parser( scriptUTF8.begin(),
			   scriptUTF8.begin() + scriptUTF8.length() );
    VacationDataExtractor vdx;
    parser.setScriptBuilder( &vdx );
    if ( !parser.parse() )
      return false;
    messageText = vdx.messageText().stripWhiteSpace();
    notificationInterval = vdx.notificationInterval();
    aliases = vdx.aliases();
    return true;
  }

  QString Vacation::defaultMessageText() {
    return i18n("I am out of office till %1.\n"
		"\n"
		"In urgent cases, please contact Mrs. <vacation replacement>\n"
		"\n"
		"email: <email address of vacation replacement>\n"
		"phone: +49 711 1111 11\n"
		"fax.:  +49 711 1111 12\n"
		"\n"
		"Yours sincerely,\n"
		"-- <enter your name and email address here>\n")
      .arg( KGlobal::locale()->formatDate( QDate::currentDate().addDays( 1 ) ) );
  }

  int Vacation::defaultNotificationInterval() {
    return 7; // days
  }

  QStringList Vacation::defaultMailAliases() {
    QStringList sl;
    for ( KPIM::IdentityManager::ConstIterator it = kmkernel->identityManager()->begin() ;
	  it != kmkernel->identityManager()->end() ; ++it )
      if ( !(*it).emailAddr().isEmpty() )
	sl.push_back( (*it).emailAddr() );
    return sl;
  }


  void Vacation::slotGetResult( SieveJob * job, bool success,
				const QString & script, bool active ) {
    kdDebug(5006) << "Vacation::slotGetResult( ??, " << success
	      << ", ?, " << active << " )" << endl
	      << "script:" << endl
	      << script << endl;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    if ( mUrl.protocol() == "sieve" && !job->sieveCapabilities().isEmpty() &&
	 !job->sieveCapabilities().contains("vacation") ) {
      KMessageBox::sorry( 0, i18n("Your server did not list \"vacation\" in "
				  "its list of supported Sieve extensions;\n"
				  "without it, KMail cannot install out-of-"
				  "office replies for you.\n"
				  "Please contact you system administrator.") );
      emit result( false );
      return;
    }

    if ( !mDialog )
      mDialog = new VacationDialog( i18n("Configure \"Out of Office\" Replies"), 0, 0, false );

    QString messageText = defaultMessageText();
    int notificationInterval = defaultNotificationInterval();
    QStringList aliases = defaultMailAliases();
    if ( !success ) active = false; // default to inactive

    if ( !success || !parseScript( script, messageText, notificationInterval, aliases ) )
      KMessageBox::information( 0, i18n("Someone (probably you) changed the "
					"vacation script on the server.\n"
					"KMail is no longer able to determine "
					"the parameters for the autoreplies.\n"
					"Default values will be used." ) );

    mWasActive = active;
    mDialog->setActivateVacation( active );
    mDialog->setMessageText( messageText );
    mDialog->setNotificationInterval( notificationInterval );
    mDialog->setMailAliases( aliases.join(", ") );

    connect( mDialog, SIGNAL(okClicked()), SLOT(slotDialogOk()) );
    connect( mDialog, SIGNAL(cancelClicked()), SLOT(slotDialogCancel()) );
    connect( mDialog, SIGNAL(defaultClicked()), SLOT(slotDialogDefaults()) );

    mDialog->show();
  }

  void Vacation::slotDialogDefaults() {
    if ( !mDialog )
      return;
    mDialog->setActivateVacation( true );
    mDialog->setMessageText( defaultMessageText() );
    mDialog->setNotificationInterval( defaultNotificationInterval() );
    mDialog->setMailAliases( defaultMailAliases().join(", ") );
  }

  void Vacation::slotDialogOk() {
    kdDebug(5006) << "Vacation::slotDialogOk()" << endl;
    // compose a new script:
    const QString script = composeScript( mDialog->messageText(),
				    mDialog->notificationInterval(),
				    mDialog->mailAliases() );
    const bool active = mDialog->activateVacation();

    kdDebug(5006) << "script:" << endl << script << endl;

    // and commit the dialog's settings to the server:
    mSieveJob = SieveJob::put( mUrl, script, active, mWasActive );
    connect( mSieveJob, SIGNAL(result(KMail::SieveJob*,bool,const QString&,bool)),
	     active
	     ? SLOT(slotPutActiveResult(KMail::SieveJob*,bool))
	     : SLOT(slotPutInactiveResult(KMail::SieveJob*,bool)) );

    // destroy the dialog:
    mDialog->delayedDestruct();
    mDialog = 0;
  }

  void Vacation::slotDialogCancel() {
    kdDebug(5006) << "Vacation::slotDialogCancel()" << endl;
    mDialog->delayedDestruct();
    mDialog = 0;
    emit result( false );
  }

  void Vacation::slotPutActiveResult( SieveJob * job, bool success ) {
    handlePutResult( job, success, true );
  }

  void Vacation::slotPutInactiveResult( SieveJob * job, bool success ) {
    handlePutResult( job, success, false );
  }

  void Vacation::handlePutResult( SieveJob *, bool success, bool activated ) {
    if ( success )
      KMessageBox::information( 0, activated
				? i18n("Sieve script installed successfully on the server.\n"
				       "Out of Office reply is now active.")
				: i18n("Sieve script installed successfully on the server.\n"
				       "Out of Office reply has been deactivated.") );

    kdDebug(5006) << "Vacation::handlePutResult( ???, " << success << ", ? )"
		  << endl;
    mSieveJob = 0; // job deletes itself after returning from this slot!
    emit result( success );
  }


} // namespace KMail

#include "vacation.moc"
