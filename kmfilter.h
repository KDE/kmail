/* Mail Filter Rule : incoming mail is sent trough the list of mail filter
 * rules before it is placed in the associated mail folder (usually "inbox").
 * This class represents one mail filter rule.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilter_h
#define kmfilter_h

#include <qstring.h>

#include "kmfilteraction.h"

class KMMessage;
class KMFilter;
class KConfig;

// maximum number of filter actions per filter
#define FILTER_MAX_ACTIONS 5


//-----------------------------------------------------------------------------
class KMFilterRule
{
public:
  /** Operators for comparison of field and contents. */
  // If you change the order or contents of the enum: do not forget
  // to change the string list in kmfilter.cpp
  enum Function { FuncEquals, FuncContains, FuncRegExp };

  /** Initializing constructor. */
  KMFilterRule();

  /** Initialize the rule. */
  init(const QString field, Function function, const QString contents);

  /** Return TRUE if this rule matches the given message. */
  bool matches(const KMMessage* msg);

  /** Return filter function. */
  Function function(void) const { return mFunction; }

  /** Return message field name. */
  const QString field(void) const { return mField; }

  /** Return expected field contents. */
  const QString contents(void) const { return mContents; }

protected:
  QString  mField;
  Function mFunction;
  QString  mContents;
};


//-----------------------------------------------------------------------------
class KMFilter
{
public:
  /** Filter operators. Boolean operators on how rule A and B shall
   * be handled together. 
   */
  // If you change the order or contents of the enum: do not forget
  // to change the string list in kmfilter.cpp
  enum Operator { OpIgnore=0, OpAnd, OpAndNot, OpOr, OpOrNot };

  /** Constructor that initializes from given config file. The config
    * group is preset. */
  KMFilter(KConfig* config);

  /** Cleanup. */
  virtual ~KMFilter();

  /** Returns TRUE if the filter rules match the given message. */
  virtual bool matches(const KMMessage* msg);

  /** Execute the filter action(s) on the given message. stopIt contains
   * TRUE if the caller may apply other filters and FALSE if he shall
   * stop the filtering of this message. 
   * Returns TRUE if the caller is still the owner of the message. */
  virtual bool execActions(KMMessage* msg, bool& stopIt);

  /**
   * Write contents to given config file. The config group is preset. 
   * The config object will be deleted, so it is not allowed to
   * store a pointer to it anywhere. */
  virtual void writeConfig(KConfig* config);

  /**
   * Initialize from given config file. The config group is preset. 
   * The config object will be deleted, so it is not allowed to
   * store a pointer to it anywhere. */
  virtual void readConfig(KConfig* config);

  /** Get/set name of the filter. */
  const QString name(void) const { return mName; }
  virtual void setName(const QString newName);

  /** Access to the filter rules. */
  KMFilterRule& ruleA(void) { return mRuleA; }
  KMFilterRule& ruleB(void) { return mRuleB; }
  const KMFilterRule& ruleA(void) const { return mRuleA; }
  const KMFilterRule& ruleB(void) const { return mRuleB; }

  /** Get/set filter operator. */
  KMFilter::Operator oper(void) const { return mOperator; }
  virtual void setOper(KMFilter::Operator op);

  /** Get/set filter actions. */
  KMFilterAction* action(int index) const;
  virtual void setAction(int index, KMFilterAction* action);

protected:
  QString         mName;
  KMFilterRule    mRuleA, mRuleB;
  Operator        mOperator;
  KMFilterAction* mAction[FILTER_MAX_ACTIONS+1];

  static KMFilterActionDict* sActionDict;
};

#endif /*kmfilter_h*/
