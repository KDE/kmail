// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qstring.h>
#include <qdir.h>
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmacctmgr.h"
#include "kmfoldermgr.h"
#include "kmfilteraction.h"
#include "kmfolder.h"
#include "kmsender.h"
#include "kbusyptr.h"
#include "kmfiltermgr.h"
#include "kmversion.h"
#include "kmmessage.h"
#include "kmcomposewin.h"
#include "kmaddrbook.h"
#include "kcharsets.h"

#include <kapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <kmsgbox.h>
#include <kapp.h>
#include <kstdaccel.h>
#include <kmidentity.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kdebug.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

KBusyPtr* kbp = NULL;
KApplication* app = NULL;
KMAcctMgr* acctMgr = NULL;
KMFolderMgr* folderMgr = NULL;
KMFilterMgr* filterMgr = NULL;
KMSender* msgSender = NULL;
KMFolder* inboxFolder = NULL;
KMFolder* outboxFolder = NULL;
KMFolder* queuedFolder = NULL;
KMFolder* sentFolder = NULL;
KMFolder* trashFolder = NULL;
KStdAccel* keys = NULL;
KMIdentity* identity = NULL;
KMFilterActionDict* filterActionDict = NULL;
KMAddrBook* addrBook = NULL;
WindowList* windowList = NULL;


bool shuttingDown = FALSE;
const char* aboutText = 
    "KMail [" KMAIL_VERSION "] by\n\n"
    "Stefan Taferner <taferner@kde.org>,\n"
    "Markus Wübben <markus.wuebben@kde.org>\n\n" 
    "based on the work of:\n"
    "Lynx <lynx@topaz.hknet.com>,\n"
    "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
    "and the above authors.\n\n"
    "This program is covered by the GPL.";

static msg_handler oldMsgHandler = NULL;

static void kmailMsgHandler(QtMsgType aType, const char* aMsg);
static void signalHandler(int sigId);
static void testDir(const char *_name);
static void transferMail(void);
static void initFolders(KConfig* cfg);
static void init(int argc, char *argv[]);
static void cleanup(void);
static void setSignalHandler(void (*handler)(int));
static void recoverDeadLetters(void);
static void mailto(int argc, char *argv[]);


//-----------------------------------------------------------------------------
// Message handler
static void kmailMsgHandler(QtMsgType aType, const char* aMsg)
{
  QString appName = app->appName();
  QString msg = aMsg;
  msg.detach();

  switch (aType)
  {
  case QtDebugMsg:
    kdebug(KDEBUG_INFO, 0, msg);
    break;

  case QtWarningMsg:
    fprintf(stderr, "%s: %s\n", (const char*)app->appName(), msg.data());
    if (strncmp(aMsg,"KCharset:",9) != 0 &&
	strncmp(aMsg,"QGManager:",10) != 0)
    {
      KMsgBox::message(NULL, appName+" "+i18n("warning"), msg.data(), 
		       KMsgBox::EXCLAMATION);
    }
    break;

  case QtFatalMsg:
    fprintf(stderr, appName+" "+i18n("fatal error")+": %s\n", msg.data());
    KMsgBox::message(NULL, appName+" "+i18n("fatal error"),
		     aMsg, KMsgBox::STOP);
    abort();
  }
}


//-----------------------------------------------------------------------------
// Crash recovery signal handler
static void signalHandler(int sigId)
{
  QWidget* win;

  fprintf(stderr, "*** KMail got signal %d: %s\n", sigId,
	  strsignal(sigId));

  // try to cleanup all windows
  while (windowList->first() != NULL)
  {
    win = windowList->take();
    if (win->inherits("KMComposeWin")) ((KMComposeWin*)win)->deadLetter();
    delete win;
  }

  // If KMail crashes again below this line we consider the data lost :-|
  // Otherwise KMail will end in an infinite loop.
  setSignalHandler(SIG_DFL);
  cleanup();
  exit(1);
}


//-----------------------------------------------------------------------------
static void setSignalHandler(void (*handler)(int))
{
  signal(SIGSEGV, handler);
  signal(SIGKILL, handler);
  signal(SIGTERM, handler);
  signal(SIGHUP,  handler);
  signal(SIGFPE,  handler);
  signal(SIGABRT, handler);
}


//-----------------------------------------------------------------------------
static void testDir(const char *_name)
{
  DIR *dp;
  QString c = getenv("HOME");
  c += _name;
  dp = opendir(c.data());
  if (dp == NULL) ::mkdir(c.data(), S_IRWXU);
  else closedir(dp);
}


//-----------------------------------------------------------------------------
// Open a composer for each message found in ~/dead.letter
static void recoverDeadLetters(void)
{
  KMComposeWin* win;
  KMMessage* msg;
  QDir dir = QDir::home();
  QString fname = dir.path();
  int i, rc, num;

  if (!dir.exists("dead.letter")) return;
  fname += "/dead.letter";
  KMFolder folder(NULL, fname);

  folder.setAutoCreateIndex(FALSE);
  rc = folder.open();
  if (rc)
  {
    perror("cannot open file "+fname);
    return;
  }

  folder.quiet(TRUE);
  folder.open();

  num = folder.count();
  debug("%d messages in %s", num, (const char*)fname);

  for (i=0; i<num; i++)
  {
    debug("processing dead letter #%d", i);
    msg = folder.take(0);
    if (msg)
    {
      win = new KMComposeWin;
      win->setMsg(msg, FALSE);
      win->show();
    }
  }
  folder.close();
  unlink(fname);
}


