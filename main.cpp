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
#include "kmsettings.h"
#include "kmreaderwin.h"

#include <kapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <kapp.h>
#include <kstdaccel.h>
#include <kmidentity.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kdebug.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
//--- Sven's pseudo IPC&locking start ---
#include "kfileio.h"
#include "kwm.h"
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
//--- Sven's pseudo IPC&locking end ---
 // Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_TMP
#define _PATH_TMP "/tmp/"
#endif

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


bool mailto = FALSE;
bool checkNewMail = FALSE;
bool firstStart = TRUE;
bool shuttingDown = FALSE;
bool checkingMail = FALSE;
const char* aboutText =
    "KMail [" KMAIL_VERSION "] by\n\n"
    "Stefan Taferner <taferner@kde.org>,\n"
    "Markus Wübben <markus.wuebben@kde.org>\n\n"
    "based on the work of:\n"
    "Lynx <lynx@topaz.hknet.com>,\n"
    "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
    "and the above authors.\n\n"
    "This program is covered by the GPL.\n\n"
    "Please send bugreports to taferner@kde.org";

static msg_handler oldMsgHandler = NULL;

static void kmailMsgHandler(QtMsgType aType, const char* aMsg);
static void signalHandler(int sigId);
static void testDir(const char *_name);
static void transferMail(void);
static void initFolders(KConfig* cfg);
static void init(int& argc, char *argv[]);
static void cleanup(void);
static void setSignalHandler(void (*handler)(int));
static void recoverDeadLetters(void);
static void processArgs(int argc, char *argv[]);
static void checkMessage(void);
static void writePid(bool ready);

//-----------------------------------------------------------------------------
static void ungrabPtrKb(void)
{
  if(!KTMainWindow::memberList) return;
  QWidget* widg = KTMainWindow::memberList->first();
  Display* dpy;

  if (!widg) return;
  dpy = widg->x11Display();
  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
}


//-----------------------------------------------------------------------------
// Message handler
static void kmailMsgHandler(QtMsgType aType, const char* aMsg)
{
  QString appName = app->getCaption();
  QString msg = aMsg;
  static int recurse=-1;

  recurse++;

  switch (aType)
  {
  case QtDebugMsg:
    kdebug(KDEBUG_INFO, 0, msg);
    break;

  case QtWarningMsg:
    fprintf(stderr, "%s: %s\n", (const char*)app->name(), msg.data());
    if (strncmp(aMsg,"KCharset:",9) != 0 &&
	strncmp(aMsg,"QGManager:",10) != 0 &&
	strncmp(aMsg,"QPainter:",9) != 0 &&
	strncmp(aMsg,"Could not load", 14) != 0 &&
	strncmp(aMsg,"QPixmap:",8) != 0 &&
	!recurse)
    {
      ungrabPtrKb();
      QMessageBox::warning(NULL, kapp->getCaption(), msg, i18n("OK"));
    }
    else kdebug(KDEBUG_INFO, 0, msg);
    break;

  case QtFatalMsg:
    ungrabPtrKb();
    fprintf(stderr, appName+" "+i18n("fatal error")+": %s\n", msg.data());
    QMessageBox::critical(NULL, appName+" "+i18n("fatal error"),
		       aMsg, i18n("OK"));
    abort();
  }

  recurse--;
}
//--- Sven's pseudo IPC&locking start ---
void serverReady(bool flag)
{
  writePid(flag);
  if (flag) // are we ready now?
    checkMessage(); //check for any pending mesages
}

static void writePid (bool ready)
{
  FILE* lck;
  char nlck[80];
  sprintf (nlck, "%s.kmail%d.lck", _PATH_TMP, getuid());
  lck = fopen (nlck, "w");
  if (!ready)
    fprintf (lck, "%d", 0-getpid());
  else
    fprintf (lck, "%d", getpid());
  fclose (lck);
}

