// -*- mode: C++; c-file-style: "gnu" -*-
// kmsearchpattern.cpp
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL!

#include <config.h>

#include "kmaddrbook.h"
#include "kmsearchpattern.h"
#include "kmmsgdict.h"
#include "filterlog.h"
using KMail::FilterLog;
#include "kmkernel.h"
#include "kmmsgdict.h"
#include "kmfolder.h"

#include <emailfunctions/email.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <kabc/stdaddressbook.h>

#include <QRegExp>
#include <QByteArray>

#include <mimelib/string.h>
#include <mimelib/boyermor.h>

#include <assert.h>

static const char* funcConfigNames[] =
  { "contains", "contains-not", "equals", "not-equal", "regexp",
    "not-regexp", "greater", "less-or-equal", "less", "greater-or-equal",
    "is-in-addressbook", "is-not-in-addressbook" , "is-in-category", "is-not-in-category",
    "has-attachment", "has-no-attachment"};
static const int numFuncConfigNames = sizeof funcConfigNames / sizeof *funcConfigNames;

struct _statusNames {
  const char* name;
  MessageStatus status;
};

static struct _statusNames statusNames[] = {
  { "Important", MessageStatus::statusImportant() },
  { "New", MessageStatus::statusNew() },
  { "Unread", MessageStatus::statusNewAndUnread() },
  { "Read", MessageStatus::statusRead() },
  { "Old", MessageStatus::statusOld() },
  { "Deleted", MessageStatus::statusDeleted() },
  { "Replied", MessageStatus::statusReplied() },
  { "Forwarded", MessageStatus::statusForwarded() },
  { "Queued", MessageStatus::statusQueued() },
  { "Sent", MessageStatus::statusSent() },
  { "Watched", MessageStatus::statusWatched() },
  { "Ignored", MessageStatus::statusIgnored() },
  { "To Do", MessageStatus::statusTodo() },
  { "Spam", MessageStatus::statusSpam() },
  { "Ham", MessageStatus::statusHam() },
  { "Has Attachment", MessageStatus::statusHasAttachment() }
};

static const int numStatusNames = sizeof statusNames / sizeof ( struct _statusNames );


//==================================================
//
// class KMSearchRule (was: KMFilterRule)
//
//==================================================

KMSearchRule::KMSearchRule( const QByteArray & field, Function func, const QString & contents )
  : mField( field ),
    mFunction( func ),
    mContents( contents )
{
}

KMSearchRule::KMSearchRule( const KMSearchRule & other )
  : mField( other.mField ),
    mFunction( other.mFunction ),
    mContents( other.mContents )
{
}

const KMSearchRule & KMSearchRule::operator=( const KMSearchRule & other ) {
  if ( this == &other )
    return *this;

  mField = other.mField;
  mFunction = other.mFunction;
  mContents = other.mContents;

  return *this;
}

KMSearchRule * KMSearchRule::createInstance( const QByteArray & field,
                                             Function func,
                                             const QString & contents )
{
  KMSearchRule *ret;
  if (field == "<status>")
    ret = new KMSearchRuleStatus( field, func, contents );
  else if ( field == "<age in days>" || field == "<size>" )
    ret = new KMSearchRuleNumerical( field, func, contents );
  else
    ret = new KMSearchRuleString( field, func, contents );

  return ret;
}

KMSearchRule * KMSearchRule::createInstance( const QByteArray & field,
                                     const char *func,
                                     const QString & contents )
{
  return ( createInstance( field, configValueToFunc( func ), contents ) );
}

KMSearchRule * KMSearchRule::createInstance( const KMSearchRule & other )
{
  return ( createInstance( other.field(), other.function(), other.contents() ) );
}

