// kmsearchpattern.h
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL!

#ifndef _kmsearchpattern_h_
#define _kmsearchpattern_h_

#include <qptrlist.h>
#include <qstring.h>
#include <qcstring.h>

class KMMessage;
class KConfig;
class DwBoyerMoore;
class DwString;


// maximum number of filter rules per filter
const int FILTER_MAX_RULES=8;

/** Incoming mail is sent through the list of mail filter
    rules before it is placed in the associated mail folder (usually "inbox").
    This class represents one mail filter rule.

    @short This class represents one search pattern rule.
*/
class KMSearchRule
{
public:
  /** Operators for comparison of field and contents.
      If you change the order or contents of the enum: do not forget
      to change funcConfNames[], sFilterFuncList and matches()
      in @ref KMSearchRule, too.
      Also, it is assumed that these functions come in pairs of logical
      opposites (ie. "=" <-> "!=", ">" <-> "<=", etc.).
  */
  enum Function { FuncContains=0, FuncContainsNot,
		  FuncEquals, FuncNotEqual,
		  FuncRegExp, FuncNotRegExp,
		  FuncIsGreater, FuncIsLessOrEqual,
		  FuncIsLess, FuncIsGreaterOrEqual };

  /** Constructor. Initializes the field and the value to the empty
      string and the function to @p FuncEquals. Use @ref init to set
      other data.*/
  KMSearchRule();
  ~KMSearchRule();

  /** Initialize the rule.
      @param field
      The header field to search or one of the pseudo-headers, see @ref field.
      @param function
      The function/operator to use, see @ref function.
      @param contents
      The value to use, see @ref value.
  */
  void init( const QCString & aField, Function aFunction, const QString & aContents );

  /** This is an overloaded member funcion, provided for convenience.
      It differs from the above only in what arguments it takes. */
  void init( const KMSearchRule * aRule=0 );

  /** This is an overloaded member funcion, provided for convenience.
      It differs from the above only in what arguments it takes,
      namely the stringified version of the function, as used in the
      config file. */
  void init( const QCString & aField, const char * aStrFunction, const QString & aContents );

  /** Tries to match the rule against the given @ref KMMessage.
      @return TRUE if the rule matched, FALSE otherwise.
  */
  bool matches( const KMMessage * msg ) const;

  /** Optimized version tries to match the rule against the given @ref DwString.
      @return TRUE if the rule matched, FALSE otherwise.
  */
  bool matches( const DwString & str, KMMessage & msg, DwBoyerMoore * headerField=0, int headerLen=-1 ) const;

  /** Initialize the object from a given config file. The group must
      be preset. @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function does no
      validation of the data obtained from the config file. You should
      call @ref isEmpty yourself if you need valid rules. */
  void readConfig( const KConfig * config, int aIdx );
  /** Save the object into a given config file. The group must be
      preset. @p aIdx is an identifier that is used to distinguish
      rules within a single config group. This function will happily
      write itself even when it's not valid, assuming higher layers to
      Do The Right Thing(TM). */
  void writeConfig( KConfig * config, int aIdx ) const;

  /** Determine whether the rule is worth considering. It isn't if
      either the field is not set or the contents is empty.
      @ref KFilter should make sure that it's rule list contains
      only non-empty rules, as @ref matches doesn't check this. */
  bool isEmpty() const;

  /** Return filter function. This can be any of the operators
      defined in @ref Function. */
  Function function() const { return mFunction; }

  /** Set filter function. */
  void setFunction( Function aFunction ) { mFunction = aFunction; }

  /** Return message header field name (without the trailing ':').
      There are also five pseudo-headers:
      @li <message>: Try to match against the whole message.
      @li <body>: Try to match against the body of the message.
      @li <any header>: Try to match against any header field.
      @li <recipients>: Try to match against both To: and Cc: header fields.
      @li <size>: Try to match against size of message (numerical).
      @li <age in days>: Try to match against age of message (numerical).
  */
  QCString field() const { return mField; }
  /** Set message header field name (make sure there's no trailing
      colon ':') */
  void setField( const QCString & aField ) { mField = aField; }

  /** Returns true if the rule only depends on fields stored in
      a KMFolder index, otherwise returns false. */
  bool requiresBody() const { return true; }

  /** Return the value. This can be either a substring to search for in
      or a regexp pattern to match against the header. */
  QString contents() const { return mContents; }
  /** Set the value. */
  void setContents( const QString & aContents ) { mContents = aContents; }