static void checkMessage()
{
  char lf[80];
  sprintf (lf, "%s.kmail%d.msg", _PATH_TMP, getuid());
  if (access(lf, F_OK) != 0)
  {
    debug ("No message for me");
    printf ("No message for me\n");
    return;
  }
  QString cmd;
  QString delcmd;
  cmd = kFileToString(lf);
  delcmd.sprintf("%s.kmail%d.msg", _PATH_TMP, getuid());
  unlink (delcmd.data()); // unlink
  //system (delcmd.data()); // delete message if any
  // find a KMMainWin
  KMMainWin *kmmWin = 0;
  if (kapp->topWidget() && kapp->topWidget()->isA("KMMainWin"))
    kmmWin = (KMMainWin *) kapp->topWidget();

  if (cmd.find ("show") == 0)
  {
    //printf ("show();\n");
    if (!kmmWin)
    {
      kmmWin = new KMMainWin;
      kmmWin->show(); // thanks, Patrick!
    }
    else
      KWM::activate(kmmWin->winId());
    //kmmWin->show();
    //kmmWin->raise();
  }
  else if (cmd.find ("check") == 0)
  {
    //printf ("check();\n");
    if (!kmmWin)
      kmmWin = new KMMainWin;
    KWM::activate(kmmWin->winId());
    //kmmWin->show();
    //kmmWin->raise();
    kmmWin->slotCheckMail();
  }
  else
  {
    int j;
    int i = cmd.find ("To: ", 0, true);
    if (i==-1)
      return; // not a mailto command

    QString to = "";
    QString cc = "";
    QString bcc = "";
    QString subj = "";

    j = cmd.find('\n', i);
    to = cmd.mid(i+4,j-i-4);

    i = cmd.find ("Cc: ", j, true);
    if (i!=-1)
    {
      j = cmd.find('\n', i);
      cc= cmd.mid(i+4,j-i-4);
    }

    i = cmd.find ("Bcc: ", j, true);
    if (i!=-1)
    {
      j = cmd.find('\n', i);
      bcc= cmd.mid(i+5,j-i-5);
    }
    i = cmd.find ("Subject: ", j, true);
    if (i!=-1)
    {
      j = cmd.find('\n', i);
      subj= cmd.mid(i+9,j-i-9);
    }
    //printf ("To: %s\nCc: %s\nBcc: %s\nSubject: %s\n",
    //        to.data(), cc.data(), bcc.data(), subj.data());

    KMComposeWin* win;
    KMMessage* msg = new KMMessage;

    msg->initHeader();
    if (!cc.isEmpty()) msg->setCc(cc);
    if (!bcc.isEmpty()) msg->setBcc(bcc);
    if (!subj.isEmpty()) msg->setSubject(subj);
    if (!to.isEmpty()) msg->setTo(to);

    win = new KMComposeWin(msg);
    assert(win != NULL);
    win->show();
  }
  
}
//--- Sven's pseudo IPC&locking end ---

