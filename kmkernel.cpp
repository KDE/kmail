/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "config.h"
#include "kmkernel.h"

#include <weaver.h>
#include <weaverlogger.h>

#include "kmstartup.h"
#include "kmmsgindex.h"
#include "kmmainwin.h"
#include "kmcomposewin.h"
#include "kmfoldermgr.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "kmfiltermgr.h"
#include "kmfilteraction.h"
#include "kmsender.h"
#include "undostack.h"
#include "kmacctmgr.h"
#include "kfileio.h"
#include "kmversion.h"
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;
#include "kmmsgdict.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "configuredialog.h"
// #### disabled for now #include "startupwizard.h"
#include <kwin.h>
#include "kmgroupware.h"
#include "kmailicalifaceimpl.h"
#include "mailserviceimpl.h"
using KMail::MailServiceImpl;
#include "folderIface.h"
using KMail::FolderIface;
#include "cryptplugwrapperlist.h"

#include <kapplication.h>
#include <kaboutdata.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <kstaticdeleter.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kprogress.h>
#include <kpassivepopup.h>
#include <dcopclient.h>
#include <ksystemtray.h>

#include <kdebug.h>

#include <qutf7codec.h>
#include <qvbox.h>
#include <qdir.h>
#include <qobjectlist.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <fixx11h.h>
#include <kcmdlineargs.h>
#include <kstartupinfo.h>

KMKernel *KMKernel::mySelf = 0;

/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel (QObject *parent, const char *name) :
  DCOPObject("KMailIface"), QObject(parent, name),
  mIdentityManager(0), mProgress(0), mConfigureDialog(0),
  mContextMenuShown( false )
{
  //kdDebug(5006) << "KMKernel::KMKernel" << endl;
  mySelf = this;
  the_startingUp = true;
  closed_by_user = true;
  the_firstInstance = true;
  the_msgDict = 0;
  the_msgIndex = 0;

  the_inboxFolder = 0;
  the_outboxFolder = 0;
  the_sentFolder = 0;
  the_trashFolder = 0;
  the_draftsFolder = 0;

  the_folderMgr = 0;
  the_imapFolderMgr = 0;
  the_dimapFolderMgr = 0;
  the_searchFolderMgr = 0;
  the_undoStack = 0;
  the_acctMgr = 0;
  the_filterMgr = 0;
  the_popFilterMgr = 0;
  the_filterActionDict = 0;
  the_msgSender = 0;
  mWin = 0;

  mGroupware = new KMGroupware( this );

  // Set up DCOP interface
  mICalIface = new KMailICalIfaceImpl();

  mXmlGuiInstance = 0;
  mDeadLetterTimer = 0;
  mDeadLetterInterval = 1000*120; // 2 minutes
  allowedToExpire = false;

  new Kpgp::Module();

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
  mMailService =  new MailServiceImpl();
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

  delete mICalIface;
  mICalIface = 0L;

  delete mMailService;
  mySelf = 0;
  kdDebug(5006) << "KMKernel::~KMKernel" << endl;
}

bool KMKernel::handleCommandLine( bool noArgsOpensReader )
{
  QString to, cc, bcc, subj, body;
  KURL messageFile = QString::null;
  KURL::List attachURLs;
  bool mailto = false;
  bool checkMail = false;
  //bool viewOnly = false;

  // process args:
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->getOption("subject"))
  {
     mailto = true;
     subj = QString::fromLocal8Bit(args->getOption("subject"));
  }

  if (args->getOption("cc"))
  {
     mailto = true;
     cc = QString::fromLocal8Bit(args->getOption("cc"));
  }

  if (args->getOption("bcc"))
  {
     mailto = true;
     bcc = QString::fromLocal8Bit(args->getOption("bcc"));
  }

  if (args->getOption("msg"))
  {
     mailto = true;
     messageFile.setPath( QString::fromLocal8Bit(args->getOption("msg")) );
  }

  if (args->getOption("body"))
  {
     mailto = true;
     body = QString::fromLocal8Bit(args->getOption("body"));
  }

  QCStringList attachList = args->getOptionList("attach");
  if (!attachList.isEmpty())
  {
     mailto = true;
     for ( QCStringList::Iterator it = attachList.begin() ; it != attachList.end() ; ++it )
       if ( !(*it).isEmpty() )
         attachURLs += KURL( QString::fromLocal8Bit( *it ) );
  }

  if (args->isSet("composer"))
    mailto = true;

  if (args->isSet("check"))
    checkMail = true;

  for(int i= 0; i < args->count(); i++)
  {
    if (strncasecmp(args->arg(i),"mailto:",7)==0)
      to += args->url(i).path() + ", ";
    else {
      QString tmpArg = QString::fromLocal8Bit( args->arg(i) );
      KURL url( tmpArg );
      if ( url.isValid() )
        attachURLs += url;
      else
        to += tmpArg + ", ";
    }
    mailto = true;
  }
  if ( !to.isEmpty() ) {
    // cut off the superfluous trailing ", "
    to.truncate( to.length() - 2 );
  }

  args->clear();

  if (!noArgsOpensReader && !mailto && !checkMail)
    return false;

  action (mailto, checkMail, to, cc, bcc, subj, body, messageFile, attachURLs);
  return true;
}

