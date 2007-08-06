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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sievedebugdialog.h"

#include <cassert>
#include <limits.h>

#include <qdatetime.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kmime_header_parsing.h>
#include <ksieve/error.h>
#include <ksieve/parser.h>
#include <ksieve/scriptbuilder.h>
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>

#include "kmacctimap.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "kmkernel.h"
#include "sievejob.h"
#include <qtextedit.h>

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
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    virtual ~SieveDebugDataExtractor()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

private:
    void commandStart( const QString & identifier )
    {
        kdDebug( 5006 ) << k_funcinfo << "Identifier: '" << identifier << "'" << endl;
        reset();
    }

    void commandEnd()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void testStart( const QString & )
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void testEnd()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void testListStart()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void testListEnd()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void blockStart()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void blockEnd()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void hashComment( const QString & )
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void bracketComment( const QString & )
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void lineFeed()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void error( const KSieve::Error & e )
    {
        kdDebug( 5006 ) << "### " << k_funcinfo << "Error: " <<
            e.asString() << " @ " << e.line() << "," << e.column() << endl;
    }

    void finished()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void taggedArgument( const QString & tag )
    {
        kdDebug( 5006 ) << k_funcinfo << "Tag: '" << tag << "'" << endl;
    }

    void stringArgument( const QString & string, bool, const QString & )
    {
        kdDebug( 5006 ) << k_funcinfo << "String: '" << string << "'" << endl;
    }

    void numberArgument( unsigned long number, char )
    {
        kdDebug( 5006 ) << k_funcinfo << "Number: " << number << endl;
    }

    void stringListArgumentStart()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

    void stringListEntry( const QString & string, bool, const QString & )
    {
        kdDebug( 5006 ) << k_funcinfo << "String: '" << string << "'" << endl;
    }

    void stringListArgumentEnd()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }

private:
    void reset()
    {
        kdDebug( 5006 ) << k_funcinfo << endl;
    }
};

} // Anon namespace

namespace KMail
{

SieveDebugDialog::SieveDebugDialog( QWidget *parent, const char *name )
:   KDialogBase( parent, name, true, i18n( "Sieve Diagnostics" ), KDialogBase::Ok,
    KDialogBase::Ok, true ),
    mSieveJob( 0 )
{
    // Collect all accounts
    AccountManager *am = kmkernel->acctMgr();
    assert( am );
    for ( KMAccount *a = am->first(); a; a = am->next() )
        mAccountList.append( a );

    mEdit = new QTextEdit( this );
    mEdit->setReadOnly(true);
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
    kdDebug( 5006 ) << k_funcinfo << endl;
}

void SieveDebugDialog::slotDiagNextAccount()
{
    if ( mAccountList.isEmpty() )
        return;

    KMAccount *acc = mAccountList.first();
    mAccountList.pop_front();

    mEdit->append( i18n( "Collecting data for account '%1'...\n" ).arg( acc->name() ) );
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

    mEdit->append( i18n( "Contents of script '%1':\n" ).arg( scriptFile ) );
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
    kdDebug( 5006 ) << "SieveDebugDialog::slotGetScript( ??, " << success
              << ", ?, " << active << " )" << endl
              << "script:" << endl
              << script << endl;
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
            "------------------------------------------------------------\n\n" ).arg( script ) );
    }

    // Fetch next script
    QTimer::singleShot( 0, this, SLOT( slotDiagNextScript() ) );
}

void SieveDebugDialog::slotGetScriptList( SieveJob *job, bool success,
    const QStringList &scriptList, const QString &activeScript )
{
    kdDebug( 5006 ) << k_funcinfo << "Success: " << success << ", List: " << scriptList.join( ", " ) <<
        ", active: " << activeScript << endl;
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
            mEdit->append( "* " + *it + "\n" );
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
            mEdit->append( "* " + *it + "\n" );
        mEdit->append( "\n" );
        mEdit->append( i18n( "Active script: %1\n\n" ).arg( activeScript ) );
    }

    // Handle next job: dump scripts for this server
    QTimer::singleShot( 0, this, SLOT( slotDiagNextScript() ) );
}

void SieveDebugDialog::slotDialogOk()
{
    kdDebug(5006) << "SieveDebugDialog::slotDialogOk()" << endl;
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

    kdDebug( 5006 ) << "SieveDebugDialog::handlePutResult( ???, " << success << ", ? )" << endl;
    mSieveJob = 0; // job deletes itself after returning from this slot!
}


} // namespace KMail

#include "sievedebugdialog.moc"

#endif // NDEBUG

