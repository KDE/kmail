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
        kDebug() ;
    }

    virtual ~SieveDebugDataExtractor()
    {
        kDebug() ;
    }

private:
    void commandStart( const QString & identifier )
    {
        kDebug() << "Identifier: '" << identifier <<"'";
        reset();
    }

    void commandEnd()
    {
        kDebug() ;
    }

    void testStart( const QString & )
    {
        kDebug() ;
    }

    void testEnd()
    {
        kDebug() ;
    }

    void testListStart()
    {
        kDebug() ;
    }

    void testListEnd()
    {
        kDebug() ;
    }

    void blockStart()
    {
        kDebug() ;
    }

    void blockEnd()
    {
        kDebug() ;
    }

    void hashComment( const QString & )
    {
        kDebug() ;
    }

    void bracketComment( const QString & )
    {
        kDebug() ;
    }

    void lineFeed()
    {
        kDebug() ;
    }

    void error( const KSieve::Error & e )
    {
        kDebug() << "###" <<"Error:" <<
            e.asString() << "@" << e.line() << "," << e.column();
    }

    void finished()
    {
        kDebug() ;
    }

    void taggedArgument( const QString & tag )
    {
        kDebug() << "Tag: '" << tag <<"'";
    }

    void stringArgument( const QString & string, bool, const QString & )
    {
        kDebug() << "String: '" << string <<"'";
    }

    void numberArgument( unsigned long number, char )
    {
        kDebug() << "Number:" << number;
    }

    void stringListArgumentStart()
    {
        kDebug() ;
    }

    void stringListEntry( const QString & string, bool, const QString & )
    {
        kDebug() << "String: '" << string <<"'";
    }

    void stringListArgumentEnd()
    {
        kDebug() ;
    }

private:
    void reset()
    {
        kDebug() ;
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
    kDebug() ;
}

static KUrl urlFromAccount( const KMail::ImapAccountBase * a ) {
    const SieveConfig sieve = a->sieveConfig();
    if ( !sieve.managesieveSupported() )
        return KUrl();

    KUrl u;
    if ( sieve.reuseConfig() ) {
        // assemble Sieve url from the settings of the account:
        u.setProtocol( "sieve" );
        u.setHost( a->host() );
        u.setUser( a->login() );
        u.setPass( a->passwd() );
        u.setPort( sieve.port() );

        // Translate IMAP LOGIN to PLAIN:
        u.addQueryItem( "x-mech", a->auth() == "*" ? "PLAIN" : a->auth() );
        if ( !a->useSSL() && !a->useTLS() )
            u.addQueryItem( "x-allow-unencrypted", "true" );
    } else {
        u = sieve.alternateURL();
        if ( u.protocol().toLower() == "sieve" && !a->useSSL() && !a->useTLS() && u.queryItem("x-allow-unencrypted").isEmpty() )
            u.addQueryItem( "x-allow-unencrypted", "true" );
    }
    return u;
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
        const KUrl url = urlFromAccount( mAccountBase );
        if ( !url.isValid() )
        {
            mEdit->append( i18n( "(Account does not support Sieve)\n\n" ) );
        } else {
            mUrl = url;

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
    mUrl = urlFromAccount( mAccountBase );
    mUrl.setFileName( scriptFile );

    mSieveJob = SieveJob::get( mUrl );

    connect( mSieveJob, SIGNAL( gotScript( KMail::SieveJob *, bool, const QString &, bool ) ),
        SLOT( slotGetScript( KMail::SieveJob *, bool, const QString &, bool ) ) );
}

void SieveDebugDialog::slotGetScript( SieveJob * /* job */, bool success,
    const QString &script, bool active )
{
    kDebug() << "( ??," << success
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
    kDebug() << "Success:" << success <<", List:" << scriptList.join("," ) <<
        ", active:" << activeScript;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    mEdit->append( i18n( "Sieve capabilities:\n" ) );
    const QStringList caps = job->sieveCapabilities();
    if ( caps.isEmpty() )
    {
        mEdit->append( i18n( "(No special capabilities available)" ) );
    }
    else
    {
        for ( QStringList::const_iterator it = caps.constBegin(); it != caps.constEnd(); ++it )
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
    kDebug();
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

    kDebug() << "( ???," << success <<", ? )";
    mSieveJob = 0; // job deletes itself after returning from this slot!
}


} // namespace KMail

#include "sievedebugdialog.moc"

#endif // NDEBUG

