// kmcommands
// (c) 2002 Don Sanders <sanders@kde.org>
// License: GPL
//
// This file implements various "command" classes. These command classes
// are based on the command design pattern.
//
// Historically various operations were implemented as slots of KMMainWin.
// This proved inadequate as KMail has multiple top level windows
// (KMMainWin, KMReaderMainWin, KMFldSearch, KMComposeWin) that may
// benefit from using these operations. It is desirable that these
// classes can operate without depending on or altering the state of
// a KMMainWin, in fact it is possible no KMMainWin object even exists.
//
// Now these operations have been rewritten as KMCommand based classes,
// making them independent of KMMainWin.
//
// The base command class KMCommand is async, which is a difference
// from the conventional command pattern. As normal derived classes implement
// the execute method, but client classes call start() instead of
// calling execute() directly. start() initiates async operations,
// and on completion of these operations calls execute() and then deletes
// the command. (So the client must not construct commands on the stack).
//
// The type of async operation supported by KMCommand is retrieval
// of messages from an IMAP server.

#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/browserextension.h>
#include <kprogress.h>
#include <krun.h>

#include "kbusyptr.h"
#include "mailinglist-magic.h"
#include "kmaddrbook.h"
#include "kmcomposewin.h"
#include "kmfiltermgr.h"
#include "kmfolder.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "kmkernel.h"
#include "kmmainwin.h"
#include "kmmessage.h"
#include "kmreaderwin.h"
#include "kmsender.h"
#include "kmundostack.h"

#include "kmcommands.h"
#include "kmcommands.moc"

#define IDENTITY_UOIDs

KMCommand::KMCommand( QWidget *parent )
  :mParent( parent ), mFolder( 0 )
{
}

KMCommand::KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList,
				KMFolder *folder )
  :mParent( parent ), mMsgList( msgList ), mFolder( folder )
{
}

KMCommand::KMCommand( QWidget *parent, KMMsgBase *msgBase )
  :mParent( parent ), mFolder( msgBase->parent() )
{
  QPtrList< KMMsgBase > msgList;
  msgList.append( msgBase );
  mMsgList = msgList;
}

KMCommand::~KMCommand()
{
}

void KMCommand::start()
{
  preTransfer();
}


const QPtrList<KMMessage> KMCommand::retrievedMsgs() const
{
  return mRetrievedMsgs;
}

KMMessage *KMCommand::retrievedMessage() const
{
  return mRetrievedMsgs.getFirst();
}

int KMCommand::mCountJobs = 0;

void KMCommand::preTransfer()
{
  connect(this, SIGNAL(messagesTransfered(bool)),
	  this, SLOT(slotPostTransfer(bool)));
  if (!mFolder && (mMsgList.count() == 1) &&
      (mMsgList.getFirst()->isMessage())) {
    mRetrievedMsgs.append((KMMessage*)mMsgList.getFirst());
    emit messagesTransfered(true);
    return;
  }
  if ( (mMsgList.count() > 0) && !mFolder) {
    emit messagesTransfered(false);
    return;
  }

  // transfer the selected messages first
  transferSelectedMsgs();
}

void KMCommand::slotPostTransfer(bool success)
{
  disconnect(this, SIGNAL(messagesTransfered(bool)),
	     this, SLOT(slotPostTransfer(bool)));
  if (success)
    execute();
  QPtrListIterator<KMMessage> it( mRetrievedMsgs );
  KMMessage* msg;
  while ( (msg = it.current()) != 0 )
  {
    ++it;
    msg->setTransferInProgress(false);
  }
  delete this;
}

