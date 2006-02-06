/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "config.h"
#include "kmkernel.h"

#warning Port me!
//#include <weaver.h>
//#include <weaverlogger.h>

#include "globalsettings.h"
#include "kmstartup.h"
#include "index.h"
#include "kmmainwin.h"
#include "composer.h"
#include "kmmsgpart.h"
#include "kmreadermainwin.h"
#include "kmfoldermgr.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "kmfiltermgr.h"
#include "kmfilteraction.h"
#define REALLY_WANT_KMSENDER
#include "kmsender.h"
#undef REALLY_WANT_KMSENDER
#include "undostack.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include <libkdepim/kfileio.h>
#include "kmversion.h"
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;
#include "kmmsgdict.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include "configuredialog.h"
#include "kmcommands.h"
#include "kmsystemtray.h"
#include "transportmanager.h"

#include <kwin.h>
#include "kmailicalifaceimpl.h"
#include "mailserviceimpl.h"
using KMail::MailServiceImpl;
#include "mailcomposerIface.h"
#include "folderIface.h"
using KMail::FolderIface;
#include "jobscheduler.h"

#include <kapplication.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <kstaticdeleter.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kprogressbar.h>
#include <kpassivepopup.h>
#include <dcopclient.h>
#include <ksystemtray.h>
#include <kpgp.h>
#include <kdebug.h>
#include <kio/netaccess.h>
#include <kwallet.h>
using KWallet::Wallet;
#include "actionscheduler.h"

#include <QByteArray>
#include <QDir>
#include <QList>
#include <QObject>
#include <QWidget>
//Added by qt3to4:
#include <q3vbox.h>
#include <qutf7codec.h>

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
  mIdentityManager(0), mConfigureDialog(0),
  mContextMenuShown( false ), mWallet( 0 )
{
  kDebug(5006) << "KMKernel::KMKernel" << endl;
  mySelf = this;
  the_startingUp = true;
  closed_by_user = true;
  the_firstInstance = true;
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
  mMailCheckAborted = false;

  // make sure that we check for config updates before doing anything else
  KMKernel::config();
  // this shares the kmailrc parsing too (via KSharedConfig), and reads values from it
  // so better do it here, than in some code where changing the group of config()
  // would be unexpected
  GlobalSettings::self();

  // Set up DCOP interface
  mICalIface = new KMailICalIfaceImpl();

  mJobScheduler = new JobScheduler( this );

  mXmlGuiInstance = 0;

  new Kpgp::Module();

  // register our own (libkdenetwork) utf-7 codec as long as Qt
  // doesn't have it's own:
  if ( !QTextCodec::codecForName("utf-7") ) {
    kDebug(5006) << "No Qt-native utf-7 codec found; registering QUtf7Codec from libkdenetwork" << endl;
    (void) new QUtf7Codec();
  }

  // In the case of Japan. Japanese locale name is "eucjp" but
  // The Japanese mail systems normally used "iso-2022-jp" of locale name.
  // We want to change locale name from eucjp to iso-2022-jp at KMail only.
  if ( QByteArray(QTextCodec::codecForLocale()->name()).toLower() == "eucjp" )
  {
    netCodec = QTextCodec::codecForName("jis7");
    // QTextCodec *cdc = QTextCodec::codecForName("jis7");
    // QTextCodec::setCodecForLocale(cdc);
    // KGlobal::locale()->setEncoding(cdc->mibEnum());
  } else {
    netCodec = QTextCodec::codecForLocale();
  }
  mMailService =  new MailServiceImpl();

  connectDCOPSignal( 0, 0, "kmailSelectFolder(QString)",
                     "selectFolder(QString)", false );
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
  mICalIface = 0;
  delete mMailService;
  mMailService = 0;

  GlobalSettings::self()->writeConfig();
  delete mWallet;
  mWallet = 0;
  mySelf = 0;
  kDebug(5006) << "KMKernel::~KMKernel" << endl;
}

