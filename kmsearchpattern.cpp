// kmsearchpattern.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL!

#include <config.h>

#include "kmsearchpattern.h"
#include "kmmessage.h"

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qregexp.h>

#include <mimelib/string.h>
#include <mimelib/boyermor.h>

#include <assert.h>

static const char* funcConfigNames[] =
  { "contains", "contains-not", "equals", "not-equal", "regexp",
    "not-regexp", "greater", "less-or-equal", "less", "greater-or-equal" };
static const int numFuncConfigNames = sizeof funcConfigNames / sizeof *funcConfigNames;


//==================================================
//
// class KMSearchRule (was: KMFilterRule)
//
//==================================================

KMSearchRule::KMSearchRule() {
  init();
}


KMSearchRule::~KMSearchRule() {
  delete mBmHeaderField;
  delete mBmEndHeader;
  delete mBmEndHeaders2;
  delete mBmEndHeaders1;
}


void KMSearchRule::init( const QCString & aField, Function aFunction,
			 const QString & aContents )
{
  mField    = aField;
  mFunction = aFunction;
  mContents = aContents;
  if ( mField.isEmpty() )
      mBmHeaderField = 0;
  else //TODO handle the unrealistic case of the message starting with mField
      mBmHeaderField = new DwBoyerMoore(("\n" + mField + ": ").data()); 	
  mBmEndHeaders1 = new DwBoyerMoore("\n\n");
  mBmEndHeaders2 = new DwBoyerMoore("\n\r\n");
  mBmEndHeader = new DwBoyerMoore("\n");
}


void KMSearchRule::init( const KMSearchRule * aRule ) {
  if ( aRule )
    init( aRule->field(), aRule->function(), aRule->contents() );
  else
    init ( 0, FuncContains, 0 );
}


void KMSearchRule::init( const QCString & aField, const char * aStrFunction,
			 const QString & aContents )
{
  init ( aField, configValueToFunc( aStrFunction ), aContents );
}


KMSearchRule::Function KMSearchRule::configValueToFunc( const char * str ) {
  if ( !str ) return FuncContains;

  for ( int i = 0 ; i < numFuncConfigNames ; ++i )
    if ( qstricmp( funcConfigNames[i], str ) == 0 ) return (Function)i;

  return FuncContains;
}


bool KMSearchRule::matches( const DwString & aStr, KMMessage & msg,
			    DwBoyerMoore * aHeaderField, int aHeaderLen ) const
{
  DwBoyerMoore * headerField = aHeaderField ? aHeaderField : mBmHeaderField ;
  if ( !headerField )
    return false;

  const int headerLen = ( aHeaderLen > -1 ? aHeaderLen : mField.length() ) + 2 ; // +1 for ': '

  if ( aHeaderField || ( mField.length() && mField[0] != '<' ) ) {
    size_t endOfHeader = mBmEndHeaders1->FindIn( aStr, 0 );
    if ( endOfHeader == DwString::npos )
      endOfHeader = mBmEndHeaders2->FindIn( aStr, 0 );
    const DwString headers = ( endOfHeader == DwString::npos ) ? aStr : aStr.substr( 0, endOfHeader );
    size_t start = headerField->FindIn( headers, 0 );
    if ( start == DwString::npos )
      return false;
    start += headerLen;
    size_t stop = mBmEndHeader->FindIn( aStr, start );
    char ch = '\0';
    while ( stop != DwString::npos && ( ch = aStr.at( stop + 1 ) ) == ' ' || ch == '\t' )
      stop = mBmEndHeader->FindIn( aStr, stop + 1 );
    const int len = stop == DwString::npos ? aStr.length() - start : stop - start ;
    const QCString codedValue( aStr.data() + start, len + 1 );
    const QString msgContents = KMMsgBase::decodeRFC2047String( codedValue ).stripWhiteSpace();
    return matches( false, 0, 0, msgContents );
  } else if ( mField == "<recipients>" ) {
    DwBoyerMoore to("\nTo: ");
    bool res = matches( aStr, msg, &to, 2 );
    if ( !res ) {
      DwBoyerMoore cc("\nCc: ");
      res = matches( aStr, msg, &cc, 2 );
    }
    return res;
  } else {
    if ( !msg.isComplete() ) {
      msg.fromDwString( aStr );
      msg.setComplete( true );
    }
    return matches( &msg );
  }
}