void KMCommand::transferSelectedMsgs()
{
  // make sure no other transfer is active
  if (KMCommand::mCountJobs > 0) {
    emit messagesTransfered(false);
    return;
  }

  bool complete = true;
  KMCommand::mCountJobs = 0;
  mCountMsgs = 0;
  mRetrievedMsgs.clear();
  mCountMsgs = mMsgList.count();
  // the KProgressDialog for the user-feedback
  mProgressDialog = new KProgressDialog(mParent, "transferProgress",
      i18n("Please wait"),
      i18n("Please wait while the message is transferred",
        "Please wait while the %n messages are transferred", mMsgList.count()),
      true);
  mProgressDialog->setMinimumDuration(1000);
  for (KMMsgBase *mb = mMsgList.first(); mb; mb = mMsgList.next())
  {
    // check if all messages are complete
    int idx = mFolder->find(mb);
    if (idx < 0) continue;
    KMMessage *thisMsg = mFolder->getMsg(idx);
    if (!thisMsg) continue;

    if (thisMsg->parent() && thisMsg->parent()->protocol() == "imap" &&
        !thisMsg->isComplete() && !mProgressDialog->wasCancelled())
    {
      // the message needs to be transferred first
      complete = false;
      KMCommand::mCountJobs++;
      KMImapJob *imapJob = new KMImapJob(thisMsg);
      // emitted when the message was transferred successfully
      connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
          this, SLOT(slotMsgTransfered(KMMessage*)));
      // emitted when the job is destroyed
      connect(imapJob, SIGNAL(finished()),
          this, SLOT(slotJobFinished()));
      // msg musn't be deleted
      thisMsg->setTransferInProgress(true);
    } else {
	if (thisMsg->parent() && thisMsg->parent()->protocol() == "imap")
	    thisMsg->setTransferInProgress(true);
      mRetrievedMsgs.append(thisMsg);
    }
  }

  if (complete)
  {
    delete mProgressDialog;
    emit messagesTransfered(true);
  } else {
    // wait for the transfer and tell the progressBar the necessary steps
    connect(mProgressDialog, SIGNAL(cancelClicked()),
        this, SLOT(slotTransferCancelled()));
    mProgressDialog->progressBar()->setTotalSteps(KMCommand::mCountJobs);
  }
}

void KMCommand::slotMsgTransfered(KMMessage* msg)
{
  if (mProgressDialog->wasCancelled()) {
    emit messagesTransfered(false);
    return;
  }

  // save the complete messages
  mRetrievedMsgs.append(msg);
}

void KMCommand::slotJobFinished()
{
  // the job is finished (with / without error)
  KMCommand::mCountJobs--;

  if (mProgressDialog->wasCancelled()) return;

  if ( (mCountMsgs - static_cast<int>(mRetrievedMsgs.count())) > KMCommand::mCountJobs )
  {
    // the message wasn't retrieved before => error
    mProgressDialog->hide();
    slotTransferCancelled();
    return;
  }
  // update the progressbar
  mProgressDialog->progressBar()->advance(1);
  mProgressDialog->setLabel(i18n("Please wait while the message is transferred",
          "Please wait while the %n messages are transferred", KMCommand::mCountJobs));
  if (KMCommand::mCountJobs == 0)
  {
    // all done
    delete mProgressDialog;
    emit messagesTransfered(true);
  }
}

void KMCommand::slotTransferCancelled()
{
  if (mFolder->protocol() != "imap") {
    emit messagesTransfered(false);
    return;
  }

  // kill the pending jobs
  KMAcctImap* acct = static_cast<KMFolderImap*>(mFolder)->account();
  if (acct)
  {
    acct->killAllJobs();
    acct->setIdle(true);
  }

  KMCommand::mCountJobs = 0;
  mCountMsgs = 0;
  // unget the transfered messages
  QPtrListIterator<KMMessage> it( mRetrievedMsgs );
  KMMessage* msg;
  while ( (msg = it.current()) != 0 )
  {
    ++it;
    msg->setTransferInProgress(false);
    int idx = mFolder->find(msg);
    if (idx > 0) mFolder->unGetMsg(idx);
  }
  mRetrievedMsgs.clear();
  emit messagesTransfered(false);
}

KMMailtoComposeCommand::KMMailtoComposeCommand( const KURL &url,
						KMMsgBase *msgBase )
  :mUrl( url ), mMsgBase( msgBase )
{
}

void KMMailtoComposeCommand::execute()
{
  KMComposeWin *win;
  KMMessage *msg = new KMMessage;
#ifdef IDENTITY_UOIDs
  uint id = 0;
#else
  QString id = "";
#endif

  if ( mMsgBase && mMsgBase->parent() )
    id = mMsgBase->parent()->identity();

  msg->initHeader(id);
  msg->setCharset("utf-8");
  msg->setTo(mUrl.path());

  win = new KMComposeWin(msg, id);
  win->setCharset("", TRUE);
  win->show();
}


KMMailtoReplyCommand::KMMailtoReplyCommand( KMainWindow *parent,
   const KURL &url, KMMsgBase *msgBase, const QString &selection )
  :KMCommand( parent, msgBase ), mUrl( url ), mSelection( selection  )
{
}

void KMMailtoReplyCommand::execute()
{
  //TODO : consider factoring createReply into this method.
  KMMessage *msg = retrievedMessage();
  KMComposeWin *win;
  KMMessage *rmsg = msg->createReply(FALSE, FALSE, mSelection );
  rmsg->setTo(mUrl.path());

#ifdef IDENTITY_UOIDs
  win = new KMComposeWin(rmsg, 0);
#else
  win = new KMComposeWin(rmsg, QString::null);
#endif
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus();
  win->show();
}


