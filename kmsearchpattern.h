// -*- mode: C++; c-file-style: "gnu" -*-
// kmsearchpattern.h
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL!

#ifndef _kmsearchpattern_h_
#define _kmsearchpattern_h_

#include <klocale.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qcstring.h>
#include "kmmsgbase.h" // for KMMsgStatus

class KMMessage;
class KConfig;
class DwBoyerMoore;
class DwString;


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
  /** Operators for comparison of field and contents.
      If you change the order or contents of the enum: do not forget
      to change funcConfigNames[], sFilterFuncList and matches()
      in @see KMSearchRule, too.
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
  KMSearchRule ( const QCString & field=0, Function=FuncContains,
                 const QString &contents=QString::null );
  KMSearchRule ( const KMSearchRule &other );

  const KMSearchRule & operator=( const KMSearchRule & other );

  /** Create a search rule of a certain type by instantiating the appro-
      priate subclass depending on the @p field. */
  static KMSearchRule* createInstance( const QCString & field=0,
                                      Function function=FuncContains,
		                      const QString & contents=QString::null );

  static KMSearchRule* createInstance( const QCString & field,
                                       const char * function,
                                       const QString & contents );

  static KMSearchRule * createInstance( const KMSearchRule & other );

  /** Initialize the object from a given config file. The group must
      be preset. @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function does no
      validation of the data obtained from the config file. You should
      call @see isEmpty yourself if you need valid rules. */
  static KMSearchRule* createInstanceFromConfig( const KConfig * config, int aIdx );

  virtual ~KMSearchRule() {};

  /** Tries to match the rule against the given @see KMMessage.
      @return TRUE if the rule matched, FALSE otherwise. Must be
      implemented by subclasses.
  */
  virtual bool matches( const KMMessage * msg ) const = 0;

   /** Optimized version tries to match the rule against the given
       @see DwString.
       @return TRUE if the rule matched, FALSE otherwise.
   */
   virtual bool matches( const DwString & str, KMMessage & msg,
                         const DwBoyerMoore * headerField=0,
                         int headerLen=-1 ) const;

  /** Determine whether the rule is worth considering. It isn't if
      either the field is not set or the contents is empty.
      @see KFilter should make sure that it's rule list contains
      only non-empty rules, as @see matches doesn't check this. */
  virtual bool isEmpty() const = 0;

  /** Returns true if the rule depends on a complete message,
      otherwise returns false. */
  virtual bool requiresBody() const { return true; }


  /** Save the object into a given config file. The group must be
      preset. @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function will happily
      write itself even when it's not valid, assuming higher layers to
      Do The Right Thing(TM). */
  void writeConfig( KConfig * config, int aIdx ) const;

  /** Return filter function. This can be any of the operators
      defined in @see Function. */
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
  */
  QCString field() const { return mField; }

  /** Set message header field name (make sure there's no trailing
      colon ':') */
  void setField( const QCString & field ) { mField = field; }

  /** Return the value. This can be either a substring to search for in
      or a regexp pattern to match against the header. */
  QString contents() const { return mContents; }
  /** Set the value. */
  void setContents( const QString & aContents ) { mContents = aContents; }

  /** Returns the rule as string. For debugging.*/
  const QString asString() const;

private:
  static Function configValueToFunc( const char * str );
  static QString functionToString( Function function );

  QCString mField;
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
  KMSearchRuleString( const QCString & field=0, Function function=FuncContains,
		const QString & contents=QString::null );
  KMSearchRuleString( const KMSearchRuleString & other );
  const KMSearchRuleString & operator=( const KMSearchRuleString & other );

  virtual ~KMSearchRuleString();
  virtual bool isEmpty() const ;
  virtual bool requiresBody() const;

  virtual bool matches( const KMMessage * msg ) const;

  /** Optimized version tries to match the rule against the given @see DwString.
      @return TRUE if the rule matched, FALSE otherwise.
  */
  virtual bool matches( const DwString & str, KMMessage & msg,
                        const DwBoyerMoore * headerField=0,
                        int headerLen=-1 ) const;

  /** Helper for the main matches() method. Does the actual comparing. */
  bool matchesInternal( const QString & msgContents ) const;