bool KMSearchRule::matches( const KMMessage * msg ) const {
  assert( msg );

  QString msgContents;
  int numericalMsgContents = 0;
  int numericalValue = 0;
  bool numerical = false;

  if( mField == "<message>" ) {
    msgContents = msg->asString();
  } else if ( mField == "<body>" ) {
    msgContents = msg->bodyDecoded();
  } else if ( mField == "<any header>" ) {
    msgContents = msg->headerAsString();
  } else if ( mField == "<recipients>" ) {
    // (mmutz 2001-11-05) hack to fix "<recipients> !contains foo" to
    // meet user's expectations. See FAQ entry in KDE 2.2.2's KMail
    // handbook
    if ( mFunction == KMSearchRule::FuncEquals
	 || mFunction == KMSearchRule::FuncNotEqual )
      // do we need to treat this case specially? Ie.: What shall
      // "equality" mean for recipients.
      return matches( false, 0, 0, msg->headerField("To") )
        || matches( false, 0, 0, msg->headerField("Cc") );
    //
    msgContents = msg->headerField("To") + msg->headerField("Cc");
  } else if ( mField == "<size>" ) {
    numerical = true;
    numericalMsgContents = int( msg->msgLength() );
    numericalValue = mContents.toInt(); // isEmpty() checks this
    msgContents.setNum( numericalMsgContents );
  } else if ( mField == "<age in days>" ) {
    numerical = true;
    QDateTime msgDateTime;
    msgDateTime.setTime_t( msg->date() );
    numericalMsgContents = msgDateTime.daysTo( QDateTime::currentDateTime() );
    numericalValue = mContents.toInt(); // isEmpty() checks this
    msgContents.setNum( numericalMsgContents );
  } else {
    msgContents = msg->headerField( mField );
  }
  return matches( numerical, numericalValue, numericalMsgContents, msgContents );
}


bool KMSearchRule::matches( bool numerical, unsigned long numericalValue, unsigned long numericalMsgContents, const QString & msgContents ) const
{
  // also see KMFldSearchRule::matches() for a similar function:
  switch ( mFunction ) {
  case KMSearchRule::FuncEquals:
    if ( numerical )
      return ( numericalValue == numericalMsgContents );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) == 0 );

  case KMSearchRule::FuncNotEqual:
    if ( numerical )
      return ( numericalValue != numericalMsgContents );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) != 0 );

  case KMSearchRule::FuncContains:
    return ( msgContents.find( mContents, 0, false ) >= 0 );

  case KMSearchRule::FuncContainsNot:
    return ( msgContents.find( mContents, 0, false ) < 0 );

  case KMSearchRule::FuncRegExp:
    {
      QRegExp regexp( mContents, false );
      return ( regexp.search( msgContents ) >= 0 );
    }

  case KMSearchRule::FuncNotRegExp:
    {
      QRegExp regexp( mContents, false );
      return ( regexp.search( msgContents ) < 0 );
    }

  case FuncIsGreater:
    if ( numerical )
      return ( numericalMsgContents > numericalValue );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) > 0 );

  case FuncIsLessOrEqual:
    if ( numerical )
      return ( numericalMsgContents <= numericalValue );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) <= 0 );

  case FuncIsLess:
    if ( numerical )
      return ( numericalMsgContents < numericalValue );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) < 0 );

  case FuncIsGreaterOrEqual:
    if ( numerical )
      return ( numericalMsgContents >= numericalValue );
    else
      return ( QString::compare( msgContents.lower(), mContents.lower() ) >= 0 );
  }

  return false;
}


void KMSearchRule::readConfig( const KConfig * config, int aIdx ) {
  const char cIdx = char( int('A') + aIdx );

  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  const QString requestedField = config->readEntry( field + cIdx );
  if ( requestedField == "<To or Cc>" ) // backwards compat
    init( "<recipients>",
	  config->readEntry( func + cIdx ).latin1(),
	  config->readEntry( contents + cIdx ) );
  else
    init( config->readEntry( field + cIdx  ).latin1(),
	  config->readEntry( func + cIdx ).latin1(),
	  config->readEntry( contents + cIdx ) );
}


void KMSearchRule::writeConfig( KConfig * config, int aIdx ) const {
  const char cIdx = char('A' + aIdx);
  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  config->writeEntry( field + cIdx, QString(mField) );
  config->writeEntry( func + cIdx, funcConfigNames[(int)mFunction] );
  config->writeEntry( contents + cIdx, mContents );
}


bool KMSearchRule::isEmpty() const {
  bool ok = false;

  if ( mField == "<size>" || mField == "<age in days>" )
    mContents.toInt( &ok );
  else
    ok = true;

  return mField.stripWhiteSpace().isEmpty()
    || mContents.isEmpty() || !ok;
}


#ifndef NDEBUG
const QString KMSearchRule::asString() const
{
  QString result  = "\"" + mField + "\" <";
  result += funcConfigNames[(int)mFunction];
  result += "> \"" + mContents + "\"";

  return result;
}
#endif


//==================================================
//
// class KMSearchPattern
//
//==================================================

KMSearchPattern::KMSearchPattern( const KConfig * config )
  : QPtrList<KMSearchRule>()
{
  setAutoDelete( true );
  if ( config )
    readConfig( config );
  else
    init();
}