KMMailtoForwardCommand::KMMailtoForwardCommand( KMainWindow *parent,
   const KURL &url, KMMsgBase *msgBase )
  :KMCommand( parent, msgBase ), mUrl( url )
{
}

void KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  KMMessage *msg = retrievedMessage();
  KMComposeWin *win;
  KMMessage *fmsg = msg->createForward();
  fmsg->setTo(mUrl.path());

  win = new KMComposeWin(fmsg);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
}


KMMailtoAddAddrBookCommand::KMMailtoAddAddrBookCommand( const KURL &url,
   QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMMailtoAddAddrBookCommand::execute()
{
  KMAddrBookExternal::addEmail(mUrl.path(), mParent );
}


KMMailtoOpenAddrBookCommand::KMMailtoOpenAddrBookCommand( const KURL &url,
   QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMMailtoOpenAddrBookCommand::execute()
{
  KMAddrBookExternal::openEmail(mUrl.path(), mParent );
}


KMUrlCopyCommand::KMUrlCopyCommand( const KURL &url, KMMainWin *mainWin )
  :mUrl( url ), mMainWin( mainWin )
{
}

void KMUrlCopyCommand::execute()
{
  QClipboard* clip = QApplication::clipboard();

  if (mUrl.protocol() == "mailto") {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode( true );
    clip->setText( mUrl.path() );
    clip->setSelectionMode( false );
    clip->setText( mUrl.path() );
    if (mMainWin)
      mMainWin->statusMsg( i18n( "Address copied to clipboard." ));
  } else {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode( true );
    clip->setText( mUrl.url() );
    clip->setSelectionMode( false );
    clip->setText( mUrl.url() );
    if ( mMainWin )
      mMainWin->statusMsg( i18n( "URL copied to clipboard." ));
  }
}


KMUrlOpenCommand::KMUrlOpenCommand( const KURL &url, KMReaderWin *readerWin )
  :mUrl( url ), mReaderWin( readerWin )
{
}

void KMUrlOpenCommand::execute()
{
  if (mUrl.isEmpty()) return;
  mReaderWin->slotUrlOpen( mUrl, KParts::URLArgs() );
}


KMUrlSaveCommand::KMUrlSaveCommand( const KURL &url, QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMUrlSaveCommand::execute()
{
  if (mUrl.isEmpty()) return;
  KURL saveUrl = KFileDialog::getSaveURL(mUrl.fileName(), QString::null,
    mParent);
  if (saveUrl.isEmpty()) return;
  if (KIO::NetAccess::exists(saveUrl))
  {
    if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?")
        .arg(saveUrl.prettyURL()), i18n("Save to File"), i18n("&Replace"))
        != KMessageBox::Continue)
      return;
  }
  KIO::Job *job = KIO::file_copy(mUrl, saveUrl, -1, true);
  connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotUrlSaveResult(KIO::Job*)));
}

void KMUrlSaveCommand::slotUrlSaveResult( KIO::Job *job )
{
  if (job->error()) job->showErrorDialog();
}


KMEditMsgCommand::KMEditMsgCommand( KMainWindow *parent, KMMsgBase *msgBase )
  :KMCommand( parent, msgBase )
{
}

void KMEditMsgCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  if (!msg || !msg->parent() ||
      !kernel->folderIsDraftOrOutbox( msg->parent() ))
    return;

  if (msg->parent() == kernel->outboxFolder() && msg->transferInProgress())
    return;

  msg->parent()->removeMsg(msg);
#if 0
  // Useful?
  mHeaders->setSelected(mHeaders->currentItem(), TRUE);
  mHeaders->highlightMessage(mHeaders->currentItem(), true);
#endif

  KMComposeWin *win = new KMComposeWin();
#if 0
  // FIXME: Poor solution, won't work for multiple readerwins should use kmkernel as an observer
  QObject::connect( win, SIGNAL( messageQueuedOrDrafted()),
		    this, SLOT( slotMessageQueuedOrDrafted()) );
#endif
  win->setMsg(msg, FALSE, TRUE);
  win->setFolder( msg->parent() );
  win->show();
}


KMShowMsgSrcCommand::KMShowMsgSrcCommand( KMainWindow *parent,
  KMMsgBase *msgBase, const QTextCodec *codec, bool fixedFont )
  :KMCommand( parent, msgBase ), mCodec( codec ), mFixedFont( fixedFont )
{
}