bool KMKernel::handleCommandLine( bool noArgsOpensReader )
{
  QString to, cc, bcc, subj, body;
  KUrl messageFile;
  KUrl::List attachURLs;
  bool mailto = false;
  bool checkMail = false;
  bool viewOnly = false;
  bool calledWithSession = false; // for ignoring '-session foo'

  // process args:
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->isSet("subject"))
  {
     subj = QString::fromLocal8Bit(args->getOption("subject"));
     // if kmail is called with 'kmail -session abc' then this doesn't mean
     // that the user wants to send a message with subject "ession" but
     // (most likely) that the user clicked on KMail's system tray applet
     // which results in KMKernel::raise() calling "kmail kmail newInstance"
     // via dcop which apparently executes the application with the original
     // command line arguments and those include "-session ..." if
     // kmail/kontact was restored by session management
     if ( subj == "ession" ) {
       subj.clear();
       calledWithSession = true;
     }
     else
       mailto = true;
  }

  if (args->isSet("cc"))
  {
     mailto = true;
     cc = QString::fromLocal8Bit(args->getOption("cc"));
  }

  if (args->isSet("bcc"))
  {
     mailto = true;
     bcc = QString::fromLocal8Bit(args->getOption("bcc"));
  }

  if (args->isSet("msg"))
  {
     mailto = true;
     messageFile.setPath( QString::fromLocal8Bit(args->getOption("msg")) );
  }

  if (args->isSet("body"))
  {
     mailto = true;
     body = QString::fromLocal8Bit(args->getOption("body"));
  }

  QByteArrayList attachList = args->getOptionList("attach");
  if (!attachList.isEmpty())
  {
     mailto = true;
     for ( QByteArrayList::Iterator it = attachList.begin() ; it != attachList.end() ; ++it )
       if ( !(*it).isEmpty() )
         attachURLs += KURL( QString::fromLocal8Bit( *it ) );
  }

  if (args->isSet("composer"))
    mailto = true;

  if (args->isSet("check"))
    checkMail = true;

  if ( args->isSet( "view" ) ) {
    viewOnly = true;
    const QString filename =
      QString::fromLocal8Bit( args->getOption( "view" ) );
    messageFile = KUrl::fromPathOrURL( filename );
    if ( !messageFile.isValid() ) {
      messageFile = KURL();
      messageFile.setPath( filename );
    }
  }

  if ( !calledWithSession ) {
    // only read additional command line arguments if kmail/kontact is
    // not called with "-session foo"
    for(int i= 0; i < args->count(); i++)
    {
      if (strncasecmp(args->arg(i),"mailto:",7)==0)
        to += args->url(i).path() + ", ";
      else {
        QString tmpArg = QString::fromLocal8Bit( args->arg(i) );
        KUrl url( tmpArg );
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
  }

  if ( !calledWithSession )
    args->clear();

  if ( !noArgsOpensReader && !mailto && !checkMail && !viewOnly )
    return false;

  if ( viewOnly )
    viewMessage( messageFile );
  else
    action( mailto, checkMail, to, cc, bcc, subj, body, messageFile,
            attachURLs );
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
  kDebug(5006) << "KMKernel::checkMail called" << endl;

  KMAccount* acct = kmkernel->acctMgr()->findByName(account);
  if (acct)
    kmkernel->acctMgr()->singleCheckMail(acct, false);
}

void KMKernel::openReader( bool onlyCheck )
{
  KMMainWin *mWin = 0;
  KMainWindow *ktmw = 0;
  kDebug(5006) << "KMKernel::openReader called" << endl;

  for ( QList<KMainWindow*>::const_iterator it = KMainWindow::memberList().begin();
       it != KMainWindow::memberList().end(); ++it ) {
    if ( (*it)->isA("KMMainWin") ) {
      ktmw = (*it);
      break;
    }
  }

  bool activate;
  if (ktmw) {
    mWin = (KMMainWin *) ktmw;
    activate = !onlyCheck; // existing window: only activate if not --check
    if ( activate )
       mWin->show();
  }
  else {
    mWin = new KMMainWin;
    mWin->show();
    activate = false; // new window: no explicit activation (#73591)
  }

  if ( activate ) {
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
                            const KUrl &messageFile,
                            const KUrl::List &attachURLs)
{
  kDebug(5006) << "KMKernel::openComposer called" << endl;
  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  // tentatively decode to, cc and bcc because invokeMailer calls us with
  // RFC 2047 encoded addresses in order to protect non-ASCII email addresses
  if (!to.isEmpty())
    msg->setTo( KMMsgBase::decodeRFC2047String( to.toLatin1() ) );
  if (!cc.isEmpty())
    msg->setCc( KMMsgBase::decodeRFC2047String( cc.toLatin1() ) );
  if (!bcc.isEmpty())
    msg->setBcc( KMMsgBase::decodeRFC2047String( bcc.toLatin1() ) );
  if (!subject.isEmpty()) msg->setSubject(subject);
  if (!messageFile.isEmpty() && messageFile.isLocalFile()) {
    QByteArray str = KPIM::kFileToByteArray( messageFile.path(), true, false );
    if( !str.isEmpty() )
      msg->setBody( QString::fromLocal8Bit( str.data(), str.size() ).utf8() );
  }
  else if (!body.isEmpty())
    msg->setBody(body.utf8());

  KMail::Composer * cWin = KMail::makeComposer( msg );
  cWin->setCharset("", TRUE);
  for ( KUrl::List::ConstIterator it = attachURLs.begin() ; it != attachURLs.end() ; ++it )
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
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp )
{
  kDebug(5006) << "KMKernel::openComposer called (deprecated version)" << endl;

  return openComposer ( to, cc, bcc, subject, body, hidden,
                        attachName, attachCte, attachData,
                        attachType, attachSubType, attachParamAttr,
                        attachParamValue, attachContDisp, QByteArray() );
}

int KMKernel::openComposer (const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, int hidden,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset )
{
  kDebug(5006) << "KMKernel::openComposer()" << endl;

  KMMessage *msg = new KMMessage;
  KMMessagePart *msgPart = 0;
  msg->initHeader();
  msg->setCharset( "utf-8" );
  if ( !cc.isEmpty() ) msg->setCc(cc);
  if ( !bcc.isEmpty() ) msg->setBcc(bcc);
  if ( !subject.isEmpty() ) msg->setSubject(subject);
  if ( !to.isEmpty() ) msg->setTo(to);
  if ( !body.isEmpty() ) msg->setBody(body.utf8());

  bool iCalAutoSend = false;
  bool noWordWrap = false;
  bool isICalInvitation = false;
  KConfigGroup options( config(), "Groupware" );
  if ( !attachData.isEmpty() ) {
    isICalInvitation = attachName == "cal.ics" &&
      attachType == "text" &&
      attachSubType == "calendar" &&
      attachParamAttr == "method";
    // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
    if ( isICalInvitation && bcc.isEmpty() )
      msg->setBcc( "" );
    if ( isICalInvitation &&
        GlobalSettings::self()->legacyBodyInvites() ) {
      // KOrganizer invitation caught and to be sent as body instead
      msg->setBody( attachData );
      msg->setHeaderField( "Content-Type",
                           QString( "text/calendar; method=%1; "
                                    "charset=\"utf-8\"" ).
                           arg( attachParamValue ) );

      iCalAutoSend = true; // no point in editing raw ICAL
      noWordWrap = true; // we shant word wrap inline invitations
    } else {
      // Just do what we're told to do
      msgPart = new KMMessagePart;
      msgPart->setName( attachName );
      msgPart->setCteStr( attachCte );
      msgPart->setBodyEncoded( attachData );
      msgPart->setTypeStr( attachType );
      msgPart->setSubtypeStr( attachSubType );
      msgPart->setParameter( attachParamAttr, attachParamValue );
      msgPart->setContentDisposition( attachContDisp );
      if( !attachCharset.isEmpty() ) {
        // kDebug(5006) << "KMKernel::openComposer set attachCharset to "
        // << attachCharset << endl;
        msgPart->setCharset( attachCharset );
      }
      // Don't show the composer window, if the automatic sending is checked
      KConfigGroup options(  config(), "Groupware" );
      iCalAutoSend = options.readEntry( "AutomaticSending", true );
    }
  }

  KMail::Composer * cWin = KMail::makeComposer();
  cWin->setMsg( msg, !isICalInvitation /* mayAutoSign */ );
  cWin->setSigningAndEncryptionDisabled( isICalInvitation
      && GlobalSettings::self()->legacyBodyInvites() );
  cWin->setAutoDelete( true );
  if( noWordWrap )
    cWin->slotWordWrapToggled( false );
  else
    cWin->setCharset( "", true );
  if ( msgPart )
    cWin->addAttach(msgPart);

  if ( hidden == 0 && !iCalAutoSend ) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  } else {
    cWin->setAutoDeleteWindow( true );
    cWin->slotSendNow();
  }

  return 1;
}

void KMKernel::setDefaultTransport( const QString & transport )
{
  QStringList availTransports = KMail::TransportManager::transportNames();
  QStringList::const_iterator it = availTransports.find( transport );
  if ( it == availTransports.end() ) {
    kWarning() << "The transport you entered is not available" << endl;
    return;
  }
  GlobalSettings::self()->setDefaultTransport( transport );
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

  KMail::Composer * cWin = KMail::makeComposer( msg );
  cWin->setCharset("", TRUE);
  if (!hidden) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  }

  return DCOPRef( cWin->asMailComposerIFace() );
}

DCOPRef KMKernel::newMessage(const QString &to,
                             const QString &cc,
                             const QString &bcc,
                             bool hidden,
                             bool useFolderId,
                             const KUrl & /*messageFile*/,
                             const KUrl &attachURL)
{
  KMail::Composer * win = 0;
  KMMessage *msg = new KMMessage;

  if ( useFolderId ) {
    //create message with required folder identity
    KMFolder *folder = currentFolder();
    uint id = folder ? folder->identity() : 0;
    msg->initHeader( id );
    win = makeComposer( msg, id );
  } else {
    msg->initHeader();
    win = makeComposer( msg );
  }
  msg->setCharset("utf-8");
  //set basic headers
  if (!to.isEmpty()) msg->setTo(to);
  if (!cc.isEmpty()) msg->setCc(cc);
  if (!bcc.isEmpty()) msg->setBcc(bcc);

  //Add the attachment if we have one
  if(!attachURL.isEmpty() && attachURL.isValid()) {
    win->addAttach(attachURL);
  }
  //only show window when required
  if(!hidden) {
    win->show();
  }
  return DCOPRef( win->asMailComposerIFace() );
}

