/*  -*- c++ -*-
    vacation.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "vacation.h"
#include <limits.h>

#include "imapsettings.h"
#include "vacationdialog.h"
#include "sievejob.h"
using KMail::SieveJob;
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "kmagentmanager.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include "globalsettings.h"

#include <kmime/kmime_header_parsing.h>
using KMime::Types::AddrSpecList;

#include <ksieve/parser.h>
#include <ksieve/scriptbuilder.h>
#include <ksieve/error.h>

#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include "util.h"

#include <akonadi/agentinstance.h>

#include <kimap/loginjob.h>


#include <QByteArray>
#include <QDateTime>

#include <cassert>
#include <vector>
#include <map>
#include <set>

namespace KSieveExt {

  class MultiScriptBuilder : public KSieve::ScriptBuilder {
    std::vector<KSieve::ScriptBuilder*> mBuilders;
  public:
    MultiScriptBuilder() : KSieve::ScriptBuilder() {}
    MultiScriptBuilder( KSieve::ScriptBuilder * sb1 )
      : KSieve::ScriptBuilder(), mBuilders( 1 )
    {
      mBuilders[0] = sb1;
      assert( sb1 );
    }
    MultiScriptBuilder( KSieve::ScriptBuilder * sb1,
                        KSieve::ScriptBuilder * sb2 )
      : KSieve::ScriptBuilder(), mBuilders( 2 )
    {
      mBuilders[0] = sb1;
      mBuilders[1] = sb2;
      assert( sb1 ); assert( sb2 );
    }
    MultiScriptBuilder( KSieve::ScriptBuilder * sb1,
                        KSieve::ScriptBuilder * sb2,
                        KSieve::ScriptBuilder * sb3 )
      : KSieve::ScriptBuilder(), mBuilders( 3 )
    {
      mBuilders[0] = sb1;
      mBuilders[1] = sb2;
      mBuilders[2] = sb3;
      assert( sb1 ); assert( sb2 ); assert( sb3 );
    }
    ~MultiScriptBuilder() {}
  private:
#ifdef FOREACH
#undef FOREACH
#endif
#define FOREACH for ( std::vector<KSieve::ScriptBuilder*>::const_iterator it = mBuilders.begin(), end = mBuilders.end() ; it != end ; ++it ) (*it)->
    void commandStart( const QString & identifier ) { FOREACH commandStart( identifier ); }
    void commandEnd() { FOREACH commandEnd(); }
    void testStart( const QString & identifier ) { FOREACH testStart( identifier ); }
    void testEnd() { FOREACH testEnd(); }
    void testListStart() { FOREACH testListStart(); }
    void testListEnd() { FOREACH testListEnd(); }
    void blockStart() { FOREACH blockStart(); }
    void blockEnd() { FOREACH blockEnd(); }
    void hashComment( const QString & comment ) { FOREACH hashComment( comment ); }
    void bracketComment( const QString & comment ) { FOREACH bracketComment( comment ); }
    void lineFeed() { FOREACH lineFeed(); }
    void error( const KSieve::Error & e ) { FOREACH error( e ); }
    void finished() { FOREACH finished(); }
    void taggedArgument( const QString & tag ) { FOREACH taggedArgument( tag ); }
    void stringArgument( const QString & string, bool multiline, const QString & fixme ) { FOREACH stringArgument( string, multiline, fixme ); }
    void numberArgument( unsigned long number, char quantifier ) { FOREACH numberArgument( number, quantifier ); }
    void stringListArgumentStart() { FOREACH stringListArgumentStart(); }
    void stringListEntry( const QString & string, bool multiline, const QString & fixme) { FOREACH stringListEntry( string, multiline, fixme ); }
    void stringListArgumentEnd() { FOREACH stringListArgumentEnd(); }
#undef FOREACH
  };

}

namespace {

  class GenericInformationExtractor : public KSieve::ScriptBuilder {
  public:
    enum BuilderMethod {
      Any,
      TaggedArgument,
      StringArgument,
      NumberArgument,
      CommandStart,
      CommandEnd,
      TestStart,
      TestEnd,
      TestListStart,
      TestListEnd,
      BlockStart,
      BlockEnd,
      StringListArgumentStart,
      StringListEntry,
      StringListArgumentEnd
    };

    struct StateNode {
      // expectation:
      int depth;
      BuilderMethod method;
      const char * string;
      // actions:
      int if_found;
      int if_not_found;
      const char * save_tag;
    };

    const std::vector<StateNode> mNodes;
    std::map<QString,QString> mResults;
    std::set<unsigned int> mRecursionGuard;
    unsigned int mState;
    int mNestingDepth;

  public:
    GenericInformationExtractor( const std::vector<StateNode> & nodes )
      : KSieve::ScriptBuilder(), mNodes( nodes ), mState( 0 ), mNestingDepth( 0 ) {}

    const std::map<QString,QString> & results() const { return mResults; }

  private:
    void process( BuilderMethod method, const QString & string=QString() ) {
      doProcess( method, string );
      mRecursionGuard.clear();
    }
    void doProcess( BuilderMethod method, const QString & string ) {
      mRecursionGuard.insert( mState );
      bool found = true;
      const StateNode & expected = mNodes[mState];
      if ( expected.depth != -1 && mNestingDepth != expected.depth )
        found = false;
      if ( expected.method != Any && method != expected.method )
        found = false;
      if ( const char * str = expected.string )
        if ( string.toLower() != QString::fromUtf8( str ).toLower() )
          found = false;
      kDebug() << ( found ?"found:" :"not found:" )
                   << mState << "->"
                   << ( found ? expected.if_found : expected.if_not_found );
      mState = found ? expected.if_found : expected.if_not_found ;
      assert( mState < mNodes.size() );
      if ( found )
        if ( const char * save_tag = expected.save_tag )
          mResults[save_tag] = string;
      if ( !found && !mRecursionGuard.count( mState ) ) {
        doProcess( method, string );
      }
    }
    void commandStart( const QString & identifier ) { kDebug() ; process( CommandStart, identifier ); }
    void commandEnd() { kDebug() ; process( CommandEnd ); }
    void testStart( const QString & identifier ) { kDebug() ; process( TestStart, identifier ); }
    void testEnd() { kDebug() ; process( TestEnd ); }
    void testListStart() { kDebug() ; process( TestListStart ); }
    void testListEnd() { kDebug() ; process( TestListEnd ); }
    void blockStart() { kDebug() ; process( BlockStart ); ++mNestingDepth; }
    void blockEnd() { kDebug() ; --mNestingDepth; process( BlockEnd ); }
    void hashComment( const QString & ) { kDebug() ; }
    void bracketComment( const QString & ) { kDebug() ; }
    void lineFeed() { kDebug() ; }
    void error( const KSieve::Error & ) {
      kDebug() ;
      mState = 0;
    }
    void finished() { kDebug() ; }

    void taggedArgument( const QString & tag ) { kDebug() ; process( TaggedArgument, tag ); }
    void stringArgument( const QString & string, bool, const QString & ) { kDebug() ; process( StringArgument, string ); }
    void numberArgument( unsigned long number, char ) { kDebug(); process( NumberArgument, QString::number( number ) ); }
    void stringListArgumentStart() { kDebug() ; process( StringListArgumentStart ); }
    void stringListEntry( const QString & string, bool, const QString & ) { kDebug() ; process( StringListEntry, string ); }
    void stringListArgumentEnd() { kDebug() ; process( StringListArgumentEnd ); }
  };

  typedef GenericInformationExtractor GIE;
  static const GenericInformationExtractor::StateNode spamNodes[] = {
    { 0, GIE::CommandStart, "if",  1, 0, 0 },              // 0
    { 0,   GIE::TestStart, "header", 2, 0, 0 },            // 1
    { 0,     GIE::TaggedArgument, "contains", 3, 0, 0 },   // 2

    // accept both string and string-list:
    { 0,     GIE::StringArgument, "x-spam-flag", 9, 4, "x-spam-flag" },    // 3
    { 0,     GIE::StringListArgumentStart, 0, 5, 0, 0 },                   // 4
    { 0,       GIE::StringListEntry, "x-spam-flag", 6, 7, "x-spam-flag" }, // 5
    { 0,       GIE::StringListEntry, 0, 6, 8, 0 },                         // 6
    { 0,     GIE::StringListArgumentEnd, 0, 0, 5, 0 },                     // 7
    { 0,     GIE::StringListArgumentEnd, 0, 9, 0, 0 },                     // 8

    // accept both string and string-list:
    { 0,     GIE::StringArgument, "yes", 15, 10, "spam-flag-yes" },    // 9
    { 0,     GIE::StringListArgumentStart, 0, 11, 0, 0 },              // 10
    { 0,       GIE::StringListEntry, "yes", 12, 13, "spam-flag-yes" }, // 11
    { 0,       GIE::StringListEntry, 0, 12, 14, 0 },                   // 12
    { 0,     GIE::StringListArgumentEnd, 0, 0, 11, 0 },                // 13
    { 0,     GIE::StringListArgumentEnd, 0, 15, 0, 0 },                // 14

    { 0,   GIE::TestEnd, 0, 16, 0, 0 }, // 15

    // block of command, find "stop", take nested if's into account:
    { 0,   GIE::BlockStart, 0, 17, 0, 0 },                // 16
    { 1,     GIE::CommandStart, "stop", 20, 19, "stop" }, // 17
    { -1,    GIE::Any, 0, 17, 0, 0 },                     // 18
    { 0,   GIE::BlockEnd, 0, 0, 18, 0 },                  // 19

    { -1, GIE::Any, 0, 20, 20, 0 }, // 20 end state
  };
  static const unsigned int numSpamNodes = sizeof spamNodes / sizeof *spamNodes ;

  class SpamDataExtractor : public GenericInformationExtractor {
  public:
    SpamDataExtractor()
      : GenericInformationExtractor( std::vector<StateNode>( spamNodes, spamNodes + numSpamNodes ) )
    {

    }

    bool found() const {
      return mResults.count( "x-spam-flag" ) &&
        mResults.count( "spam-flag-yes" ) &&
        mResults.count( "stop" ) ;
    }
  };

  // to understand this table, study the output of
  // libksieve/tests/parsertest
  //   'if not address :domain :contains ["from"] ["mydomain.org"] { keep; stop; }'
  static const GenericInformationExtractor::StateNode domainNodes[] = {
    { 0, GIE::CommandStart, "if", 1, 0, 0 },       // 0
    { 0,   GIE::TestStart, "not", 2, 0, 0, },      // 1
    { 0,     GIE::TestStart, "address", 3, 0, 0 }, // 2

    // :domain and :contains in arbitrary order:
    { 0,       GIE::TaggedArgument, "domain", 4, 5, 0 },     // 3
    { 0,       GIE::TaggedArgument, "contains", 7, 0, 0 },   // 4
    { 0,       GIE::TaggedArgument, "contains", 6, 0, 0 },   // 5
    { 0,       GIE::TaggedArgument, "domain", 7, 0, 0 },     // 6

    // accept both string and string-list:
    { 0,       GIE::StringArgument, "from", 13, 8, "from" },     // 7
    { 0,       GIE::StringListArgumentStart, 0, 9, 0, 0 },       // 8
    { 0,         GIE::StringListEntry, "from", 10, 11, "from" }, // 9
    { 0,         GIE::StringListEntry, 0, 10, 12, 0 },           // 10
    { 0,       GIE::StringListArgumentEnd, 0, 0, 9, 0 },         // 11
    { 0,       GIE::StringListArgumentEnd, 0, 13, 0, 0 },        // 12

    // string: save, string-list: save last
    { 0,       GIE::StringArgument, 0, 17, 14, "domainName" },    // 13
    { 0,       GIE::StringListArgumentStart, 0, 15, 0, 0 },       // 14
    { 0,         GIE::StringListEntry, 0, 15, 16, "domainName" }, // 15
    { 0,       GIE::StringListArgumentEnd, 0, 17, 0, 0 },         // 16

    { 0,     GIE::TestEnd, 0, 18, 0, 0 },  // 17
    { 0,   GIE::TestEnd, 0, 19, 0, 0 },    // 18

    // block of commands, find "stop", take nested if's into account:
    { 0,   GIE::BlockStart, 0, 20, 0, 0 },                 // 19
    { 1,     GIE::CommandStart, "stop", 23, 22, "stop" },  // 20
    { -1,    GIE::Any, 0, 20, 0, 0 },                      // 21
    { 0,   GIE::BlockEnd, 0, 0, 21, 0 },                   // 22

    { -1, GIE::Any, 0, 23, 23, 0 }  // 23 end state
  };
  static const unsigned int numDomainNodes = sizeof domainNodes / sizeof *domainNodes ;

  class DomainRestrictionDataExtractor : public GenericInformationExtractor {
  public:
    DomainRestrictionDataExtractor()
      : GenericInformationExtractor( std::vector<StateNode>( domainNodes, domainNodes+numDomainNodes ) )
    {

    }

    QString domainName() /*not const, since map::op[] isn't const*/ {
      return mResults.count( "stop" ) && mResults.count( "from" )
        ? mResults["domainName"] : QString();
    }
  };

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
      kDebug();
    }
    virtual ~VacationDataExtractor() {}

    int notificationInterval() const { return mNotificationInterval; }
    const QString & messageText() const { return mMessageText; }
    const QStringList & aliases() const { return mAliases; }

  private:
    void commandStart( const QString & identifier ) {
      kDebug() << "( \"" << identifier <<"\" )";
      if ( identifier != "vacation" )
        return;
      reset();
      mContext = VacationCommand;
    }

    void commandEnd() {
      kDebug();
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
      kDebug() << e.asString() << "@" << e.line() << "," << e.column();
    }
    void finished() {}

    void taggedArgument( const QString & tag ) {
      kDebug() << "( \"" << tag <<"\" )";
      if ( mContext != VacationCommand )
        return;
      if ( tag == "days" )
        mContext = Days;
      else if ( tag == "addresses" )
        mContext = Addresses;
    }

    void stringArgument( const QString & string, bool, const QString & ) {
      kDebug() << "( \"" << string <<"\" )";
      if ( mContext == Addresses ) {
        mAliases.push_back( string );
        mContext = VacationCommand;
      } else if ( mContext == VacationCommand ) {
        mMessageText = string;
        mContext = VacationCommand;
      }
    }

    void numberArgument( unsigned long number, char ) {
      kDebug() << "( \"" << number <<"\" )";
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
      kDebug() << "( \"" << string <<"\" )";
      if ( mContext != Addresses )
        return;
      mAliases.push_back( string );
    }
    void stringListArgumentEnd() {
      kDebug();
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
      kDebug();
      mContext = None;
      mNotificationInterval = 0;
      mAliases.clear();
      mMessageText.clear();
    }
  };

}

