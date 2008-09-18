/*
    sievedebugdialog.cpp

    KMail, the KDE mail client.
    Copyright (c) 2005 Martijn Klingens <klingens@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

// This file is only compiled when debug is enabled, it is
// not useful enough for non-developers to have this in releases.
#if !defined(NDEBUG)


#include "sievedebugdialog.h"

#include <cassert>
#include <limits.h>

#include <QDateTime>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktextedit.h>

#include <kmime/kmime_header_parsing.h>
#include <ksieve/error.h>
#include <ksieve/parser.h>
#include <ksieve/scriptbuilder.h>
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>

#include "kmacctimap.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmkernel.h"
#include "sievejob.h"

using KMail::SieveJob;
using KMime::Types::AddrSpecList;

namespace
{

class SieveDebugDataExtractor : public KSieve::ScriptBuilder
{
    enum Context
    {
        None = 0,

        // command itself:
        SieveDebugCommand,

        // tagged args:
        Days, Addresses
    };

public:
    SieveDebugDataExtractor()
    :   KSieve::ScriptBuilder()
    {
        kDebug( 5006 ) ;
    }

    virtual ~SieveDebugDataExtractor()
    {
        kDebug( 5006 ) ;
    }

private:
    void commandStart( const QString & identifier )
    {
        kDebug( 5006 ) <<"Identifier: '" << identifier <<"'";
        reset();
    }

    void commandEnd()
    {
        kDebug( 5006 ) ;
    }

    void testStart( const QString & )
    {
        kDebug( 5006 ) ;
    }

    void testEnd()
    {
        kDebug( 5006 ) ;
    }

    void testListStart()
    {
        kDebug( 5006 ) ;
    }

    void testListEnd()
    {
        kDebug( 5006 ) ;
    }

    void blockStart()
    {
        kDebug( 5006 ) ;
    }

    void blockEnd()
    {
        kDebug( 5006 ) ;
    }

    void hashComment( const QString & )
    {
        kDebug( 5006 ) ;
    }

    void bracketComment( const QString & )
    {
        kDebug( 5006 ) ;
    }

    void lineFeed()
    {
        kDebug( 5006 ) ;
    }

    void error( const KSieve::Error & e )
    {
        kDebug( 5006 ) <<"###" <<"Error:" <<
            e.asString() << "@" << e.line() << "," << e.column();
    }

    void finished()
    {
        kDebug( 5006 ) ;
    }

    void taggedArgument( const QString & tag )
    {
        kDebug( 5006 ) <<"Tag: '" << tag <<"'";
    }

    void stringArgument( const QString & string, bool, const QString & )
    {
        kDebug( 5006 ) <<"String: '" << string <<"'";
    }

    void numberArgument( unsigned long number, char )
    {
        kDebug( 5006 ) <<"Number:" << number;
    }

    void stringListArgumentStart()
    {
        kDebug( 5006 ) ;
    }

    void stringListEntry( const QString & string, bool, const QString & )
    {
        kDebug( 5006 ) <<"String: '" << string <<"'";
    }

    void stringListArgumentEnd()
    {
        kDebug( 5006 ) ;
    }

private:
    void reset()
    {
        kDebug( 5006 ) ;
    }
};

} // Anon namespace

namespace KMail
{

SieveDebugDialog::SieveDebugDialog( QWidget *parent )
:   KDialog( parent ),
    mSieveJob( 0 )
{
    setCaption( i18n( "Sieve Diagnostics" ) );
    setButtons( Ok );
    // Collect all accounts
    AccountManager *am = kmkernel->acctMgr();
    assert( am );
    QList<KMAccount*>::iterator accountIt = am->begin();
    while ( accountIt != am->end() ) {
      KMAccount *account = *accountIt;
      ++accountIt;
      mAccountList.append( account );
    }

    mEdit = new KTextEdit( this );
    mEdit->setReadOnly( true );
    setMainWidget( mEdit );

    mEdit->setText( i18n( "Collecting diagnostic information about Sieve support...\n\n" ) );

    setInitialSize( QSize( 640, 480 ) );

    if ( !mAccountList.isEmpty() )
        QTimer::singleShot( 0, this, SLOT( slotDiagNextAccount() ) );
}

SieveDebugDialog::~SieveDebugDialog()
{
    if ( mSieveJob )
    {
        mSieveJob->kill();
        mSieveJob = 0;
    }
    kDebug( 5006 ) ;
}

void SieveDebugDialog::slotDiagNextAccount()
{
    if ( mAccountList.isEmpty() )
        return;

    KMAccount *acc = mAccountList.first();
    mAccountList.pop_front();

    mEdit->append( i18n( "Collecting data for account '%1'...\n", acc->name() ) );
    mEdit->append( i18n( "------------------------------------------------------------\n" ) );
    mAccountBase = dynamic_cast<KMail::ImapAccountBase *>( acc );
    if ( mAccountBase )
    {
        // Detect URL for this IMAP account
        SieveConfig sieve = mAccountBase->sieveConfig();
        if ( !sieve.managesieveSupported() )
        {
            mEdit->append( i18n( "(Account does not support Sieve)\n\n" ) );
        } else {
            if ( sieve.reuseConfig() )
            {
                // assemble Sieve url from the settings of the account:
                mUrl.setProtocol( "sieve" );
                mUrl.setHost( mAccountBase->host() );
                mUrl.setUser( mAccountBase->login() );
                mUrl.setPass( mAccountBase->passwd() );
                mUrl.setPort( sieve.port() );

                // Translate IMAP LOGIN to PLAIN:
                mUrl.setQuery( "x-mech=" + ( mAccountBase->auth() == "*" ? "PLAIN" : mAccountBase->auth() ) );
            } else {
                sieve.alternateURL();
                mUrl.setFileName( sieve.vacationFileName() );
            }

            mSieveJob = SieveJob::list( mUrl );

            connect( mSieveJob, SIGNAL( gotList( KMail::SieveJob *, bool, const QStringList &, const QString & ) ),
                SLOT( slotGetScriptList( KMail::SieveJob *, bool, const QStringList &, const QString & ) ) );

            // Bypass the singleShot timer -- it's fired when we get our data
            return;
        }
    } else {
        mEdit->append( i18n( "(Account is not an IMAP account)\n\n" ) );
    }

    // Handle next account async
    QTimer::singleShot( 0, this, SLOT( slotDiagNextAccount() ) );
}

void SieveDebugDialog::slotDiagNextScript()
{
    if ( mScriptList.isEmpty() )
    {
        // Continue handling accounts instead
        mScriptList.clear();
        QTimer::singleShot( 0, this, SLOT( slotDiagNextAccount() ) );
        return;
    }

    QString scriptFile = mScriptList.first();
    mScriptList.pop_front();

    mEdit->append( i18n( "Contents of script '%1':\n", scriptFile ) );
    SieveConfig sieve = mAccountBase->sieveConfig();
    if ( sieve.reuseConfig() )
    {
        // assemble Sieve url from the settings of the account:
        mUrl.setProtocol( "sieve" );
        mUrl.setHost( mAccountBase->host() );
        mUrl.setUser( mAccountBase->login() );
        mUrl.setPass( mAccountBase->passwd() );
        mUrl.setPort( sieve.port() );
        // Translate IMAP LOGIN to PLAIN
        mUrl.setQuery( "x-mech=" + ( mAccountBase->auth() == "*" ? "PLAIN" : mAccountBase->auth() ) );
        mUrl.setFileName( scriptFile );
    } else {
        sieve.alternateURL();
        mUrl.setFileName( scriptFile );
    }

    mSieveJob = SieveJob::get( mUrl );

    connect( mSieveJob, SIGNAL( gotScript( KMail::SieveJob *, bool, const QString &, bool ) ),
        SLOT( slotGetScript( KMail::SieveJob *, bool, const QString &, bool ) ) );
}

void SieveDebugDialog::slotGetScript( SieveJob * /* job */, bool success,
    const QString &script, bool active )
{
    kDebug( 5006 ) << "( ??," << success
                   << ", ?," << active << ")" << endl
                   << "script:" << endl
                   << script;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    if ( script.isEmpty() )
    {
        mEdit->append( i18n( "(This script is empty.)\n\n" ) );
    }
    else
    {
        mEdit->append( i18n(
            "------------------------------------------------------------\n"
            "%1\n"
            "------------------------------------------------------------\n\n", script ) );
    }

    // Fetch next script
    QTimer::singleShot( 0, this, SLOT( slotDiagNextScript() ) );
}