KMSearchRule * KMSearchRule::createInstanceFromConfig( const KConfigGroup & config, int aIdx )
{
  const char cIdx = char( int('A') + aIdx );

  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  const QByteArray &field2 = config.readEntry( field + cIdx, QString() ).toLatin1();
  Function func2 = configValueToFunc( config.readEntry( func + cIdx, QString() ).toLatin1() );
  const QString & contents2 = config.readEntry( contents + cIdx, QString() );

  if ( field2 == "<To or Cc>" ) // backwards compat
    return KMSearchRule::createInstance( "<recipients>", func2, contents2 );
  else
    return KMSearchRule::createInstance( field2, func2, contents2 );
}

KMSearchRule::Function KMSearchRule::configValueToFunc( const char * str ) {
  if ( !str )
    return FuncNone;

  for ( int i = 0 ; i < numFuncConfigNames ; ++i )
    if ( qstricmp( funcConfigNames[i], str ) == 0 ) return (Function)i;

  return FuncNone;
}

QString KMSearchRule::functionToString( Function function )
{
  if ( function != FuncNone )
    return funcConfigNames[int( function )];
  else
    return "invalid";
}

void KMSearchRule::writeConfig( KConfigGroup & config, int aIdx ) const {
  const char cIdx = char('A' + aIdx);
  static const QString & field = KGlobal::staticQString( "field" );
  static const QString & func = KGlobal::staticQString( "func" );
  static const QString & contents = KGlobal::staticQString( "contents" );

  config.writeEntry( field + cIdx, QString(mField) );
  config.writeEntry( func + cIdx, functionToString( mFunction ) );
  config.writeEntry( contents + cIdx, mContents );
}

bool KMSearchRule::matches( const DwString & aStr, KMMessage & msg,
                       const DwBoyerMoore *, int ) const
{
  if ( !msg.isComplete() ) {
    msg.fromDwString( aStr );
    msg.setComplete( true );
  }
  return matches( &msg );
}

const QString KMSearchRule::asString() const
{
  QString result  = "\"" + mField + "\" <";
  result += functionToString( mFunction );
  result += "> \"" + mContents + "\"";

  return result;
}

//==================================================
//
// class KMSearchRuleString
//
//==================================================

KMSearchRuleString::KMSearchRuleString( const QByteArray & field,
                                        Function func, const QString & contents )
          : KMSearchRule(field, func, contents)
{
  if ( field.isEmpty() || field[0] == '<' )
    mBmHeaderField = 0;
  else // make sure you handle the unrealistic case of the message starting with mField
    mBmHeaderField = new DwBoyerMoore(('\n' + field + ": ").data());
}

KMSearchRuleString::KMSearchRuleString( const KMSearchRuleString & other )
  : KMSearchRule( other ),
    mBmHeaderField( 0 )
{
  if ( other.mBmHeaderField )
    mBmHeaderField = new DwBoyerMoore( *other.mBmHeaderField );
}

const KMSearchRuleString & KMSearchRuleString::operator=( const KMSearchRuleString & other )
{
  if ( this == &other )
    return *this;

  setField( other.field() );
  setFunction( other.function() );
  setContents( other.contents() );
  delete mBmHeaderField; mBmHeaderField = 0;
  if ( other.mBmHeaderField )
    mBmHeaderField = new DwBoyerMoore( *other.mBmHeaderField );

  return *this;
}

KMSearchRuleString::~KMSearchRuleString()
{
  delete mBmHeaderField;
  mBmHeaderField = 0;
}

bool KMSearchRuleString::isEmpty() const
{
  return field().trimmed().isEmpty() || contents().isEmpty();
}

bool KMSearchRuleString::requiresBody() const
{
  if (mBmHeaderField || (field() == "<recipients>" ))
    return false;
  return true;
}

