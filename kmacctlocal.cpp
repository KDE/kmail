// kmacctlocal.cpp

#include "kmacctlocal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include "kmfiltermgr.h"

#include <klocale.h>
#include <kconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctLocalInherited(aOwner, aAccountName)
{
  initMetaObject();
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
  mLocation = NULL;
}


//-----------------------------------------------------------------------------
const char* KMAcctLocal::type(void) const
{
  return "local";
}


//-----------------------------------------------------------------------------
void KMAcctLocal::init(void)
{
  mLocation = "/var/spool/mail/";
  mLocation.detach();
  mLocation += getenv("USER");
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::processNewMail(void)
{
  KMFolder mailFolder(NULL, location());
  long num = 0;
  long i;
  int rc;
  KMMessage* msg;

  if (mFolder==NULL) return FALSE;

  printf("processNewMail: %s\n", (const char*)location());

  mailFolder.setAutoCreateIndex(FALSE);
  rc = mailFolder.open();
  if (rc)
  {
    perror("cannot open file "+mailFolder.path()+"/"+mailFolder.name());
    return FALSE;
  }

  mFolder->quiet(TRUE);
  mFolder->open();

  num = mailFolder.numMsgs();
  debug("%ld messages in %s", num, (const char*)location());

  for (i=num; i>=1; i--)
  {
    debug("processing message %ld", i);
    msg = mailFolder.getMsg(i);
    //msg->viewSource("KMAcctLocal::processNewMail: from getMsg()");
    mailFolder.detachMsg(i);
    if (msg) 
    {
      if (filterMgr->process(msg))
      {
	rc = mFolder->addMsg(msg);
	if (rc) perror("failed to add message");
	if (rc) warning("Failed to add message:\n" + QString(strerror(rc)));
      }
    }
  }
  debug("done, closing folders");

  rc = mailFolder.expunge();
  if (rc)
    warning(nls->translate("Cannot remove mail from\nmailbox `%s':\n%s"),
	    (const char*)mailFolder.location(), strerror(rc));

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);

  return (num > 0);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(KConfig& config)
{
  QString defaultPath("/var/spool/mail/");
  defaultPath += getenv("USER");

  KMAcctLocalInherited::readConfig(config);
  mLocation = config.readEntry("Location", defaultPath);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::writeConfig(KConfig& config)
{
  KMAcctLocalInherited::writeConfig(config);

  config.writeEntry("Location", mLocation);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::setLocation(const QString& aLocation)
{
  mLocation = aLocation;
}
