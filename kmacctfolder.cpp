// kmacctfolder.cpp

#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmglobal.h"

#define MAX_ACCOUNTS 16

//-----------------------------------------------------------------------------
KMAcctFolder::KMAcctFolder(KMFolderDir* aParent, const char* aName):
  KMAcctFolderInherited(aParent,aName)
{
}


//-----------------------------------------------------------------------------
KMAcctFolder::~KMAcctFolder()
{
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::account(void)
{
  return mAcctList.first();
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctFolder::nextAccount(void)
{
  return mAcctList.next();
}


//-----------------------------------------------------------------------------
void KMAcctFolder::addAccount(KMAccount* aAcct)
{
  if (!aAcct) return;

  mAcctList.append(aAcct);
  aAcct->setFolder(this);
}


//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList(void)
{
  mAcctList.clear();
  debug ("KMAcctFolder::clearAccountList():\n"
	   "unregister folders in accounts here !");
}


//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount(KMAccount* aAcct)
{
  if (!aAcct) return;

  mAcctList.remove(aAcct);
  aAcct->setFolder(NULL);
}


//-----------------------------------------------------------------------------
int KMAcctFolder::createTocHeader(void)
{
  KMAccount* act;
  int i;

  if (!mAutoCreateToc) return 0;

  fprintf(mTocStream, "%d\n", MAX_ACCOUNTS);
  for (i=0,act=account(); act && i<MAX_ACCOUNTS; act=nextAccount(), i++)
  {
    fprintf(mTocStream, "%.64s\n", (const char*)act->name());
  }
  while (i<MAX_ACCOUNTS)
  {
    fprintf(mTocStream, "%.64s\n", "");
    i++;
  }

  return 0;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::readTocHeader(void)
{
  char line[256];
  int  numAcct, i;
  KMAccount* act;

  clearAccountList();

  fgets(line, 255, mTocStream);
  numAcct = atoi(line);

  for (; numAcct > 0; numAcct--)
  {
    fgets(line, 255, mTocStream);
    for (i=strlen(line)-1; line[i]<=' ' && i>=0; i--)
      ;
    if (i < 0) continue;
    line[i+1] = '\0';

    act = acctMgr->find(line);
    if (act) addAccount(act);
    else warning("cannot find account '" + QString(line) + "' for folder '" +
		 name() + "'.");
  }
}


