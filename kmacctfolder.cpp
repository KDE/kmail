// kmacctfolder.cpp

#include "kmacctfolder.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmglobal.h"
#include <stdlib.h>

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
const char* KMAcctFolder::type(void) const
{
  if (!mAcctList.isEmpty()) return "in";
  return KMAcctFolderInherited::type();
}


//-----------------------------------------------------------------------------
const QString KMAcctFolder::label(void) const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  return name();
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
  mTocDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::clearAccountList(void)
{
  KMAccount* acct;

  for (acct=mAcctList.first(); acct; acct=mAcctList.next())
    acct->setFolder(NULL);
  mAcctList.clear();

  mTocDirty = TRUE;
}


//-----------------------------------------------------------------------------
void KMAcctFolder::removeAccount(KMAccount* aAcct)
{
  if (!aAcct) return;

  mAcctList.remove(aAcct);
  aAcct->setFolder(NULL);
  mTocDirty = TRUE;
}


//-----------------------------------------------------------------------------
int KMAcctFolder::createTocHeader(void)
{
  KMAccount* act;
  int i;

  if (!mAutoCreateToc) return 0;

  fprintf(mTocStream, "%d %c\n", MAX_ACCOUNTS, mIsSystemFolder ? 'S' : 'U');
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

  if (strlen(line) > 3 && line[3]=='S')
  {
    mIsSystemFolder = TRUE;
  }
  else mIsSystemFolder = FALSE;

  line[2] = '\0';
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
  }
}