bool KMSearchRuleString::matches( const DwString & aStr, KMMessage & msg,
                       const DwBoyerMoore * aHeaderField, int aHeaderLen ) const
{
  if ( isEmpty() )
    return false;

  bool rc = false;

  const DwBoyerMoore * headerField = aHeaderField ? aHeaderField : mBmHeaderField ;

  const int headerLen = ( aHeaderLen > -1 ? aHeaderLen : field().length() ) + 2 ; // +1 for ': '

  if ( headerField ) {
    static const DwBoyerMoore lflf( "\n\n" );
    static const DwBoyerMoore lfcrlf( "\n\r\n" );

    size_t endOfHeader = lflf.FindIn( aStr, 0 );
    if ( endOfHeader == DwString::npos )
      endOfHeader = lfcrlf.FindIn( aStr, 0 );
    const DwString headers = ( endOfHeader == DwString::npos ) ? aStr : aStr.substr( 0, endOfHeader );
    // In case the searched header is at the beginning, we have to prepend
    // a newline - see the comment in KMSearchRuleString constructor
    DwString fakedHeaders( "\n" );
    size_t start = headerField->FindIn( fakedHeaders.append( headers ), 0, false );
    // if the header field doesn't exist then return false for positive
    // functions and true for negated functions (e.g. "does not
    // contain"); note that all negated string functions correspond
    // to an odd value
    if ( start == DwString::npos )
      rc = ( ( function() & 1 ) == 1 );
    else {
      start += headerLen;
      size_t stop = aStr.find( '\n', start );
      char ch = '\0';
      while ( stop != DwString::npos && ( ch = aStr.at( stop + 1 ) ) == ' ' || ch == '\t' )
        stop = aStr.find( '\n', stop + 1 );
      const int len = stop == DwString::npos ? aStr.length() - start : stop - start ;
      const QByteArray codedValue( aStr.data() + start, len + 1 );
      const QString msgContents = KMMsgBase::decodeRFC2047String( codedValue ).trimmed(); // FIXME: This needs to be changed for IDN support.
      rc = matchesInternal( msgContents );
    }
  } else if ( field() == "<recipients>" ) {
    static const DwBoyerMoore to("\nTo: ");
    static const DwBoyerMoore cc("\nCc: ");
    static const DwBoyerMoore bcc("\nBcc: ");
    // <recipients> "contains" "foo" is true if any of the fields contains
    // "foo", while <recipients> "does not contain" "foo" is true if none
    // of the fields contains "foo"
    if ( ( function() & 1 ) == 0 ) {
      // positive function, e.g. "contains"
      rc = ( matches( aStr, msg, &to, 2 ) ||
             matches( aStr, msg, &cc, 2 ) ||
             matches( aStr, msg, &bcc, 3 ) );
    }
    else {
      // negated function, e.g. "does not contain"
      rc = ( matches( aStr, msg, &to, 2 ) &&
             matches( aStr, msg, &cc, 2 ) &&
             matches( aStr, msg, &bcc, 3 ) );
    }
  }
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    // only log headers bcause messages and bodies can be pretty large
// FIXME We have to separate the text which is used for filtering to be able to show it in the log
//    if ( logContents )
//      msg += " (<i>" + FilterLog::recode( msgContents ) + "</i>)";
    FilterLog::instance()->add( msg, FilterLog::ruleResult );
  }
  return rc;
}