bool KMSearchPattern::matches( const KMMessage * msg ) const {
  QPtrListIterator<KMSearchRule> it( *this );
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it.toFirst() ; it.current() ; ++it )
      if ( !(*it)->matches( msg ) )
	return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it.toFirst() ; it.current() ; ++it )
      if ( (*it)->matches( msg ) )
	return true;
    // fall through
  default:
    return false;
  }
}

bool KMSearchPattern::matches( const DwString & aStr ) const
{
  KMMessage msg;
  QPtrListIterator<KMSearchRule> it( *this );
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it.toFirst() ; it.current() ; ++it )
      if ( !(*it)->matches( aStr, msg ) )
	return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it.toFirst() ; it.current() ; ++it )
      if ( (*it)->matches( aStr, msg ) )
	return true;
    // fall through
  default:
    return false;
  }
}

void KMSearchPattern::purify() {
  QPtrListIterator<KMSearchRule> it( *this );
  it.toLast();
  while ( it.current() )
    if ( (*it)->isEmpty() ) {
#ifndef NDEBUG
      kdDebug(5006) << "KMSearchPattern::purify(): removing " << (*it)->asString() << endl;
#endif
      remove( *it );
    } else {
      --it;
    }
}

void KMSearchPattern::readConfig( const KConfig * config ) {
  init();

  mName = config->readEntry("name");
  if ( !config->hasKey("rules") ) {
    kdDebug(5006) << "KMSearchPattern::readConfig: found legacy config! Converting." << endl;
    importLegacyConfig( config );
    return;
  }

  mOperator = config->readEntry("operator") == "or" ? OpOr : OpAnd;

  const int nRules = config->readNumEntry( "rules", 0 );

  for ( int i = 0 ; i < nRules ; i++ ) {
    KMSearchRule * r = new KMSearchRule();
    r->readConfig( config, i );
    if ( r->isEmpty() )
      delete r;
    else
      append( r );
  }
}

void KMSearchPattern::importLegacyConfig( const KConfig * config ) {
  KMSearchRule * rule = new KMSearchRule();
  rule->init( config->readEntry("fieldA").latin1(),
	      config->readEntry("funcA").latin1(),
	      config->readEntry("contentsA") );
  if ( rule->isEmpty() ) {
    // if the first rule is invalid,
    // we really can't do much heuristics...
    delete rule;
    return;
  }
  append( rule );

  const QString sOperator = config->readEntry("operator");
  if ( sOperator == "ignore" ) return;

  rule = new KMSearchRule();
  rule->init( config->readEntry("fieldB").latin1(),
	      config->readEntry("funcB").latin1(),
	      config->readEntry("contentsB") );
  if ( rule->isEmpty() ) {
    delete rule;
    return;
  }
  append( rule );

  if ( sOperator == "or"  ) {
    mOperator = OpOr;
    return;
  }
  // This is the interesting case...
  if ( sOperator == "unless" ) { // meaning "and not", ie we need to...
    // ...invert the function (e.g. "equals" <-> "doesn't equal")
    // We simply toggle the last bit (xor with 0x1)... This aasumes that
    // KMSearchRule::Function's come in adjacent pairs of pros and cons
    KMSearchRule::Function func = last()->function();
    unsigned int intFunc = (unsigned int)func;
    func = KMSearchRule::Function( intFunc ^ 0x1 );

    last()->setFunction( func );
  }

  // treat any other case as "and" (our default).
}

void KMSearchPattern::writeConfig( KConfig * config ) const {
  config->writeEntry("name", mName );
  config->writeEntry("operator", (mOperator == KMSearchPattern::OpOr) ? "or" : "and" );

  int i = 0;
  for ( QPtrListIterator<KMSearchRule> it( *this ) ; it.current() && i < FILTER_MAX_RULES ; ++i , ++it )
    // we could do this ourselves, but we want the rules to be extensible,
    // so we give the rule it's number and let it do the rest.
    (*it)->writeConfig( config, i );

  // save the total number of rules.
  config->writeEntry( "rules", i );
}

void KMSearchPattern::init() {
  clear();
  mOperator = OpAnd;
  mName = '<' + i18n("name used for a virgin filter","unknown") + '>';
}

#ifndef NDEBUG
QString KMSearchPattern::asString() const {
  QString result = "Match ";
  result += ( mOperator == OpOr ) ? "any" : "all";
  result += " of the following:\n";

  for ( QPtrListIterator<KMSearchRule> it( *this ) ; it.current() ; ++it )
    result += (*it)->asString() + '\n';

  return result;
}
#endif

const KMSearchPattern & KMSearchPattern::operator=( const KMSearchPattern & aPattern )
{
  setOp( aPattern.op() );
  setName( aPattern.name() );

  for ( QPtrListIterator<KMSearchRule> it( aPattern ) ; it.current() ; ++it ) {
    KMSearchRule * r = new KMSearchRule;
    r->init( *it ); // deep copy
    append( r );
  }
  return *this;
}
