// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qstring.h>
#include <qdir.h>
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmacctmgr.h"
#include "kmfoldermgr.h"
#include "kmacctfolder.h"
#include "kmsender.h"
#include "kbusyptr.h"
#include "kmcomposewin.h"
#include <kapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <kmsgbox.h>
#include <klocale.h>
#include <kshortcut.h>

KBusyPtr* kbp = NULL;
KApplication* app = NULL;
KMAcctMgr* acctMgr = NULL;
KMFolderMgr* folderMgr = NULL;
KMSender* msgSender = NULL;
KLocale* nls = NULL;
KMFolder* trashFolder = NULL;
KShortCut* keys = NULL;

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
    fprintf(stderr, appName+" "+nls->translate("fatal")+": %s\n", aMsg);
    KMsgBox::message(NULL, appName+nls->translate("fatal"),
		     aMsg, KMsgBox::STOP);
    abort();
  }
}


#include <dirent.h>
#include <sys/stat.h>

// Torben
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
  QString  fname, trashName;
  KConfig* cfg;
  KMAcctFolder* fld;

  app = new KApplication(argc, argv, "kmail");
  nls = app->getLocale();

  kbp = new KBusyPtr;
  cfg = app->getConfig();

  keys = new KShortCut(cfg);

  oldMsgHandler = qInstallMsgHandler(kmailMsgHandler);

  cfg->setGroup("General");

  // Torben
  testDir( "/.kde" );
  testDir( "/.kde/share" );  
  testDir( "/.kde/share/config" );  
  testDir( "/.kde/share/apps" );
  testDir( "/.kde/share/apps/kmail" );

  fname = QDir::homeDirPath() + 
    cfg->readEntry("accounts", QString("/.kde/share/apps/kmail/mail-accounts"));
  acctMgr = new KMAcctMgr(fname);

  fname = QDir::homeDirPath() + 
    cfg->readEntry("folders", QString("/.kde/share/apps/kmail/mail-folders"));
  folderMgr = new KMFolderMgr(fname);

  trashName   = cfg->readEntry("trashFolder", QString("trash"));
  fld = folderMgr->find(trashName);

  if (!fld)
  {
    warning(nls->translate("The folder `%s'\ndoes not exist in the\n"
			   "mail folder directory\nbut will be created now."),
	    (const char*)trashName);

    fld = folderMgr->createFolder(trashName, TRUE);
    if (!fld) fatal(nls->translate("Cannot create trash folder '%s'"),
		    (const char*)trashName);
  }
  fld->setLabel(nls->translate("trash"));
  trashFolder = (KMFolder*)fld;
  trashFolder->setAutoCreateToc(FALSE);
  trashFolder->open(); // otherwise we have to open/close the folder for
                       // each and every delete.

  msgSender = new KMSender(folderMgr);
}


//-----------------------------------------------------------------------------
static void cleanup(void)
{
  trashFolder->close();

  if (msgSender) delete msgSender;
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