bool KMSearchRuleString::matches( const KMMessage * msg ) const
{
  assert( msg );

  if ( isEmpty() )
    return false;

  QString msgContents;
  // Show the value used to compare the rules against in the log.
  // Overwrite the value for complete messages and all headers!
  bool logContents = true;

  if( field() == "<message>" ) {
    msgContents = msg->asString();
    logContents = false;
  } else if ( field() == "<body>" ) {
    msgContents = msg->bodyToUnicode();
    logContents = false;
  } else if ( field() == "<any header>" ) {
    msgContents = msg->headerAsString();
    logContents = false;
  } else if ( field() == "<recipients>" ) {
    // (mmutz 2001-11-05) hack to fix "<recipients> !contains foo" to
    // meet user's expectations. See FAQ entry in KDE 2.2.2's KMail
    // handbook
    if ( function() == FuncEquals || function() == FuncNotEqual )
      // do we need to treat this case specially? Ie.: What shall
      // "equality" mean for recipients.
      return matchesInternal( msg->headerField("To") )
          || matchesInternal( msg->headerField("Cc") )
          || matchesInternal( msg->headerField("Bcc") )
          // sometimes messages have multiple Cc headers
          || matchesInternal( msg->cc() );

    msgContents = msg->headerField("To");
    if ( !msg->headerField("Cc").compare( msg->cc() ) )
      msgContents += ", " + msg->headerField("Cc");
    else
      msgContents += ", " + msg->cc();
    msgContents += ", " + msg->headerField("Bcc");
  }  else {
    // make sure to treat messages with multiple header lines for
    // the same header correctly
    msgContents = msg->headerFields( field() ).join( " " );
  }

  if ( function() == FuncIsInAddressbook ||
       function() == FuncIsNotInAddressbook ) {
    // I think only the "from"-field makes sense.
    msgContents = msg->headerField( field() );
    if ( msgContents.isEmpty() )
      return ( function() == FuncIsInAddressbook ) ? false : true;
  }

  // these two functions need the kmmessage therefore they don't call matchesInternal
  if ( function() == FuncHasAttachment )
    return ( msg->toMsgBase().attachmentState() == KMMsgHasAttachment );
  if ( function() == FuncHasNoAttachment )
    return ( ((KMMsgAttachmentState) msg->toMsgBase().attachmentState()) == KMMsgHasNoAttachment );

  bool rc = matchesInternal( msgContents );
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    // only log headers bcause messages and bodies can be pretty large
    if ( logContents )
      msg += " (<i>" + FilterLog::recode( msgContents ) + "</i>)";
    FilterLog::instance()->add( msg, FilterLog::ruleResult );
  }
  return rc;
}

