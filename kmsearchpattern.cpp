/* -*- mode: C++; c-file-style: "gnu" -*-
  kmsearchpattern.cpp
  Author: Marc Mutz <Marc@Mutz.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kmsearchpattern.h"
#include "filterlog.h"
using KMail::FilterLog;
#include "kmkernel.h"
#include <kpimutils/email.h>

#include <nie.h>
#include <nmo.h>
#include <nco.h>

#include <Nepomuk/Tag>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/OrTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/NegationTerm>
#include <Nepomuk/Query/ResourceTypeTerm>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <kmime/kmime_message.h>
#include <kmime/kmime_util.h>

#include <akonadi/contact/contactsearchjob.h>

#include <QRegExp>
#include <QByteArray>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>

#include <assert.h>

static const char* funcConfigNames[] =
  { "contains", "contains-not", "equals", "not-equal", "regexp",
    "not-regexp", "greater", "less-or-equal", "less", "greater-or-equal",
    "is-in-addressbook", "is-not-in-addressbook", "is-in-category", "is-not-in-category",
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
  { "Action Item", MessageStatus::statusToAct() },
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
  KMSearchRule *ret = 0;
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

const QString KMSearchRule::asString() const
{
  QString result  = "\"" + mField + "\" <";
  result += functionToString( mFunction );
  result += "> \"" + mContents + "\"";

  return result;
}

Nepomuk::Query::ComparisonTerm::Comparator KMSearchRule::nepomukComparator() const
{
  switch ( function() ) {
    case KMSearchRule::FuncContains:
    case KMSearchRule::FuncContainsNot:
      return Nepomuk::Query::ComparisonTerm::Contains;
    case KMSearchRule::FuncEquals:
    case KMSearchRule::FuncNotEqual:
      return Nepomuk::Query::ComparisonTerm::Equal;
    case KMSearchRule::FuncIsGreater:
      return Nepomuk::Query::ComparisonTerm::Greater;
    case KMSearchRule::FuncIsGreaterOrEqual:
      return Nepomuk::Query::ComparisonTerm::GreaterOrEqual;
    case KMSearchRule::FuncIsLess:
      return Nepomuk::Query::ComparisonTerm::Smaller;
    case KMSearchRule::FuncIsLessOrEqual:
      return Nepomuk::Query::ComparisonTerm::SmallerOrEqual;
    case KMSearchRule::FuncRegExp:
    case KMSearchRule::FuncNotRegExp:
      return Nepomuk::Query::ComparisonTerm::Regexp;
    default:
      kDebug() << "Unhandled function type: " << function();
  }
  return Nepomuk::Query::ComparisonTerm::Equal;
}

void KMSearchRule::addAndNegateTerm(const Nepomuk::Query::Term& term, Nepomuk::Query::GroupTerm& termGroup) const
{
  bool negate = false;
  switch ( function() ) {
    case KMSearchRule::FuncContainsNot:
    case KMSearchRule::FuncNotEqual:
    case KMSearchRule::FuncNotRegExp:
    case KMSearchRule::FuncHasNoAttachment:
    case KMSearchRule::FuncIsNotInCategory:
    case KMSearchRule::FuncIsNotInAddressbook:
      negate = true;
    default:
      break;
  }
  if ( negate ) {
    Nepomuk::Query::NegationTerm neg;
    neg.setSubTerm( term );
    termGroup.addSubTerm( neg );
  } else {
    termGroup.addSubTerm( term );
  }
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
}

KMSearchRuleString::KMSearchRuleString( const KMSearchRuleString & other )
  : KMSearchRule( other )
{
}

const KMSearchRuleString & KMSearchRuleString::operator=( const KMSearchRuleString & other )
{
  if ( this == &other )
    return *this;

  setField( other.field() );
  setFunction( other.function() );
  setContents( other.contents() );

  return *this;
}

KMSearchRuleString::~KMSearchRuleString()
{
}

bool KMSearchRuleString::isEmpty() const
{
  return field().trimmed().isEmpty() || contents().isEmpty();
}

bool KMSearchRuleString::requiresBody() const
{
  if ( field().startsWith( '<' ) || field() == "<recipients>" )
    return false;
  return true;
}

bool KMSearchRuleString::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  assert( msg.get() );

  if ( isEmpty() )
    return false;

  QString msgContents;
  // Show the value used to compare the rules against in the log.
  // Overwrite the value for complete messages and all headers!
  bool logContents = true;

  if( field() == "<message>" ) {
    msgContents = msg->encodedContent();
    logContents = false;
  } else if ( field() == "<body>" ) {
    msgContents = msg->body();
    logContents = false;
  } else if ( field() == "<any header>" ) {
    msgContents = msg->head();
    logContents = false;
  } else if ( field() == "<recipients>" ) {
    // (mmutz 2001-11-05) hack to fix "<recipients> !contains foo" to
    // meet user's expectations. See FAQ entry in KDE 2.2.2's KMail
    // handbook
    if ( function() == FuncEquals || function() == FuncNotEqual )
      // do we need to treat this case specially? Ie.: What shall
      // "equality" mean for recipients.
      return matchesInternal( msg->to()->asUnicodeString() )
          || matchesInternal( msg->cc()->asUnicodeString() )
          || matchesInternal( msg->bcc()->asUnicodeString() ) ;
          // sometimes messages have multiple Cc headers //TODO: check if this can happen for KMime!
      //    || matchesInternal( msg->cc() )
          

    msgContents = msg->to()->asUnicodeString();
#if 0  //TODO port to akonadi - check if this is needed for KMime. 
    if ( !msg->headerField("Cc").compare( msg->cc() ) )
      msgContents += ", " + msg->headerField("Cc");
    else
      msgContents += ", " + msg->cc();
#else
      kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  msgContents += ", " + msg->bcc()->asUnicodeString();
  } else if ( field() == "<tag>" ) {
#if 0  //TODO port to akonadi
    if ( msg->tagList() ) {
      foreach ( const QString &label, * msg->tagList() ) {
        const KMime::MessageTagDescription * tagDesc = kmkernel->msgTagMgr()->find( label );
        if ( tagDesc )
          msgContents += tagDesc->name();
      }
      logContents = false;
    }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  } else
 {
    // make sure to treat messages with multiple header lines for
    // the same header correctly
    msgContents = msg->headerByType( field() ) ? msg->headerByType( field() )->asUnicodeString() : "";
  }

  if ( function() == FuncIsInAddressbook ||
       function() == FuncIsNotInAddressbook ) {
    // I think only the "from"-field makes sense.
    msgContents = msg->headerByType( field() ) ? msg->headerByType( field() )->asUnicodeString() : "";
    if ( msgContents.isEmpty() )
      return ( function() == FuncIsInAddressbook ) ? false : true;
  }

  // these two functions need the kmmessage therefore they don't call matchesInternal
  if ( function() == FuncHasAttachment )
    return ( msg->attachments().size() > 0 );
  if ( function() == FuncHasNoAttachment )
    return ( msg->attachments().size() == 0 );

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

void KMSearchRuleString::addPersonTerm(Nepomuk::Query::GroupTerm& groupTerm, const QUrl& field) const
{
  // TODO split contents() into address/name and adapt the query accordingly
  const Nepomuk::Query::ComparisonTerm valueTerm( Vocabulary::NCO::emailAddress(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
  const Nepomuk::Query::ComparisonTerm addressTerm( Vocabulary::NCO::hasEmailAddress(), valueTerm, Nepomuk::Query::ComparisonTerm::Equal );
  const Nepomuk::Query::ComparisonTerm personTerm( field, addressTerm, Nepomuk::Query::ComparisonTerm::Equal );
  groupTerm.addSubTerm( personTerm );
}

void KMSearchRuleString::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  Nepomuk::Query::OrTerm termGroup;
  if ( field().toLower() == "to" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::to() );
  if ( field().toLower() == "cc" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::cc() );
  if ( field().toLower() == "bcc" || field() == "<recipients>" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::bcc() );

  if ( field().toLower() == "from" || field() == "<any header>" || field() == "<message>" )
    addPersonTerm( termGroup, Vocabulary::NMO::from() );

  if ( field().toLower() == "subject" || field() == "<any header>" || field() == "<message>" ) {
    const Nepomuk::Query::ComparisonTerm subjectTerm( Vocabulary::NMO::messageSubject(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    termGroup.addSubTerm( subjectTerm );
  }

  // TODO complete for other headers, generic headers

  if ( field() == "<tag>" ) {
    const Nepomuk::Tag tag( contents() );
    addAndNegateTerm( Nepomuk::Query::ComparisonTerm( Soprano::Vocabulary::NAO::hasTag(),
                                                      Nepomuk::Query::ResourceTerm( tag ),
                                                      Nepomuk::Query::ComparisonTerm::Equal ),
                                                      groupTerm );
  }

  if ( field() == "<body>" || field() == "<message>" ) {
    const Nepomuk::Query::ComparisonTerm bodyTerm( Vocabulary::NMO::plainTextMessageContent(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    termGroup.addSubTerm( bodyTerm );

    const Nepomuk::Query::ComparisonTerm attachmentBodyTerm( Vocabulary::NMO::plainTextMessageContent(), Nepomuk::Query::LiteralTerm( contents() ), nepomukComparator() );
    const Nepomuk::Query::ComparisonTerm attachmentTerm( Vocabulary::NIE::isPartOf(), attachmentBodyTerm, Nepomuk::Query::ComparisonTerm::Equal );
    termGroup.addSubTerm( attachmentTerm );
  }

  if ( !termGroup.subTerms().isEmpty() )
    addAndNegateTerm( termGroup, groupTerm );
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
    const QStringList addressList = KPIMUtils::splitAddressList( msgContents.toLower() );
    for ( QStringList::ConstIterator it = addressList.constBegin(); ( it != addressList.constEnd() ); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setLimit( 1 );
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      if ( !job->contacts().isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsNotInAddressbook: {
    const QStringList addressList = KPIMUtils::splitAddressList( msgContents.toLower() );
    for ( QStringList::ConstIterator it = addressList.constBegin(); ( it != addressList.constEnd() ); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setLimit( 1 );
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      if ( job->contacts().isEmpty() )
        return true;
    }
    return false;
  }

  case FuncIsInCategory: {
    QString category = contents();
    const QStringList addressList =  KPIMUtils::splitAddressList( msgContents.toLower() );

    for ( QStringList::ConstIterator it = addressList.constBegin(); it != addressList.constEnd(); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      const KABC::Addressee::List contacts = job->contacts();

      foreach ( const KABC::Addressee &contact, contacts ) {
        if ( contact.hasCategory( category ) )
          return true;
      }
    }
    return false;
  }

  case FuncIsNotInCategory: {
    QString category = contents();
    const QStringList addressList =  KPIMUtils::splitAddressList( msgContents.toLower() );

    for ( QStringList::ConstIterator it = addressList.constBegin(); it != addressList.constEnd(); ++it ) {
      Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
      job->setQuery( Akonadi::ContactSearchJob::Email, KPIMUtils::extractEmailAddress( *it ) );
      job->exec();

      const KABC::Addressee::List contacts = job->contacts();

      foreach ( const KABC::Addressee &contact, contacts ) {
        if ( contact.hasCategory( category ) )
          return false;
      }

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


bool KMSearchRuleNumerical::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  QString msgContents;
  qint64 numericalMsgContents = 0;
  qint64 numericalValue = 0;

  if ( field() == "<size>" ) {
    numericalMsgContents = item.size();
    numericalValue = contents().toLongLong();
    msgContents.setNum( numericalMsgContents );
  } else if ( field() == "<age in days>" ) {
    QDateTime msgDateTime = msg->date()->dateTime().dateTime();
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

void KMSearchRuleNumerical::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  if ( field() == "<size>" ) {
    const Nepomuk::Query::ComparisonTerm sizeTerm( Vocabulary::NIE::byteSize(),
                                                   Nepomuk::Query::LiteralTerm( contents().toInt() ),
                                                   nepomukComparator() );
    addAndNegateTerm( sizeTerm, groupTerm );
  } else if ( field() == "<age in days>" ) {
    // TODO
  }
}

//==================================================
//
// class KMSearchRuleStatus
//
//==================================================
QString englishNameForStatus( const MessageStatus &status )
{
  for ( int i=0; i< numStatusNames; i++ ) {
    if ( statusNames[i].status == status ) {
      return statusNames[i].name;
    }
  }
  return QString();
}

KMSearchRuleStatus::KMSearchRuleStatus( const QByteArray & field,
                                        Function func, const QString & aContents )
          : KMSearchRule(field, func, aContents)
{
  // the values are always in english, both from the conf file as well as
  // the patternedit gui
  mStatus = statusFromEnglishName( aContents );
}

KMSearchRuleStatus::KMSearchRuleStatus( MessageStatus status, Function func )
 : KMSearchRule( "<status>", func, englishNameForStatus( status ) )
{
  mStatus = status;
}

MessageStatus KMSearchRuleStatus::statusFromEnglishName( const QString &aStatusString )
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

bool KMSearchRuleStatus::matches( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  KPIM::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  bool rc = false;
  switch ( function() ) {
    case FuncEquals: // fallthrough. So that "<status> 'is' 'read'" works
    case FuncContains:
      if (status & mStatus)
        rc = true;
      break;
    case FuncNotEqual: // fallthrough. So that "<status> 'is not' 'read'" works
    case FuncContainsNot:
      if (! (status & mStatus) )
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

void KMSearchRuleStatus::addTagTerm( Nepomuk::Query::GroupTerm &groupTerm, const QString &tagId ) const
{
  // TODO handle function() == NOT
  const Nepomuk::Tag tag( tagId );
  addAndNegateTerm( Nepomuk::Query::ComparisonTerm( Soprano::Vocabulary::NAO::hasTag(),
                                                    Nepomuk::Query::ResourceTerm( tag.resourceUri() ),
                                                    Nepomuk::Query::ComparisonTerm::Equal ),
                                                    groupTerm );
}

void KMSearchRuleStatus::addQueryTerms(Nepomuk::Query::GroupTerm& groupTerm) const
{
  if ( mStatus.isRead() || mStatus.isUnread() ) {
    bool read = false;
    if ( function() == FuncContains || function() == FuncEquals )
      read = true;
    if ( mStatus.isUnread() )
      read = !read;
    groupTerm.addSubTerm( Nepomuk::Query::ComparisonTerm( Vocabulary::NMO::isRead(), Nepomuk::Query::LiteralTerm( read ), Nepomuk::Query::ComparisonTerm::Equal ) );
  }

  if ( mStatus.isImportant() )
    addTagTerm( groupTerm, "important" );
  if ( mStatus.isToAct() )
    addTagTerm( groupTerm, "todo" );
  if ( mStatus.isWatched() )
    addTagTerm( groupTerm, "watched" );

  // TODO
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

bool KMSearchPattern::matches( const Akonadi::Item &item, bool ignoreBody ) const
{
  if ( isEmpty() )
    return true;
  if ( !item.hasPayload<KMime::Message::Ptr>() )
    return false;

  QList<KMSearchRule*>::const_iterator it;
  switch ( mOperator ) {
  case OpAnd: // all rules must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( !(*it)->matches( item ) )
          return false;
    return true;
  case OpOr:  // at least one rule must match
    for ( it = begin() ; it != end() ; ++it )
      if ( !((*it)->requiresBody() && ignoreBody) )
        if ( (*it)->matches( item ) )
          return true;
    // fall through
  default:
    return false;
  }
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
      kDebug() << "Removing" << (*it)->asString();
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
    kDebug() << "Found legacy config! Converting.";
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
  for ( it = begin() ; it != end() && i < FILTER_MAX_RULES ; ++i, ++it )
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

static Nepomuk::Query::GroupTerm makeGroupTerm( KMSearchPattern::Operator op )
{
  if ( op == KMSearchPattern::OpOr )
    return Nepomuk::Query::OrTerm();
  return Nepomuk::Query::AndTerm();
}

QString KMSearchPattern::asSparqlQuery() const
{
  Nepomuk::Query::Query query;

  Nepomuk::Query::AndTerm outerGroup;
  Nepomuk::Query::ResourceTypeTerm typeTerm( Nepomuk::Types::Class( Vocabulary::NMO::Email() ) );

  Nepomuk::Query::GroupTerm innerGroup = makeGroupTerm( mOperator );
  for ( const_iterator it = begin(); it != end(); ++it )
    (*it)->addQueryTerms( innerGroup );

  if ( innerGroup.subTerms().isEmpty() )
    return QString();
  outerGroup.addSubTerm( innerGroup );
  outerGroup.addSubTerm( typeTerm );
  query.setTerm( outerGroup );
  return query.toSparqlQuery();
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
