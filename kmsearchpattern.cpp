// kmsearchpattern.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL!

#include <config.h>
#include <mimelib/boyermor.h>
#include "kmsearchpattern.h"
#include "kmmessage.h"

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qregexp.h>

#include <assert.h>

static const char* funcConfigNames[] =
  { "contains", "contains-not", "equals", "not-equal", "regexp",
    "not-regexp", "greater", "less-or-equal", "less", "greater-or-equal", 0 };


//-----------------------------------------------------------------------------
// Case-insensitive. Searches for string in string list and returns index
// or -1 if not found.
static int findInStrList(const char* strList[], const char* str)
{
  int i;

  //assert(strList != 0);
  if(!strList)
    {
      kdDebug(5006) << "KMFilter::findInStrList() : strList == 0\n" << endl;
      return -1; // we return -1 here. Fake unsuccessfull search
    }

  if (!str) return -1;

  for (i=0; strList[i]; i++)
    if (qstricmp(strList[i], str)==0) return i;

  return -1;
}

//==================================================
//
// class KMSearchRule (was: KMFilterRule)
//
//==================================================

KMSearchRule::KMSearchRule()
{
  init();
}


//-----------------------------------------------------------------------------
void KMSearchRule::init(const QCString aField, Function aFunction,
			const QString aContents)
{
  mField    = aField;
  mFunction = aFunction;
  mContents = aContents;
  if (mField.isEmpty())
      mBmHeaderField = 0;
  else //TODO handle the unrealistic case of the message starting with mField
      mBmHeaderField = new DwBoyerMoore(("\n" + mField + ": ").data()); 	
  mBmEndHeaders1 = new DwBoyerMoore("\n\n");
  mBmEndHeaders2 = new DwBoyerMoore("\n\r\n");
  mBmEndHeader = new DwBoyerMoore("\n");
}

//-----------------------------------------------------------------------------
void KMSearchRule::init(const KMSearchRule* aRule)
{
  if (aRule)
    init( aRule->field(), aRule->function(), aRule->contents() );
  else
    init ( "", FuncContains, "" );
}

//-----------------------------------------------------------------------------
void KMSearchRule::init(const QCString aField, const char* aStrFunction,
			const QString aContents)
{
  int intFunc = findInStrList( funcConfigNames, aStrFunction );
  Function func = Function( ( intFunc >= 0 ) ? (Function)intFunc : FuncContains );
  init ( aField, func, aContents );
}

//-----------------------------------------------------------------------------
bool KMSearchRule::matches(const DwString &aStr, KMMessage &msg,
			   DwBoyerMoore *aHeaderField, int aHeaderLen) const
{
    DwBoyerMoore *headerField = mBmHeaderField;
    int headerLen = mField.length() + 2; // +1 for ': '
    if (aHeaderLen > -1)
	headerLen = aHeaderLen + 2;
    if (aHeaderField)
	headerField = aHeaderField;
    if (!headerField)
	return false;
    if (aHeaderField || (mField.length() && mField[0] != '<')) {
	QString msgContents;
	int start, stop, len;
	char ch = '\0';
	int endOfHeader = mBmEndHeaders1->FindIn(aStr, 0);
	if (endOfHeader == -1)
	    endOfHeader = mBmEndHeaders2->FindIn(aStr, 0);
	DwString headers = (endOfHeader < 0) ? aStr : aStr.substr(0, endOfHeader);
	start = headerField->FindIn(headers, 0);
	if (start == -1)
	    return false;
	start += headerLen;
	stop = mBmEndHeader->FindIn(aStr, start);
	while (stop != -1 && (ch = aStr.at(stop + 1)) == ' ' || ch == '\t')
	    stop = mBmEndHeader->FindIn(aStr, stop + 1);
	if (stop == -1)
	    len = aStr.length() - start;
	else
	    len = stop - start;
	QCString codedValue(aStr.data() + start, len + 1);
	msgContents = KMMsgBase::decodeRFC2047String(codedValue);
	return matches( false, 0, 0, msgContents );
    } else if (mField == "<recipients>") {
	bool res;
	DwBoyerMoore to("\nTo: ");
	DwBoyerMoore cc("\nCc: ");
	res = matches(aStr, msg, &to, 2);
	if (!res)
	    res = matches(aStr, msg, &cc, 2);
	return res;
    } else {
	if (!msg.isComplete()) {
	    msg.fromDwString(aStr);
	    msg.setComplete(true);
	}
	return matches(&msg);
    }
}