// helper, does the actual comparing
bool KMSearchRuleString::matchesInternal( const QString & msgContents ) const
{
  switch ( function() ) {
  case KMSearchRule::FuncEquals:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) == 0 );

  case KMSearchRule::FuncNotEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) != 0 );

  case KMSearchRule::FuncContains:
    return ( msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case KMSearchRule::FuncContainsNot:
    return ( !msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case KMSearchRule::FuncRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) >= 0 );
    }

  case KMSearchRule::FuncNotRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) < 0 );
    }

  case FuncIsGreater:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) > 0 );

  case FuncIsLessOrEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) <= 0 );

  case FuncIsLess:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) < 0 );

  case FuncIsGreaterOrEqual:
      return ( QString::compare( msgContents.toLower(), contents().toLower() ) >= 0 );

  case FuncIsInAddressbook: {
    KABC::AddressBook *stdAb = KABC::StdAddressBook::self( true );
    QStringList addressList =
      EmailAddressTools::splitAddressList( msgContents.toLower() );
    for( QStringList::ConstIterator it = addressList.begin();
         ( it != addressList.end() );
         ++it ) {
      if ( !stdAb->findByEmail( EmailAddressTools::extractEmailAddress( *it ) ).isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsNotInAddressbook: {
    KABC::AddressBook *stdAb = KABC::StdAddressBook::self( true );
    QStringList addressList =
      EmailAddressTools::splitAddressList( msgContents.toLower() );
    for( QStringList::ConstIterator it = addressList.begin();
         ( it != addressList.end() );
         ++it ) {
      if ( stdAb->findByEmail( EmailAddressTools::extractEmailAddress( *it ) ).isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsInCategory: {
    QString category = contents();
    QStringList addressList =  EmailAddressTools::splitAddressList( msgContents.toLower() );
    KABC::AddressBook *stdAb = KABC::StdAddressBook::self( true );

    for( QStringList::ConstIterator it = addressList.begin();
      it != addressList.end(); ++it ) {
        KABC::Addressee::List addresses = stdAb->findByEmail( EmailAddressTools::extractEmailAddress( *it ) );

          for ( KABC::Addressee::List::Iterator itAd = addresses.begin(); itAd != addresses.end(); ++itAd )
              if ( (*itAd).hasCategory(category) )
                return true;

      }
      return false;
    }

    case FuncIsNotInCategory: {
      QString category = contents();
      QStringList addressList =  EmailAddressTools::splitAddressList( msgContents.toLower() );
      KABC::AddressBook *stdAb = KABC::StdAddressBook::self( true );

      for( QStringList::ConstIterator it = addressList.begin();
        it != addressList.end(); ++it ) {
          KABC::Addressee::List addresses = stdAb->findByEmail( EmailAddressTools::extractEmailAddress( *it ) );

            for ( KABC::Addressee::List::Iterator itAd = addresses.begin(); itAd != addresses.end(); ++itAd )
                if ( (*itAd).hasCategory(category) )
                  return false;

      }
      return true;
    }
  default:
    ;
  }

  return false;
}


//==================================================
//
// class KMSearchRuleNumerical
//
//==================================================

KMSearchRuleNumerical::KMSearchRuleNumerical( const QByteArray & field,
                                        Function func, const QString & contents )
          : KMSearchRule(field, func, contents)
{
}

bool KMSearchRuleNumerical::isEmpty() const
{
  bool ok = false;
  contents().toInt( &ok );

  return !ok;
}


bool KMSearchRuleNumerical::matches( const KMMessage * msg ) const
{

  QString msgContents;
  int numericalMsgContents = 0;
  int numericalValue = 0;

  if ( field() == "<size>" ) {
    numericalMsgContents = int( msg->msgLength() );
    numericalValue = contents().toInt();
    msgContents.setNum( numericalMsgContents );
  } else if ( field() == "<age in days>" ) {
    QDateTime msgDateTime;
    msgDateTime.setTime_t( msg->date() );
    numericalMsgContents = msgDateTime.daysTo( QDateTime::currentDateTime() );
    numericalValue = contents().toInt();
    msgContents.setNum( numericalMsgContents );
  }
  bool rc = matchesInternal( numericalValue, numericalMsgContents, msgContents );
  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    msg += " ( <i>" + QString::number( numericalMsgContents ) + "</i> )";
    FilterLog::instance()->add( msg, FilterLog::ruleResult );
  }
  return rc;
}

bool KMSearchRuleNumerical::matchesInternal( long numericalValue,
    long numericalMsgContents, const QString & msgContents ) const
{
  switch ( function() ) {
  case KMSearchRule::FuncEquals:
      return ( numericalValue == numericalMsgContents );

  case KMSearchRule::FuncNotEqual:
      return ( numericalValue != numericalMsgContents );

  case KMSearchRule::FuncContains:
    return ( msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case KMSearchRule::FuncContainsNot:
    return ( !msgContents.contains( contents(), Qt::CaseInsensitive ) );

  case KMSearchRule::FuncRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) >= 0 );
    }

  case KMSearchRule::FuncNotRegExp:
    {
      QRegExp regexp( contents(), Qt::CaseInsensitive );
      return ( regexp.indexIn( msgContents ) < 0 );
    }

  case FuncIsGreater:
      return ( numericalMsgContents > numericalValue );

  case FuncIsLessOrEqual:
      return ( numericalMsgContents <= numericalValue );

  case FuncIsLess:
      return ( numericalMsgContents < numericalValue );

  case FuncIsGreaterOrEqual:
      return ( numericalMsgContents >= numericalValue );

  case FuncIsInAddressbook:  // since email-addresses are not numerical, I settle for false here
    return false;

  case FuncIsNotInAddressbook:
    return false;

  default:
    ;
  }

  return false;
}



//==================================================
//
// class KMSearchRuleStatus
//
//==================================================


KMSearchRuleStatus::KMSearchRuleStatus( const QByteArray & field,
                                        Function func, const QString & aContents )
          : KMSearchRule(field, func, aContents)
{
  // the values are always in english, both from the conf file as well as
  // the patternedit gui
  mStatus = statusFromEnglishName( aContents );
}

MessageStatus KMSearchRuleStatus::statusFromEnglishName(
      const QString & aStatusString )
{
  for ( int i=0; i< numStatusNames; i++ ) {
    if ( !aStatusString.compare( statusNames[i].name ) ) {
      return statusNames[i].status;
    }
  }
  MessageStatus unknown;
  return unknown;
}

bool KMSearchRuleStatus::isEmpty() const
{
  return field().trimmed().isEmpty() || contents().isEmpty();
}

bool KMSearchRuleStatus::matches( const DwString &, KMMessage &,
				  const DwBoyerMoore *, int ) const
{
  assert( 0 );
  return false; // don't warn
}

bool KMSearchRuleStatus::matches( const KMMessage * msg ) const
{

  bool rc = false;

  switch ( function() ) {
    case FuncEquals: // fallthrough. So that "<status> 'is' 'read'" works
    case FuncContains:
      if (msg->messageStatus() & mStatus)
        rc = true;
      break;
    case FuncNotEqual: // fallthrough. So that "<status> 'is not' 'read'" works
    case FuncContainsNot:
      if (! (msg->messageStatus() & mStatus) )
        rc = true;
      break;
    // FIXME what about the remaining funcs, how can they make sense for
    // stati?
    default:
      break;
  }

  if ( FilterLog::instance()->isLogging() ) {
    QString msg = ( rc ? "<font color=#00FF00>1 = </font>"
                       : "<font color=#FF0000>0 = </font>" );
    msg += FilterLog::recode( asString() );
    FilterLog::instance()->add( msg, FilterLog::ruleResult );
  }
  return rc;
}

// ----------------------------------------------------------------------------

//==================================================
//
// class KMSearchPattern
//
//==================================================

KMSearchPattern::KMSearchPattern()
  : QList<KMSearchRule*>()
{
  init();
}

KMSearchPattern::KMSearchPattern( const KConfigGroup & config )
  : QList<KMSearchRule*>()
{
  readConfig( config );
}

KMSearchPattern::~KMSearchPattern()
{
  qDeleteAll( *this );
}

bool KMSearchPattern::matches( const KMMessage * msg, bool ignoreBody ) const
{
  if ( isEmpty() )
    return true;

  QList<KMSearchRule*>::const_iterator it;
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( !(*it)->matches( msg ) )
          return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( (*it)->matches( msg ) )
          return true;
    // fall through
  default:
    return false;
  }
}