void KMShowMsgSrcCommand::execute()
{
  //TODO: move KMMessage::viewSource to here
  KMMessage *msg = retrievedMessage();
  if (!mCodec) //this is Auto setting
  {
    QCString cset = msg->charset();
    if (!cset.isEmpty())
      mCodec = KMMsgBase::codecForName( cset );
  }
  msg->viewSource( i18n("Message as Plain Text"), mCodec,
		   mFixedFont );
}

KMSaveMsgCommand::KMSaveMsgCommand( KMainWindow *parent,
  const QPtrList<KMMsgBase> &msgList, KMFolder *folder )
  :KMCommand( parent, msgList, folder )
{
  if (!msgList.getFirst())
    return;
  KMMsgBase *msgBase = msgList.getFirst();
  QString subject = msgBase->subject();
  while (subject.find(':') != -1)
    subject = subject.mid(subject.find(':') + 1).stripWhiteSpace();
  subject.replace(QRegExp(QChar(QDir::separator())), "_");
  mUrl = KFileDialog::getSaveURL(subject, QString::null);
}

KURL KMSaveMsgCommand::url()
{
  return mUrl;
}

void KMSaveMsgCommand::execute()
{
//TODO: Handle messages one by one
  QPtrList<KMMessage> msgList = retrievedMsgs();
  QCString str;

  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
    str += "From " + msg->fromEmail() + " " + msg->dateShortStr() + "\n";
    str += msg->asString();
    str += "\n";
  }

  QByteArray ba = str;
  ba.resize(ba.size() - 1);
  kernel->byteArrayToRemoteFile(ba, mUrl);
}

//TODO: ReplyTo, NoQuoteReplyTo, ReplyList, ReplyToAll are all similar
//and should be factored
KMReplyToCommand::KMReplyToCommand( KMainWindow *parent, KMMsgBase *msgBase,
				    const QString &selection )
  : KMCommand( parent, msgBase ), mSelection( selection )
{
}

void KMReplyToCommand::execute()
{
  kernel->kbp()->busy();
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( FALSE, FALSE, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}


KMNoQuoteReplyToCommand::KMNoQuoteReplyToCommand( KMainWindow *parent,
						  KMMsgBase *msgBase )
  : KMCommand( parent, msgBase )
{
}

void KMNoQuoteReplyToCommand::execute()
{
  kernel->kbp()->busy();
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply(FALSE, FALSE, "", TRUE);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();
  kernel->kbp()->idle();
}


KMReplyListCommand::KMReplyListCommand( KMainWindow *parent,
  KMMsgBase *msgBase, const QString &selection )
 : KMCommand( parent, msgBase ), mSelection( selection )
{
}

void KMReplyListCommand::execute()
{
  kernel->kbp()->busy();
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply(true, true, mSelection);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();
  kernel->kbp()->idle();
}


KMReplyToAllCommand::KMReplyToAllCommand( KMainWindow *parent,
  KMMsgBase *msgBase, const QString &selection )
  :KMCommand( parent, msgBase ), mSelection( selection )
{
}

void KMReplyToAllCommand::execute()
{
  kernel->kbp()->busy();
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( TRUE, FALSE, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}


KMForwardCommand::KMForwardCommand( KMainWindow *parent,
  const QPtrList<KMMsgBase> &msgList, KMFolder *folder )
  : KMCommand( parent, msgList, folder ),
    mParent( parent ), mFolder( folder )
{
}

void KMForwardCommand::execute()
{
  KMComposeWin *win;
  QPtrList<KMMessage> msgList = retrievedMsgs();

  if (msgList.count() >= 2) {
    // ask if they want a mime digest forward

    if (KMessageBox::questionYesNo(mParent, i18n("Forward selected messages as"
						 " a MIME digest?"))
	== KMessageBox::Yes) {
      // we default to the first identity to save prompting the user
      // (the messages could have different identities)
      uint id = 0;
      KMMessage *fwdMsg = new KMMessage;
      KMMessagePart *msgPart = new KMMessagePart;
      QString msgPartText;
      int msgCnt = 0; // incase there are some we can't forward for some reason

      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->mMsg->Headers().ContentType().CreateBoundary(1);
      msgPartText = i18n("\nThis is a MIME digest forward. The content of the"
                         " message is contained in the attachment(s).\n\n\n"
                         "--\n");
      // iterate through all the messages to be forwarded
      for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
        // set the identity
        if (id == 0)
          id = msg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();
        // set the part header
        msgPartText += "--";
        msgPartText += fwdMsg->mMsg->Headers().ContentType().Boundary().c_str();
        msgPartText += "\nContent-Type: MESSAGE/RFC822";
        msgPartText += QString("; CHARSET=%1").arg(msg->charset());
        msgPartText += "\n";
        DwHeaders dwh;
        dwh.MessageId().CreateDefault();
        msgPartText += QString("Content-ID: %1\n").arg(dwh.MessageId().AsString().c_str());
        msgPartText += QString("Content-Description: %1").arg(msg->subject());
        if (!msg->subject().contains("(fwd)"))
          msgPartText += " (fwd)";
        msgPartText += "\n\n";
        // set the part
        msgPartText += msg->headerAsString();
        msgPartText += "\n";
        msgPartText += msg->body();
        msgPartText += "\n";     // eot
        msgCnt++;
        fwdMsg->link(msg, KMMsgStatusForwarded);
      }
      QCString tmp;
      msgPart->setTypeStr("MULTIPART");
      tmp.sprintf( "Digest; boundary=\"%s\"",
		   fwdMsg->mMsg->Headers().ContentType().Boundary().c_str() );
      msgPart->setSubtypeStr( tmp );
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(QCString(msgPartText.ascii()));
      kernel->kbp()->busy();
      win = new KMComposeWin(fwdMsg, id);
      win->addAttach(msgPart);
      win->show();
      kernel->kbp()->idle();
      return;
    } else {            // NO MIME DIGEST, Multiple forward
      uint id = 0;
      QCString msgText = "";
      QPtrList<KMMessage> linklist;
      for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
        // set the identity
        if (id == 0)
          id = msg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();

        msgText += msg->createForwardBody();
        linklist.append(msg);
      }

      KMMessage *fwdMsg = new KMMessage;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");
      fwdMsg->setBody(msgText);

      for (KMMessage *msg = linklist.first(); msg; msg = linklist.next())
        fwdMsg->link(msg, KMMsgStatusForwarded);

      kernel->kbp()->busy();
      win = new KMComposeWin(fwdMsg, id);
      win->setCharset("");
      win->show();
      kernel->kbp()->idle();
      return;
    }
  }

  // forward a single message at most.

  KMMessage *msg = msgList.getFirst();
  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin(msg->createForward());
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
  kernel->kbp()->idle();
}


