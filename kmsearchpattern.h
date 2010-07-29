/* -*- mode: C++; c-file-style: "gnu" -*-
  kmsearchpattern.h
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

#ifndef _kmsearchpattern_h_
#define _kmsearchpattern_h_

#include <klocale.h>
#include <messagecore/messagestatus.h>
using KPIM::MessageStatus;

#include <Nepomuk/Query/GroupTerm>
#include <Nepomuk/Query/ComparisonTerm>

#include <QList>
#include <QString>

#include <boost/shared_ptr.hpp>

namespace Akonadi {
  class Item;
}

namespace KMime {
  class Message;
}

class KConfigGroup;


// maximum number of filter rules per filter
const int FILTER_MAX_RULES=8;

/** Incoming mail is sent through the list of mail filter
    rules before it is placed in the associated mail folder (usually "inbox").
    This class represents one mail filter rule. It is also used to represent
    a search rule as used by the search dialog and folders.

    @short This class represents one search pattern rule.
*/
class KMSearchRule
{
public:
    typedef  boost::shared_ptr<KMSearchRule> Ptr;

  /** Operators for comparison of field and contents.
      If you change the order or contents of the enum: do not forget
      to change funcConfigNames[], sFilterFuncList and matches()
      in KMSearchRule, too.
      Also, it is assumed that these functions come in pairs of logical
      opposites (ie. "=" <-> "!=", ">" <-> "<=", etc.).
  */
  enum Function { FuncNone = -1,
                  FuncContains=0, FuncContainsNot,
                  FuncEquals, FuncNotEqual,
                  FuncRegExp, FuncNotRegExp,
                  FuncIsGreater, FuncIsLessOrEqual,
                  FuncIsLess, FuncIsGreaterOrEqual,
                  FuncIsInAddressbook, FuncIsNotInAddressbook,
                  FuncIsInCategory, FuncIsNotInCategory,
                  FuncHasAttachment, FuncHasNoAttachment};
  explicit KMSearchRule ( const QByteArray & field=0, Function=FuncContains,
                 const QString &contents=QString() );
  KMSearchRule ( const KMSearchRule &other );

  const KMSearchRule & operator=( const KMSearchRule & other );

  /** Create a search rule of a certain type by instantiating the appro-
      priate subclass depending on the @p field. */
  static KMSearchRule::Ptr createInstance( const QByteArray & field=0,
                                      Function function=FuncContains,
                                      const QString & contents=QString() );

  static KMSearchRule::Ptr createInstance( const QByteArray & field,
                                       const char * function,
                                       const QString & contents );

  static KMSearchRule::Ptr createInstance( const KMSearchRule & other );

  static KMSearchRule::Ptr createInstance( QDataStream& s );

  /** Initialize the object from a given config group.
      @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function does no
      validation of the data obtained from the config file. You should
      call isEmpty yourself if you need valid rules. */
  static KMSearchRule::Ptr createInstanceFromConfig( const KConfigGroup & config, int aIdx );

  virtual ~KMSearchRule() {}

  /** Tries to match the rule against the given KMime::Message.
      @return true if the rule matched, false otherwise. Must be
      implemented by subclasses.
  */
  virtual bool matches( const Akonadi::Item &item ) const = 0;

  /** Determine whether the rule is worth considering. It isn't if
      either the field is not set or the contents is empty.
      KFilter should make sure that it's rule list contains
      only non-empty rules, as matches doesn't check this. */
  virtual bool isEmpty() const = 0;

  /** Returns true if the rule depends on a complete message,
      otherwise returns false. */
  virtual bool requiresBody() const { return true; }


  /** Save the object into a given config group.
      @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function will happily
      write itself even when it's not valid, assuming higher layers to
      Do The Right Thing(TM). */
  void writeConfig( KConfigGroup & config, int aIdx ) const;

  /** Return filter function. This can be any of the operators
      defined in Function. */
  Function function() const { return mFunction; }

  /** Set filter function. */
  void setFunction( Function aFunction ) { mFunction = aFunction; }

