// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qstring.h>
#include <qdir.h>
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmacctmgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmsender.h"
#include "kbusyptr.h"
#include "kmcomposewin.h"
#include "kmfiltermgr.h"
#include <kapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <kmsgbox.h>
#include <klocale.h>
#include <kstdaccel.h>
#include <kmidentity.h>
#include <dirent.h>
#include <sys/stat.h>

KBusyPtr* kbp = NULL;
KApplication* app = NULL;
KMAcctMgr* acctMgr = NULL;
KMFolderMgr* folderMgr = NULL;
KMFilterMgr* filterMgr = NULL;
KMSender* msgSender = NULL;
KLocale* nls = NULL;
KMFolder* inboxFolder = NULL;
KMFolder* outboxFolder = NULL;
KMFolder* queuedFolder = NULL;
KMFolder* sentFolder = NULL;
KMFolder* trashFolder = NULL;
KStdAccel* keys = NULL;
KMIdentity* identity = NULL;
bool shuttingDown = FALSE;

static msg_handler oldMsgHandler = NULL;


//-----------------------------------------------------------------------------
// Message handler
static void kmailMsgHandler(QtMsgType aType, const char* aMsg)
{
  QString appName = app->appName();

  switch (aType)
  {
  case QtDebugMsg:
    fprintf(stderr, "%s: %s\n", (const char*)app->appName(), aMsg);
    break;

  case QtWarningMsg:
    KMsgBox::message(NULL, appName+" "+nls->translate("warning"), aMsg, 
		     KMsgBox::EXCLAMATION);
    break;

  case QtFatalMsg:
    fprintf(stderr, appName+" "+nls->translate("fatal error")+": %s\n", aMsg);
    KMsgBox::message(NULL, appName+" "+nls->translate("fatal error"),
		     aMsg, KMsgBox::STOP);
    abort();
  }
}


//-----------------------------------------------------------------------------
void testDir( const char *_name )
{
    DIR *dp;
    QString c = getenv( "HOME" );
    c += _name;
    dp = opendir( c.data() );
    if ( dp == NULL )
	::mkdir( c.data(), S_IRWXU );
    else
	closedir( dp );
}


//-----------------------------------------------------------------------------
static void init(int argc, char *argv[])
{
  QString  acctPath, foldersPath;
  KConfig* cfg;

  app = new KApplication(argc, argv, "kmail");
  nls = app->getLocale();

  kbp = new KBusyPtr;
  cfg = app->getConfig();

  keys = new KStdAccel(cfg);

  oldMsgHandler = qInstallMsgHandler(kmailMsgHandler);

  testDir("/.kde");
  testDir("/.kde/share");
  testDir("/.kde/share/config");  
  testDir("/.kde/share/apps");
  testDir("/.kde/share/apps/kmail");

  identity = new KMIdentity;

  cfg->setGroup("General");
  foldersPath = cfg->readEntry("folders", 
			       QDir::homeDirPath() + QString("/KMail"));
  acctPath = cfg->readEntry("accounts", foldersPath + "/.kmail-accounts");

  folderMgr = new KMFolderMgr(foldersPath);
  acctMgr   = new KMAcctMgr(acctPath);
  filterMgr = new KMFilterMgr;

  inboxFolder  = (KMFolder*)folderMgr->findOrCreate(
				         cfg->readEntry("inboxFolder", "inbox"));
  inboxFolder->open();

  outboxFolder = folderMgr->findOrCreate(cfg->readEntry("outboxFolder", "outbox"));
  outboxFolder->setType("out");
  outboxFolder->open();

  queuedFolder = folderMgr->findOrCreate(cfg->readEntry("queuedFolder", "queued"));
  queuedFolder->open();

  sentFolder   = folderMgr->findOrCreate(cfg->readEntry("sentFolder", "sent_mail"));
  sentFolder->setType("st");
  sentFolder->open();

  trashFolder  = folderMgr->findOrCreate(cfg->readEntry("trashFolder", "trash"));
  trashFolder->setType("tr");
  trashFolder->open();

  acctMgr->readConfig();
  filterMgr->readConfig();

  msgSender = new KMSender;
}


//-----------------------------------------------------------------------------
static void cleanup(void)
{
  shuttingDown = TRUE;

  if (inboxFolder) inboxFolder->close(TRUE);

  if (msgSender) delete msgSender;
  if (filterMgr) delete filterMgr;
  if (acctMgr) delete acctMgr;
  if (folderMgr) delete folderMgr;
  if (kbp) delete kbp;

  qInstallMsgHandler(oldMsgHandler);
  app->getConfig()->sync();
}


//-----------------------------------------------------------------------------
main(int argc, char *argv[])
{
  KMMainWin* mainWin;

  init(argc, argv);

  mainWin = new KMMainWin;
  mainWin->show();
    
  app->exec();

  cleanup();
}