int KMKernel::viewMessage( const KUrl & messageFile )
{
  KMOpenMsgCommand *openCommand = new KMOpenMsgCommand( 0, messageFile );

  openCommand->start();

  return 1;
}

int KMKernel::sendCertificate( const QString& to, const QByteArray& certData )
{
  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setCharset("utf-8");
  msg->setSubject( i18n( "Certificate Signature Request" ) );
  if (!to.isEmpty()) msg->setTo(to);
  // ### Make this message customizable via KIOSK
  msg->setBody( i18n( "Please create a certificate from attachment and return to sender." ).utf8() );

  KMail::Composer * cWin = KMail::makeComposer( msg );
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

int KMKernel::dcopAddMessage( const QString & foldername, const QString & msgUrlString,
                              const QString & MsgStatusFlags)
{
  return dcopAddMessage(foldername, KURL(msgUrlString), MsgStatusFlags);
}

int KMKernel::dcopAddMessage( const QString & foldername,const KUrl & msgUrl,
                              const QString & MsgStatusFlags)
{
  kDebug(5006) << "KMKernel::dcopAddMessage called" << endl;

  if ( foldername.isEmpty() || foldername.startsWith("."))
    return -1;

  int retval;
  bool readFolderMsgIds = false;
  QString _foldername = foldername.trimmed();
  _foldername = _foldername.replace('\\',""); //try to prevent ESCAPE Sequences

  if ( foldername != mAddMessageLastFolder ) {
    mAddMessageMsgIds.clear();
    readFolderMsgIds = true;
    mAddMessageLastFolder = foldername;
  }

  if (!msgUrl.isEmpty() && msgUrl.isLocalFile()) {

    const QByteArray messageText =
      KPIM::kFileToByteArray( msgUrl.path(), true, false );
    if ( messageText.isEmpty() )
      return -2;

    KMMessage *msg = new KMMessage();
    msg->fromString( messageText );

    if (readFolderMsgIds) {
      if ( foldername.contains("/")) {
        QString tmp_fname = "";
        KMFolder *folder = NULL;
        KMFolderDir *subfolder;
        bool root = true;

        QStringList subFList = QStringList::split("/",_foldername,FALSE);

        for ( QStringList::Iterator it = subFList.begin(); it != subFList.end(); ++it ) {
          QString _newFolder = *it;
          if(_newFolder.startsWith(".")) return -1;

          if(root) {
            folder = the_folderMgr->findOrCreate(*it, false);
            if (folder) {
              root = false;
              tmp_fname = "/" + *it;
            }
            else return -1;
          }
          else {
            subfolder = folder->createChildFolder();
            tmp_fname += "/" + *it;
            if(!the_folderMgr->getFolderByURL( tmp_fname )) {
             folder = the_folderMgr->createFolder(*it, FALSE, folder->folderType(), subfolder);
            }

            if(!(folder = the_folderMgr->getFolderByURL( tmp_fname ))) return -1;
          }
        }

        mAddMsgCurrentFolder = the_folderMgr->getFolderByURL( tmp_fname );
        if(!folder) return -1;

      }
      else {
        mAddMsgCurrentFolder = the_folderMgr->findOrCreate(_foldername, false);
      }
    }

    if ( mAddMsgCurrentFolder ) {
      if (readFolderMsgIds) {

      	// OLD COMMENT:
        // Try to determine if a message already exists in
        // the folder. The message id that is searched for, is
        // the subject line + the date. This should be quite
        // unique. The change that a given date with a given
        // subject is in the folder twice is very small.
        // If the subject is empty, the fromStrip string
        // is taken.

	// NEW COMMENT from Danny Kukawka (danny.kukawka@web.de):
	// subject line + the date is only unique if the following
	// return a correct unique value:
	// 	time_t  DT = mb->date();
        // 	QString dt = ctime(&DT);
	// But if the datestring in the Header isn't RFC conform
	// subject line + the date isn't unique.
	//
	// The only uique headerfield is the Message-ID. In some
	// cases this could be empty. I then I use the
	// subject line + dateStr .

        int i;

        mAddMsgCurrentFolder->open();
        for( i=0; i<mAddMsgCurrentFolder->count(); i++) {
          KMMsgBase *mb = mAddMsgCurrentFolder->getMsgBase(i);
	  QString id = mb->msgIdMD5();
	  if ( id.isEmpty() ) {
            id = mb->subject();
            if ( id.isEmpty() )
              id = mb->fromStrip();
            if ( id.isEmpty() )
              id = mb->toStrip();

            id += mb->dateStr();
	  }

          //fprintf(stderr,"%s\n",(const char *) id);
          if ( !id.isEmpty() ) {
            mAddMessageMsgIds.append(id);
          }
        }
        mAddMsgCurrentFolder->close();
      }

      QString msgId = msg->msgIdMD5();
      if ( msgId.isEmpty()) {
	msgId = msg->subject();
	if ( msgId.isEmpty() )
          msgId = msg->fromStrip();
        if ( msgId.isEmpty() )
          msgId = msg->toStrip();

	msgId += msg->dateStr();
      }

      int k = mAddMessageMsgIds.findIndex( msgId );
      //fprintf(stderr,"find %s = %d\n",(const char *) msgId,k);

      if ( k == -1 ) {
        if ( !msgId.isEmpty() ) {
          mAddMessageMsgIds.append( msgId );
        }

        if ( !MsgStatusFlags.isEmpty() ) {
          msg->status().setStatusFromStr(MsgStatusFlags);
        }

        int index;
        if ( mAddMsgCurrentFolder->addMsg( msg, &index ) == 0 ) {
          mAddMsgCurrentFolder->unGetMsg( index );
          retval = 1;
        } else {
          retval =- 2;
          delete msg;
          msg = 0;
        }
      } else {
        //qDebug( "duplicate: " + msgId + "; subj: " + msg->subject() + ", from: " + msgId = msg->fromStrip());
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

void KMKernel::dcopResetAddMessage()
{
  mAddMessageMsgIds.clear();
  mAddMessageLastFolder.clear();
}

int KMKernel::dcopAddMessage_fastImport( const QString & foldername,
                                         const QString & msgUrlString,
                                         const QString & MsgStatusFlags)
{
  return dcopAddMessage_fastImport(foldername, KURL(msgUrlString), MsgStatusFlags);
}

int KMKernel::dcopAddMessage_fastImport( const QString & foldername,
                                         const KUrl & msgUrl,
                                         const QString & MsgStatusFlags)
{
  // Use this function to import messages without
  // search for already existing emails.
  kDebug(5006) << "KMKernel::dcopAddMessage_fastImport called" << endl;

  if ( foldername.isEmpty() || foldername.startsWith("."))
    return -1;

  int retval;
  bool createNewFolder = false;

  QString _foldername = foldername.trimmed();
  _foldername = _foldername.replace('\\',""); //try to prevent ESCAPE Sequences

  if ( foldername != mAddMessageLastFolder ) {
    createNewFolder = true;
    mAddMessageLastFolder = foldername;
  }


  if ( !msgUrl.isEmpty() && msgUrl.isLocalFile() ) {
    const QByteArray messageText =
      KPIM::kFileToByteArray( msgUrl.path(), true, false );
    if ( messageText.isEmpty() )
      return -2;

    KMMessage *msg = new KMMessage();
    msg->fromString( messageText );

    if (createNewFolder) {
      if ( foldername.contains("/")) {
        QString tmp_fname = "";
        KMFolder *folder = NULL;
        KMFolderDir *subfolder;
        bool root = true;

        QStringList subFList = QStringList::split("/",_foldername,FALSE);

        for ( QStringList::Iterator it = subFList.begin(); it != subFList.end(); ++it ) {
          QString _newFolder = *it;
          if(_newFolder.startsWith(".")) return -1;

          if(root) {
            folder = the_folderMgr->findOrCreate(*it, false);
            if (folder) {
              root = false;
              tmp_fname = "/" + *it;
            }
            else return -1;
          }
          else {
            subfolder = folder->createChildFolder();
            tmp_fname += "/" + *it;
            if(!the_folderMgr->getFolderByURL( tmp_fname )) {
              folder = the_folderMgr->createFolder(*it, FALSE, folder->folderType(), subfolder);
            }
            if(!(folder = the_folderMgr->getFolderByURL( tmp_fname ))) return -1;
          }
        }

      mAddMsgCurrentFolder = the_folderMgr->getFolderByURL( tmp_fname );
      if(!folder) return -1;

      }
      else {
        mAddMsgCurrentFolder = the_folderMgr->findOrCreate(_foldername, false);
      }
    }

    if ( mAddMsgCurrentFolder ) {
      int index;

      if( !MsgStatusFlags.isEmpty() ) {
        msg->status().setStatusFromStr(MsgStatusFlags);
      }

      if ( mAddMsgCurrentFolder->addMsg( msg, &index ) == 0 ) {
        mAddMsgCurrentFolder->unGetMsg( index );
        retval = 1;
      } else {
        retval =- 2;
        delete msg;
        msg = 0;
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
  const QString localPrefix = "/Local";
  folders << localPrefix;
  the_folderMgr->getFolderURLS( folders, localPrefix );
  the_imapFolderMgr->getFolderURLS( folders );
  the_dimapFolderMgr->getFolderURLS( folders );
  return folders;
}

DCOPRef KMKernel::getFolder( const QString& vpath )
{
  const QString localPrefix = "/Local";
  if ( the_folderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  else if ( vpath.startsWith( localPrefix ) &&
            the_folderMgr->getFolderByURL( vpath.mid( localPrefix.length() ) ) )
    return DCOPRef( new FolderIface( vpath.mid( localPrefix.length() ) ) );
  else if ( the_imapFolderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  else if ( the_dimapFolderMgr->getFolderByURL( vpath ) )
    return DCOPRef( new FolderIface( vpath ) );
  return DCOPRef();
}

void KMKernel::raise()
{
  DCOPRef kmail( "kmail", "kmail" );
  kmail.call( "newInstance" );
}

bool KMKernel::showMail( quint32 serialNumber, QString /* messageId */ )
{
  KMMainWidget *mainWidget = 0;
  KMainWindow *win = 0;
  QObjectList l;

  // First look for a KMainWindow.
  for ( QList<KMainWindow*>::const_iterator it = KMainWindow::memberList().begin();
       it != KMainWindow::memberList().end(); ++it ) {
    // Then look for a KMMainWidget.
    l	= (*it)->queryList("KMMainWidget");
    if (!l.isEmpty() && l.first()) {
      mainWidget = dynamic_cast<KMMainWidget *>(l.first());
      if ( (*it)->isActiveWindow() )
        break;
    }
  }

  if (mainWidget) {
    int idx = -1;
    KMFolder *folder = 0;
    KMMsgDict::instance()->getLocation(serialNumber, &folder, &idx);
    if (!folder || (idx == -1))
      return false;
    folder->open();
    KMMsgBase *msgBase = folder->getMsgBase(idx);
    if (!msgBase)
      return false;
    bool unGet = !msgBase->isMessage();
    KMMessage *msg = folder->getMsg(idx);

    KMReaderMainWin *win = new KMReaderMainWin( false, false );
    KMMessage *newMessage = new KMMessage( *msg );
    newMessage->setParent( msg->parent() );
    newMessage->setMsgSerNum( msg->getMsgSerNum() );
    newMessage->setReadyToShow( true );
    win->showMsg( GlobalSettings::self()->overrideCharacterEncoding(), newMessage );
    win->show();

    if (unGet)
      folder->unGetMsg(idx);
    folder->close();
    return true;
  }

  return false;
}

QString KMKernel::getFrom( quint32 serialNumber )
{
  int idx = -1;
  KMFolder *folder = 0;
  KMMsgDict::instance()->getLocation(serialNumber, &folder, &idx);
  if (!folder || (idx == -1))
    return QString();
  folder->open();
  KMMsgBase *msgBase = folder->getMsgBase(idx);
  if (!msgBase)
    return QString();
  bool unGet = !msgBase->isMessage();
  KMMessage *msg = folder->getMsg(idx);
  QString result = msg->from();
  if (unGet)
    folder->unGetMsg(idx);
  folder->close();
  return result;
}

QString KMKernel::debugScheduler()
{
  QString res = KMail::ActionScheduler::debug();
  return res;
}

QString KMKernel::debugSernum( quint32 serialNumber )
{
  QString res;
  if (serialNumber != 0) {
    int idx = -1;
    KMFolder *folder = 0;
    KMMsgBase *msg = 0;
    KMMsgDict::instance()->getLocation( serialNumber, &folder, &idx );
    // It's possible that the message has been deleted or moved into a
    // different folder
    if (folder && (idx != -1)) {
      // everything is ok
      folder->open();
      msg = folder->getMsgBase( idx );
      if (msg) {
	res.append( QString( " subject %s,\n sender %s,\n date %s.\n" )
		    .arg( msg->subject() )
		    .arg( msg->fromStrip() )
		    .arg( msg->dateStr() ) );
      } else {
	res.append( QString( "Invalid serial number." ) );
      }
      folder->close();
    } else {
      res.append( QString( "Invalid serial number." ) );
    }
  }
  return res;
}


void KMKernel::pauseBackgroundJobs()
{
  mBackgroundTasksTimer->stop();
  mJobScheduler->pause();
}

void KMKernel::resumeBackgroundJobs()
{
  mJobScheduler->resume();
  mBackgroundTasksTimer->start( 4 * 60 * 60 * 1000, true );
}

void KMKernel::stopNetworkJobs()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return;

  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Offline );

}

void KMKernel::resumeNetworkJobs()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online )
    return;

  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Online );

  if ( kmkernel->msgSender()->sendImmediate() ) {
    kmkernel->msgSender()->sendQueued();
  }
}

bool KMKernel::isOffline()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return true;
  else
    return false;
}

bool KMKernel::askToGoOnline()
{
  if ( kmkernel->isOffline() ) {
    int rc =
    KMessageBox::questionYesNo( KMKernel::self()->mainWin(),
                                i18n("KMail is currently in offline mode. "
                                     "How do you want to proceed?"),
                                i18n("Online/Offline"),
                                i18n("Work Online"),
                                i18n("Work Offline"));

    if( rc == KMessageBox::No ) {
      return false;
    } else {
      kmkernel->resumeNetworkJobs();
    }
  }
  return true;
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

   o If we are getting mail, stop it (but dont lose something!)
         [Done already, see mailCheckAborted]
   o If we are sending mail, go on UNLESS this was called by SM,
       in which case stop ASAP that too (can we warn? should we continue
       on next start?)
   o If we are compacting, or expunging, go on UNLESS this was SM call.
       In that case stop compacting ASAP and continue on next start, before
       touching any folders. [Not needed anymore with CompactionJob]

   KMKernel::quit ()
   {
     SM call?
       if compacting, stop;
       if sending, stop;
       if receiving, stop;
       Windows will take care of themselves (composer should dump
        its messages, if any but not in deadMail)
       declare us ready for the End of the Session

     No, normal quit call
       All windows are off. Anything to do, should compact or sender sends?
         Yes, maybe put an icon in panel as a sign of life
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
*/


/********************************************************************/
/*            Init, Exit, and handler  methods                      */
/********************************************************************/
void KMKernel::testDir(const char *_name)
{
  QString foldersPath = QDir::homePath() + QString( _name );
  QFileInfo info( foldersPath );
  if ( !info.exists() ) {
    if ( ::mkdir( QFile::encodeName( foldersPath ) , S_IRWXU ) == -1 ) {
      KMessageBox::sorry(0, i18n("KMail could not create folder '%1';\n"
                                 "please make sure that you can view and "
                                 "modify the content of the folder '%2'.")
                            .arg( foldersPath ).arg( QDir::homePath() ) );
      ::exit(-1);
    }
  }
  if ( !info.isDir() || !info.isReadable() || !info.isWritable() ) {
    KMessageBox::sorry(0, i18n("The permissions of the folder '%1' are "
                               "incorrect;\n"
                               "please make sure that you can view and modify "
                               "the content of this folder.")
                          .arg( foldersPath ) );
    ::exit(-1);
  }
}


//-----------------------------------------------------------------------------
// Open a composer for each message found in the dead.letter folder
void KMKernel::recoverDeadLetters()
{
  const QString pathName = localDataPath();
  QDir dir( pathName );
  if ( !dir.exists( "autosave" ) )
    return;

  KMFolder folder( 0, pathName + "autosave", KMFolderTypeMaildir, false /* no index */ );
  const int rc = folder.open();
  if ( rc ) {
    perror( "cannot open autosave folder" );
    return;
  }

  const int num = folder.count();
  for ( int i = 0; i < num; i++ ) {
    KMMessage *msg = folder.take( 0 );
    if ( msg ) {
      KMail::Composer * win = KMail::makeComposer();
      win->setMsg( msg, false, false, true );
      win->setAutoSaveFilename( msg->fileName() );
      win->show();
    }
  }
  folder.close();
}

//-----------------------------------------------------------------------------
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
    the_inboxFolder->setUserWhoField( QString() );
  // inboxFolder->open();

  the_outboxFolder = the_folderMgr->findOrCreate(cfg->readEntry("outboxFolder", I18N_NOOP("outbox")));
  if (the_outboxFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your outbox folder.") );
  }
  the_outboxFolder->setNoChildren(true);

  the_outboxFolder->setSystemFolder(TRUE);
  if ( the_outboxFolder->userWhoField().isEmpty() )
    the_outboxFolder->setUserWhoField( QString() );
  /* Nuke the oubox's index file, to make sure that no ghost messages are in
   * it from a previous crash. Ghost messages happen in the outbox because it
   * the only folder where messages enter and leave within 5 seconds, which is
   * the leniency period for index invalidation. Since the number of mails in
   * this folder is expected to be very small, we can live with regenerating
   * the index on each start to be on the save side. */
  //if ( the_outboxFolder->folderType() == KMFolderTypeMaildir )
  //  unlink( QFile::encodeName( the_outboxFolder->indexLocation() ) );
  the_outboxFolder->open();

  the_sentFolder = the_folderMgr->findOrCreate(cfg->readEntry("sentFolder", I18N_NOOP("sent-mail")));
  if (the_sentFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your sent-mail folder.") );
  }
  the_sentFolder->setSystemFolder(TRUE);
  if ( the_sentFolder->userWhoField().isEmpty() )
    the_sentFolder->setUserWhoField( QString() );
  // the_sentFolder->open();

  the_trashFolder  = the_folderMgr->findOrCreate(cfg->readEntry("trashFolder", I18N_NOOP("trash")));
  if (the_trashFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your trash folder.") );
  }
  the_trashFolder->setSystemFolder(TRUE);
  if ( the_trashFolder->userWhoField().isEmpty() )
    the_trashFolder->setUserWhoField( QString() );
  // the_trashFolder->open();

  the_draftsFolder = the_folderMgr->findOrCreate(cfg->readEntry("draftsFolder", I18N_NOOP("drafts")));
  if (the_draftsFolder->canAccess() != 0) {
    emergencyExit( i18n("You do not have read/write permission to your drafts folder.") );
  }
  the_draftsFolder->setSystemFolder(TRUE);
  if ( the_draftsFolder->userWhoField().isEmpty() )
    the_draftsFolder->setUserWhoField( QString() );
  the_draftsFolder->open();
}


void KMKernel::init()
{
  the_shuttingDown = false;
  the_server_is_ready = false;

  KConfig* cfg = KMKernel::config();

  QDir dir;

  KConfigGroup group(cfg, "General");
  the_firstStart = group.readEntry("first-start", true );
  group.writeEntry("first-start", false);
  the_previousVersion = group.readEntry("previous-version");
  group.writeEntry("previous-version", KMAIL_VERSION);
  QString foldersPath = group.readPathEntry( "folders" );
  kDebug(5006) << k_funcinfo << "foldersPath (from config): '" << foldersPath << "'" << endl;

  if ( foldersPath.isEmpty() ) {
    foldersPath = localDataPath() + "mail";
    if ( transferMail( foldersPath ) ) {
      group.writePathEntry( "folders", foldersPath );
    }
    kDebug(5006) << k_funcinfo << "foldersPath (after transferMail): '" << foldersPath << "'" << endl;
  }

  the_undoStack     = new UndoStack(20);
  the_folderMgr     = new KMFolderMgr(foldersPath);
  the_imapFolderMgr = new KMFolderMgr( KMFolderImap::cacheLocation(), KMImapDir);
  the_dimapFolderMgr = new KMFolderMgr( KMFolderCachedImap::cacheLocation(), KMDImapDir);

  the_searchFolderMgr = new KMFolderMgr(locateLocal("data","kmail/search"), KMSearchDir);
  KMFolder *lsf = the_searchFolderMgr->find( i18n("Last Search") );
  if (lsf)
    the_searchFolderMgr->remove( lsf );

  the_acctMgr       = new AccountManager();
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
  imProxy()->initialize();
  { // area for config group "Composer"
    KConfigGroup group(cfg, "Composer");
    if (group.readEntry("pref-charsets", QStringList() ).isEmpty())
    {
      group.writeEntry("pref-charsets", "us-ascii,iso-8859-1,locale,utf-8");
    }
  }
  readConfig();
  mICalIface->readConfig();
  // filterMgr->dump();
#ifdef HAVE_INDEXLIB
  the_msgIndex = new KMMsgIndex(this); //create the indexer
#else
  the_msgIndex = 0;
#endif

#if 0
  the_weaver =  new KPIM::ThreadWeaver::Weaver( this );
  the_weaverLogger = new KPIM::ThreadWeaver::WeaverThreadLogger(this);
  the_weaverLogger->attach (the_weaver);
#else
  // make it obvious that this instance has not been initialized
  // the_weaver = 0;
#endif

  connect( the_folderMgr, SIGNAL( folderRemoved(KMFolder*) ),
           this, SIGNAL( folderRemoved(KMFolder*) ) );
  connect( the_dimapFolderMgr, SIGNAL( folderRemoved(KMFolder*) ),
           this, SIGNAL( folderRemoved(KMFolder*) ) );
  connect( the_imapFolderMgr, SIGNAL( folderRemoved(KMFolder*) ),
           this, SIGNAL( folderRemoved(KMFolder*) ) );
  connect( the_searchFolderMgr, SIGNAL( folderRemoved(KMFolder*) ),
           this, SIGNAL( folderRemoved(KMFolder*) ) );

  mBackgroundTasksTimer = new QTimer( this );
  connect( mBackgroundTasksTimer, SIGNAL( timeout() ), this, SLOT( slotRunBackgroundTasks() ) );
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
  mBackgroundTasksTimer->start( 10000, true ); // 10s minute, singleshot
#else
  mBackgroundTasksTimer->start( 5 * 60000, true ); // 5 minutes, singleshot
#endif
}

void KMKernel::readConfig()
{
}

void KMKernel::cleanupImapFolders()
{
  KMAccount *acct = 0;
  KMFolderNode *node;
  QList<KMFolderNode*>::iterator it = the_imapFolderMgr->dir().begin();
  while ( ( node = *it ) && it != the_imapFolderMgr->dir().end() )
  {
    if (node->isDir() || ((acct = the_acctMgr->find(node->id()))
			  && ( acct->type() == "imap" )) )
    {
      ++it;
    } else {
      KMFolder* folder = static_cast<KMFolder*>(node);
      // delete only local
      static_cast<KMFolderImap*>( folder->storage() )->setAlreadyRemoved( true );
      the_imapFolderMgr->remove(folder);
      it = the_imapFolderMgr->dir().begin();
    }
  }

  it = the_dimapFolderMgr->dir().begin();
  while ( ( node = *it ) && it != the_dimapFolderMgr->dir().end() )
  {
    if (node->isDir() || ((acct = the_acctMgr->find(node->id()))
			  && ( acct->type() == "cachedimap" )) )
    {
      ++it;
    } else {
      the_dimapFolderMgr->remove(static_cast<KMFolder*>(node));
      it = the_dimapFolderMgr->dir().begin();
    }
  }

  the_imapFolderMgr->quiet(true);
  for (acct = the_acctMgr->first(); acct; acct = the_acctMgr->next())
  {
    KMFolderImap *fld;
    KMAcctImap *imapAcct;

    if (acct->type() != "imap") continue;
    fld = static_cast<KMFolderImap*>(the_imapFolderMgr
      ->findOrCreate(QString::number(acct->id()), false, acct->id())->storage());
    fld->setNoContent(true);
    fld->folder()->setLabel(acct->name());
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

    KMFolder* fld = the_dimapFolderMgr->find(QString::number(acct->id()));
    if( fld )
      cfld = static_cast<KMFolderCachedImap*>( fld->storage() );
    if (cfld == 0) {
      // Folder doesn't exist yet
      cfld = static_cast<KMFolderCachedImap*>(the_dimapFolderMgr->createFolder(QString::number(acct->id()),
            false, KMFolderTypeCachedImap)->storage());
      if (!cfld) {
        KMessageBox::error(0,(i18n("Cannot create file `%1' in %2.\nKMail cannot start without it.").arg(acct->name()).arg(the_dimapFolderMgr->basePath())));
        exit(-1);
      }
      cfld->folder()->setId( acct->id() );
    }

    cfld->setNoContent(true);
    cfld->folder()->setLabel(acct->name());
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
  if (kapp->isSessionRestored()){
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
  QListIterator<KMainWindow*> it( KMainWindow::memberList() );
  KMainWindow *window = 0;
  while ( it.hasNext() ) {
    window = it.next();
    if ( window && ( window->isA("KMMainWindow") ||
                     window->inherits("KMail::SecondaryWindow") ) )
      window->close( true ); // close and delete the window
  }
}

void KMKernel::cleanup(void)
{
  dumpDeadLetters();
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

#if 0
  delete the_weaver;
  the_weaver = 0;
#endif

  KConfig* config =  KMKernel::config();
  KConfigGroup group(config, "General");

  if (the_trashFolder) {

    the_trashFolder->close(TRUE);

    if (group.readEntry("empty-trash-on-exit", true ) )
    {
      if ( the_trashFolder->count( true ) > 0 )
        the_trashFolder->expunge();
    }
  }

  mICalIface->cleanup();

  QList<QPointer<KMFolder> > folders;
  QStringList strList;
  KMFolder *folder;
  the_folderMgr->createFolderList(&strList, &folders);

  QList<QPointer<KMFolder> >::const_iterator it;
  for ( it = folders.begin(); it != folders.end(); it++ )
  {
    folder = *it;
    if (!folder || folder->isDir()) continue;
    folder->close(TRUE);
  }
  strList.clear();
  folders.clear();
  the_searchFolderMgr->createFolderList(&strList, &folders);
  for ( it = folders.begin(); it != folders.end(); it++ )
  {
    folder = *it;
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
  delete mConfigureDialog;
  mConfigureDialog = 0;
  delete mWin;
  mWin = 0;

  if ( RecentAddresses::exists() )
    RecentAddresses::self( config )->save( config );
  config->sync();
}

bool KMKernel::transferMail( QString & destinationDir )
{
  QString dir;

  // check whether the user has a ~/KMail folder
  QFileInfo fi( QDir::home(), "KMail" );
  if ( fi.exists() && fi.isDir() ) {
    dir = QDir::homePath() + "/KMail";
    // the following two lines can be removed once moving mail is reactivated
    destinationDir = dir;
    return true;
  }

  if ( dir.isEmpty() ) {
    // check whether the user has a ~/Mail folder
    fi.setFile( QDir::home(), "Mail" );
    if ( fi.exists() && fi.isDir() &&
         QFile::exists( QDir::homePath() + "/Mail/.inbox.index" ) ) {
      // there's a ~/Mail folder which seems to be used by KMail (because of the
      // index file)
      dir = QDir::homePath() + "/Mail";
      // the following two lines can be removed once moving mail is reactivated
      destinationDir = dir;
      return true;
    }
  }

  if ( dir.isEmpty() ) {
    return true; // there's no old mail folder
  }

#if 0
  // disabled for now since moving fails in certain cases (e.g. if symbolic links are involved)
  const QString kmailName = kapp->aboutData()->programName();
  QString msg;
  if ( KIO::NetAccess::exists( destinationDir, true, 0 ) ) {
    // if destinationDir exists, we need to warn about possible
    // overwriting of files. otherwise, we don't have to
    msg = i18n( "%1-%3 is the application name, %4-%7 are folder path",
                "<qt>The <i>%4</i> folder exists. "
                "%1 now uses the <i>%5</i> folder for "
                "its messages.<p>"
                "%2 can move the contents of <i>%6<i> into this folder for "
                "you, though this may replace any existing files with "
                "the same name in <i>%7</i>.<p>"
                "<strong>Would you like %3 to move the mail "
                "files now?</strong></qt>" )
          .arg( kmailName, kmailName, kmailName )
          .arg( dir, destinationDir, dir, destinationDir );
  }
  else {
    msg = i18n( "%1-%3 is the application name, %4-%6 are folder path",
                "<qt>The <i>%4</i> folder exists. "
                "%1 now uses the <i>%5</i> folder for "
                "its messages. %2 can move the contents of <i>%6</i> into "
                "this folder for you.<p>"
                "<strong>Would you like %3 to move the mail "
                "files now?</strong></qt>" )
          .arg( kmailName, kmailName, kmailName )
          .arg( dir, destinationDir, dir );
  }
  QString title = i18n( "Migrate Mail Files?" );
  QString buttonText = i18n( "Move" );

  if ( KMessageBox::questionYesNo( 0, msg, title, buttonText, i18n("Do Not Move") ) ==
       KMessageBox::No ) {
    destinationDir = dir;
    return true;
  }

  if ( !KIO::NetAccess::move( dir, destinationDir ) ) {
    kDebug(5006) << k_funcinfo << "Moving " << dir << " to " << destinationDir << " failed: " << KIO::NetAccess::lastErrorString() << endl;
    kDebug(5006) << k_funcinfo << "Deleting " << destinationDir << endl;
    KIO::NetAccess::del( destinationDir, 0 );
    destinationDir = dir;
    return false;
  }
#endif

  return true;
}


void KMKernel::ungrabPtrKb(void)
{
  QWidget* widg = KMainWindow::memberList().first();
  Display* dpy;

  if (!widg) return;
  dpy = widg->x11Display();
  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
}


void KMKernel::dumpDeadLetters()
{
  if ( shuttingDown() )
    return; //All documents should be saved before shutting down is set!

  // make all composer windows autosave their contents
  QListIterator<KMainWindow*> it( KMainWindow::memberList() );
  while ( it.hasNext() )
    if ( KMail::Composer * win = ::qobject_cast<KMail::Composer*>( it.next() ) )
      win->autoSaveMessage();
}



void KMKernel::action(bool mailto, bool check, const QString &to,
                      const QString &cc, const QString &bcc,
                      const QString &subj, const QString &body,
                      const KUrl &messageFile,
                      const KUrl::List &attachURLs)
{
  if (mailto)
    openComposer (to, cc, bcc, subj, body, 0, messageFile, attachURLs);
  else
    openReader( check );

  if (check)
    checkMail();
  //Anything else?
}

void KMKernel::byteArrayToRemoteFile(const QByteArray &aData, const KUrl &aURL,
  bool overwrite)
{
  // ## when KDE 3.3 is out: use KIO::storedPut to remove slotDataReq altogether
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
    //kDebug( 5006 ) << "Sending " << MAX_CHUNK_SIZE << " bytes ("
    //                << remainingBytes - MAX_CHUNK_SIZE << " bytes remain)\n";
  }
  else
  {
    // send the remaining bytes to the receiver (deep copy)
    data.duplicate( (*it).data.data() + (*it).offset, remainingBytes );
    (*it).data = QByteArray();
    (*it).offset = 0;
    //kDebug( 5006 ) << "Sending " << remainingBytes << " bytes\n";
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

void KMKernel::slotRequestConfigSync() {
  // ### FIXME: delay as promised in the kdoc of this function ;-)
  KMKernel::config()->sync();
}

void KMKernel::slotShowConfigurationDialog()
{
  if( !mConfigureDialog ) {
    mConfigureDialog = new ConfigureDialog( 0, "configure", false );
    connect( mConfigureDialog, SIGNAL( configCommitted() ),
             this, SLOT( slotConfigChanged() ) );
  }

  if( mConfigureDialog->isHidden() )
    mConfigureDialog->show();
  else
    mConfigureDialog->raise();
}

void KMKernel::slotConfigChanged()
{
  readConfig();
  emit configChanged();
}

//-------------------------------------------------------------------------------
//static
QString KMKernel::localDataPath()
{
  return locateLocal( "data", "kmail/" );
}

//-------------------------------------------------------------------------------

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
  QList<const KSystemTray*>::iterator it =
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

  kWarning() << mesg << endl;
  KNotifyClient::userEvent( 0, mesg, KNotifyClient::Messagebox, KNotifyClient::Error );

  ::exit(1);
}

/**
 * Returns true if the folder is either the outbox or one of the drafts-folders
 */
bool KMKernel::folderIsDraftOrOutbox(const KMFolder * folder)
{
  assert( folder );
  if ( folder == the_outboxFolder )
    return true;
  return folderIsDrafts( folder );
}

bool KMKernel::folderIsDrafts(const KMFolder * folder)
{
  assert( folder );
  if ( folder == the_draftsFolder )
    return true;

  QString idString = folder->idString();
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the drafts-folder
  const KPIM::IdentityManager * im = identityManager();
  for( KPIM::IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).drafts() == idString ) return true;
  return false;
}

bool KMKernel::folderIsTrash(KMFolder * folder)
{
  assert(folder);
  if (folder == the_trashFolder) return true;
  QStringList actList = acctMgr()->getAccounts();
  QStringList::Iterator it( actList.begin() );
  for( ; it != actList.end() ; ++it ) {
    KMAccount* act = acctMgr()->findByName( *it );
    if ( act && ( act->trash() == folder->idString() ) )
      return true;
  }
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
  const KPIM::IdentityManager * im = identityManager();
  for( KPIM::IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).fcc() == idString ) return true;
  return false;
}

KPIM::IdentityManager * KMKernel::identityManager() {
  if ( !mIdentityManager ) {
    kDebug(5006) << "instantating KPIM::IdentityManager" << endl;
    mIdentityManager = new KPIM::IdentityManager( false, this, "mIdentityManager" );
  }
  return mIdentityManager;
}

KMMsgIndex *KMKernel::msgIndex()
{
    return the_msgIndex;
}

KMainWindow* KMKernel::mainWin()
{
  KMainWindow *kmWin = 0;

  // First look for a KMMainWin.
  for ( QList<KMainWindow*>::const_iterator it = KMainWindow::memberList().begin();
       it != KMainWindow::memberList().end(); ++it )
    if ( (*it)->isA("KMMainWin") )
      return kmWin;

  // There is no KMMainWin. Use any other KMainWindow instead (e.g. in
  // case we are running inside Kontact) because we anyway only need
  // it for modal message boxes and for KNotify events.
  kmWin = KMainWindow::memberList().first();
  if ( kmWin )
    return kmWin;

  // There's not a single KMainWindow. Create a KMMainWin.
  // This could happen if we want to pop up an error message
  // while we are still doing the startup wizard and no other
  // KMainWindow is running.
  mWin = new KMMainWin;
  return mWin;
}


/**
 * Empties all trash folders
 */
void KMKernel::slotEmptyTrash()
{
  QString title = i18n("Empty Trash");
  QString text = i18n("Are you sure you want to empty the trash folders of all accounts?");
  if (KMessageBox::warningContinueCancel(0, text, title,
                                         KStdGuiItem::cont(), "confirm_empty_trash")
      != KMessageBox::Continue)
  {
    return;
  }

  for (KMAccount* acct = acctMgr()->first(); acct; acct = acctMgr()->next())
  {
    KMFolder* trash = findFolderById(acct->trash());
    if (trash)
    {
      trash->expunge();
    }
  }
}

KConfig* KMKernel::config()
{
  assert(mySelf);
  if (!mySelf->mConfig)
  {
    mySelf->mConfig = KSharedConfig::openConfig( "kmailrc" );
    // Check that all updates have been run on the config file:
    KMail::checkConfigUpdates();
  }
  return mySelf->mConfig.data();
}

KMailICalIfaceImpl& KMKernel::iCalIface()
{
  assert( mICalIface );
  return *mICalIface;
}

void KMKernel::selectFolder( QString folderPath )
{
  kDebug(5006)<<"Selecting a folder "<<folderPath<<endl;
  const QString localPrefix = "/Local";
  KMFolder *folder = kmkernel->folderMgr()->getFolderByURL( folderPath );
  if ( !folder && folderPath.startsWith( localPrefix ) )
    folder = the_folderMgr->getFolderByURL( folderPath.mid( localPrefix.length() ) );
  if ( !folder )
    folder = kmkernel->imapFolderMgr()->getFolderByURL( folderPath );
  if ( !folder )
    folder = kmkernel->dimapFolderMgr()->getFolderByURL( folderPath );
  Q_ASSERT( folder );

  KMMainWidget *widget = getKMMainWidget();
  Q_ASSERT( widget );
  if ( !widget )
    return;

  KMFolderTree *tree = widget->folderTree();
  tree->doFolderSelected( tree->indexOfFolder( folder ) );
  tree->ensureItemVisible( tree->indexOfFolder( folder ) );
}

KMMainWidget *KMKernel::getKMMainWidget()
{
  //This could definitely use a speadup
  QWidgetList l = kapp->topLevelWidgets();
  QWidget *wid;

  Q_FOREACH( wid, l ) {
    QObjectList l2 = wid->topLevelWidget()->queryList( "KMMainWidget" );
    if (!l2.isEmpty() && l2.first()) {
      KMMainWidget* kmmw = dynamic_cast<KMMainWidget *>( l2.first() );
      Q_ASSERT( kmmw );
      return kmmw;
    }
  }
  return 0;
}

void KMKernel::slotRunBackgroundTasks() // called regularly by timer
{
  // Hidden KConfig keys. Not meant to be used, but a nice fallback in case
  // a stable kmail release goes out with a nasty bug in CompactionJob...
  KConfigGroup generalGroup( config(), "General" );

  if ( generalGroup.readEntry( "auto-expiring", true ) ) {
    the_folderMgr->expireAllFolders( false /*scheduled, not immediate*/ );
    the_imapFolderMgr->expireAllFolders( false /*scheduled, not immediate*/ );
    the_dimapFolderMgr->expireAllFolders( false /*scheduled, not immediate*/ );
    // the_searchFolderMgr: no expiry there
  }

  if ( generalGroup.readEntry( "auto-compaction", true ) ) {
    the_folderMgr->compactAllFolders( false /*scheduled, not immediate*/ );
    // the_imapFolderMgr: no compaction
    the_dimapFolderMgr->compactAllFolders( false /*scheduled, not immediate*/ );
    // the_searchFolderMgr: no compaction
  }

#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
  mBackgroundTasksTimer->start( 60 * 1000, true ); // check again in 1 minute
#else
  mBackgroundTasksTimer->start( 4 * 60 * 60 * 1000, true ); // check again in 4 hours
#endif

}

void KMKernel::expireAllFoldersNow() // called by the GUI
{
  the_folderMgr->expireAllFolders( true /*immediate*/ );
  the_imapFolderMgr->expireAllFolders( true /*immediate*/ );
  the_dimapFolderMgr->expireAllFolders( true /*immediate*/ );
}

void KMKernel::compactAllFolders() // called by the GUI
{
  the_folderMgr->compactAllFolders( true /*immediate*/ );
  //the_imapFolderMgr->compactAllFolders( true /*immediate*/ );
  the_dimapFolderMgr->compactAllFolders( true /*immediate*/ );
}

KMFolder* KMKernel::findFolderById( const QString& idString )
{
  KMFolder * folder = the_folderMgr->findIdString( idString );
  if ( !folder )
    folder = the_imapFolderMgr->findIdString( idString );
  if ( !folder )
    folder = the_dimapFolderMgr->findIdString( idString );
  if ( !folder )
    folder = the_searchFolderMgr->findIdString( idString );
  return folder;
}

::KIMProxy* KMKernel::imProxy()
{
  return KIMProxy::instance( kapp->dcopClient() );
}

void KMKernel::enableMailCheck()
{
  mMailCheckAborted = false;
}

bool KMKernel::mailCheckAborted() const
{
  return mMailCheckAborted;
}

void KMKernel::abortMailCheck()
{
  mMailCheckAborted = true;
}

bool KMKernel::canQueryClose()
{
  if ( KMMainWidget::mainWidgetList() &&
       KMMainWidget::mainWidgetList()->count() > 1 )
    return true;
  KMMainWidget *widget = getKMMainWidget();
  if ( !widget )
    return true;
  KMSystemTray* systray = widget->systray();
  if ( systray && systray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowAlways ) {
    systray->hideKMail();
    return false;
  } else if ( systray && systray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
    systray->show();
    systray->hideKMail();
    return false;
  }
  return true;
}

void KMKernel::messageCountChanged()
{
  mTimeOfLastMessageCountChange = ::time( 0 );
}

int KMKernel::timeOfLastMessageCountChange() const
{
  return mTimeOfLastMessageCountChange;
}

Wallet *KMKernel::wallet() {
  static bool walletOpenFailed = false;
  if ( mWallet && mWallet->isOpen() )
    return mWallet;

  if ( !Wallet::isEnabled() || walletOpenFailed )
    return 0;

  delete mWallet;
  mWallet = Wallet::openWallet( Wallet::NetworkWallet(),
        getKMMainWidget() ? getKMMainWidget()->topLevelWidget()->winId() : 0 );

  if ( !mWallet ) {
    walletOpenFailed = true;
    return 0;
  }

  if ( !mWallet->hasFolder( "kmail" ) )
    mWallet->createFolder( "kmail" );
  mWallet->setFolder( "kmail" );
  return mWallet;
}

QList< QPointer<KMFolder> > KMKernel::allFolders()
{
  QStringList names;
  QList<QPointer<KMFolder> > folders;
  folderMgr()->createFolderList(&names, &folders);
  imapFolderMgr()->createFolderList(&names, &folders);
  dimapFolderMgr()->createFolderList(&names, &folders);
  searchFolderMgr()->createFolderList(&names, &folders);

  return folders;
}

KMFolder *KMKernel::currentFolder() {
  KMMainWidget *widget = getKMMainWidget();
  KMFolder *folder = 0;
  if ( widget && widget->folderTree() ) {
    folder = widget->folderTree()->currentFolder();
  }
  return folder;
}

// can't be inline, since KMSender isn't known to implement
// KMail::MessageSender outside this .cpp file
KMail::MessageSender * KMKernel::msgSender() { return the_msgSender; }

#include "kmkernel.moc"