/********************************************************************/
/*             DCOP-callable, and command line actions              */
/********************************************************************/
void KMKernel::checkMail () //might create a new reader but won't show!!
{
  kmkernel->acctMgr()->checkMail(false);
}

QStringList KMKernel::accounts()
{
  return kmkernel->acctMgr()->getAccounts();
}

void KMKernel::checkAccount (const QString &account) //might create a new reader but won't show!!
{
  kdDebug(5006) << "KMKernel::checkMail called" << endl;

  KMAccount* acct = kmkernel->acctMgr()->find(account);
  if (acct)
    kmkernel->acctMgr()->singleCheckMail(acct, false);
}

void KMKernel::openReader( bool onlyCheck )
{
  KMMainWin *mWin = 0;
  KMainWindow *ktmw = 0;
  kdDebug(5006) << "KMKernel::openReader called" << endl;

  if (KMainWindow::memberList)
    for (ktmw = KMainWindow::memberList->first(); ktmw;
         ktmw = KMainWindow::memberList->next())
      if (ktmw->isA("KMMainWin"))
        break;

  bool activate;
  if (ktmw) {
    mWin = (KMMainWin *) ktmw;
    activate = !onlyCheck; // existing window: only activate if not --check
  }
  else {
    mWin = new KMMainWin;
    activate = true; // new window: always activate
  }

  if ( activate ) {
    mWin->show();
    // Activate window - doing this instead of KWin::activateWindow(mWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( mWin, kapp->startupId() );
#endif
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

  if (!messageFile.isEmpty() && messageFile.isLocalFile()) {
    QCString str = kFileToString( messageFile.path(), true, false );
    if( !str.isEmpty() )
      msg->setBody( str );
  }

  if (!body.isEmpty()) msg->setBody(body.utf8());

  KMComposeWin *cWin = new KMComposeWin(msg);
  cWin->setCharset("", TRUE);
  for ( KURL::List::ConstIterator it = attachURLs.begin() ; it != attachURLs.end() ; ++it )
    cWin->addAttach((*it));
  if (hidden == 0) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  }
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

  if (hidden == 0) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  }
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
  if (!hidden) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  }

  return DCOPRef(cWin);
}

int KMKernel::sendCertificate( const QString& to, const QByteArray& certData )
{
  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  msg->setSubject( i18n( "Certificate Signature Request" ) );
  if (!to.isEmpty()) msg->setTo(to);
  msg->setBody( i18n( "Please sign this certificate and return to sender." ).utf8() );

  KMComposeWin *cWin = new KMComposeWin(msg);
  cWin->setCharset("", TRUE);
  cWin->slotSetAlwaysSend( true );
  if (!certData.isEmpty()) {
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName("smime.p10");
    msgPart->setCteStr("base64");
    msgPart->setBodyEncodedBinary(certData);
    msgPart->setTypeStr("application");
    msgPart->setSubtypeStr("pkcs10");
    msgPart->setContentDisposition("attachment; filename=smime.p10");
    cWin->addAttach(msgPart);
  }

  cWin->show();
  return 1;
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
  if ( foldername.isEmpty() )
    return -1;

  int retval;
  QCString messageText;
  static QStringList *msgIds = 0;
  static QString      lastFolder = "";
  bool readFolderMsgIds = false;

  //kdDebug(5006) << "KMKernel::dcopAddMessage called" << endl;

  if ( foldername != lastFolder ) {
    if ( msgIds != 0 ) {
      delete msgIds;
      msgIds = 0;
    }
    msgIds = new QStringList;
    readFolderMsgIds = true;
    lastFolder = foldername;
  }

  if (!msgUrl.isEmpty() && msgUrl.isLocalFile()) {

    // This is a proposed change by Daniel Andor.
    // He proposed to change from the fopen(blah)
    // to a kFileToString(blah).
    // Although it assigns a QString to a QString,
    // because of the implicit sharing this poses
    // no memory or performance penalty.

    messageText = kFileToString( msgUrl.path(), true, false);
    if ( messageText.isNull() )
      return -2;

    KMMessage *msg = new KMMessage();
    msg->fromString( messageText );

    KMFolder *folder = the_folderMgr->findOrCreate(foldername, false);

    if ( folder ) {
      if (readFolderMsgIds) {

        // Try to determine if a message already exists in
        // the folder. The message id that is searched for, is
        // the subject line + the date. This should be quite
        // unique. The change that a given date with a given
        // subject is in the folder twice is very small.

        // If the subject is empty, the fromStrip string
        // is taken.
        int i;

        folder->open();
        for( i=0; i<folder->count(); i++) {
          KMMsgBase *mb = folder->getMsgBase(i);
          time_t  DT = mb->date();
          QString dt = ctime(&DT);
          QString id = mb->subject();

          if (id.isEmpty())
            id = mb->fromStrip();
          if (id.isEmpty())
            id = mb->toStrip();

          id+=dt;

          //fprintf(stderr,"%s\n",(const char *) id);
          if (!id.isEmpty()) {
            msgIds->append(id);
          }
        }
        folder->close();
      }

      time_t DT = msg->date();
      QString dt = ctime( &DT );
      QString msgId = msg->subject();

      if ( msgId.isEmpty() )
        msgId = msg->fromStrip();
      if ( msgId.isEmpty() )
        msgId = msg->toStrip();

      msgId += dt;

      int k = msgIds->findIndex( msgId );
      //fprintf(stderr,"find %s = %d\n",(const char *) msgId,k);

      if ( k == -1 ) {
        if ( !msgId.isEmpty() ) {
          msgIds->append( msgId );
        }
        if ( folder->addMsg( msg ) == 0 ) {
          retval = 1;
        } else {
          retval =- 2;
          delete msg;
          msg = 0;
        }
      } else {
        retval = -4;
      }
    } else {
      retval = -1;
    }
  } else {
    retval = -2;
  }
  return retval;
}