  /** Return message header field name (without the trailing ':').
      There are also six pseudo-headers:
      @li \<message\>: Try to match against the whole message.
      @li \<body\>: Try to match against the body of the message.
      @li \<any header\>: Try to match against any header field.
      @li \<recipients\>: Try to match against both To: and Cc: header fields.
      @li \<size\>: Try to match against size of message (numerical).
      @li \<age in days\>: Try to match against age of message (numerical).
      @li \<status\>: Try to match against status of message (status).
      @li \<tag\>: Try to match against message tags.
  */
  QByteArray field() const { return mField; }

  /** Set message header field name (make sure there's no trailing
      colon ':') */
  void setField( const QByteArray & field ) { mField = field; }

  /** Return the value. This can be either a substring to search for in
      or a regexp pattern to match against the header. */
  QString contents() const { return mContents; }
  /** Set the value. */
  void setContents( const QString & aContents ) { mContents = aContents; }

  /** Returns the rule as string. For debugging.*/
  const QString asString() const;

  /** Adds query terms to the given term group. */
  virtual void addQueryTerms( Nepomuk::Query::GroupTerm &groupTerm ) const = 0;

  QDataStream & operator>>( QDataStream& ) const;

protected:
  /** Converts function() into the corresponding Nepomuk query operator. */
  Nepomuk::Query::ComparisonTerm::Comparator nepomukComparator() const;

  /** Adds @p term to @p termGroup and adds a negation term inbetween if needed. */
  void addAndNegateTerm( const Nepomuk::Query::Term &term, Nepomuk::Query::GroupTerm &termGroup ) const;

private:
  static Function configValueToFunc( const char * str );
  static QString functionToString( Function function );

  QByteArray mField;
  Function mFunction;
  QString  mContents;
};


// subclasses representing the different kinds of searches

/** This class represents a search to be performed against a string.
 *  The string can be either a message header, or a pseudo header, such
 *  as \<body\>
    @short This class represents a search pattern rule operating on a string.
*/
class KMSearchRuleString : public KMSearchRule
{
public:
  explicit KMSearchRuleString( const QByteArray & field=0,
                Function function=FuncContains, const QString & contents=QString() );
  KMSearchRuleString( const KMSearchRuleString & other );
  const KMSearchRuleString & operator=( const KMSearchRuleString & other );

  virtual ~KMSearchRuleString();
  virtual bool isEmpty() const ;
  virtual bool requiresBody() const;

  virtual bool matches( const Akonadi::Item &item ) const;
  virtual void addQueryTerms( Nepomuk::Query::GroupTerm &groupTerm ) const;

  /** Helper for the main matches() method. Does the actual comparing. */
  bool matchesInternal( const QString & msgContents ) const;

  private:
    void addPersonTerm( Nepomuk::Query::GroupTerm &groupTerm, const QUrl &field ) const;
};


/** This class represents a search to be performed against a numerical value,
 *  such as the age of the message in days or its size.
    @short This class represents a search pattern rule operating on numerical
    values.
*/
class KMSearchRuleNumerical : public KMSearchRule
{
public:
  explicit KMSearchRuleNumerical( const QByteArray & field=0,
                         Function function=FuncContains, const QString & contents=QString() );
  virtual bool isEmpty() const ;

  virtual bool matches( const Akonadi::Item &item ) const;
  virtual void addQueryTerms( Nepomuk::Query::GroupTerm &groupTerm ) const;

  // Optimized matching not implemented, will use the unoptimized matching
  // from KMSearchRule
  using KMSearchRule::matches;

  /** Helper for the main matches() method. Does the actual comparing. */
  bool matchesInternal( long numericalValue, long numericalMsgContents,
                        const QString & msgContents ) const;
};


namespace KMail {
// The below are used in several places and here so they are accessible.
  struct MessageStatus {
    const char * const text;
    const char * const icon;
  };

