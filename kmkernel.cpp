#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <qutf7codec.h>
#include <kio/job.h>

#include "kmmainwin.h"
#include "kmcomposewin.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmfiltermgr.h"
#include "kmfilteraction.h"
#include "kmsender.h"
#include "kmundostack.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmaddrbook.h"
#include "kfileio.h"
#include "kmpgpwrap.h"
#include "kmversion.h"
#include "kmrecentaddr.h"
#include "kmmsgdict.h"
#include "kmidentity.h"
#include <kwin.h>

#include <X11/Xlib.h>
#include <kapplication.h>


KMKernel *KMKernel::mySelf = 0;

/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel (QObject *parent, const char *name) :
  QObject(parent, name),  DCOPObject("KMailIface")
{
  //kdDebug(5006) << "KMKernel::KMKernel" << endl;
  mySelf = this;
  closed_by_user = true;
  the_firstInstance = true;
  the_msgDict = 0;

  the_inboxFolder = 0;
  the_outboxFolder = 0;
  the_sentFolder = 0;
  the_trashFolder = 0;
  the_draftsFolder = 0;

  the_kbp = 0;
  the_folderMgr = 0;
  the_imapFolderMgr = 0;
  the_undoStack = 0;
  the_acctMgr = 0;
  the_filterMgr = 0;
  the_popFilterMgr = 0;
  the_filterActionDict = 0;
  the_msgSender = 0;
  the_msgDict = 0;

  new KMpgpWrap();
  // register our own (libkdenetwork) utf-7 codec as long as Qt
  // doesn't have it's own:
  if ( !QTextCodec::codecForName("utf-7") ) {
    kdDebug(5006) << "No Qt-native utf-7 codec found; registering QUtf7Codec from libkdenetwork" << endl;
    (void) new QUtf7Codec();
  }

  // In the case of Japan. Japanese locale name is "eucjp" but
  // The Japanese mail systems normally used "iso-2022-jp" of locale name.
  // We want to change locale name from eucjp to iso-2022-jp at KMail only.
  if ( QCString(QTextCodec::codecForLocale()->name()).lower() == "eucjp" )
  {
    netCodec = QTextCodec::codecForName("jis7");
    // QTextCodec *cdc = QTextCodec::codecForName("jis7");
    // QTextCodec::setCodecForLocale(cdc);
    // KGlobal::locale()->setEncoding(cdc->mibEnum());
  } else {
    netCodec = QTextCodec::codecForLocale();
  }
}

KMKernel::~KMKernel ()
{
  QMap<KIO::Job*, putData>::Iterator it = mPutJobs.begin();
  while ( it != mPutJobs.end() )
  {
    KIO::Job *job = it.key();
    mPutJobs.remove( it );
    job->kill();
    it = mPutJobs.begin();
  }
  mySelf = 0;
  kdDebug(5006) << "KMKernel::~KMKernel" << endl;
}


/********************************************************************/
/*             DCOP-callable, and command line actions              */
/********************************************************************/
void KMKernel::checkMail () //might create a new reader but won�t show!!
{
  kdDebug(5006) << "KMKernel::checkMail called" << endl;
  KMMainWin *mWin = 0;
  KMainWindow *kmWin = 0;

  for (kmWin = KMainWindow::memberList->first(); kmWin;
    kmWin = KMainWindow::memberList->next())
      if (kmWin->isA("KMMainWin")) break;
  if (kmWin && kmWin->isA("KMMainWin")) {
    mWin = (KMMainWin *) kmWin;
    mWin->slotCheckMail();
  } else {
    mWin = new KMMainWin;
    mWin->slotCheckMail();
    delete mWin;
  }
}