KMForwardAttachedCommand::KMForwardAttachedCommand( KMainWindow *parent,
  const QPtrList<KMMsgBase> &msgList, KMFolder *folder )
  : KMCommand( parent, msgList, folder ), mFolder( folder )
{
}

void KMForwardAttachedCommand::execute()
{
  QPtrList<KMMessage> msgList = retrievedMsgs();
  KMComposeWin *win;
  uint id = 0;
  KMMessage *fwdMsg = new KMMessage;

  if (msgList.count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    id = mFolder->identity();
    fwdMsg->initHeader(id);
  }
  else if (msgList.count() == 1) {
    KMMessage *msg = msgList.getFirst();
    fwdMsg->initFromMessage(msg);
  }

  fwdMsg->setAutomaticFields(true);

  kernel->kbp()->busy();
  win = new KMComposeWin(fwdMsg, id);

  // iterate through all the messages to be forwarded
  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
    // set the part
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setTypeStr("message");
    msgPart->setSubtypeStr("rfc822");
    msgPart->setCharset(msg->charset());
    msgPart->setName("forwarded message");
    msgPart->setCte(DwMime::kCte8bit);   // is 8bit O.K.?
    msgPart->setContentDescription(msg->from()+": "+msg->subject());
    // THIS HAS TO BE AFTER setCte()!!!!
    msgPart->setBodyEncoded(msg->asString());
    msgPart->setCharset("");

    fwdMsg->link(msg, KMMsgStatusForwarded);
    win->addAttach(msgPart);
  }

  win->show();
  kernel->kbp()->idle();
}


KMRedirectCommand::KMRedirectCommand( KMainWindow *parent,
  KMMsgBase *msgBase )
  : KMCommand( parent, msgBase )
{
}

void KMRedirectCommand::execute()
{
  //TODO: move KMMessage::createRedirect to here
  KMComposeWin *win;
  KMMessage *msg = retrievedMessage();
  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin();
  win->setMsg(msg->createRedirect(), FALSE);
  win->setCharset(msg->codec()->mimeName());
  win->show();
  kernel->kbp()->idle();
}


KMBounceCommand::KMBounceCommand( KMainWindow *parent,
  KMMsgBase *msgBase )
  : KMCommand( parent, msgBase )
{
}

void KMBounceCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  KMMessage *newMsg = msg->createBounce( TRUE /* with UI */);
  if (newMsg)
    kernel->msgSender()->send(newMsg, kernel->msgSender()->sendImmediate());
}