  // If you change the ordering here; also do it in the enum below
  static const MessageStatus StatusValues[] = {
    { I18N_NOOP2( "message status", "Important" ),              "emblem-important"    },
    { I18N_NOOP2( "message status", "Action Item" ),            "mail-task"           },
    { I18N_NOOP2( "message status", "New" ),                    "mail-unread-new"     },
    { I18N_NOOP2( "message status", "Unread" ),                 "mail-unread"         },
    { I18N_NOOP2( "message status", "Read" ),                   "mail-read"           },
    { I18N_NOOP2( "message status", "Deleted" ),                "mail-deleted"        },
    { I18N_NOOP2( "message status", "Replied" ),                "mail-replied"        },
    { I18N_NOOP2( "message status", "Forwarded" ),              "mail-forwarded"      },
    { I18N_NOOP2( "message status", "Queued" ),                 "mail-queued"         },
    { I18N_NOOP2( "message status", "Sent" ),                   "mail-sent"           },
    { I18N_NOOP2( "message status", "Watched" ),                "mail-thread-watch"   },
    { I18N_NOOP2( "message status", "Ignored" ),                "mail-thread-ignored" },
    { I18N_NOOP2( "message status", "Spam" ),                   "mail-mark-junk"      },
    { I18N_NOOP2( "message status", "Ham" ),                    "mail-mark-notjunk"   },
    { I18N_NOOP2( "message status", "Has Attachment"),          "mail-attachment"     } //must be last
  };
  // If you change the ordering here; also do it in the array above
  enum StatusValueTypes {
    StatusImportant = 0,
    StatusToAct = 1,
    StatusNew = 2,
    StatusUnread = 3,
    StatusRead = 4,
    StatusDeleted = 5,
    StatusReplied = 6,
    StatusForwarded = 7,
    StatusQueued = 8,
    StatusSent = 9,
    StatusWatched = 10,
    StatusIgnored = 11,
    StatusSpam = 12,
    StatusHam = 13,
    StatusHasAttachment = 14 //must be last
  };

  static const int StatusValueCount =
    sizeof( StatusValues ) / sizeof( MessageStatus );
  // we want to show all status entries in the quick search bar, but only the
  // ones up to attachment in the search/filter dialog, because there the
  // attachment case is handled separately.
  static const int StatusValueCountWithoutHidden = StatusValueCount - 1;
}

/** This class represents a search to be performed against the status of a
 *  messsage. The status is represented by a bitfield.
    @short This class represents a search pattern rule operating on message
    status.
*/
class KMSearchRuleStatus : public KMSearchRule
{
public:
   explicit KMSearchRuleStatus( const QByteArray & field=0, Function function=FuncContains,
                                const QString & contents=QString() );
   explicit KMSearchRuleStatus( MessageStatus status, Function function=FuncContains );

   virtual bool isEmpty() const ;
   virtual bool matches( const Akonadi::Item &item ) const;
   virtual void addQueryTerms( Nepomuk::Query::GroupTerm &groupTerm ) const;

   //Not possible to implement optimized form for status searching
   using KMSearchRule::matches;


   static MessageStatus statusFromEnglishName(const QString&);
private:
  void addTagTerm( Nepomuk::Query::GroupTerm &groupTerm, const QString &tagId ) const;

private:
   MessageStatus mStatus;
};

// ------------------------------------------------------------------------

/** This class is an abstraction of a search over messages.  It is
    intended to be used inside a KFilter (which adds KFilterAction's),
    as well as in KMSearch. It can read and write itself into a
    KConfig group and there is a constructor, mainly used by KMFilter
    to initialize from a preset KConfig-Group.

    From a class hierarchy point of view, it is a QPtrList of
    KMSearchRule's that adds the boolean operators (see Operator)
    'and' and 'or' that connect the rules logically, and has a name
    under which it could be stored in the config file.

    As a QPtrList with autoDelete enabled, it assumes that it is the
    central repository for the rules it contains. So if you want to
    reuse a rule in another pattern, make a deep copy of that rule.

    @short An abstraction of a search over messages.
    @author Marc Mutz <Marc@Mutz.com>
*/
class KMSearchPattern : public QList<KMSearchRule::Ptr>
{

public:
  /** Boolean operators that connect the return values of the
      individual rules. A pattern with @p OpAnd will match iff all
      it's rules match, whereas a pattern with @p OpOr will match iff
      any of it's rules matches.
  */
  enum Operator { OpAnd, OpOr };