QStringList KMKernel::folderList() const
{
  QStringList folders;
  the_folderMgr->getFolderURLS( folders );
  the_imapFolderMgr->getFolderURLS( folders );
  the_dimapFolderMgr->getFolderURLS( folders );
  return folders;
}

DCOPRef KMKernel::getFolder( const QString& vpath )
{
  if ( the_folderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  else if ( the_imapFolderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  else if ( the_dimapFolderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  return DCOPRef();
}

bool KMKernel::showMail( Q_UINT32 serialNumber, QString /* messageId */ )
{
  KMMainWidget *mainWidget = 0;
  if (KMainWindow::memberList) {
    KMainWindow *win = 0;
    QObjectList *l;

    // First look for a KMainWindow.
    for (win = KMainWindow::memberList->first(); win;
         win = KMainWindow::memberList->next()) {
      // Then look for a KMMainWidget.
      l	= win->queryList("KMMainWidget");
      if (l && l->first()) {
	mainWidget = dynamic_cast<KMMainWidget *>(l->first());
	if (win->isActiveWindow())
	  break;
      }
    }
  }

  if (mainWidget) {
    int idx = -1;
    KMFolder *folder = 0;
    msgDict()->getLocation(serialNumber, &folder, &idx);
    if (!folder || (idx == -1))
      return false;
    folder->open();
    KMMsgBase *msgBase = folder->getMsgBase(idx);
    if (!msgBase)
      return false;
    bool unGet = !msgBase->isMessage();
    KMMessage *msg = folder->getMsg(idx);
    mainWidget->slotSelectFolder(folder);
    mainWidget->slotSelectMessage(msg);
    if (unGet)
      folder->unGetMsg(idx);
    folder->close();
    return true;
  }

  return false;
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

   o If we are getting mail, stop it (but don´t lose something!)
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
        it´s messages, if any but not in deadMail)
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
  QString foldersPath = QDir::homeDirPath() + QString( _name );
  QFileInfo info( foldersPath );
  if ( !info.exists() ) {
    if ( ::mkdir( QFile::encodeName( foldersPath ) , S_IRWXU ) == -1 ) {
      KMessageBox::sorry(0, i18n("KMail couldn't create folder '%1'.\n"
                                 "Please make sure that you can view and "
                                 "modify the content of the folder '%2'.")
                            .arg( foldersPath ).arg( QDir::homeDirPath() ) );
      ::exit(-1);
    }
  }
  if ( !info.isDir() || !info.isReadable() || !info.isWritable() ) {
    KMessageBox::sorry(0, i18n("The permissions of the folder '%1' are "
                               "incorrect.\n"
                               "Please make sure that you can view and modify "
                               "the content of this folder.")
                          .arg( foldersPath ) );
    ::exit(-1);
  }
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
  mDeadLetterTimer = new QTimer(this);
  connect(mDeadLetterTimer, SIGNAL(timeout()), this, SLOT(dumpDeadLetters()));

  if (!dir.exists("dead.letter")) {
      mDeadLetterTimer->start(mDeadLetterInterval);
      return;
  }

  fname += "/dead.letter";
  KMFolder folder(0, fname, KMFolderTypeMbox);

  folder.setAutoCreateIndex(FALSE);
  rc = folder.open();
  if (rc)
  {
    perror(QString("cannot open file "+fname).latin1());
    mDeadLetterTimer->start(mDeadLetterInterval);
    return;
  }

  folder.open();

  num = folder.count();
  for (i=0; i<num; i++)
  {
    msg = folder.take(0);
    if (msg)
    {
      win = new KMComposeWin();
      win->setMsg(msg, false, false, true);
      win->show();
    }
  }
  folder.close();
  QFile::remove(fname);
  mDeadLetterTimer->start(mDeadLetterInterval);
}

void KMKernel::initFolders(KConfig* cfg)
{
  QString name;

  name = cfg->readEntry("inboxFolder");

  // Currently the folder manager cannot manage folders which are not
  // in the base folder directory.
  //if (name.isEmpty()) name = getenv("MAIL");

  if (name.isEmpty()) name = I18N_NOOP("inbox");

  the_inboxFolder  = (KMFolder*)the_folderMgr->findOrCreate(name);

  if (the_inboxFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your inbox folder.") );
  }

  the_inboxFolder->setSystemFolder(TRUE);
  if ( the_inboxFolder->userWhoField().isEmpty() )
    the_inboxFolder->setUserWhoField( QString::null );
  // inboxFolder->open();

  the_outboxFolder = the_folderMgr->findOrCreate(cfg->readEntry("outboxFolder", I18N_NOOP("outbox")));
  if (the_outboxFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your outbox folder.") );
  }

  the_outboxFolder->setType("Out");
  the_outboxFolder->setSystemFolder(TRUE);
  if ( the_outboxFolder->userWhoField().isEmpty() )
    the_outboxFolder->setUserWhoField( QString::null );
  the_outboxFolder->open();

  the_sentFolder = the_folderMgr->findOrCreate(cfg->readEntry("sentFolder", I18N_NOOP("sent-mail")));
  if (the_sentFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your sent-mail folder.") );
  }
  the_sentFolder->setType("St");
  the_sentFolder->setSystemFolder(TRUE);
  if ( the_sentFolder->userWhoField().isEmpty() )
    the_sentFolder->setUserWhoField( QString::null );
  // the_sentFolder->open();

  the_trashFolder  = the_folderMgr->findOrCreate(cfg->readEntry("trashFolder", I18N_NOOP("trash")));
  if (the_trashFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your trash folder.") );
  }
  the_trashFolder->setType("Tr");
  the_trashFolder->setSystemFolder(TRUE);
  if ( the_trashFolder->userWhoField().isEmpty() )
    the_trashFolder->setUserWhoField( QString::null );
  // the_trashFolder->open();

  the_draftsFolder = the_folderMgr->findOrCreate(cfg->readEntry("draftsFolder", I18N_NOOP("drafts")));
  if (the_draftsFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your drafts folder.") );
  }
  the_draftsFolder->setType("Df");
  the_draftsFolder->setSystemFolder(TRUE);
  if ( the_draftsFolder->userWhoField().isEmpty() )
    the_draftsFolder->setUserWhoField( QString::null );
  the_draftsFolder->open();
}


void KMKernel::init()
{
  QString foldersPath;
  KConfig* cfg;

  the_shuttingDown = false;
  the_server_is_ready = false;

  cfg = KMKernel::config();

  mCryptPlugList = new CryptPlugWrapperList();
  mCryptPlugList->loadFromConfig( cfg );

  QDir dir;
  QString d = locateLocal("data", "kmail/");

  KConfigGroupSaver saver(cfg, "General");
  the_firstStart = cfg->readBoolEntry("first-start", true);
  cfg->writeEntry("first-start", false);
  the_previousVersion = cfg->readEntry("previous-version");
  cfg->writeEntry("previous-version", KMAIL_VERSION);
  foldersPath = cfg->readEntry("folders");

  if (foldersPath.isEmpty())
  {
    foldersPath = QDir::homeDirPath() + QString("/Mail");
    transferMail();
  }
  the_undoStack     = new UndoStack(20);
  the_folderMgr     = new KMFolderMgr(foldersPath);
  the_imapFolderMgr = new KMFolderMgr( KMFolderImap::cacheLocation(), KMImapDir);
  the_dimapFolderMgr = new KMFolderMgr( KMFolderCachedImap::cacheLocation(), KMDImapDir);
  the_searchFolderMgr = new KMFolderMgr(locateLocal("data","kmail/search"), KMSearchDir);
  the_acctMgr       = new KMAcctMgr();
  the_filterMgr     = new KMFilterMgr();
  the_popFilterMgr     = new KMFilterMgr(true);
  the_filterActionDict = new KMFilterActionDict;

  // moved up here because KMMessage::stripOffPrefixes is used below -ta
  KMMessage::readConfig();
  initFolders(cfg);
  the_acctMgr->readConfig();
  the_filterMgr->readConfig();
  the_popFilterMgr->readConfig();
  cleanupImapFolders();

  the_msgSender = new KMSender;
  the_server_is_ready = true;

  { // area for config group "Composer"
    KConfigGroupSaver saver(cfg, "Composer");
    if (cfg->readListEntry("pref-charsets").isEmpty())
    {
      cfg->writeEntry("pref-charsets", "us-ascii,iso-8859-1,locale,utf-8");
    }
  }
  mGroupware->readConfig();
  mICalIface->readConfig();
  // filterMgr->dump();
#if 0 //disabled for now..
  the_msgIndex = new KMMsgIndex(this, "the_index"); //create the indexer
  the_msgIndex->init();
  the_msgIndex->remove();
  delete the_msgIndex;
  the_msgIndex = 0;
#endif

#if 0
  the_weaver =  new KPIM::ThreadWeaver::Weaver( this );
  the_weaverLogger = new KPIM::ThreadWeaver::WeaverThreadLogger(this);
  the_weaverLogger->attach (the_weaver);
#endif

}

void KMKernel::cleanupImapFolders()
{
  KMAccount *acct;
  KMFolderNode *node = the_imapFolderMgr->dir().first();
  while (node)
  {
    if (node->isDir() || ((acct = the_acctMgr->find(node->name()))
			  && ( acct->type() == "imap" )) )
    {
      node = the_imapFolderMgr->dir().next();
    } else {
      the_imapFolderMgr->remove(static_cast<KMFolder*>(node));
      node = the_imapFolderMgr->dir().first();
    }
  }

  node = the_dimapFolderMgr->dir().first();
  while (node)
  {
    if (node->isDir() || ((acct = the_acctMgr->find(node->name()))
			  && ( acct->type() == "cachedimap" )) )
    {
      node = the_dimapFolderMgr->dir().next();
    } else {
      static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->removeRightAway();
      the_dimapFolderMgr->remove(static_cast<KMFolder*>(node));
      node = the_imapFolderMgr->dir().first();
    }
  }

  the_imapFolderMgr->quiet(true);
  for (acct = the_acctMgr->first(); acct; acct = the_acctMgr->next())
  {
    KMFolderImap *fld;
    KMAcctImap *imapAcct;

    if (acct->type() != "imap") continue;
    fld = static_cast<KMFolderImap*>(the_imapFolderMgr
      ->findOrCreate(acct->name(), false)->storage());
    fld->setNoContent(true);
    imapAcct = static_cast<KMAcctImap*>(acct);
    fld->setAccount(imapAcct);
    imapAcct->setImapFolder(fld);
    fld->close();
  }
  the_imapFolderMgr->quiet(false);

  the_dimapFolderMgr->quiet( true );
  for (acct = the_acctMgr->first(); acct; acct = the_acctMgr->next())
  {
    KMFolderCachedImap *cfld = 0;
    KMAcctCachedImap *cachedImapAcct;

    if (acct->type() != "cachedimap" ) continue;

    KMFolder* fld = the_dimapFolderMgr->find(acct->name());
    if( fld )
      cfld = static_cast<KMFolderCachedImap*>( fld->storage() );
    if (cfld == 0) {
      // Folder doesn't exist yet
      cfld = static_cast<KMFolderCachedImap*>(the_dimapFolderMgr->createFolder(acct->name(), false, KMFolderTypeCachedImap)->storage());
      if (!cfld) {
	KMessageBox::error(0,(i18n("Cannot create file `%1' in %2.\nKMail cannot start without it.").arg(acct->name()).arg(the_dimapFolderMgr->basePath())));
	exit(-1);
      }
    }

    cfld->setNoContent(true);
    cachedImapAcct = static_cast<KMAcctCachedImap*>(acct);
    cfld->setAccount(cachedImapAcct);
    cachedImapAcct->setImapFolder(cfld);
    cfld->close();
  }
  the_dimapFolderMgr->quiet( false );
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

void KMKernel::closeAllKMailWindows()
{
  QPtrListIterator<KMainWindow> it(*KMainWindow::memberList);
  KMainWindow *window = 0;
  while ((window = it.current()) != 0) {
    ++it;
    if (window->isA("KMMainWindow") || window->inherits("SecondaryWindow"))
      window->close( true ); // close and delete the window
  }
}

void KMKernel::notClosedByUser()
{
  if (!closed_by_user) // already closed
    return;
  closed_by_user = false;
  the_shuttingDown = true;
  closeAllKMailWindows();

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

  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folders;
  KMFolder *folder;
  the_folderMgr->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    folder->close(TRUE);
  }
  strList.clear();
  folders.clear();
  the_searchFolderMgr->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    folder->close(TRUE);
  }
  folderMgr()->writeMsgDict(msgDict());
  imapFolderMgr()->writeMsgDict(msgDict());
  dimapFolderMgr()->writeMsgDict(msgDict());
  delete the_msgIndex;
  the_msgIndex = 0;
  delete the_folderMgr;
  the_folderMgr = 0;
  delete the_imapFolderMgr;
  the_imapFolderMgr = 0;
  delete the_dimapFolderMgr;
  the_dimapFolderMgr = 0;
  delete the_searchFolderMgr;
  the_searchFolderMgr = 0;
  delete the_msgDict;
  the_msgDict = 0;
  delete mConfigureDialog;
  mConfigureDialog = 0;
  delete mWin;
  mWin = 0;
}

void KMKernel::cleanup(void)
{
  dumpDeadLetters();
  mDeadLetterTimer->stop();
  the_shuttingDown = TRUE;
  closeAllKMailWindows();

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

#if 0
  delete the_weaver;
  the_weaver = 0;
#endif

  // Since the application has already quit we can't use
  // kapp->processEvents() because it will return immediately:
  // We first have to fire up a new event loop.
  // We use the timer to transfer control to the cleanupLoop function
  // once the event loop is running.

  // Don't handle DCOP requests from the event loop
  kapp->dcopClient()->suspend();

  // Schedule execution of cleanupLoop
  QTimer::singleShot(0, this, SLOT(cleanupLoop()));

  // Start new event loop
  kapp->enter_loop();
}

void KMKernel::cleanupProgress()
{
  mProgress->advance( 1 );
}

void KMKernel::cleanupLoop()
{
  QStringList cleanupMsgs;
  cleanupMsgs << i18n("Cleaning up...")
              << i18n("Emptying trash...")
              << i18n("Expiring old messages...")
              << i18n("Compacting folders...");
  enum { CleaningUpMsgNo = 0,
         EmptyTrashMsgNo = 1,
         ExpiringOldMessagesMsgNo = 2,
         CompactingFoldersMsgNo = 3 };
  mProgress = 0;
  mCleanupLabel = 0;
  mCleanupPopup = 0;
  int nrFolders = the_folderMgr->folderCount();
  if (closed_by_user)
  {
    mCleanupPopup = new KPassivePopup();
    QVBox *box = mCleanupPopup->standardView( kapp->aboutData()->programName(),
                                              QString::null, kapp->miniIcon());
    mCleanupLabel = new QLabel( cleanupMsgs[CleaningUpMsgNo], box );
    // determine the maximal width of the clean up messages
    QFontMetrics fm = mCleanupLabel->fontMetrics();
    int maxTextWidth = 0;
    for( QStringList::ConstIterator it = cleanupMsgs.begin();
         it != cleanupMsgs.end();
         ++it ) {
      int w;
      if( maxTextWidth < ( w = fm.width( *it ) ) )
        maxTextWidth = w;
    }

    mProgress = new KProgress( box, "kmail-cleanupProgress" );
    mProgress->setMinimumWidth( maxTextWidth+20 );
    mCleanupPopup->setView( box );

    mProgress->setTotalSteps(nrFolders*2+2);
    mProgress->setProgress(1);
    QApplication::syncX();
    mCleanupPopup->adjustSize();
    mCleanupPopup->show();
    kapp->processEvents();
    connect(the_folderMgr, SIGNAL(progress()), this, SLOT(cleanupProgress()));
  }


  KConfig* config =  KMKernel::config();
  KConfigGroupSaver saver(config, "General");

  bool expire = false;
  // Expire old messages in all folders.
  if (closed_by_user) {
    if (config->readNumEntry("when-to-expire")==expireAtExit) {
      expire = true;

      if (config->readBoolEntry("warn-before-expire", true)) {
	expire = canExpire();
      }
    }
  }

  if (!closed_by_user) {
      if (the_trashFolder)
	  the_trashFolder->close();
  }
  else if (the_trashFolder) {

    the_trashFolder->close(TRUE);

    if (config->readBoolEntry("empty-trash-on-exit", true))
    {
      if (mCleanupLabel)
      {
        mCleanupLabel->setText( cleanupMsgs[EmptyTrashMsgNo] );
        QApplication::syncX();
        kapp->processEvents();
      }
      if ( the_trashFolder->count( true ) > 0 )
        the_trashFolder->expunge();
    }
  }

  if (mProgress)
    mProgress->setProgress(2);

  if (expire) {
    if (mCleanupLabel)
    {
       mCleanupLabel->setText( cleanupMsgs[ExpiringOldMessagesMsgNo] );
       QApplication::syncX();
       kapp->processEvents();
    }
    the_folderMgr->expireAllFolders(0);
  }

  if (mProgress)
     mProgress->setProgress(2+nrFolders);

  if (closed_by_user && the_folderMgr) {
    if (config->readBoolEntry("compact-all-on-exit", true))
    {
      if (mCleanupLabel)
      {
        mCleanupLabel->setText( cleanupMsgs[CompactingFoldersMsgNo] );
        QApplication::syncX();
        kapp->processEvents();
      }
      the_folderMgr->compactAll(); // I can compact for ages in peace now!
    }
  }
  if (mProgress) {
    mCleanupLabel->setText( cleanupMsgs[CleaningUpMsgNo] );
    mProgress->setProgress(2+2*nrFolders);
    QApplication::syncX();
    kapp->processEvents();
  }

  if (the_inboxFolder) the_inboxFolder->close(TRUE);
  if (the_outboxFolder) the_outboxFolder->close(TRUE);
  if (the_sentFolder) the_sentFolder->close(TRUE);
  if (the_draftsFolder) the_draftsFolder->close(TRUE);

  mICalIface->cleanup();

  folderMgr()->writeMsgDict(msgDict());
  imapFolderMgr()->writeMsgDict(msgDict());
  dimapFolderMgr()->writeMsgDict(msgDict());
  QValueList<QGuardedPtr<KMFolder> > folders;
  QStringList strList;
  KMFolder *folder;
  the_searchFolderMgr->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    folder->close(TRUE);
  }
  delete the_msgIndex;
  the_msgIndex = 0;
  delete the_folderMgr;
  the_folderMgr = 0;
  delete the_imapFolderMgr;
  the_imapFolderMgr = 0;
  delete the_dimapFolderMgr;
  the_dimapFolderMgr = 0;
  delete the_searchFolderMgr;
  the_searchFolderMgr = 0;
  delete the_msgDict;
  the_msgDict = 0;
  delete mConfigureDialog;
  mConfigureDialog = 0;
  delete mWin;
  mWin = 0;
  mCryptPlugList->setAutoDelete(true);
  mCryptPlugList->clear();
  delete mCryptPlugList;
  mCryptPlugList = 0;

  //qInstallMsgHandler(oldMsgHandler);
  RecentAddresses::self( KMKernel::config() )->save( KMKernel::config() );
  KMKernel::config()->sync();
  if (mCleanupPopup)
  {
    sleep(1); // Give the user some time to realize what's going on
    delete mCleanupPopup;
    mCleanupPopup = 0;
    mCleanupLabel = 0; // auto-deleted child of mCleanupPopup
    mProgress = 0;     // ditto
  }
  kapp->exit_loop();
}

