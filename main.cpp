// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qstring.h>
#include <qdir.h>
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmacctmgr.h"
#include "kmfoldermgr.h"
#include "kbusyptr.h"
#include <kapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <qmsgbox.h>

KBusyPtr* kbp = NULL;
KApplication* app = NULL;
KMAcctMgr* acctMgr = NULL;
KMFolderMgr* folderMgr = NULL;

static msg_handler oldMsgHandler = NULL;


//-----------------------------------------------------------------------------
// Message handler
static void kmailMsgHandler(QtMsgType aType, const char* aMsg)
{
  switch (aType)
  {
  case QtDebugMsg:
    fprintf(stderr, "kmail: %s\n", aMsg);
    break;

  case QtWarningMsg:
    QMessageBox::message("KMail Warning", aMsg, " Ok ");
    break;

  case QtFatalMsg:
    fprintf(stderr, "kmail fatal error: %s\n", aMsg);
    if (QMessageBox::query("KMail Fatal Error", aMsg, "Ignore", "Abort"))
      abort(); // coredump
    break;
  }
}


//-----------------------------------------------------------------------------
static void init(int argc, char *argv[])
{
  QString  fname;
  KConfig* cfg;

  app = new KApplication(argc, argv, "kmail");
  kbp = new KBusyPtr(app);
  cfg = app->getConfig();

  oldMsgHandler = qInstallMsgHandler(kmailMsgHandler);

  cfg->setGroup("General");

  fname = QDir::homeDirPath() + 
    cfg->readEntry("accounts", &QString("/.kde/mail-accounts"));
  acctMgr = new KMAcctMgr(fname);

  fname = QDir::homeDirPath() + 
    cfg->readEntry("folders", &QString("/.kde/mail-folders"));
  folderMgr = new KMFolderMgr(fname);

  debug("init done");
}


//-----------------------------------------------------------------------------
static void cleanup(void)
{
  qInstallMsgHandler(oldMsgHandler);

  if (folderMgr) delete folderMgr;
  if (acctMgr) delete acctMgr;
  if (kbp) delete kbp;
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