//-----------------------------------------------------------------------------
// Crash recovery signal handler
static void signalHandler(int sigId)
{
  QWidget* win;
  //--- Sven's pseudo IPC&locking start ---
  if (sigId == SIGUSR1)
  {
    fprintf(stderr, "*** KMail got message\n");
    checkMessage();
    setSignalHandler(signalHandler);
    return;
  }
  //--- Sven's pseudo IPC&locking end ---
  fprintf(stderr, "*** KMail got signal %d\n", sigId);

  // try to cleanup all windows
  while (KTMainWindow::memberList->first() != NULL)
  {
    win = KTMainWindow::memberList->take();
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
  //--- Sven's pseudo IPC&locking start ---
  signal(SIGUSR1,  handler);
  //--- Sven's pseudo IPC&locking end ---
}


//-----------------------------------------------------------------------------
static void testDir(const char *_name)
{
  DIR *dp;
  QString c = getenv("HOME");
  if(c.isEmpty())
    {
     QMessageBox::critical(0,i18n("KMail notification"),
			   i18n("$HOME is not set!\n"
				"KMail cannot start without it.\n"),
			   i18n("OK"));
      exit(-1);
    }
		
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
  for (i=0; i<num; i++)
  {
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

  // Stefan: This function is for all the whiners who think that KMail is
  // broken because they cannot read mail with pine and do not
  // know how to fix this problem with a simple symbolic link  =;-)
  // Markus: lol ;-)
  if (!dir.cd("KMail")) return;

  rc = QMessageBox::information(NULL, QString(app->name())+" "+i18n("warning"),
		      i18n(
	    "The directory ~/KMail exists. From now on, KMail uses the\n"
	    "directory ~/Mail for it's messages.\n"
	    "KMail can move the contents of the directory ~/KMail into\n"
	    "~/Mail, but this will replace existing files with the same\n"
	    "name in the directory ~/Mail (e.g. inbox).\n\n"
	    "Shall KMail move the mail folders now ?"),
		      i18n("Yes"), i18n("No"));
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
  outboxFolder->setWhoField("To");
  outboxFolder->setSystemFolder(TRUE);
  outboxFolder->open();

  sentFolder = folderMgr->findOrCreate(cfg->readEntry("sentFolder", "sent-mail"));
  sentFolder->setType("St");
  sentFolder->setWhoField("To");
  sentFolder->setSystemFolder(TRUE);
  sentFolder->open();

  trashFolder  = folderMgr->findOrCreate(cfg->readEntry("trashFolder", "trash"));
  trashFolder->setType("Tr");
  trashFolder->setSystemFolder(TRUE);
  trashFolder->open();
}


//-----------------------------------------------------------------------------
static void init(int& argc, char *argv[])
{
  QString  acctPath, foldersPath;
  KConfig* cfg;
  //--- Sven's pseudo IPC&locking start ---
  if (!app) // because we might have constructed it before to remove KDE args
  //--- Sven's pseudo IPC&locking end ---
  app = new KApplication(argc, argv, "kmail");
  kbp = new KBusyPtr;
  cfg = app->getConfig();

  keys = new KStdAccel(cfg);

  //oldMsgHandler = qInstallMsgHandler(kmailMsgHandler);

  QDir dir;
  QString d = locateLocal("data", "kmail/");

  identity = new KMIdentity;

  cfg->setGroup("General");
  firstStart = cfg->readBoolEntry("first-start", true);
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
  if(addrBook->load() == IO_FatalError)
    {
      QMessageBox::warning(0,i18n("KMail error"),
			   i18n("Loading addressbook failed"), 
			   i18n("OK"));
    }
  KMMessage::readConfig();

  msgSender = new KMSender;
  assert(msgSender != NULL);

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

  //qInstallMsgHandler(oldMsgHandler);
  app->getConfig()->sync();
  //--- Sven's save attachments to /tmp start ---
  //debug ("cleaned");
  QString cmd;
  // This is a dir with attachments and it is not critical if they are
  // left behind.
  if (!KMReaderWin::attachDir().isEmpty())
  {
    cmd.sprintf("rm -rf '%s'", (const char*)KMReaderWin::attachDir());
    system (cmd.data()); // delete your owns only
  }
  //--- Sven's save attachments to /tmp end ---
  //--- Sven's pseudo IPC&locking start ---
  cmd.sprintf("%s.kmail%d.lck", _PATH_TMP, getuid());
  unlink(cmd.data()); // delete your owns only
  cmd.sprintf("%s.kmail%d.msg", _PATH_TMP, getuid());
  unlink(cmd.data()); // delete your owns only
  //--- Sven's pseudo IPC&locking end ---
}

//-----------------------------------------------------------------------------

// Sven: new from Jens Kristian Soegard:
static void processArgs(int argc, char *argv[])
{
  KMComposeWin* win;
  KMMessage* msg = new KMMessage;
  QString to, cc, bcc, subj;
  int i, x=0;
 
  msg->initHeader();
 
  for (i=0; i<argc; i++)
  {
      if (strcmp(argv[i],"-s")==0)
      {
          if (i<argc-1) subj = argv[++i];
          mailto = TRUE;
      }
      else if (strcmp(argv[i],"-c")==0)
      {
          if (i<argc-1) cc = argv[++i];
          mailto = TRUE;
      }
      else if (strcmp(argv[i],"-b")==0)
      {
          if (i<argc-1) bcc = argv[++i];
          mailto = TRUE;
      }
      else if (strcmp(argv[i],"-h")==0)
      {
          if (i<argc-1) {
              QString headerString = argv[++i];
              if( (x = headerString.find( '=' )) != -1 )
                  msg->setHeaderField( headerString.left( x ), headerString.right( headerString.length()-x-1 ) );
          } 
          mailto = TRUE;
      }
      else if(strcmp(argv[i],"-msg")==0)
      {
          if(i<argc-1)
              msg->setBodyEncoded( argv[++i] );
          mailto = TRUE;
      }
      else if (strcmp(argv[i],"-check")==0)
          checkNewMail = TRUE;
      else if (argv[i][0]=='-')
      {
          warning(i18n("Unknown command line option: %s"), argv[i]);
          // unknown parameter
      }
      else
      {
          if (!to.isEmpty()) to += ", ";
          if (strncasecmp(argv[i],"mailto:",7)==0) to += argv[i]+7;
          else to += argv[i];
          mailto = TRUE;
      }
  }
 
  if (mailto)
  {
      if (!cc.isEmpty()) msg->setCc(cc);
      if (!bcc.isEmpty()) msg->setBcc(bcc);
      if (!subj.isEmpty()) msg->setSubject(subj);
      if (!to.isEmpty()) msg->setTo(to);
 
      win = new KMComposeWin(msg);
      assert(win != NULL);
      win->show();
  }
}

// Old original
/*
static void processArgs(int argc, char *argv[])
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
      mailto = TRUE;
    }
    else if (strcmp(argv[i],"-c")==0)
    {
      if (i<argc-1) cc = argv[++i];
      mailto = TRUE;
    }
    else if (strcmp(argv[i],"-b")==0)
    {
      if (i<argc-1) bcc = argv[++i];
      mailto = TRUE;
    }
    else if (strcmp(argv[i],"-check")==0)
      checkNewMail = TRUE;
    else if (argv[i][0]=='-')
    {
      warning(i18n("Unknown command line option: %s"), argv[i]);
      // unknown parameter
    }
    else
    {
      if (!to.isEmpty()) to += ", ";
      if (strncasecmp(argv[i],"mailto:",7)==0) to += argv[i]+7;
      else to += argv[i];
      mailto = TRUE;
    }
  }

  if (mailto)
  {
    msg->initHeader();
    if (!cc.isEmpty()) msg->setCc(cc);
    if (!bcc.isEmpty()) msg->setBcc(bcc);
    if (!subj.isEmpty()) msg->setSubject(subj);
    if (!to.isEmpty()) msg->setTo(to);

    win = new KMComposeWin(msg);
    assert(win != NULL);
    win->show();
  }
}
*/

//-----------------------------------------------------------------------------
main(int argc, char *argv[])
{
  //--- Sven's pseudo IPC&locking start ---
  app=0;
  {
    int pId;
    
    char lf[80];
    sprintf (lf, "%s.kmail%d.lck", _PATH_TMP, getuid());
    if (access (lf, F_OK) != 0)
      writePid(true); // we are server and ready
    else
    {
      FILE *lock, *msg;
      lock = fopen (lf, "r");
      fscanf (lock, "%d", &pId);
      fclose (lock);
      // Check if pid is 0 - this would kill everything
      if (pId == 0)
      {
        debug ("\nAccording to %s.kmail%d.lck there is existing kmail", _PATH_TMP,
               getuid());
        debug ("process with pid 0, which is wrong. Please close running kmail");
        debug ("(if any), and delete this file like this:\n rm -f %s.kmail*", _PATH_TMP);
        debug ("Then restart kmail");
        exit (0);
      }
      sprintf (lf, "%s.kmail%d.msg", _PATH_TMP, getuid());
      msg = fopen (lf, "w");
      int i;
      KGlobal::dirs()->
	addResourceType("kmail_pic", 
			KStandardDirs::kde_default("data") + "kmail/pics/");

      app = new KApplication(argc, argv, "kmail"); // clear arg list
      argc--;
      argv++;

      // process args:
      QString to, cc, bcc, subj;

      for (i=0; i<argc; i++)
      {
        if (strcmp(argv[i],"-s")==0)
        {
          if (i<argc-1) subj = argv[++i];
          mailto = TRUE;
        }
        else if (strcmp(argv[i],"-c")==0)
        {
          if (i<argc-1) cc = argv[++i];
          mailto = TRUE;
        }
        else if (strcmp(argv[i],"-b")==0)
        {
          if (i<argc-1) bcc = argv[++i];
          mailto = TRUE;
        }
        else if (strcmp(argv[i],"-check")==0)
          checkNewMail = TRUE;
        else if (argv[i][0]=='-')
        {
          warning(i18n("Unknown command line option: %s"), argv[i]);
          // unknown parameter
        }
        else
        {
          if (!to.isEmpty()) to += ", ";
          if (strncasecmp(argv[i],"mailto:",7)==0) to += argv[i]+7;
          else to += argv[i];
          mailto = TRUE;
        }
      }

      if (checkNewMail)
        fprintf (msg, "check");
      else if (mailto)
      {
        if (!to.isEmpty()) fprintf (msg, "To: %s\n", to.data());
        if (!cc.isEmpty()) fprintf (msg, "Cc: %s\n", cc.data());
        if (!bcc.isEmpty()) fprintf (msg, "Bcc: %s\n", bcc.data());
        if (!subj.isEmpty()) fprintf (msg, "Subject: %s\n",subj.data());
      }
      else
        fprintf (msg, "show");
      fclose (msg);                   //message written

      
      if (pId < 0)                    // server busy?
      {
        if (kill(0-pId, 0) != 0)      // try if it lives at all
        {
          debug ("Server died whyle busy");
          writePid(true);             // he diedd and left his pid uncleaned
        }
        else
        {
          debug ("Server is busy - message pending");
          exit (0);                   // ok he lives but is busy
        }
      }
      else                            // if not busy
      {
        if (kill (pId, SIGUSR1) != 0) // Dead?
        {
          debug ("Server died whyle ready");
          writePid(true);             // then we are server
        }
        else
        {
          debug ("Server is ready - message sent");
          exit (0);
        }
      }
    }
    debug ("We are starting normaly");
    sprintf(lf, "%s.kmail%d.msg", _PATH_TMP, getuid());
    unlink(lf); // clear old mesage
  }
  //--- Sven's pseudo IPC&locking end ---

  KMMainWin* mainWin;

  init(argc, argv);
  // filterMgr->dump();

  if (argc > 1)
    processArgs(argc-1, argv+1);

  if (!mailto)
  {

      if (kapp->isRestored()){
	  int n = 1;
	  while (KTMainWindow::canBeRestored(n)){
	      //only restore main windows! (Matthias);
	      if (KTMainWindow::classNameOfToplevel(n) == "KMMainWin")
		  (new KMMainWin)->restore(n);
	      n++;
	  }
      } else {
 	  mainWin = new KMMainWin;
 	  assert( mainWin != NULL);
 	  mainWin->show();
      }
  }
//   if(kapp->isRestored())
//       RESTORE(KMMainWin)
// 	else
// 	  {
// 	  mainWin = new KMMainWin;
// 	  assert( mainWin != NULL);
// 	  mainWin->show();
// 	  }
//   }

  if (checkNewMail) acctMgr->checkMail();
  recoverDeadLetters();

  if (firstStart)
  {
    KMSettings* dlg = new KMSettings;
    assert(dlg != NULL);
    dlg->show();
  }

  app->exec();
  cleanup();
}