KMToggleFixedCommand::KMToggleFixedCommand( KMReaderWin *readerWin )
  : KMCommand(), mReaderWin( readerWin )
{
}

void KMToggleFixedCommand::execute()
{
  mReaderWin->slotToggleFixedFont();
}


KMPrintCommand::KMPrintCommand( KMainWindow *parent,
  KMMsgBase *msgBase )
  : KMCommand( parent, msgBase)
{
}

void KMPrintCommand::execute()
{
  KMReaderWin printWin;
  printWin.setPrinting(TRUE);
  printWin.readConfig();
  printWin.setMsg(retrievedMessage(), TRUE);
  printWin.printMsg();
}


//TODO: Use serial numbers
KMSetStatusCommand::KMSetStatusCommand( KMMsgStatus status,
  const QValueList<int> &ids, KMFolder *folder )
  : mStatus( status ), mIds( ids ), mFolder( folder )
{
}

void KMSetStatusCommand::execute()
{
   mFolder->setStatus( mIds, mStatus );
}


KMFilterCommand::KMFilterCommand( const QCString &field, const QString &value )
  : mField( field ), mValue( value )
{
}

void KMFilterCommand::execute()
{
  kernel->filterMgr()->createFilter( mField, mValue );
}


KMMailingListFilterCommand::KMMailingListFilterCommand( KMainWindow *parent,
  KMMsgBase *msgBase )
  : KMCommand( parent, msgBase )
{
}

void KMMailingListFilterCommand::execute()
{
  QCString name;
  QString value;
  KMMessage *msg = retrievedMessage();
  if (!msg)
    return;

  if (!KMMLInfo::name( msg, name, value ).isNull())
    kernel->filterMgr()->createFilter( name, value );
}


QPopupMenu* KMMenuCommand::folderToPopupMenu(bool move,
  QObject *receiver, KMMenuToFolder *aMenuToFolder, QPopupMenu *menu )
{
  while ( menu->count() )
  {
    QPopupMenu *popup = menu->findItem( menu->idAt( 0 ) )->popup();
    if (popup)
      delete popup;
    else
      menu->removeItemAt( 0 );
  }

  if (!kernel->imapFolderMgr()->dir().first()) {
    KMMenuCommand::makeFolderMenu(  &kernel->folderMgr()->dir(), move,
      receiver, aMenuToFolder, menu );		
  } else {
    // operate on top-level items
    QPopupMenu* subMenu = new QPopupMenu(menu);
    subMenu = KMMenuCommand::makeFolderMenu(  &kernel->folderMgr()->dir(),
        move, receiver, aMenuToFolder, subMenu );
    menu->insertItem( i18n( "Local Folders" ), subMenu );
    KMFolderDir* fdir = &kernel->imapFolderMgr()->dir();
    for (KMFolderNode *node = fdir->first(); node; node = fdir->next()) {
      if (node->isDir())
        continue;	
      subMenu = new QPopupMenu(menu);
      subMenu = makeFolderMenu( node, move, receiver, aMenuToFolder, subMenu );
      menu->insertItem( node->label(), subMenu );
    }
  }

  return menu;
}

QPopupMenu* KMMenuCommand::makeFolderMenu(KMFolderNode* node, bool move,
  QObject *receiver, KMMenuToFolder *aMenuToFolder, QPopupMenu *menu )
{
  // connect the signals
  if (move)
  {
    disconnect(menu, SIGNAL(activated(int)), receiver,
           SLOT(moveSelectedToFolder(int)));
    connect(menu, SIGNAL(activated(int)), receiver,
             SLOT(moveSelectedToFolder(int)));
  } else {
    disconnect(menu, SIGNAL(activated(int)), receiver,
           SLOT(copySelectedToFolder(int)));
    connect(menu, SIGNAL(activated(int)), receiver,
             SLOT(copySelectedToFolder(int)));
  }

  KMFolder *folder = 0;
  KMFolderDir *folderDir = 0;
  if (node->isDir()) {
    folderDir = static_cast<KMFolderDir*>(node);
  } else {
    folder = static_cast<KMFolder*>(node);
    folderDir = folder->child();
  }

  if (folder && !folder->noContent())
  {
    int menuId;
    if (move)
      menuId = menu->insertItem(i18n("Move to this Folder"));
    else
      menuId = menu->insertItem(i18n("Copy to this Folder"));
    aMenuToFolder->insert( menuId, folder );
    menu->insertSeparator();
  }

  if (!folderDir)
    return menu;

  for (KMFolderNode *it = folderDir->first(); it; it = folderDir->next() ) {
    if (it->isDir())
      continue;
    KMFolder *child = static_cast<KMFolder*>(it);
    QString label = child->label();
    label.replace(QRegExp("&"),QString("&&"));
    if (child->child() && child->child()->first()) {
      // descend
      QPopupMenu *subMenu = makeFolderMenu(child, move, receiver,
	aMenuToFolder, new QPopupMenu(menu, "subMenu"));
      menu->insertItem(label, subMenu);
    } else {
      // insert an item
      int menuId = menu->insertItem(label);
      aMenuToFolder->insert( menuId, child );
    }
  }
  return menu;
}