namespace KMail {

  Vacation::Vacation( QObject * parent, bool checkOnly, const char * name )
    : QObject( parent ), mSieveJob( 0 ), mDialog( 0 ), mWasActive( false ),
      mCheckOnly( checkOnly )
  {
    setObjectName( name );
    mUrl = findURL();
    kDebug() << "Vacation: found url \"" << mUrl.prettyUrl() <<"\"";
    if ( mUrl.isEmpty() ) // nothing to do...
      return;
    mSieveJob = SieveJob::get( mUrl );
    if (checkOnly) {
      mSieveJob->setInteractive( false );
    }
    connect( mSieveJob, SIGNAL(gotScript(KMail::SieveJob*,bool,const QString&,bool)),
	     SLOT(slotGetResult(KMail::SieveJob*,bool,const QString&,bool)) );
  }

  Vacation::~Vacation() {
    if ( mSieveJob ) mSieveJob->kill(); mSieveJob = 0;
    delete mDialog; mDialog = 0;
    kDebug() << "~Vacation()";
  }

  static inline QString dotstuff( QString s ) { // krazy:exclude=passbyvalue
    if ( s.startsWith( '.' ) )
      return '.' + s.replace( "\n.", "\n.." );
    else
      return s.replace( "\n.", "\n.." );
  }