void SieveDebugDialog::slotGetScriptList( SieveJob *job, bool success,
    const QStringList &scriptList, const QString &activeScript )
{
    kDebug( 5006 ) <<"Success:" << success <<", List:" << scriptList.join("," ) <<
        ", active:" << activeScript;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    mEdit->append( i18n( "Sieve capabilities:\n" ) );
    QStringList caps = job->sieveCapabilities();
    if ( caps.isEmpty() )
    {
        mEdit->append( i18n( "(No special capabilities available)" ) );
    }
    else
    {
        for ( QStringList::const_iterator it = caps.begin(); it != caps.end(); ++it )
            mEdit->append( "* " + *it + '\n' );
        mEdit->append( "\n" );
    }

    mEdit->append( i18n( "Available Sieve scripts:\n" ) );

    if ( scriptList.isEmpty() )
    {
        mEdit->append( i18n( "(No Sieve scripts available on this server)\n\n" ) );
    }
    else
    {
        mScriptList = scriptList;
        for ( QStringList::const_iterator it = scriptList.begin(); it != scriptList.end(); ++it )
            mEdit->append( "* " + *it + '\n' );
        mEdit->append( "\n" );
        mEdit->append( i18n( "Active script: %1\n\n", activeScript ) );
    }

    // Handle next job: dump scripts for this server
    QTimer::singleShot( 0, this, SLOT( slotDiagNextScript() ) );
}

void SieveDebugDialog::slotDialogOk()
{
    kDebug(5006);
}

void SieveDebugDialog::slotPutActiveResult( SieveJob * job, bool success )
{
    handlePutResult( job, success, true );
}

void SieveDebugDialog::slotPutInactiveResult( SieveJob * job, bool success )
{
    handlePutResult( job, success, false );
}

void SieveDebugDialog::handlePutResult( SieveJob *, bool success, bool activated )
{
    if ( success )
    {
        KMessageBox::information( 0, activated ? i18n(
            "Sieve script installed successfully on the server.\n"
            "Out of Office reply is now active." )
            : i18n( "Sieve script installed successfully on the server.\n"
            "Out of Office reply has been deactivated." ) );
    }

    kDebug( 5006 ) << "( ???," << success <<", ? )";
    mSieveJob = 0; // job deletes itself after returning from this slot!
}


} // namespace KMail

#include "sievedebugdialog.moc"

#endif // NDEBUG