void KMKernel::openReader()
{
  KMMainWin *mWin = 0;
  KMainWindow *ktmw = 0;
  kdDebug(5006) << "KMKernel::openReader called" << endl;

  if (KMainWindow::memberList)
    for (ktmw = KMainWindow::memberList->first(); ktmw;
         ktmw = KMainWindow::memberList->next())
      if (ktmw->isA("KMMainWin"))
        break;

  if (ktmw) {
    mWin = (KMMainWin *) ktmw;
    mWin->show();
    KWin::setActiveWindow(mWin->winId());
  }
  else {
    mWin = new KMMainWin;
    mWin->show();
  }
}

int KMKernel::openComposer (const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, int hidden,
                            const KURL &messageFile,
			    const KURL::List &attachURLs)
{
  kdDebug(5006) << "KMKernel::openComposer called" << endl;

  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  if (!cc.isEmpty()) msg->setCc(cc);
  if (!bcc.isEmpty()) msg->setBcc(bcc);
  if (!subject.isEmpty()) msg->setSubject(subject);
  if (!to.isEmpty()) msg->setTo(to);

  if (!messageFile.isEmpty() && messageFile.isLocalFile())
    msg->setBody( kFileToString( messageFile.path(), true, false ) );

  if (!body.isEmpty()) msg->setBody(body.utf8());

  KMComposeWin *cWin = new KMComposeWin(msg);
  cWin->setCharset("", TRUE);
  for ( KURL::List::ConstIterator it = attachURLs.begin() ; it != attachURLs.end() ; ++it )
    cWin->addAttach((*it));
  if (hidden == 0)
    cWin->show();
  return 1;
}

int KMKernel::openComposer (const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, int hidden,
                            const QString &attachName,
                            const QCString &attachCte,
                            const QCString &attachData,
                            const QCString &attachType,
                            const QCString &attachSubType,
                            const QCString &attachParamAttr,
                            const QString &attachParamValue,
                            const QCString &attachContDisp)
{
  kdDebug(5006) << "KMKernel::openComposer called" << endl;

  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  if (!cc.isEmpty()) msg->setCc(cc);
  if (!bcc.isEmpty()) msg->setBcc(bcc);
  if (!subject.isEmpty()) msg->setSubject(subject);
  if (!to.isEmpty()) msg->setTo(to);
  if (!body.isEmpty()) msg->setBody(body.utf8());

  KMComposeWin *cWin = new KMComposeWin(msg);
  cWin->setCharset("", TRUE);
  if (!attachData.isEmpty()) {
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName(attachName);
    msgPart->setCteStr(attachCte);
    msgPart->setBodyEncoded(attachData);
    msgPart->setTypeStr(attachType);
    msgPart->setSubtypeStr(attachSubType);
    msgPart->setParameter(attachParamAttr,attachParamValue);
    msgPart->setContentDisposition(attachContDisp);
    cWin->addAttach(msgPart);
  }

  if (hidden == 0)
    cWin->show();
  return 1;
}

DCOPRef KMKernel::openComposer(const QString &to, const QString &cc,
                               const QString &bcc, const QString &subject,
                               const QString &body,bool hidden)
{
  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  if (!cc.isEmpty()) msg->setCc(cc);
  if (!bcc.isEmpty()) msg->setBcc(bcc);
  if (!subject.isEmpty()) msg->setSubject(subject);
  if (!to.isEmpty()) msg->setTo(to);
  if (!body.isEmpty()) msg->setBody(body.utf8());

  KMComposeWin *cWin = new KMComposeWin(msg);
  cWin->setCharset("", TRUE);
  if (!hidden) cWin->show();

  return DCOPRef(cWin);
}

void KMKernel::compactAllFolders ()
{
  kdDebug(5006) << "KMKernel::compactAllFolders called" << endl;
  the_folderMgr->compactAll();
  kdDebug(5006) << "KMKernel::compactAllFolders finished" << endl;
}

int KMKernel::dcopAddMessage(const QString & foldername,const QString & msgUrlString)
{
  return dcopAddMessage(foldername, KURL(msgUrlString));
}