KMCopyCommand::KMCopyCommand( KMFolder* srcFolder, KMFolder* destFolder,
			      const QPtrList<KMMsgBase> &msgList )
  :mSrcFolder( srcFolder ), mDestFolder( destFolder ), mMsgList( msgList )
{
}

void KMCopyCommand::execute()
{
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int idx = -1;
  bool isMessage;
  QPtrList<KMMessage> list;

  kernel->kbp()->busy();
  mDestFolder->open();

  for (msgBase = mMsgList.first(); msgBase; msgBase = mMsgList.next() )
  {
    if (isMessage = msgBase->isMessage())
    {
      msg = static_cast<KMMessage*>(msgBase);
    } else {
      idx = mSrcFolder->find(msgBase);
      assert(idx != -1);
      msg = mSrcFolder->getMsg(idx);
    }

    if (mSrcFolder &&
	(mSrcFolder->protocol() == "imap") &&
	(mDestFolder->protocol() == "imap") &&
	(static_cast<KMFolderImap*>(mSrcFolder)->account() ==
	 static_cast<KMFolderImap*>(mDestFolder)->account()))
    {
      list.append(msg);
    } else {
      newMsg = new KMMessage;
      newMsg->fromString(msg->asString());
      newMsg->setStatus(msg->status());
      newMsg->setComplete(msg->isComplete());

      if (mSrcFolder && (mSrcFolder->protocol() == "imap") &&
	  !newMsg->isComplete())
      {
        kernel->filterMgr()->tempOpenFolder(mDestFolder);
	newMsg->setParent(msg->parent());
        KMImapJob *imapJob = new KMImapJob(newMsg);
        connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
		mDestFolder, SLOT(reallyAddCopyOfMsg(KMMessage*)));
      } else {
	int rc, index;
	rc = mDestFolder->addMsg(newMsg, &index);
	if (rc == 0 && index != -1)
	  mDestFolder->unGetMsg( mDestFolder->count() - 1 );
      }
    }

    if (!isMessage && list.isEmpty())
    {
      assert(idx != -1);
      mSrcFolder->unGetMsg( idx );
    }

  } // end for

//TODO: Get rid of the other cases just use this one for all types of folder
//TODO: requires adding copyMsg and getFolder methods to KMFolder.h

  if (!list.isEmpty())
  {
    // copy the message(s); note: the list is empty afterwards!
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(mDestFolder);
    imapDestFolder->copyMsg(list);
    imapDestFolder->getFolder();
  }

  mDestFolder->close();
  kernel->kbp()->idle();
}


KMMoveCommand::KMMoveCommand( KMFolder* srcFolder, KMFolder* destFolder,
			      const QPtrList<KMMsgBase> &msgList,
			      KMHeaders *headers )
  :mSrcFolder( srcFolder ), mDestFolder( destFolder ), mMsgList( msgList ),
   mHeaders( headers )
{
// TODO: mHeaders is optional ...
}

void KMMoveCommand::execute()
{
  if (mDestFolder == mSrcFolder)
    return;

  if (mDestFolder && mDestFolder->open() != 0)
    return;
  kernel->kbp()->busy();

  KMMsgBase *curMsg = 0;
  int contentX, contentY;
  if (mHeaders)
    mHeaders->prepareMove( &curMsg, &contentX, &contentY );

  KMMessage *msg;
  KMMsgBase *msgBase;
  int rc = 0;
  int index;
  QPtrList<KMMessage> list;
  for (msgBase=mMsgList.first(); msgBase && !rc; msgBase=mMsgList.next())
  {
    int idx = mSrcFolder->find(msgBase);
    assert(idx != -1);
    msg = mSrcFolder->getMsg(idx);
    if (msg->transferInProgress()) continue;

    if (mDestFolder) {
      if (mDestFolder->protocol() == "imap")
        list.append(msg);
      else
      {
        rc = mDestFolder->moveMsg(msg, &index);
        if (rc == 0 && index != -1) {
          KMMsgBase *mb = mDestFolder->unGetMsg( mDestFolder->count() - 1 );
          kernel->undoStack()->pushAction( mb->getMsgSerNum(), mSrcFolder,
					   mDestFolder );
        }
      }
    }
    else
    {
      // really delete messages that are already in the trash folder
      if (mSrcFolder->protocol() == "imap")
        list.append(msg);
      else
      {
        mSrcFolder->removeMsg(msg);
        delete msg;
      }
    }
  }
  if (!list.isEmpty())
  {
    if (mDestFolder)
      mDestFolder->moveMsg(list, &index);
    else
      mSrcFolder->removeMsg(list);
  }

  if (mHeaders)
    mHeaders->finalizeMove( curMsg, contentX, contentY );

  if (mDestFolder) {
     mDestFolder->sync();
     mDestFolder->close();
  }
  kernel->kbp()->idle();
}


