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
  debug("KMAcctFolder: folder %s has %d accounts", (const char*)name(),
	mAcctList.count());
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
  debug("KMAcctFolder: adding account `%s' to folder `%s'",
	(const char*)aAcct->name(), (const char*)name());
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
  int i, num;

  if (!mAutoCreateToc) return 0;

  num = mAcctList.count();
  fprintf(mTocStream, "%d %c\n", num, mIsSystemFolder ? 'S' : 'U');
  for (i=0,act=account(); act && i<num; act=nextAccount(), i++)
  {
    fprintf(mTocStream, "%.64s\n", (const char*)act->name());
  }
  while (i < num)
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

#ifdef OBSOLETE
  clearAccountList();
#endif

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

#ifdef OBSOLETE
    act = acctMgr->find(line);
    if (act) addAccount(act);
#endif
  }
}