  QString Vacation::composeScript( const QString & messageText,
                                   int notificationInterval,
                                   const AddrSpecList & addrSpecs,
                                   bool sendForSpam, const QString & domain )
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
    QString script = QString::fromLatin1("require \"vacation\";\n\n" );
    if ( !sendForSpam )
      script += QString::fromLatin1( "if header :contains \"X-Spam-Flag\" \"YES\""
                                     " { keep; stop; }\n" ); // FIXME?

    if ( !domain.isEmpty() ) // FIXME
      script += QString::fromLatin1( "if not address :domain :contains \"from\" \"%1\" { keep; stop; }\n" ).arg( domain );

    script += "vacation ";
    script += addressesArgument;
    if ( notificationInterval > 0 )
      script += QString::fromLatin1(":days %1 ").arg( notificationInterval );
    script += QString::fromLatin1("text:\n");
    script += dotstuff( messageText.isEmpty() ? defaultMessageText() : messageText );
    script += QString::fromLatin1( "\n.\n;\n" );
    return script;
  }


  KUrl Vacation::findURL() const {
    Akonadi::AgentInstance::List lst = kmkernel->agentManager()->instanceList();
    foreach ( const Akonadi::AgentInstance& type, lst )
    {
      //TODO verify it.
      if ( type.identifier().contains( "akonadi_imap_resource" ) ) {
        OrgKdeAkonadiImapSettingsInterface *iface = KMail::Util::createImapSettingsInterface(type.identifier());
        if ( iface->isValid() ) {
          KUrl u = KMail::Util::findSieveUrlForAccount( iface );
          if ( !u.isEmpty() ) {
            delete iface;
            return u;
          }
        }
        delete iface;
      }
    }
    return KUrl();
  }

  bool Vacation::parseScript( const QString & script, QString & messageText,
                              int & notificationInterval, QStringList & aliases,
                              bool & sendForSpam, QString & domainName ) {
    if ( script.trimmed().isEmpty() ) {
      messageText = defaultMessageText();
      notificationInterval = defaultNotificationInterval();
      aliases = defaultMailAliases();
      sendForSpam = defaultSendForSpam();
      domainName = defaultDomainName();
      return true;
    }

    // The trimmed() call below prevents parsing errors. The
    // slave somehow omits the last \n, which results in a lone \r at
    // the end, leading to a parse error.
    const QByteArray scriptUTF8 = script.trimmed().toUtf8();
    kDebug() << "scriptUtf8 = \"" + scriptUTF8 +"\"";
    KSieve::Parser parser( scriptUTF8.begin(),
                           scriptUTF8.begin() + scriptUTF8.length() );
    VacationDataExtractor vdx;
    SpamDataExtractor sdx;
    DomainRestrictionDataExtractor drdx;
    KSieveExt::MultiScriptBuilder tsb( &vdx, &sdx, &drdx );
    parser.setScriptBuilder( &tsb );
    if ( !parser.parse() )
      return false;
    messageText = vdx.messageText().trimmed();
    notificationInterval = vdx.notificationInterval();
    aliases = vdx.aliases();
    if ( !GlobalSettings::allowOutOfOfficeUploadButNoSettings() ) {
      sendForSpam = !sdx.found();
      domainName = drdx.domainName();
    }
    return true;
  }

  QString Vacation::defaultMessageText() {
    return i18n( "I am out of office till %1.\n"
                 "\n"
                 "In urgent cases, please contact Mrs. <placeholder>vacation replacement</placeholder>\n"
                 "\n"
                 "email: <placeholder>email address of vacation replacement</placeholder>\n"
                 "phone: +49 711 1111 11\n"
                 "fax.:  +49 711 1111 12\n"
                 "\n"
                 "Yours sincerely,\n"
                 "-- <placeholder>enter your name and email address here</placeholder>\n",
        KGlobal::locale()->formatDate( QDate::currentDate().addDays( 1 ) ) );
  }

  int Vacation::defaultNotificationInterval() {
    return 7; // days
  }

  QStringList Vacation::defaultMailAliases() {
    QStringList sl;
    for ( KPIMIdentities::IdentityManager::ConstIterator it = kmkernel->identityManager()->begin() ;
          it != kmkernel->identityManager()->end() ; ++it )
      if ( !(*it).emailAddr().isEmpty() )
        sl.push_back( (*it).emailAddr() );
    return sl;
  }

  bool Vacation::defaultSendForSpam() {
    return GlobalSettings::outOfOfficeReactToSpam();
  }

  QString Vacation::defaultDomainName() {
    return GlobalSettings::outOfOfficeDomain();
  }

  void Vacation::slotGetResult( SieveJob * job, bool success,
                                const QString & script, bool active ) {
    kDebug() << success
                 << ", ?," << active << ")" << endl
                 << "script:" << endl
                 << script;
    mSieveJob = 0; // job deletes itself after returning from this slot!

    if ( !mCheckOnly && mUrl.protocol() == "sieve" && !job->sieveCapabilities().isEmpty() &&
         !job->sieveCapabilities().contains("vacation") ) {
      KMessageBox::sorry( 0, i18n( "Your server did not list \"vacation\" in "
                                   "its list of supported Sieve extensions;\n"
                                   "without it, KMail cannot install out-of-"
                                   "office replies for you.\n"
                                   "Please contact you system administrator." ) );
      emit result( false );
      return;
    }

    if ( !mDialog && !mCheckOnly )
      mDialog = new VacationDialog( i18n("Configure \"Out of Office\" Replies"), 0, 0, false );

    QString messageText = defaultMessageText();
    int notificationInterval = defaultNotificationInterval();
    QStringList aliases = defaultMailAliases();
    bool sendForSpam = defaultSendForSpam();
    QString domainName = defaultDomainName();
    if ( !success ) active = false; // default to inactive

    if ( !mCheckOnly && ( !success || !parseScript( script, messageText, notificationInterval, aliases, sendForSpam, domainName ) ) )
      KMessageBox::information( 0, i18n("Someone (probably you) changed the "
                                        "vacation script on the server.\n"
                                        "KMail is no longer able to determine "
                                        "the parameters for the autoreplies.\n"
                                        "Default values will be used." ) );

    mWasActive = active;
    if ( mDialog ) {
      mDialog->setActivateVacation( active );
      mDialog->setMessageText( messageText );
      mDialog->setNotificationInterval( notificationInterval );
      mDialog->setMailAliases( aliases.join(", ") );
      mDialog->setSendForSpam( sendForSpam );
      mDialog->setDomainName( domainName );
      mDialog->enableDomainAndSendForSpam( !GlobalSettings::allowOutOfOfficeUploadButNoSettings() );

      connect( mDialog, SIGNAL(okClicked()), SLOT(slotDialogOk()) );
      connect( mDialog, SIGNAL(cancelClicked()), SLOT(slotDialogCancel()) );
      connect( mDialog, SIGNAL(defaultClicked()), SLOT(slotDialogDefaults()) );

      mDialog->show();
    }

    emit scriptActive( mWasActive );
    if ( mCheckOnly && mWasActive ) {
      if ( KMessageBox::questionYesNo( 0, i18n( "There is still an active out-of-office reply configured.\n"
                                        "Do you want to edit it?"), i18n("Out-of-office reply still active"),
                                        KGuiItem( i18n( "Edit"), "document-properties" ),
                                        KGuiItem( i18n("Ignore"), "dialog-cancel" ) )
           == KMessageBox::Yes ) {
        kmkernel->getKMMainWidget()->slotEditVacation();
      }
    }
  }

  void Vacation::slotDialogDefaults() {
    if ( !mDialog )
      return;
    mDialog->setActivateVacation( true );
    mDialog->setMessageText( defaultMessageText() );
    mDialog->setNotificationInterval( defaultNotificationInterval() );
    mDialog->setMailAliases( defaultMailAliases().join(", ") );
    mDialog->setSendForSpam( defaultSendForSpam() );
    mDialog->setDomainName( defaultDomainName() );
  }

  void Vacation::slotDialogOk() {
    kDebug();
    // compose a new script:
    const QString script = composeScript( mDialog->messageText(),
                                          mDialog->notificationInterval(),
                                          mDialog->mailAliases(),
                                          mDialog->sendForSpam(),
                                          mDialog->domainName() );
    const bool active = mDialog->activateVacation();
    emit scriptActive( active );

    kDebug() << "script:" << endl << script;

    // and commit the dialog's settings to the server:
    mSieveJob = SieveJob::put( mUrl, script, active, mWasActive );
    connect( mSieveJob, SIGNAL(gotScript(KMail::SieveJob*,bool,const QString&,bool)),
             active
                 ? SLOT(slotPutActiveResult(KMail::SieveJob*,bool))
                 : SLOT(slotPutInactiveResult(KMail::SieveJob*,bool)) );

    // destroy the dialog:
    mDialog->delayedDestruct();
    mDialog = 0;
  }

  void Vacation::slotDialogCancel() {
    kDebug();
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

    kDebug() << "( ???," << success << ", ? )";
    mSieveJob = 0; // job deletes itself after returning from this slot!
    emit result( success );
    emit scriptActive( activated );
  }


} // namespace KMail

#include "vacation.moc"