KMDeleteMsgCommand::KMDeleteMsgCommand( KMFolder* srcFolder,
  const QPtrList<KMMsgBase> &msgList, KMHeaders *headers )
{
  KMFolder* folder = NULL;
  if (srcFolder->protocol() == "imap")
  {
    KMFolderImap* fi = static_cast<KMFolderImap*> (srcFolder);
    QString trashStr = fi->account()->trash();
    KMFolder* trash = kernel->imapFolderMgr()->findIdString( trashStr );
    if (!trash) trash = kernel->trashFolder();
    if (srcFolder != trash)
      folder = trash;
  } else {
    if (srcFolder != kernel->trashFolder())
    {
      // move to trash folder
      folder = kernel->trashFolder();
    }
  }

  if (folder) {
    KMCommand *command = new KMMoveCommand( srcFolder, folder, msgList,
					    headers );
    command->start();
  }
}

void KMDeleteMsgCommand::execute()
{
}


KMUrlClickedCommand::KMUrlClickedCommand( const KURL &url, KMFolder* folder,
  KMReaderWin *readerWin, bool htmlPref, KMMainWin *mainWin )
  :mUrl( url ), mFolder( folder ), mReaderWin( readerWin ),
   mHtmlPref( htmlPref ), mMainWin( mainWin )
{
}

void KMUrlClickedCommand::execute()
{
  KMComposeWin *win;
  KMMessage* msg;

  if (mUrl.protocol() == "mailto")
  {
#ifdef IDENTITY_UOIDs
    uint id = 0;
#else
    QString id = "";
#endif
    if ( mFolder )
      id = mFolder->identity();

    msg = new KMMessage;
    msg->initHeader(id);
    msg->setCharset("utf-8");
    msg->setTo(mUrl.path());
    QString query=mUrl.query();
    while (!query.isEmpty()) {
      QString queryPart;
      int secondQuery = query.find('?',1);
      if (secondQuery != -1)
	queryPart = query.left(secondQuery);
      else
	queryPart = query;
      query = query.mid(queryPart.length());

      if (queryPart.left(9) == "?subject=")
	msg->setSubject( KURL::decode_string(queryPart.mid(9)) );
      else if (queryPart.left(6) == "?body=")
	// It is correct to convert to latin1() as URL should not contain
	// anything except ascii.
	msg->setBody( KURL::decode_string(queryPart.mid(6)).latin1() );
      else if (queryPart.left(4) == "?cc=")
	msg->setCc( KURL::decode_string(queryPart.mid(4)) );
    }

    win = new KMComposeWin(msg, id);
    win->setCharset("", TRUE);
    win->show();
  }
  else if ((mUrl.protocol() == "http") || (mUrl.protocol() == "https") ||
           (mUrl.protocol() == "ftp") || (mUrl.protocol() == "file") ||
           (mUrl.protocol() == "ftps") || (mUrl.protocol() == "sftp" ) ||
           (mUrl.protocol() == "help") || (mUrl.protocol() == "vnc") ||
           (mUrl.protocol() == "smb"))
  {
    if (mMainWin)
      mMainWin->statusMsg(i18n("Opening URL..."));
    KMimeType::Ptr mime = KMimeType::findByURL( mUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18n( "Do you really want to execute"
        " '%1'? " ).arg( mUrl.prettyURL() ) ) != KMessageBox::Yes) return;
    }
    (void) new KRun( mUrl );
  }
  // handle own links
  else if( mUrl.protocol() == "kmail" )
  {
    if( mUrl.path() == "showHTML" )
    {
      mReaderWin->setHtmlOverride(!mHtmlPref);
      mReaderWin->update( true );
    }
  }
}
