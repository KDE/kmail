// kmfilter.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmfilter.h"
#include "kmglobal.h"
#include "kmmessage.h"

#include <kconfig.h>
#include <qregexp.h>
#include <assert.h>
#include <string.h>
#include <klocale.h>

static const char* opConfigNames[] = 
  { "ignore", "and", "unless", "or", NULL };

static const char* funcConfigNames[] =
  { "equals", "not-equal", "contains", "contains-not", "regexp", 
    "not-regexp", NULL };


KMFilterActionDict* KMFilter::sActionDict = NULL;


//-----------------------------------------------------------------------------
// Case-insensitive. Searches for string in string list and returns index
// or -1 if not found.
static int findInStrList(const char* strList[], const char* str)
{
  int i;
  assert(strList != NULL);
  if (!str) return -1;

  for (i=0; strList[i]; i++)
    if (strcasecmp(strList[i], str)) return i;

  return -1;
}


//-----------------------------------------------------------------------------
KMFilter::KMFilter(KConfig* config)
{
  if (!sActionDict) sActionDict = new KMFilterActionDict;
  readConfig(config);
}


//-----------------------------------------------------------------------------
KMFilter::~KMFilter()
{
}


//-----------------------------------------------------------------------------
bool KMFilter::matches(const KMMessage* msg)
{
  bool matchesA, matchesB;

  matchesA = mRuleA.matches(msg);
  if (mOperator==OpIgnore) return matchesA;
  if (matchesA && mOperator==OpOr) return TRUE;
  if (!matchesA && (mOperator==OpAnd || mOperator==OpAndNot)) return FALSE;

  matchesB = mRuleB.matches(msg);
  switch (mOperator)
  {
  case OpAnd: 
    return (matchesA && matchesB);
  case OpAndNot:
    return (matchesA && !matchesB);
  case OpOr:
    return (matchesA || matchesB);
  default:
    return FALSE;
  }
}


//-----------------------------------------------------------------------------
bool KMFilter::execActions(KMMessage* msg, bool& stopIt)
{
  int  i;
  bool stillOwner = TRUE;
  stopIt = FALSE;

  for (i=0; !stopIt && mAction[i]; i++)
    if (!mAction[i]->process(msg,stopIt)) stillOwner = FALSE;

  return stillOwner;
}


//-----------------------------------------------------------------------------
void KMFilter::setName(const QString aName)
{
  mName = aName.copy();
}


//-----------------------------------------------------------------------------
void KMFilter::setOper(KMFilter::Operator aOperator)
{
  mOperator = aOperator;
}


//-----------------------------------------------------------------------------
KMFilterAction* KMFilter::action(int idx) const
{
  if (idx<0 || idx >= FILTER_MAX_ACTIONS) return NULL;
  return mAction[idx];
}


//-----------------------------------------------------------------------------
void KMFilter::setAction(int idx, KMFilterAction* aAction)
{
  if (idx>=0 && idx < FILTER_MAX_ACTIONS)
    mAction[idx] = aAction;
}


//-----------------------------------------------------------------------------
void KMFilter::readConfig(KConfig* config)
{
  KMFilterRule::Function funcA, funcB;
  QString fieldA, fieldB;
  QString contA, contB;
  int idx, i, num;
  QString actName, argsName;

  mName = config->readEntry("name").copy();

  idx = findInStrList(funcConfigNames, config->readEntry("funcA"));
  funcA = (KMFilterRule::Function)(idx >= 0 ? idx : 0);

  idx = findInStrList(funcConfigNames, config->readEntry("funcB"));
  funcB = (KMFilterRule::Function)(idx >= 0 ? idx : 0);

  fieldA = config->readEntry("fieldA");
  fieldB = config->readEntry("fieldB");

  contA = config->readEntry("contentsA");
  contB = config->readEntry("contentsB");

  idx = findInStrList(opConfigNames, config->readEntry("operator"));
  mOperator = (KMFilter::Operator)(idx >= 0 ? idx : 0);

  mRuleA.init(fieldA, funcA, contA);
  mRuleB.init(fieldB, funcB, contB);

  num = config->readNumEntry("actions",0);
  if (num > FILTER_MAX_ACTIONS)
  {
    num = FILTER_MAX_ACTIONS;
    debug("Too many filter actions in filter rule `%s'", (const char*)mName);
  }

  for (i=0; i<num; i++)
  {
    actName.sprintf("action-name-%d", i);
    argsName.sprintf("action-args-%d", i);
    actName = config->readEntry(actName);
    mAction[i] = NULL; //sActionDict->find(actName);
    if (!mAction[i])
    {
      warning(nls->translate("Unknown filter action `%s'\n"
			     "in filter rule `%s'.\n"
			     "Ignoring it."),
	      (const char*)actName, (const char*)mName);
      i--;
      continue;
    }
    mAction[i]->argsFromString(config->readEntry(argsName));
  }

  while (i<=FILTER_MAX_ACTIONS)
    mAction[i++] = NULL;
}


//-----------------------------------------------------------------------------
void KMFilter::writeConfig(KConfig* config)
{
  QString key;
  int i;

  config->writeEntry("name", mName);
  config->writeEntry("funcA", funcConfigNames[(int)mRuleA.function()]);
  config->writeEntry("funcB", funcConfigNames[(int)mRuleB.function()]);
  config->writeEntry("fieldA", mRuleA.field());
  config->writeEntry("fieldB", mRuleB.field());
  config->writeEntry("contentsA", mRuleA.contents());
  config->writeEntry("contentsB", mRuleB.contents());
  config->writeEntry("operator", opConfigNames[(int)mOperator]);

  for (i=0; i<FILTER_MAX_ACTIONS && mAction[i]; i++)
  {
    key.sprintf("action-name-%d", i);
    config->writeEntry(key, mAction[i]->name());
    key.sprintf("action-args-%d", i);
    config->writeEntry(key, mAction[i]->argsAsString());
  }

  config->writeEntry("actions", i);
}




//=============================================================================
//
//  Class  KMFilterRule
//
//=============================================================================

KMFilterRule::KMFilterRule()
{
  mField    = NULL;
  mFunction = FuncContains;
  mContents = "";
}


//-----------------------------------------------------------------------------
void KMFilterRule::init(const QString aField, Function aFunction,
			const QString aContents)
{
  mField    = aField.copy();
  mFunction = aFunction;
  mContents = aContents.copy();
}


//-----------------------------------------------------------------------------
bool KMFilterRule::matches(const KMMessage* msg)
{
  QString msgContents;

  assert(msg != NULL);
  msgContents = msg->headerField(mField);

  switch (mFunction)
  {
  case KMFilterRule::FuncEquals:
    return (stricmp(mContents, msgContents) == 0);

  case KMFilterRule::FuncNotEqual:
    return (stricmp(mContents, msgContents) != 0);

  case KMFilterRule::FuncContains:
    return (msgContents.find(mContents, 0, FALSE) >= 0);

  case KMFilterRule::FuncContainsNot:
    return (msgContents.find(mContents, 0, FALSE) < 0);

  case KMFilterRule::FuncRegExp:
    return (msgContents.find(QRegExp(mContents, FALSE)) >= 0);

  case KMFilterRule::FuncNotRegExp:
    return (msgContents.find(QRegExp(mContents, FALSE)) < 0);
  }

  return FALSE;
}