//Isn´t this obsolete? (sven)
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
    kdDebug(5006) << aMsg << endl;
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
  if (shuttingDown())
    return; //All documents should be saved before shutting down is set!
  mDeadLetterTimer->stop();
  QWidget *win;
  QDir dir = QDir::home();
  QString fname = dir.path();
  QFile::remove(fname + "/dead.letter.tmp");
  if (KMainWindow::memberList) {
    QPtrListIterator<KMainWindow> it(*KMainWindow::memberList);

    while ((win = it.current()) != 0) {
      ++it;
      if (win->inherits("KMComposeWin")) ((KMComposeWin*)win)->deadLetter();
      //    delete win; // WABA: Don't delete, we might crash in there!
    }
  }
  QFile::remove(fname + "/dead.letter");
  dir.rename("dead.letter.tmp","dead.letter");
  mDeadLetterTimer->start(mDeadLetterInterval);
}



void KMKernel::action(bool mailto, bool check, const QString &to,
                      const QString &cc, const QString &bcc,
                      const QString &subj, const QString &body,
                      const KURL &messageFile,
                      const KURL::List &attachURLs)
{
  // Run the groupware setup wizard. It doesn't do anything if this isn't
  // the first run. Replace this with a general wizard later
  // #### Disabled until we have a general startup wizard.
  // StartupWizard::run();

  if (mailto)
    openComposer (to, cc, bcc, subj, body, 0, messageFile, attachURLs);
  else
    openReader( check );

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
        .arg((*it).url.prettyURL()), i18n("Save to File"), i18n("&Replace"))
        == KMessageBox::Continue)
        byteArrayToRemoteFile((*it).data, (*it).url, TRUE);
    }
    else job->showErrorDialog();
  }
  mPutJobs.remove(it);
}