int KMKernel::dcopAddMessage(const QString & foldername,const KURL & msgUrl)
{
int retval;
QCString bericht;
static QStringList *msgIds=NULL;
static QString      lastFolder="";
bool readFolderMsgIds=false;

  //kdDebug(5006) << "KMKernel::dcopAddMessage called" << endl;

  if (foldername!=lastFolder) {
    if (msgIds!=NULL) { delete msgIds; }
    msgIds=new QStringList;
    readFolderMsgIds=true;
    lastFolder=foldername;
  }

  if (!msgUrl.isEmpty() && msgUrl.isLocalFile()) {

    // This is a proposed change by Daniel Andor.
    // He proposed to change from the fopen(blah)
    // to a kFileToString(blah).
    // Although it assigns a QString to a QString,
    // because of the implicit sharing this poses
    // no memory or performance penalty.

    bericht=kFileToString(msgUrl.path(),true,false);
    if (bericht.isNull()) { return -2; }

    KMMessage *M=new KMMessage();
    M->fromString(bericht);

    KMFolder  *F=the_folderMgr->findOrCreate(foldername, FALSE);

    if (F==NULL) { retval=-1; }
    else {

      if (readFolderMsgIds) {int i;

        // Try to determine if a message already exists in
        // the folder. The message id that is searched for, is
        // the subject line + the date. This should be quite
        // unique. The change that a given date with a given
        // subject is in the folder twice is very small.

        // If the subject is empty, the fromStrip string
        // is taken.

        F->open();
        for(i=0;i<F->count();i++) {KMMsgBase *mb=F->getMsgBase(i);
          time_t  DT=mb->date();
          QString dt=ctime(&DT);
          QString id=mb->subject();

          if (id=="") { id=mb->fromStrip(); }
          if (id=="") { id=mb->toStrip(); }

          id+=dt;

          //fprintf(stderr,"%s\n",(const char *) id);
          if (id!="") { msgIds->append(id); }
        }
        F->close();
      }

      time_t  DT=M->date();
      QString dt=ctime(&DT);
      QString msgId=M->subject();

      if (msgId=="") { msgId=M->fromStrip(); }
      if (msgId=="") { msgId=M->toStrip(); }

      msgId+=dt;

      int     k=msgIds->findIndex(msgId);
      //fprintf(stderr,"find %s = %d\n",(const char *) msgId,k);

      if (k==-1) {
        if (msgId!="") { msgIds->append(msgId); }
        if (F->addMsg(M)==0) { retval=1; }
        else { retval=-2;delete M; }
      }
      else { retval=-4; }
    }

    return retval;
  }
  else {
    return -2;
  }
}

/********************************************************************/
/*                        Kernel methods                            */
/********************************************************************/

void KMKernel::quit()
{
  // Called when all windows are closed. Will take care of compacting,
  // sending... should handle session management too!!
}
  /* TODO later:
   Asuming that:
     - msgsender is nonblocking
       (our own, QSocketNotifier based. Pops up errors and sends signal
        senderFinished when done)
     - compacting is non blocking (insert processEvents there)

   o If we are getting mail, stop it (but don�t lose something!)
   o If we are sending mail, go on UNLESS this was called by SM,
       in which case stop ASAP that too (can we warn? should we continue
       on next start?)
   o If we are compacting, or expunging, go on UNLESS this was SM call.
       In that case stop compacting ASAP and continue on next start, before
       touching any folders.

   KMKernel::quit ()
   {
     SM call?
       if compacting, stop;
       if sending, stop;
       if receiving, stop;
       Windows will take care of themselves (composer should dump
        it�s messages, if any but not in deadMail)
       declare us ready for the End of the Session

     No, normal quit call
       All windows are off. Anything to do, should compact or sender sends?
         Yes, maybe put an icon in panel as a sign of life
         Folder manager, go compacting (*except* outbox and sent-mail!)
         if sender sending, connect us to his finished slot, declare us ready
                            for quit and wait for senderFinished
         if not, Folder manager, go compact sent-mail and outbox
}                (= call slotFinished())

void KMKernel::slotSenderFinished()
{
  good, Folder manager go compact sent-mail and outbox
  clean up stage1 (release folders and config, unregister from dcop)
    -- another kmail may start now ---
  kapp->quit();
}

void KMKernel::
void KMKernel::
*/