//-----------------------------------------------------------------------------
static void transferMail(void)
{
  QDir dir = QDir::home();
  int rc;

  // This function is for all the whiners who think that KMail is
  // broken because they cannot read mail with pine and do not
  // know how to fix this problem with a simple symbolic link  =;-)
  if (!dir.cd("KMail")) return;

  rc = KMsgBox::yesNo(NULL, app->appName()+" "+i18n("warning"),
		      i18n(
	    "The directory ~/KMail exists. From now on, KMail uses the\n"
	    "directory ~/Mail for it's messages.\n"
	    "KMail can move the contents of the directory ~/KMail into\n"
	    "~/Mail, but this will replace existing files with the same\n"
	    "name in the directory ~/Mail (e.g. inbox).\n\n"
	    "Shall KMail move the mail folders now ?"),
		      KMsgBox::QUESTION);
  if (rc != 1) return;

  dir.cd("/");  // otherwise we lock the directory
  testDir("/Mail");
  system("mv -f ~/KMail/* ~/Mail");
  system("mv -f ~/KMail/.??* ~/Mail");
  system("rmdir ~/KMail");
}


//-----------------------------------------------------------------------------
static void initFolders(KConfig* cfg)
{
  QString name;

  name = cfg->readEntry("inboxFolder");

  // Currently the folder manager cannot manage folders which are not
  // in the base folder directory.
  //if (name.isEmpty()) name = getenv("MAIL");

  if (name.isEmpty()) name = "inbox";
  
  inboxFolder  = (KMFolder*)folderMgr->findOrCreate(name);
  // inboxFolder->open();

  outboxFolder = folderMgr->findOrCreate(cfg->readEntry("outboxFolder", "outbox"));
  outboxFolder->setType("Out");
  outboxFolder->open();

  sentFolder = folderMgr->findOrCreate(cfg->readEntry("sentFolder", "sent-mail"));
  sentFolder->setType("St");
  sentFolder->open();

  trashFolder  = folderMgr->findOrCreate(cfg->readEntry("trashFolder", "trash"));
  trashFolder->setType("Tr");
  trashFolder->open();
}


//-----------------------------------------------------------------------------
static void init(int argc, char *argv[])
{
  QString  acctPath, foldersPath;
  KConfig* cfg;

  app = new KApplication(argc, argv, "kmail");
  windowList = new WindowList;

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
  foldersPath = cfg->readEntry("folders", "");
  acctPath = cfg->readEntry("accounts", foldersPath + "/.kmail-accounts");

  if (foldersPath.isEmpty())
  {
    foldersPath = QDir::homeDirPath() + QString("/Mail");
    transferMail();
  }

  folderMgr = new KMFolderMgr(foldersPath);
  acctMgr   = new KMAcctMgr(acctPath);
  filterMgr = new KMFilterMgr;
  filterActionDict = new KMFilterActionDict;
  addrBook  = new KMAddrBook;

  initFolders(cfg);

  acctMgr->readConfig();
  filterMgr->readConfig();
  addrBook->readConfig();
  addrBook->load();
  KMMessage::readConfig();

  msgSender = new KMSender;

  setSignalHandler(signalHandler);
}


//-----------------------------------------------------------------------------
static void cleanup(void)
{
  KConfig* config =  kapp->getConfig();

  shuttingDown = TRUE;

  if (trashFolder)
  {
    trashFolder->close(TRUE);
    config->setGroup("General");
    if (config->readNumEntry("empty-trash-on-exit", 0))
      trashFolder->expunge();
  }
  if (inboxFolder) inboxFolder->close(TRUE);
  if (outboxFolder) outboxFolder->close(TRUE);
  if (sentFolder) sentFolder->close(TRUE);

  if (msgSender) delete msgSender;
  if (addrBook) delete addrBook;
  if (filterMgr) delete filterMgr;
  if (acctMgr) delete acctMgr;
  if (folderMgr) delete folderMgr;
  if (kbp) delete kbp;

  qInstallMsgHandler(oldMsgHandler);
  app->getConfig()->sync();
}


//-----------------------------------------------------------------------------
static void mailto(int argc, char *argv[])
{
  KMComposeWin* win;
  KMMessage* msg = new KMMessage;
  QString to, cc, bcc, subj;
  int i;

  for (i=0; i<argc; i++)
  {
    if (strcmp(argv[i],"-s")==0)
    {
      if (i<argc-1) subj = argv[++i];
    }
    else if (strcmp(argv[i],"-c")==0)
    {
      if (i<argc-1) cc = argv[++i];
    }
    else if (strcmp(argv[i],"-b")==0)
    {
      if (i<argc-1) bcc = argv[++i];
    }
    else
    {
      if (!to.isEmpty()) to += ", ";
      if (strncasecmp(argv[i],"mailto:",7)==0) to += argv[i]+7;
      else to += argv[i];
    }
  }

  msg->initHeader();
  if (!cc.isEmpty()) msg->setCc(cc);
  if (!bcc.isEmpty()) msg->setBcc(bcc);
  if (!subj.isEmpty()) msg->setSubject(subj);
  if (!to.isEmpty()) msg->setTo(to);

  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
main(int argc, char *argv[])
{
  KMMainWin* mainWin;

  init(argc, argv);
  filterMgr->dump();

  mainWin = new KMMainWin;
  mainWin->show();

  if (argc > 1)
    mailto(argc-1, argv+1);

  recoverDeadLetters();
  app->exec();

  mainWin->writeConfig(FALSE);
  delete mainWin;

  cleanup();
}

