// kmacctlocal.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qdatetime.h" 
#include "kmacctlocal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include "kmbroadcaststatus.h"

#include <kapp.h>
#include <kconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <klocale.h>
#include <kmessagebox.h>

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
void KMAcctLocal::processNewMail(bool)
{
  QTime t; 
  KMFolder mailFolder(NULL, location());
  long num = 0;
  long i;
  int rc;
  KMMessage* msg;
  bool addedOk;

  hasNewMail = false;
  if (mFolder==NULL) {
    emit finishedCheck(hasNewMail);
    return;
  }

  KMBroadcastStatus::instance()->reset();
  KMBroadcastStatus::instance()->setStatusMsg( 
                     i18n( "Preparing transmission..." ));

  app->processEvents();
  mailFolder.setAutoCreateIndex(FALSE);

  rc = mailFolder.open();
  if (rc)
  {
    QString aStr;
    aStr = i18n("Cannot open file:");
    aStr += mailFolder.path()+"/"+mailFolder.name();
    KMessageBox::sorry(0, aStr);
    perror("cannot open file "+mailFolder.path()+"/"+mailFolder.name());
    emit finishedCheck(hasNewMail); 
    return;
  }

  mFolder->quiet(TRUE);
  mFolder->open();
		       

  num = mailFolder.count();

  addedOk = true;
  t.start(); 

  KMBroadcastStatus::instance()->setStatusProgressEnable( true );
  for (i=0; i<num; i++)
  {

    if (!addedOk) break;
    if (KMBroadcastStatus::instance()->abortRequested()) break;

    KMBroadcastStatus::instance()->setStatusMsg( i18n("Message ") +
			                QString("%1/%2").arg(i).arg(num) );
    KMBroadcastStatus::instance()->setStatusProgressPercent( (i*100) / num );

    msg = mailFolder.take(0);
    if (msg)
    {
      msg->setStatus(msg->headerField("Status"), msg->headerField("X-Status"));
      addedOk = processNewMsg(msg);
      if (addedOk)
	hasNewMail = true;
    }

    if (t.elapsed() >= 200) { //hardwired constant
      app->processEvents();
      t.start();
    }
    
  }
  KMBroadcastStatus::instance()->setStatusProgressEnable( false );
  KMBroadcastStatus::instance()->reset();

  if (addedOk)
  {
  rc = mailFolder.expunge();
  if (rc)
    KMessageBox::information( 0, i18n("Cannot remove mail from\nmailbox ") +
      QString("`%1':\n%2").arg(mailFolder.location().arg(strerror(rc))));
  KMBroadcastStatus::instance()->setStatusMsg( 
		     i18n( "Transmission completed..." ));
  }
  // else warning is written already

  mailFolder.close();
  mFolder->close();
  mFolder->quiet(FALSE);

  emit finishedCheck(hasNewMail);
 
  return;
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