  /** Constructor which provides a pattern with minimal, but
      sufficient initialization. Unmodified, such a pattern will fail
      to match any KMime::Message. You can query for such an empty
      rule by using isEmpty, which is inherited from QPtrList.
  */
  KMSearchPattern();

  /** Constructor that initializes from a given KConfig group, if
      given. This feature is mainly (solely?) used in KMFilter,
      as we don't allow to store search patterns in the config (yet).
  */
  KMSearchPattern( const KConfigGroup & config );

  /** Destructor. Deletes all stored rules! */
  ~KMSearchPattern();

  /** The central function of this class. Tries to match the set of
      rules against a KMime::Message. It's virtual to allow derived
      classes with added rules to reimplement it, yet reimplemented
      methods should and (&&) the result of this function with their
      own result or else most functionality is lacking, or has to be
      reimplemented, since the rules are private to this class.

      @return true if the match was successful, false otherwise.
  */
  bool matches( const Akonadi::Item &item, bool ignoreBody = false ) const;

  /** Returns true if the pattern only depends the DwString that backs
      a message */
  bool requiresBody() const;

  /** Removes all empty rules from the list. You should call this
      method whenever the user had had control of the rules outside of
      this class. (e.g. after editing it with KMSearchPatternEdit).
  */
  void purify();

  /** Reads a search pattern from a KConfigGroup. If it does not find
      a valid saerch pattern in the preset group, initializes the pattern
      as if it were constructed using the default constructor.

      For backwards compatibility with previous versions of KMail, it
      checks for old-style filter rules (e.g. using @p OpIgnore)
      in @p config und converts them to the new format on writeConfig.

      Derived classes reimplementing readConfig() should also call this
      method, or else the rules will not be loaded.
  */
  void readConfig( const KConfigGroup & config );

  /** Writes itself into @p config. Tries to delete old-style keys by
      overwriting them with QString().

      Derived classes reimplementing writeConfig() should also call this
      method, or else the rules will not be stored.
  */
  void writeConfig( KConfigGroup & config ) const;

  /** Get the name of the search pattern. */
  QString name() const { return mName; }
  /** Set the name of the search pattern. KMFilter uses this to
      store it's own name, too. */
  void setName( const QString & newName ) { mName = newName ; }

  /** Get the filter operator */
  KMSearchPattern::Operator op() const { return mOperator; }
  /** Set the filter operator */
  void setOp( KMSearchPattern::Operator aOp ) { mOperator = aOp; }

  /** Returns the pattern as string. For debugging.*/
  QString asString() const;

  /** Returns the pattern as a SPARQL query. */
  QString asSparqlQuery() const;

  /** Overloaded assignment operator. Makes a deep copy. */
  const KMSearchPattern & operator=( const KMSearchPattern & aPattern );

  /** Writes the pattern into a byte array for persistance purposes. */
  QByteArray serialize() const;

  /** Constructs the pattern from a byte array serialization. */
  void deserialize( const QByteArray& );

  QDataStream & operator>>( QDataStream & s ) const;
  QDataStream & operator<<( QDataStream & s );

private:
  /** Tries to import a legacy search pattern, ie. one that still has
      e.g. the @p unless or @p ignore operator which were useful as
      long as the number of rules was restricted to two. This method
      is called from readConfig, which detects legacy configurations
      and also makes sure that this method is called from an initialized
      object.
  */
  void importLegacyConfig( const KConfigGroup & config );

  /** Initializes the object. Clears the list of rules, sets the name
      to "<i18n("unnamed")>", and the boolean operator to @p OpAnd. */
  void init();

  QString  mName;
  Operator mOperator;
};

#endif /* _kmsearchpattern_h_ */