//-----------------------------------------------------------------------------
bool KMSearchRule::matches(const KMMessage* msg) const
{
  QString msgContents;
  int numericalMsgContents = 0;
  int numericalValue = 0;
  bool numerical = FALSE;

  assert(msg != 0); // This assert seems to be important

  if( mField == "<message>" ) {
    // there's msg->asString(), but this way we can keep msg const (dnaber, 1999-05-27)
    msgContents = msg->headerAsString();
    msgContents += msg->bodyDecoded();
  } else if( mField == "<body>" ) {
    msgContents = msg->bodyDecoded();
  } else if( mField == "<any header>" ) {
    msgContents = msg->headerAsString();
  } else if( mField == "<recipients>" ) {
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
  } else if( mField == "<size in bytes>" ) {
    numerical = TRUE;
    numericalMsgContents = int(msg->msgLength());
    numericalValue = mContents.toInt(); // isEmpty() checks this
    msgContents.setNum( numericalMsgContents );
  } else if( mField == "<age in days>" ) {
    numerical = TRUE;
    QDateTime msgDateTime;
    msgDateTime.setTime_t( msg->date() );
    numericalMsgContents = msgDateTime.daysTo( QDateTime::currentDateTime() );
    numericalValue = mContents.toInt(); // isEmpty() checks this
    msgContents.setNum( numericalMsgContents );
  } else {
    msgContents = msg->headerField(mField);
  }
  return matches( numerical, numericalValue, numericalMsgContents, msgContents );
}

bool KMSearchRule::matches( bool numerical, unsigned long numericalValue, unsigned long numericalMsgContents, QString msgContents ) const
{
  // also see KMFldSearchRule::matches() for a similar function:
  switch (mFunction)
  {
  case KMSearchRule::FuncEquals:
    if (numerical)
      return ( numericalValue == numericalMsgContents );
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) == 0);

  case KMSearchRule::FuncNotEqual:
    if (numerical)
      return ( numericalValue != numericalMsgContents );
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) != 0);

  case KMSearchRule::FuncContains:
    return ( msgContents.find(mContents, 0, FALSE) >= 0 );

  case KMSearchRule::FuncContainsNot:
    return ( msgContents.find(mContents, 0, FALSE) < 0 );

  case KMSearchRule::FuncRegExp:
    {
      QRegExp regexp(mContents, FALSE);
      return (regexp.search( msgContents ) >= 0);
    }

  case KMSearchRule::FuncNotRegExp:
    {
      QRegExp regexp(mContents, FALSE);
      return (regexp.search( msgContents ) < 0);
    }

  case FuncIsGreater:
    if (numerical)
      return (numericalMsgContents > numericalValue);
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) > 0);

  case FuncIsLessOrEqual:
    if (numerical)
      return (numericalMsgContents <= numericalValue);
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) <= 0);

  case FuncIsLess:
    if (numerical)
      return (numericalMsgContents < numericalValue);
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) < 0);

  case FuncIsGreaterOrEqual:
    if (numerical)
      return (numericalMsgContents >= numericalValue);
    else
      return (QString::compare(msgContents.lower(), mContents.lower()) >= 0);
  }

  return FALSE;
}

void KMSearchRule::readConfig( KConfig *config, int aIdx )
{
  char cIdx = char( int('A') + aIdx );
  static const QString& field = KGlobal::staticQString( "field" );
  static const QString& func = KGlobal::staticQString( "func" );
  static const QString& contents = KGlobal::staticQString( "contents" );

  QString requestedField = config->readEntry( field + cIdx );
  if ( requestedField == "<To or Cc>" ) // backwards compat
    init( "<recipients>",
	  config->readEntry( func + cIdx ).latin1(),
	  config->readEntry( contents + cIdx ) );
  else
    init( config->readEntry( field + cIdx  ).latin1(),
	  config->readEntry( func + cIdx ).latin1(),
	  config->readEntry( contents + cIdx ) );
}


//-----------------------------------------------------------------------------
void KMSearchRule::writeConfig( KConfig *config, int aIdx ) const
{
  char cIdx = char('A' + aIdx);
  static const QString& field = KGlobal::staticQString( "field" );
  static const QString& func = KGlobal::staticQString( "func" );
  static const QString& contents = KGlobal::staticQString( "contents" );

  config->writeEntry( field + cIdx, QString(mField) );
  config->writeEntry( func + cIdx, funcConfigNames[(int)mFunction] );
  config->writeEntry( contents + cIdx, mContents );
}

//-----------------------------------------------------------------------------
bool KMSearchRule::isEmpty() const
{
  bool ok;

  if ( mField == "<size in bytes>" || mField == "<age in days>" ) {
    ok = FALSE;
    mContents.toInt(&ok);
  } else
    ok = TRUE;

  return mField.stripWhiteSpace().isEmpty()
    || mContents.isEmpty() || !ok;
}