private:
  const DwBoyerMoore *mBmHeaderField;
};


/** This class represents a search to be performed against a numerical value,
 *  such as the age of the message in days or its size.
    @short This class represents a search pattern rule operating on numerical
    values.
*/

class KMSearchRuleNumerical : public KMSearchRule
{
public:
  KMSearchRuleNumerical( const QCString & field=0, Function function=FuncContains,
		         const QString & contents=QString::null );
  virtual bool isEmpty() const ;

  virtual bool matches( const KMMessage * msg ) const;

  /** Helper for the main matches() method. Does the actual comparing. */
  bool matchesInternal( long numericalValue, long numericalMsgContents,
                        const QString & msgContents ) const;
};


/** This class represents a search to be performed against the status of a
 *  messsage. The status is represented by a bitfield.
    @short This class represents a search pattern rule operating on message
    status.
*/
namespace KMail {
// The below are used in several places and here so they are accessible.
  struct MessageStatus {
    const char * const text;
    const char * const icon;
  };

  static const MessageStatus StatusValues[] = {
    { I18N_NOOP( "Important" ),        "kmmsgflag"      },
    { I18N_NOOP( "New" ),              "kmmsgnew"       },
    { I18N_NOOP( "Unread" ),           "kmmsgunseen"    },
    { I18N_NOOP( "Read" ),             "kmmsgread"      },
    { I18N_NOOP( "Old" ),              0                },
    { I18N_NOOP( "Deleted" ),          "edittrash"      },
    { I18N_NOOP( "Replied" ),          "kmmsgreplied"   },
    { I18N_NOOP( "Forwarded" ),        "kmmsgforwarded" },
    { I18N_NOOP( "Queued" ),           "kmmsgqueued"    },
    { I18N_NOOP( "Sent" ),             "kmmsgsent"      },
    { I18N_NOOP( "Watched" ),          "kmmsgwatched"   },
    { I18N_NOOP( "Ignored" ),          "kmmsgignored"   },
    { I18N_NOOP( "Spam" ),             "mark_as_spam"   },
    { I18N_NOOP( "Ham" ),              "mark_as_ham"    },
    { I18N_NOOP( "Has an Attachment"), "attach"         },
    { I18N_NOOP( "To Do" ),            "kontact_todo"   }
  };

  static const int StatusValueCount =
    sizeof( StatusValues ) / sizeof( MessageStatus );
  // we want to show all status entries in the quick search bar, but only the
  // ones up to attachment in the search/filter dialog, because there the
  // attachment case is handled separately.
  // Todo is hidden for both because it can currently not be set anywhere
  static const int StatusValueCountWithoutHidden = StatusValueCount - 2;
}

class KMSearchRuleStatus : public KMSearchRule
{
public:
   KMSearchRuleStatus( const QCString & field=0, Function function=FuncContains,
		       const QString & contents=QString::null );
  virtual bool isEmpty() const ;
  virtual bool matches( const KMMessage * msg ) const;
  //Not possible to implement this form for status searching
  virtual bool matches( const DwString &, KMMessage &,
                        const DwBoyerMoore *,
			int ) const;
  static KMMsgStatus statusFromEnglishName(const QString&);
  private:
  KMMsgStatus mStatus;
};

// ------------------------------------------------------------------------