bool KMSearchPattern::matches( const DwString & aStr, bool ignoreBody ) const
{
  if ( isEmpty() )
    return true;

  KMMessage msg;
  QList<KMSearchRule*>::const_iterator it;
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( !(*it)->matches( aStr, msg ) )
          return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( (*it)->matches( aStr, msg ) )
          return true;
    // fall through
  default:
    return false;
  }
}

bool KMSearchPattern::matches( quint32 serNum, bool ignoreBody ) const
{
  if ( isEmpty() ) {
    return true;
  }

  bool res;
  int idx = -1;
  KMFolder *folder = 0;
  KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
  if ( !folder || ( idx == -1 ) || ( idx >= folder->count() ) ) {
    return false;
  }

  bool opened = folder->isOpened();
  if ( !opened ) {
    folder->open( "searchptr" );
  }
  KMMsgBase *msgBase = folder->getMsgBase( idx );
  if (requiresBody() && !ignoreBody) {
    bool unGet = !msgBase->isMessage();
    KMMessage *msg = folder->getMsg(idx);
    res = matches( msg, ignoreBody );
    if (unGet)
      folder->unGetMsg(idx);
  } else {
    res = matches( folder->getDwString(idx), ignoreBody );
  }
  if ( !opened ) {
    folder->close( "searchptr" );
  }
  return res;
}

bool KMSearchPattern::requiresBody() const {
  QList<KMSearchRule*>::const_iterator it;
    for ( it = begin() ; it != end() ; ++it )
      if ( (*it)->requiresBody() )
	return true;
  return false;
}