//-----------------------------------------------------------------------------
const QString KMSearchRule::asString() const
{
  QString result;

  result  = "\"" + mField + "\" ";
  result += funcConfigNames[(int)mFunction];
  result += " \"" + mContents + "\"";

  return result;
}


//==================================================
//
// class KMSearchPattern
//
//==================================================

KMSearchPattern::KMSearchPattern( KConfig *config )
  : QPtrList<KMSearchRule>()
{
  setAutoDelete(TRUE);
  if (config)
    readConfig(config);
  else
    init();
}


bool KMSearchPattern::matches( const KMMessage *msg ) const
{
  QPtrListIterator<KMSearchRule> it(*this);
  switch (mOperator ) {
    case OpAnd: // all rules must match
      for ( it.toFirst() ; it.current() ; ++it )
	if ( !(*it)->matches(msg) )
	  return FALSE;
      return TRUE;
    case OpOr:  // at least one rule must match
      for ( it.toFirst() ; it.current() ; ++it )
	if ( (*it)->matches(msg) )
	  return TRUE;
      // fall through
    default:
      return FALSE;
  }
}

bool KMSearchPattern::matches( const DwString &aStr ) const
{
  KMMessage msg;
  QPtrListIterator<KMSearchRule> it(*this);
  switch (mOperator ) {
    case OpAnd: // all rules must match
      for ( it.toFirst() ; it.current() ; ++it )
	if ( !(*it)->matches(aStr, msg) )
	  return FALSE;
      return TRUE;
    case OpOr:  // at least one rule must match
      for ( it.toFirst() ; it.current() ; ++it )
	if ( (*it)->matches(aStr, msg) )
	  return TRUE;
      // fall through
    default:
      return FALSE;
  }
}

void KMSearchPattern::purify()
{
  QPtrListIterator<KMSearchRule> it(*this);
  it.toLast();
  while ( it.current() )
    if ( (*it)->isEmpty() ) {
      kdDebug(5006) << "KMSearchPattern::purify(): removing " << (*it)->asString() << endl;
      remove( *it );
    } else {
      --it;
    }
}

void KMSearchPattern::readConfig( KConfig *config )
{
  init();

  mName = config->readEntry("name");
  if ( !config->hasKey("rules") ) {
    kdDebug(5006) << "KMSearchPattern::readConfig: found legacy config! Converting." << endl;
    importLegacyConfig(config);
    return;
  }

  mOperator = config->readEntry("operator") == "or" ? OpOr : OpAnd;

  KMSearchRule *r;
  int nRules = config->readNumEntry("rules",0);

  for ( int i = 0 ; i < nRules ; i++ ) {
    r = new KMSearchRule();
    r->readConfig(config,i);
    if ( r->isEmpty() )
      delete r;
    else
      append( r );
  }
}

void KMSearchPattern::importLegacyConfig( KConfig *config )
{
  KMSearchRule *rule = new KMSearchRule();
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

  QString sOperator = config->readEntry("operator");
  if ( sOperator == "ignore" ) return;

  rule = new KMSearchRule();
  rule->init( config->readEntry("fieldB").latin1(),
	      config->readEntry("funcB").latin1(), // or is local8Bit better?
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

void KMSearchPattern::writeConfig( KConfig *config ) const
{
  int i;

  config->writeEntry("name", mName );
  config->writeEntry("operator", (mOperator == KMSearchPattern::OpOr) ? "or" : "and" );

  QPtrListIterator<KMSearchRule> it(*this);
  for ( i=0, it.toFirst() ; it.current() && i < FILTER_MAX_RULES ; ++i , ++it ) {
    // we could do this ourselves, but we want the rules to be extensible,
    // so we give the rule it's number and let it do the rest.
    (*it)->writeConfig(config,i);
  }
  // save the total number of rules.
  config->writeEntry("rules", i);
}

void KMSearchPattern::init()
{
  mOperator = KMSearchPattern::OpAnd;
  mName = "<" + i18n("name used for a virgin filter","unknown") + ">";
  clear();
}

const QString KMSearchPattern::asString() const
{
  QString result;

  result += "Match ";
  result += ( mOperator == OpOr ) ? "any" : "all";
  result += " of the following:\n";

  QPtrListIterator<KMSearchRule> it( *this );
  for ( it.toFirst() ; it.current() ; ++it )
    result += (*it)->asString() + "\n";

  return result;
}

KMSearchPattern& KMSearchPattern::operator=( const KMSearchPattern & aPattern )
{
  setOp( aPattern.op() );
  setName( aPattern.name() );

  QPtrListIterator<KMSearchRule> it( aPattern );
  for ( it.toFirst() ; it.current() ; ++it ) {
    KMSearchRule *r = new KMSearchRule;
    r->init( (*it) ); // deep copy
    append( r );
  }
  return *this;
}
