// kmacctlocal.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctlocal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmglobal.h"

#include <kapp.h>
#include <kconfig.h>
#include <kmsgbox.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <klocale.h>

#ifdef HAVE_PATHS_H
#include <paths.h>	/* defines _PATH_MAILDIR */
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif


//-----------------------------------------------------------------------------
KMAcctLocal::KMAcctLocal(KMAcctMgr* aOwner, const char* aAccountName):
  KMAcctLocalInherited(aOwner, aAccountName)
{
  initMetaObject();
}


//-----------------------------------------------------------------------------
KMAcctLocal::~KMAcctLocal()
{
  mLocation = "";
}


//-----------------------------------------------------------------------------
const char* KMAcctLocal::type(void) const
{
  return "local";
}


//-----------------------------------------------------------------------------
void KMAcctLocal::init(void)
{
  mLocation = _PATH_MAILDIR;
  
  mLocation += "/";
  mLocation += getenv("USER");
}


//-----------------------------------------------------------------------------
bool KMAcctLocal::processNewMail(KMIOStatus *statusWdg)
{
  KMFolder mailFolder(NULL, location());
  long num = 0;
  long i;
  int rc;
  KMMessage* msg;

  if (mFolder==NULL) return FALSE;

  printf("processNewMail: %s\n", (const char*)location());

  statusWdg->prepareTransmission(location(), KMIOStatus::RETRIEVE);
  app->processEvents();
  mailFolder.setAutoCreateIndex(FALSE);
  rc = mailFolder.open();
  if (rc)
  {
    QString aStr;
    aStr = i18n("Cannot open file:");
    aStr += mailFolder.path()+"/"+mailFolder.name();
    KMsgBox::message(0,"KMail notification",aStr);
    perror("cannot open file "+mailFolder.path()+"/"+mailFolder.name());
    return FALSE;
  }

  mFolder->quiet(TRUE);
  mFolder->open();
		       

  num = mailFolder.count();

  for (i=0; i<num; i++)
  {
    //if(statusWdg->abortRequested())
    //break;
    statusWdg->updateProgressBar(i,num);
    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status"), msg->headerField("X-Status"));
      processNewMsg(msg);
    }
    app->processEvents();
  }

  rc = mailFolder.expunge();
  if (rc)
    warning(i18n("Cannot remove mail from\nmailbox `%s':\n%s"),
	    (const char*)mailFolder.location(), strerror(rc));

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);
  statusWdg->transmissionCompleted();

  return (num > 0);
}


//-----------------------------------------------------------------------------
void KMAcctLocal::readConfig(KConfig& config)
{
  QString defaultPath(_PATH_MAILDIR);
  defaultPath += "/";
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
