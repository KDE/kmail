// kmfilter.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include <config.h>
#include "kmfilter.h"
#include "kmglobal.h"
#include "kmmessage.h"

#include <kconfig.h>
#include <kdebug.h>

#include <qregexp.h>
#include <assert.h>
#include <string.h>
#include <kapp.h>
#include <klocale.h>
#include <kmessagebox.h>

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

  //assert(strList != NULL);
  if(!strList)
    {
      kdDebug() << "KMFilter::findInStrList() : strList == NULL\n" << endl;
      return -1; // we return -1 here. Fake unsuccessfull search
    }

  if (!str) return -1;

  for (i=0; strList[i]; i++)
    if (strcasecmp(strList[i], str)==0) return i;

  return -1;
}


//-----------------------------------------------------------------------------
KMFilter::KMFilter(KConfig* config)
{
  int i;

  if (!sActionDict) sActionDict = new KMFilterActionDict;
  if (config) readConfig(config);
  else
  {
    mName = QString::null;
    mOperator = OpIgnore;
    for (i=0; i<=FILTER_MAX_ACTIONS; i++)
      mAction[i] = NULL;
  }
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
int KMFilter::execActions(KMMessage* msg, bool& stopIt)
{
  int  i;
  int status = -1;
  int result;
  stopIt = FALSE;

  for (i=0; !stopIt && mAction[i] && i<=FILTER_MAX_ACTIONS; i++) {
    result = mAction[i]->process(msg,stopIt);
    if (result == 2) { // Critical error
      status = 2;
      break;
    }
    else if (result == 1) // Small problem, keep of a copy
      status = 1;
    else if ((result == 0) && (status < 0))  // Message saved in a folder
      status = 0;
  }

  if (status < 0) // No filters matched, keep copy of message
    status = 1;

  return status;
}


//-----------------------------------------------------------------------------
bool KMFilter::folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder)
{
  int  i;
  bool rem = FALSE;

  for (i=0; mAction[i] && i<=FILTER_MAX_ACTIONS; i++)
    if (mAction[i]->folderRemoved(aFolder,aNewFolder)) rem=TRUE;

  return rem;
}


//-----------------------------------------------------------------------------
void KMFilter::setName(const QString &aName)
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
  int idx, i, j, num;
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
  if (num >= FILTER_MAX_ACTIONS)
  {
    num = FILTER_MAX_ACTIONS - 1;
    KMessageBox::information(0,i18n("Too many filter actions in filter rule `%1'").arg(mName));
  }

  for (i=0, j=0; i<num; i++)
  {
    actName.sprintf("action-name-%d", i);
    argsName.sprintf("action-args-%d", i);
    actName = config->readEntry(actName);
    mAction[j] = sActionDict->create(actName);
    if (!mAction[j])
    {
      if  (actName != "skip rest")
	KMessageBox::information(0,i18n("Unknown filter action `%1'\n in filter rule `%2'."
					"\nIgnoring it.").arg(actName).arg(mName));
      continue;
    }
    mAction[j]->argsFromString(config->readEntry(argsName));
    j++;
  }

  while (j<=FILTER_MAX_ACTIONS)
    mAction[j++] = NULL;
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


//-----------------------------------------------------------------------------
QString KMFilter::asString(void) const
{
  QString result;
  int i;

  result = "Filter \"" + mName + "\":\n{\n    ";
  result += mRuleA.asString();
  result += "\n    ";
  result += opConfigNames[(int)mOperator];
  result += "\n    ";
  result += mRuleB.asString();
  result += "\n";
  for (i=0; i<=FILTER_MAX_ACTIONS && mAction[i]; i++)
  {
    result += "    action: ";
    result += mAction[i]->label();
    result += " ";
    result += mAction[i]->argsAsString();
    result += "\n";
  }
  result += "}";

  return result;
}



//=============================================================================
//
//  Class  KMFilterRule
//
//=============================================================================

KMFilterRule::KMFilterRule()
{
  mField    = "";
  mFunction = FuncContains;
  mContents = "";
}


//-----------------------------------------------------------------------------
void KMFilterRule::init(const QString &aField, Function aFunction,
			const QString &aContents)
{
  mField    = aField;
  mFunction = aFunction;
  mContents = aContents;
}


//-----------------------------------------------------------------------------
bool KMFilterRule::matches(const KMMessage* msg)
{
  QString msgContents;

  assert(msg != NULL); // This assert seems to be important

  if( mField == QString("<message>") ) {
    // there's msg->asString(), but this way we can keep msg const (dnaber, 1999-05-27)
    msgContents = msg->headerAsString();
    msgContents += msg->bodyDecoded();
  } else if( mField == QString("<body>") ) {
    msgContents = msg->bodyDecoded();
  } else if( mField == QString("<any header>") ) {
    msgContents = msg->headerAsString();
  } else if( mField == QString("<To or Cc>") ) {
    msgContents = msg->headerField("To");
    msgContents += "\n";
    msgContents += msg->headerField("Cc");
  } else {
    msgContents = msg->headerField(mField);
  }

  // also see KMFldSearchRule::matches() for a similar function:
  switch (mFunction)
  {
  case KMFilterRule::FuncEquals:
    return (qstricmp(mContents, msgContents) == 0);

  case KMFilterRule::FuncNotEqual:
    return (qstricmp(mContents, msgContents) != 0);

  case KMFilterRule::FuncContains:
    return msgContents.contains(mContents, FALSE);

  case KMFilterRule::FuncContainsNot:
    return ( msgContents.find(mContents, FALSE) < 0 );

  case KMFilterRule::FuncRegExp:
    return (msgContents.find(QRegExp(mContents, FALSE)) >= 0);

  case KMFilterRule::FuncNotRegExp:
    return (msgContents.find(QRegExp(mContents, FALSE)) < 0);
  }

  return FALSE;
}


//-----------------------------------------------------------------------------
QString KMFilterRule::asString(void) const
{
  QString result;

  result  = "\"" + mField + "\" ";
  result += funcConfigNames[(int)mFunction];
  result += " \"" + mContents + "\"";

  return result;
}