void KMKernel::slotCollectStdOut( KProcess * proc, char * buffer, int len )
{
  QByteArray & ba = mStdOutCollection[proc];
  // append data to ba:
  int oldsize = ba.size();
  ba.resize( oldsize + len );
  qmemmove( ba.begin() + oldsize, buffer, len );
}

void KMKernel::slotCollectStdErr( KProcess * proc, char * buffer, int len )
{
  QByteArray & ba = mStdErrCollection[proc];
  // append data to ba:
  int oldsize = ba.size();
  ba.resize( oldsize + len );
  qmemmove( ba.begin() + oldsize, buffer, len );
}

QByteArray KMKernel::getCollectedStdOut( KProcess * proc )
{
  QByteArray result = mStdOutCollection[proc];
  mStdOutCollection.remove(proc);
  return result;
}

QByteArray KMKernel::getCollectedStdErr( KProcess * proc )
{
  QByteArray result = mStdErrCollection[proc];
  mStdErrCollection.remove(proc);
  return result;
}

void KMKernel::slotRequestConfigSync() {
  // ### FIXME: delay as promised in the kdoc of this function ;-)
  KMKernel::config()->sync();
}

void KMKernel::slotShowConfigurationDialog()
{
  if( !mConfigureDialog ) {
    mConfigureDialog = new ConfigureDialog( 0, "configure", false );
    connect( mConfigureDialog, SIGNAL( configCommitted() ),
             this, SIGNAL( configChanged() ) );
  }

  if( mConfigureDialog->isHidden() )
    mConfigureDialog->show();
  else
    mConfigureDialog->raise();
}