/********************************************************************/
/*            Init, Exit, and handler  methods                      */
/********************************************************************/
void KMKernel::testDir(const char *_name)
{
  // FIXME: use Qt methods (QFile, QDir)
  DIR *dp;
  QCString c( getenv("HOME") );
  if(c.isEmpty())
  {
      KMessageBox::sorry(0, i18n("$HOME is not set!\n"
                                 "KMail cannot start without it.\n"));
      exit(-1);
  }

  c += _name;
  dp = opendir(c);
  if (dp == NULL) ::mkdir(c, S_IRWXU);
  else closedir(dp);
}


//-----------------------------------------------------------------------------
// Open a composer for each message found in ~/dead.letter
//to control
void KMKernel::recoverDeadLetters(void)
{
  KMComposeWin* win;
  KMMessage* msg;
  QDir dir = QDir::home();
  QString fname = dir.path();
  int i, rc, num;

  if (!dir.exists("dead.letter")) return;
  fname += "/dead.letter";
  KMFolderMbox folder(0, fname);

  folder.setAutoCreateIndex(FALSE);
  rc = folder.open();
  if (rc)
  {
    perror(QString("cannot open file "+fname).latin1());
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
  QFile::remove(fname);
}

void KMKernel::initFolders(KConfig* cfg)
{
  QString name;

  name = cfg->readEntry("inboxFolder");

  // Currently the folder manager cannot manage folders which are not
  // in the base folder directory.
  //if (name.isEmpty()) name = getenv("MAIL");

  if (name.isEmpty()) name = "inbox";

  the_inboxFolder  = (KMFolder*)the_folderMgr->findOrCreate(name);
  the_inboxFolder->setSystemFolder(TRUE);
  // inboxFolder->open();

  the_outboxFolder = the_folderMgr->findOrCreate(cfg->readEntry("outboxFolder", "outbox"));
  the_outboxFolder->setType("Out");
  the_outboxFolder->setSystemFolder(TRUE);
  the_outboxFolder->open();

  the_sentFolder = the_folderMgr->findOrCreate(cfg->readEntry("sentFolder", "sent-mail"));
  the_sentFolder->setType("St");
  the_sentFolder->setSystemFolder(TRUE);
  the_sentFolder->open();

  the_trashFolder  = the_folderMgr->findOrCreate(cfg->readEntry("trashFolder", "trash"));
  the_trashFolder->setType("Tr");
  the_trashFolder->setSystemFolder(TRUE);
  the_trashFolder->open();

  the_draftsFolder = the_folderMgr->findOrCreate(cfg->readEntry("draftsFolder", "drafts"));
  the_draftsFolder->setType("Df");
  the_draftsFolder->setSystemFolder(TRUE);
  the_draftsFolder->open();

}


void KMKernel::init()
{
  kdDebug(5006) << "entering KMKernel::init()" << endl;
  QString foldersPath;
  KConfig* cfg;

  the_checkingMail = false;
  the_shuttingDown = false;
  the_server_is_ready = false;

  the_kbp = new KBusyPtr;
  cfg = kapp->config();
  //kdDebug(5006) << "1" << endl;

  QDir dir;
  QString d = locateLocal("data", "kmail/");

  KConfigGroupSaver saver(cfg, "General");
  the_firstStart = cfg->readBoolEntry("first-start", true);
  cfg->writeEntry("first-start", false);
  the_previousVersion = cfg->readEntry("previous-version", "");
  cfg->writeEntry("previous-version", KMAIL_VERSION);
  foldersPath = cfg->readEntry("folders", "");

  if (foldersPath.isEmpty())
  {
    foldersPath = QDir::homeDirPath() + QString("/Mail");
    transferMail();
  }

  the_undoStack     = new KMUndoStack(20);
  the_folderMgr     = new KMFolderMgr(foldersPath);
  the_imapFolderMgr = new KMFolderMgr(locateLocal("appdata","imap"), TRUE);
  the_acctMgr       = new KMAcctMgr();
  the_filterMgr     = new KMFilterMgr();
  the_popFilterMgr     = new KMFilterMgr(true);
  the_filterActionDict = new KMFilterActionDict;

  initFolders(cfg);
  the_acctMgr->readConfig();
  the_filterMgr->readConfig();
  the_popFilterMgr->readConfig();
  cleanupImapFolders();

  KMMessage::readConfig();
  the_msgSender = new KMSender;

  the_msgDict = new KMMsgDict;

  the_server_is_ready = true;

  { // area for config group "Composer"
    KConfigGroupSaver saver(cfg, "Composer");
    if (cfg->readListEntry("pref-charsets").isEmpty())
    {
      cfg->writeEntry("pref-charsets", "us-ascii,locale,utf-8");
    }
  }
  // filterMgr->dump();
  kdDebug(5006) << "exiting KMKernel::init()" << endl;
}

void KMKernel::cleanupImapFolders()
{
  KMAccount *acct;
  KMFolderNode *node = the_imapFolderMgr->dir().first();
  while (node)
  {
    if (node->isDir() || ((acct = the_acctMgr->find(node->name()))
        && acct->type() == "imap"))
    {
      node = the_imapFolderMgr->dir().next();
    } else {
      the_imapFolderMgr->remove(static_cast<KMFolder*>(node));
      node = the_imapFolderMgr->dir().first();
    }
  }
  KMFolderImap *fld;
  KMAcctImap *imapAcct;
  the_imapFolderMgr->quiet(TRUE);
  for (acct = the_acctMgr->first(); acct; acct = the_acctMgr->next())
  {
    if (acct->type() != "imap") continue;
    fld = static_cast<KMFolderImap*>(the_imapFolderMgr
      ->findOrCreate(acct->name(), FALSE));
    fld->setNoContent(TRUE);
    imapAcct = static_cast<KMAcctImap*>(acct);
    fld->setAccount(imapAcct);
    imapAcct->setImapFolder(fld);
    fld->close();
  }
  the_imapFolderMgr->quiet(FALSE);
}

bool KMKernel::doSessionManagement()
{

  // Do session management
  if (kapp->isRestored()){
    int n = 1;
    while (KMMainWin::canBeRestored(n)){
      //only restore main windows! (Matthias);
      if (KMMainWin::classNameOfToplevel(n) == "KMMainWin")
        (new KMMainWin)->restore(n);
      n++;
    }
    return true; // we were restored by SM
  }
  return false;  // no, we were not restored
}

void KMKernel::cleanup(void)
{
  the_shuttingDown = TRUE;
  KConfig* config =  kapp->config();
  KConfigGroupSaver saver(config, "General");

  delete the_acctMgr;
  the_acctMgr = 0;
  delete the_filterMgr;
  the_filterMgr = 0;
  delete the_msgSender;
  the_msgSender = 0;
  delete the_filterActionDict;
  the_filterActionDict = 0;
  delete the_undoStack;
  the_undoStack = 0;
  delete the_popFilterMgr;
  the_popFilterMgr = 0;

  if (!closed_by_user) {
      if (the_trashFolder)
	  the_trashFolder->close();
  }
  else if (the_trashFolder) {

    the_trashFolder->close(TRUE);

    if (config->readBoolEntry("empty-trash-on-exit", true))
      the_trashFolder->expunge();

  } 

  // Expire old messages in all folders.
  if (closed_by_user) {
    if (config->readNumEntry("when-to-expire")==expireAtExit) {
      bool  expire = true;

      if (config->readBoolEntry("warn-before-expire")) {
	expire = canExpire();
      }
      if (expire) {
	the_folderMgr->expireAllFolders(NULL);
      }
    }
  }

  if (closed_by_user && the_folderMgr) {
    if (config->readBoolEntry("compact-all-on-exit", true))
      the_folderMgr->compactAll(); // I can compact for ages in peace now!
  }
  
  if (the_inboxFolder) the_inboxFolder->close(TRUE);
  if (the_outboxFolder) the_outboxFolder->close(TRUE);
  if (the_sentFolder) the_sentFolder->close(TRUE);
  if (the_draftsFolder) the_draftsFolder->close(TRUE);
  
  delete the_msgDict;
  
  delete the_folderMgr;
  the_folderMgr = 0;
  delete the_imapFolderMgr;
  the_imapFolderMgr = 0;
  delete the_kbp;
  the_kbp = 0;

  //qInstallMsgHandler(oldMsgHandler);
  KMRecentAddresses::self()->save( KGlobal::config() );
  kapp->config()->sync();
}

//Isn�t this obsolete? (sven)
void KMKernel::transferMail(void)
{
  QDir dir = QDir::home();
  int rc;

  // Stefan: This function is for all the whiners who think that KMail is
  // broken because they cannot read mail with pine and do not
  // know how to fix this problem with a simple symbolic link  =;-)
  // Markus: lol ;-)
  if (!dir.cd("KMail")) return;

  rc = KMessageBox::questionYesNo(0,
         i18n(
	    "The directory ~/KMail exists. From now on, KMail uses the "
	    "directory ~/Mail for its messages.\n"
	    "KMail can move the contents of the directory ~/KMail into "
	    "~/Mail, but this will replace existing files with the same "
	    "name in the directory ~/Mail (e.g. inbox).\n"
	    "Should KMail move the mail folders now?"));

  if (rc == KMessageBox::No) return;

  dir.cd("/");  // otherwise we lock the directory
  testDir("/Mail");
  system("mv -f ~/KMail/* ~/Mail");
  system("mv -f ~/KMail/.??* ~/Mail");
  system("rmdir ~/KMail");
}


void KMKernel::ungrabPtrKb(void)
{
  if(!KMainWindow::memberList) return;
  QWidget* widg = KMainWindow::memberList->first();
  Display* dpy;

  if (!widg) return;
  dpy = widg->x11Display();
  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
}


// Message handler
void KMKernel::kmailMsgHandler(QtMsgType aType, const char* aMsg)
{
  static int recurse=-1;

  recurse++;

  switch (aType)
  {
  case QtDebugMsg:
  case QtWarningMsg:
    kdDebug(5006) << aMsg << endl;;
    break;

  case QtFatalMsg: // Hm, what about using kdFatal() here?
    ungrabPtrKb();
    kdDebug(5006) << kapp->caption() << " fatal error "
		  << aMsg << endl;
    KMessageBox::error(0, aMsg);
    abort();
  }

  recurse--;
}
void KMKernel::dumpDeadLetters()
{
  QWidget *win;

  while (KMainWindow::memberList->first() != 0)
  {
    win = KMainWindow::memberList->take();
    if (win->inherits("KMComposeWin")) ((KMComposeWin*)win)->deadLetter();
//    delete win; // WABA: Don't delete, we might crash in there!
  }
}



void KMKernel::action(bool mailto, bool check, const QString &to,
                      const QString &cc, const QString &bcc,
                      const QString &subj, const QString &body,
                      const KURL &messageFile,
                      const KURL::List &attachURLs)
{

  if (mailto)
    openComposer (to, cc, bcc, subj, body, 0, messageFile, attachURLs);
  else
    openReader();

  if (check)
    checkMail();
  //Anything else?
}

void KMKernel::byteArrayToRemoteFile(const QByteArray &aData, const KURL &aURL,
  bool overwrite)
{
  KIO::Job *job = KIO::put(aURL, -1, overwrite, FALSE);
  putData pd; pd.url = aURL; pd.data = aData; pd.offset = 0;
  mPutJobs.insert(job, pd);
  connect(job, SIGNAL(dataReq(KIO::Job*,QByteArray&)),
    SLOT(slotDataReq(KIO::Job*,QByteArray&)));
  connect(job, SIGNAL(result(KIO::Job*)),
    SLOT(slotResult(KIO::Job*)));
}

void KMKernel::slotDataReq(KIO::Job *job, QByteArray &data)
{
  // send the data in 64 KB chunks
  const int MAX_CHUNK_SIZE = 64*1024;
  QMap<KIO::Job*, putData>::Iterator it = mPutJobs.find(job);
  assert(it != mPutJobs.end());
  int remainingBytes = (*it).data.size() - (*it).offset;
  if( remainingBytes > MAX_CHUNK_SIZE )
  {
    // send MAX_CHUNK_SIZE bytes to the receiver (deep copy)
    data.duplicate( (*it).data.data() + (*it).offset, MAX_CHUNK_SIZE );
    (*it).offset += MAX_CHUNK_SIZE;
    //kdDebug( 5006 ) << "Sending " << MAX_CHUNK_SIZE << " bytes ("
    //                << remainingBytes - MAX_CHUNK_SIZE << " bytes remain)\n";
  }
  else
  {
    // send the remaining bytes to the receiver (deep copy)
    data.duplicate( (*it).data.data() + (*it).offset, remainingBytes );
    (*it).data = QByteArray();
    (*it).offset = 0;
    //kdDebug( 5006 ) << "Sending " << remainingBytes << " bytes\n";
  }
}

void KMKernel::slotResult(KIO::Job *job)
{
  QMap<KIO::Job*, putData>::Iterator it = mPutJobs.find(job);
  assert(it != mPutJobs.end());
  if (job->error())
  {
    if (job->error() == KIO::ERR_FILE_ALREADY_EXIST)
    {
      if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?")
        .arg((*it).url.prettyURL()), i18n("Save to file"), i18n("&Replace"))
        == KMessageBox::Continue)
        byteArrayToRemoteFile((*it).data, (*it).url, TRUE);
    }
    else job->showErrorDialog();
  }
  mPutJobs.remove(it);
}