  /** Returns the rule as string. For debugging.*/
#ifndef NDEBUG
  const QString asString() const;
#endif

private:
  /** Helper for the main matches() method */
  bool matches( bool numerical, unsigned long numericalValue, unsigned long numericalMsgContents, const QString & msgContents ) const;
  static Function configValueToFunc( const char * str );
  QCString  mField;
  Function mFunction;
  QString  mContents;
  DwBoyerMoore *mBmHeaderField, *mBmEndHeaders1, *mBmEndHeaders2, *mBmEndHeader;
};


/** This class is an abstraction of a search over messages.  It is
    intended to be used inside a @ref KFilter (which adds
    @ref KFilterAction's), as well as in @ref KMSearch. It can read
    and write itself into a @ref KConfig group and there is a
    constructor, mainly used by @ref KMFilter to initialize from a
    preset KConfig-Group.

    From a class hierarchy point of view, it is a @ref QPtrList of @ref
    KMSearchRule's that adds the boolean operators (see @ref Operator)
    'and' and 'or' that connect the rules logically, and has a name
    under which it could be stored in the config file.

    As a @ref QPtrList with autoDelete enabled, it assumes that it is the
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
  /** Constructor that initializes from a given @ref KConfig group, if
      given. This feature is mainly (solely?) used in @ref KMFilter,
      as we don't allow to store search patterns in the config (yet).
      If config is 0, provides a pattern with minimal, but
      sufficient initialization. Unmodified, such a pattern will fail
      to match any @ref KMMessage. You can query for such an empty
      rule by using @ref isEmpty, which is inherited from @ref
      QPtrList. */
  KMSearchPattern( const KConfig * config=0 );

  /** Destructor. Deletes all stored rules! */
  ~KMSearchPattern() {}

  /** The central function of this class. Tries to match the set of
      rules against a @ref KMMessage. It's virtual to allow derived
      classes with added rules to reimplement it, yet reimplemented
      methods should and (&&) the result of this function with their
      own result or else most functionality is lacking, or has to be
      reimplemented, since the rules are private to this class.

      @return TRUE if the match was successful, FALSE otherwise.
  */
  bool matches( const KMMessage * msg ) const;
  bool matches( const DwString & str ) const;

  /** Removes all empty rules from the list. You should call this
      method whenever the user had had control of the rules outside of
      this class. (e.g. after editing it with @ref
      KMSearchPatternEdit).
  */
  void purify();

  /** Reads a search pattern from a @ref KConfig.  The group has to be
      preset. If it does not find a valid saerch pattern in the preset
      group, initializes the pattern as if it were constructed using
      the default constructor.

      For backwards compatibility with previous versions of KMail, it
      checks for old-style filter rules (e.g. using @p OpIgnore)
      in @p config und converts them to the new format on @ref
      writeConfig.

      Derived classes reimplementing readConfig() should also call this
      method, or else the rules will not be loaded.

      NOTE: I don't know yet if calling
      Kconfig::writeEntry("key",QString()) makes the key go away in
      the on-disk file. I haven't found another possible way to delete
      old keys.
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
  /** Set the name of the search pattern. @ref KMFilter uses this to
      store it's own name, too. */
  void setName( const QString & newName ) { mName = newName ; }

  /** Get the filter operator */
  KMSearchPattern::Operator op() const { return mOperator; }
  /** Set the filter operator */
  void setOp( KMSearchPattern::Operator aOp ) { mOperator = aOp; }

  /** Returns the pattern as string. For debugging.*/
#ifndef NDEBUG
  QString asString() const;
#endif

  /** Overloaded assignment operator. Makes a deep copy. */
  const KMSearchPattern & operator=( const KMSearchPattern & aPattern );

private:
  /** Tries to import a legacy search pattern, ie. one that still has
      e.g. the @p unless or @p ignore operator which were useful as
      long as the number of rules was restricted to two. This method
      is called from @ref readConfig, which detects legacy
      configurations and also makes sure that this method is called
      from an @ref init'ialized object. */
  void importLegacyConfig( const KConfig * config );
  /** Initializes the object. Clears the list of rules, sets the name
      to "<i18n("unnamed")>", and the boolean operator to @p OpAnd. */
  void init();

  QString  mName;
  Operator mOperator;
};

#endif /* _kmsearchpattern_h_ */