bool KMKernel::haveSystemTrayApplet()
{
  return !systemTrayApplets.isEmpty();
}

bool KMKernel::registerSystemTrayApplet( const KSystemTray* applet )
{
  if ( systemTrayApplets.findIndex( applet ) == -1 ) {
    systemTrayApplets.append( applet );
    return true;
  }
  else
    return false;
}

bool KMKernel::unregisterSystemTrayApplet( const KSystemTray* applet )
{
  QValueList<const KSystemTray*>::iterator it =
    systemTrayApplets.find( applet );
  if ( it != systemTrayApplets.end() ) {
    systemTrayApplets.remove( it );
    return true;
  }
  else
    return false;
}

void KMKernel::emergencyExit( const QString& reason )
{
  QString mesg;
  if ( reason.length() == 0 ) {
    mesg = i18n("KMail encountered a fatal error and will terminate now");
  }
  else {
    mesg = i18n("KMail encountered a fatal error and will "
                      "terminate now.\nThe error was:\n%1").arg( reason );
  }

  kdWarning() << mesg << endl;
  KNotifyClient::userEvent( 0, mesg, KNotifyClient::Messagebox, KNotifyClient::Error );

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
bool KMKernel::folderIsDraftOrOutbox(const KMFolder * folder)
{
  assert( folder );
  if ( folder == the_outboxFolder || folder == the_draftsFolder )
    return true;

  QString idString = folder->idString();
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the drafts-folder
  const IdentityManager * im = identityManager();
  for( IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).drafts() == idString ) return true;
  return false;
}