void KMKernel::notClosedByUser()
{
  closed_by_user = false;

  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folders;
  KMFolder *folder;
  the_folderMgr->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    if (folder->isOpened() && folder->dirty()) folder->writeIndex();
  }
}

void KMKernel::emergencyExit( const QString& reason )
{
  QString mesg = i18n("KMail encountered a fatal error and will "
                      "terminate now.\nThe error was:\n%1").arg( reason );

  kdWarning() << mesg << endl;
  KNotifyClient::userEvent( mesg, KNotifyClient::Messagebox, KNotifyClient::Error );

  ::exit(1);
}

/**
 * Sets whether the user wants to expire old email on exit.
 * When last main window is closed, user can be presented with
 * a dialog asking them whether they want to expire old email.
 * This is used to keep track of the answer for when cleanup
 * occurs.
 */
void
KMKernel::setCanExpire(bool expire) {
  allowedToExpire = expire;
}

/**
 * Returns true or false depending on whether user has given
 * their consent to old email expiry for this session.
 */
bool
KMKernel::canExpire() {
  return allowedToExpire;
}

/**
 * Returns true if the folder is either the outbox or one of the drafts-folders
 */
bool KMKernel::folderIsDraftOrOutbox(KMFolder * folder)
{
	bool test = false;
	
	if (folder == the_outboxFolder || folder == the_draftsFolder)
	{
		test = true;
		return test;
	}	

	// search the identities if the folder matches the drafts-folder
	QStringList identities = KMIdentity::identities();
	QStringList::Iterator it = identities.begin();
	for( ; it != identities.end(); ++it )
	{
		KMIdentity ident( *it );
		ident.readConfig();
		if (!ident.drafts().isEmpty() &&
			ident.drafts() == folder->idString()) test = true;
	}

	return test;
}

#include "kmkernel.moc"
