/* Mail Filter Rule : incoming mail is sent trough the list of mail filter
 * rules before it is placed in the associated mail folder (usually "inbox").
 * This class represents one mail filter rule.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilter_h
#define kmfilter_h

#include <qobject.h>
#include <qstring.h>

#include "kmfilteraction.h"

class KMMessage;
class KMFilter;

//-----------------------------------------------------------------------------
class KMFilterRule
{
public:
  // Operators for comparison of field and contents
  enum Function { FuncEquals, FuncContains, FuncRegExp };

  // Returns TRUE if this rule matches the given message.
  bool matches(const KMMessage*);

protected:
  KMFilterRule(const QString field, Function func, const QString contents);
  friend class KMFilter;

  QString  mField;
  Function mFunction;
  QString  mContents;
};


//-----------------------------------------------------------------------------
#define KMFilterInherited QObject

class KMFilter: public QObject
{
  Q_OBJECT;

public:
  enum Operator { OpIgnore=0, OpAnd, OpAndNot, OpOr, OpOrNot };

  KMFilter(const QString name, const KMFilterRule* ruleA, 
	   const KMFilterRule* ruleB, const KMFilter::Operator op,
	   const KMFilterAction* action);

  /** If the filter rule(s) match the given message the filter action
      is taken and TRUE is returned. Otherwise nothing happens and FALSE
      is returned. */
  virtual bool matches(const KMMessage*);

protected:
  QString         mName
  KMFilterRule    mRuleA, mRuleB;
  Operator        mOperator;
  KMFilterAction  mAction;
};

#endif /*kmfilter_h*/