bool KMKernel::folderIsTrash(KMFolder * folder)
{
  assert(folder);
  if (folder == the_trashFolder) return true;
  if (folder->folderType() != KMFolderTypeImap) return false;
  KMFolderImap *fi = static_cast<KMFolderImap*>(folder->storage());
  if (fi->account() && fi->account()->trash() == fi->idString())
    return true;
  return false;
}

bool KMKernel::folderIsSentMailFolder( const KMFolder * folder )
{
  assert( folder );
  if ( folder == the_sentFolder )
    return true;

  QString idString = folder->idString();
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the sent-folder
  const IdentityManager * im = identityManager();
  for( IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).fcc() == idString ) return true;
  return false;
}

IdentityManager * KMKernel::identityManager() {
  if ( !mIdentityManager ) {
    kdDebug(5006) << "instantating IdentityManager" << endl;
    mIdentityManager = new IdentityManager( this, "mIdentityManager" );
  }
  return mIdentityManager;
}

KMMsgDict *KMKernel::msgDict()
{
    if (the_msgDict)
	return the_msgDict;
    the_msgDict = new KMMsgDict;
    folderMgr()->readMsgDict(msgDict());
    imapFolderMgr()->readMsgDict(msgDict());
    dimapFolderMgr()->readMsgDict(msgDict());
    return the_msgDict;
}