void KMSearchPattern::purify() {
  QList<KMSearchRule*>::iterator it = end();
  while ( it != begin() ) {
    --it;
    if ( (*it)->isEmpty() ) {
#ifndef NDEBUG
      kDebug(5006) << "KMSearchPattern::purify(): removing " << (*it)->asString() << endl;
#endif
      erase( it );
      it = end();
    }
  }
}

void KMSearchPattern::readConfig( const KConfigGroup & config ) {
  init();

  mName = config.readEntry("name");
  if ( !config.hasKey("rules") ) {
    kDebug(5006) << "KMSearchPattern::readConfig: found legacy config! Converting." << endl;
    importLegacyConfig( config );
    return;
  }

  mOperator = config.readEntry("operator") == "or" ? OpOr : OpAnd;

  const int nRules = config.readEntry( "rules", 0 );

  for ( int i = 0 ; i < nRules ; i++ ) {
    KMSearchRule * r = KMSearchRule::createInstanceFromConfig( config, i );
    if ( r->isEmpty() )
      delete r;
    else
      append( r );
  }
}

void KMSearchPattern::importLegacyConfig( const KConfigGroup & config ) {
  KMSearchRule * rule = KMSearchRule::createInstance( config.readEntry("fieldA").toLatin1(),
					  config.readEntry("funcA").toLatin1(),
					  config.readEntry("contentsA") );
  if ( rule->isEmpty() ) {
    // if the first rule is invalid,
    // we really can't do much heuristics...
    delete rule;
    return;
  }
  append( rule );

  const QString sOperator = config.readEntry("operator");
  if ( sOperator == "ignore" ) return;

  rule = KMSearchRule::createInstance( config.readEntry("fieldB").toLatin1(),
			   config.readEntry("funcB").toLatin1(),
			   config.readEntry("contentsB") );
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
    // We simply toggle the last bit (xor with 0x1)... This assumes that
    // KMSearchRule::Function's come in adjacent pairs of pros and cons
    KMSearchRule::Function func = last()->function();
    unsigned int intFunc = (unsigned int)func;
    func = KMSearchRule::Function( intFunc ^ 0x1 );

    last()->setFunction( func );
  }

  // treat any other case as "and" (our default).
}

void KMSearchPattern::writeConfig( KConfigGroup & config ) const {
  config.writeEntry("name", mName);
  config.writeEntry("operator", (mOperator == KMSearchPattern::OpOr) ? "or" : "and" );

  int i = 0;
  QList<KMSearchRule*>::const_iterator it;
  for ( it = begin() ; it != end() && i < FILTER_MAX_RULES ; ++i , ++it )
    // we could do this ourselves, but we want the rules to be extensible,
    // so we give the rule it's number and let it do the rest.
    (*it)->writeConfig( config, i );

  // save the total number of rules.
  config.writeEntry( "rules", i );
}

void KMSearchPattern::init() {
  clear();
  mOperator = OpAnd;
  mName = '<' + i18nc("name used for a virgin filter","unknown") + '>';
}

QString KMSearchPattern::asString() const {
  QString result;
  if ( mOperator == OpOr )
    result = i18n("(match any of the following)");
  else
    result = i18n("(match all of the following)");

  QList<KMSearchRule*>::const_iterator it;
  for ( it = begin() ; it != end() ; ++it )
    result += "\n\t" + FilterLog::recode( (*it)->asString() );

  return result;
}

const KMSearchPattern & KMSearchPattern::operator=( const KMSearchPattern & other ) {
  if ( this == &other )
    return *this;

  setOp( other.op() );
  setName( other.name() );

  clear(); // ###
  QList<KMSearchRule*>::const_iterator it;
  for ( it = other.begin() ; it != other.end() ; ++it )
    append( KMSearchRule::createInstance( **it ) ); // deep copy

  return *this;
}
