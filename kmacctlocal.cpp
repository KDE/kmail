// kmacctlocal.cpp

#include "kmacctlocal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmacctfolder.h"

#include <stdlib.h>


//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctLocalInherited(aOwner, aAccountName)
{
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
  writeConfig();
  mLocation = NULL;
}


//-----------------------------------------------------------------------------
const char* KMAcctLocal::type(void) const
{
  return "local";
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::processNewMail(void)
{
  KMFolder mailFolder(NULL, location());
  long num = 0;
  long i;
  KMMessage* msg;

  assert(mFolder != NULL);

  mailFolder.setAutoCreateToc(FALSE);
  if (mailFolder.open()) return FALSE;

  num = mailFolder.numMsgs();
  for (i=1; i<=num; i++)
  {
    msg = mailFolder.getMsg(i);
    if (msg) mFolder->addMsg(msg);
  }

  mailFolder.close();
  return (num > 0);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(void)
{
  QString defaultPath("/var/spool/mail/");

  defaultPath += getenv("USER");

  mLocation = mConfig->readEntry("location", &defaultPath);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::writeConfig(void)
{
  mConfig->writeEntry("location", mLocation);
  mConfig->sync();
}


//-----------------------------------------------------------------------------
void KMAcctLocal::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