KMMsgIndex *KMKernel::msgIndex()
{
    return the_msgIndex;
}

KMainWindow* KMKernel::mainWin()
{
  if (KMainWindow::memberList) {
    KMainWindow *kmWin = 0;

    // First look for a KMMainWin.
    for (kmWin = KMainWindow::memberList->first(); kmWin;
         kmWin = KMainWindow::memberList->next())
      if (kmWin->isA("KMMainWin"))
        return kmWin;

    // There is no KMMainWin. Use any other KMainWindow instead (e.g. in
    // case we are running inside Kontact) because we anyway only need
    // it for modal message boxes and for KNotify events.
    kmWin = KMainWindow::memberList->first();
    if ( kmWin )
      return kmWin;
  }

  // There's not a single KMainWindow. Create a KMMainWin.
  // This could happen if we want to pop up an error message
  // while we are still doing the startup wizard and no other
  // KMainWindow is running.
  mWin = new KMMainWin;
  return mWin;
}


/**
 * Emptyies al the trash folders
 */
void KMKernel::slotEmptyTrash()
{
  QString title = i18n("Empty Trash");
  QString text = i18n("Are you sure you want to empty the trash?");
  if (KMessageBox::warningContinueCancel(0, text, title,
                                         KStdGuiItem::cont(), "confirm_empty_trash")
      != KMessageBox::Continue)
  {
    return;
  }

  for (KMAccount* acct = acctMgr()->first(); acct; acct = acctMgr()->next())
  {
    KMFolder* trash = folderMgr()->findIdString(acct->trash());
    if (trash)
    {
      trash->expunge();
    }
  }
}

#if KDE_IS_VERSION( 3, 1, 92 )
KConfig* KMKernel::config()
{
  assert(mySelf);
  if (!mySelf->mConfig)
  {
    mySelf->mConfig = KSharedConfig::openConfig( "kmailrc" );
    // Check that all updates have been run on the config file:
    KMail::checkConfigUpdates();
  }
  return mySelf->mConfig;
}
#else
KConfig *KMKernel::myConfig = 0;
static KStaticDeleter<KConfig> myConfigSD;

KConfig* KMKernel::config()
{
  if (!myConfig)
    myConfig = myConfigSD.setObject(myConfig, new KConfig( "kmailrc"));
  return myConfig;
}
#endif

KMGroupware & KMKernel::groupware()
{
  assert( mGroupware );
  return *mGroupware;
}

KMailICalIfaceImpl& KMKernel::iCalIface()
{
  assert( mICalIface );
  return *mICalIface;
}


#include "kmkernel.moc"