/** This class is an abstraction of a search over messages.  It is
    intended to be used inside a @see KFilter (which adds
    @see KFilterAction's), as well as in @see KMSearch. It can read
    and write itself into a @see KConfig group and there is a
    constructor, mainly used by @see KMFilter to initialize from a
    preset KConfig-Group.

    From a class hierarchy point of view, it is a @see QPtrList of @see
    KMSearchRule's that adds the boolean operators (see @see Operator)
    'and' and 'or' that connect the rules logically, and has a name
    under which it could be stored in the config file.

    As a @see QPtrList with autoDelete enabled, it assumes that it is the
    central repository for the rules it contains. So if you want to
    reuse a rule in another pattern, make a deep copy of that rule.

    @short An abstraction of a search over messages.
    @author Marc Mutz <Marc@Mutz.com>
*/
class KMSearchPattern : public QPtrList<KMSearchRule>
{

public:
  /** Boolean operators that connect the return values of the
      individual rules. A pattern with @p OpAnd will match iff all
      it's rules match, whereas a pattern with @p OpOr will match iff
      any of it's rules matches. */
  enum Operator { OpAnd, OpOr };
  /** Constructor that initializes from a given @see KConfig group, if
      given. This feature is mainly (solely?) used in @see KMFilter,
      as we don't allow to store search patterns in the config (yet).
      If config is 0, provides a pattern with minimal, but
      sufficient initialization. Unmodified, such a pattern will fail
      to match any @see KMMessage. You can query for such an empty
      rule by using @see isEmpty, which is inherited from @see
      QPtrList. */
  KMSearchPattern( const KConfig * config=0 );

  /** Destructor. Deletes all stored rules! */
  ~KMSearchPattern();

  /** The central function of this class. Tries to match the set of
      rules against a @see KMMessage. It's virtual to allow derived
      classes with added rules to reimplement it, yet reimplemented
      methods should and (&&) the result of this function with their
      own result or else most functionality is lacking, or has to be
      reimplemented, since the rules are private to this class.

      @return TRUE if the match was successful, FALSE otherwise.
  */
  bool matches( const KMMessage * msg, bool ignoreBody = false ) const;
  bool matches( const DwString & str, bool ignoreBody = false ) const;
  bool matches( Q_UINT32 sernum, bool ignoreBody = false ) const;

  /** Returns true if the pattern only depends the DwString that backs
      a message */
  bool requiresBody() const;

  /** Removes all empty rules from the list. You should call this
      method whenever the user had had control of the rules outside of
      this class. (e.g. after editing it with @see
      KMSearchPatternEdit).
  */
  void purify();

  /** Reads a search pattern from a @see KConfig.  The group has to be
      preset. If it does not find a valid saerch pattern in the preset
      group, initializes the pattern as if it were constructed using
      the default constructor.

      For backwards compatibility with previous versions of KMail, it
      checks for old-style filter rules (e.g. using @p OpIgnore)
      in @p config und converts them to the new format on @see
      writeConfig.

      Derived classes reimplementing readConfig() should also call this
      method, or else the rules will not be loaded.
  */
  void readConfig( const KConfig * config );
  /** Writes itself into @p config. The group has to be preset. Tries
      to delete old-style keys by overwriting them with QString::null.

      Derived classes reimplementing writeConfig() should also call this
      method, or else the rules will not be stored.
  */
  void writeConfig( KConfig * config ) const;

  /** Get the name of the search pattern. */
  QString name() const { return mName; }
  /** Set the name of the search pattern. @see KMFilter uses this to
      store it's own name, too. */
  void setName( const QString & newName ) { mName = newName ; }

  /** Get the filter operator */
  KMSearchPattern::Operator op() const { return mOperator; }
  /** Set the filter operator */
  void setOp( KMSearchPattern::Operator aOp ) { mOperator = aOp; }

  /** Returns the pattern as string. For debugging.*/
  QString asString() const;

  /** Overloaded assignment operator. Makes a deep copy. */
  const KMSearchPattern & operator=( const KMSearchPattern & aPattern );

private:
  /** Tries to import a legacy search pattern, ie. one that still has
      e.g. the @p unless or @p ignore operator which were useful as
      long as the number of rules was restricted to two. This method
      is called from @see readConfig, which detects legacy
      configurations and also makes sure that this method is called
      from an @see init'ialized object. */
  void importLegacyConfig( const KConfig * config );
  /** Initializes the object. Clears the list of rules, sets the name
      to "<i18n("unnamed")>", and the boolean operator to @p OpAnd. */
  void init();

  QString  mName;
  Operator mOperator;
};

#endif /* _kmsearchpattern_h_ */
