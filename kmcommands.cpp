// -*- mode: C++; c-file-style: "gnu" -*-
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
#include <errno.h>
#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>

#include <qtextcodec.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/browserextension.h>
#include <kprogress.h>
#include <krun.h>
#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include "actionscheduler.h"
using KMail::ActionScheduler;
#include "mailinglist-magic.h"
#include "kmaddrbook.h"
#include "kmcomposewin.h"
#include "kmfiltermgr.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "kmsender.h"
#include "undostack.h"
#include "partNode.h"
#include "kcursorsaver.h"
#include "partNode.h"
using KMail::FolderJob;
#include "mailsourceviewer.h"
using KMail::MailSourceViewer;

#include "kmcommands.h"
#include "kmcommands.moc"

KMCommand::KMCommand( QWidget *parent )
  :mDeletesItself( false ), mParent( parent )
{
}

KMCommand::KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList )
  :mDeletesItself( false ), mParent( parent ), mMsgList( msgList )
{
}

KMCommand::KMCommand( QWidget *parent, KMMsgBase *msgBase )
  :mDeletesItself( false ), mParent( parent )
{
  mMsgList.append( msgBase );
}

KMCommand::KMCommand( QWidget *parent, KMMessage *msg )
  :mDeletesItself( false ), mParent( parent )
{
  mMsgList.append( &msg->toMsgBase() );
}

KMCommand::~KMCommand()
{
  QValueListIterator<QGuardedPtr<KMFolder> > fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if (!(*fit))
      continue;
    (*fit)->close();
  }
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
  kmkernel->filterMgr()->ref();

  if (mMsgList.find(0) != -1) {
      emit messagesTransfered(false);
      return;
  }

  if ((mMsgList.count() == 1) &&
      (mMsgList.getFirst()->isMessage()) &&
      (mMsgList.getFirst()->parent() == 0))
  {
    // Special case of operating on message that isn't in a folder
    mRetrievedMsgs.append((KMMessage*)mMsgList.getFirst());
    emit messagesTransfered(true);
    return;
  }

  for (KMMsgBase *mb = mMsgList.first(); mb; mb = mMsgList.next())
    if (!mb->parent()) {
      emit messagesTransfered(false);
      return;
    } else {
      mFolders.append( mb->parent() );
      mb->parent()->open();
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
    if (msg->parent())
      msg->setTransferInProgress(false);
  }
  kmkernel->filterMgr()->deref();
  if ( !deletesItself() )
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
    KMMessage *thisMsg = 0;
    if ( mb->isMessage() )
      thisMsg = static_cast<KMMessage*>(mb);
    else
    {
      KMFolder *folder = mb->parent();
      int idx = folder->find(mb);
      if (idx < 0) continue;
      thisMsg = folder->getMsg(idx);
    }
    if (!thisMsg) continue;
    if ( thisMsg->transferInProgress() &&
         thisMsg->parent()->folderType() == KMFolderTypeImap )
    {
      thisMsg->setTransferInProgress( false, true );
      thisMsg->parent()->ignoreJobsForMessage( thisMsg );
    }

    if ( thisMsg->parent() && !thisMsg->isComplete() && !mProgressDialog->wasCancelled() )
    {
      kdDebug(5006)<<"### INCOMPLETE with protocol = "<<thisMsg->parent()->protocol() <<endl;
      // the message needs to be transferred first
      complete = false;
      KMCommand::mCountJobs++;
      FolderJob *job = thisMsg->parent()->createJob(thisMsg);
      // emitted when the message was transferred successfully
      connect(job, SIGNAL(messageRetrieved(KMMessage*)),
              this, SLOT(slotMsgTransfered(KMMessage*)));
      // emitted when the job is destroyed
      connect(job, SIGNAL(finished()),
              this, SLOT(slotJobFinished()));
      // msg musn't be deleted
      thisMsg->setTransferInProgress(true);
      job->start();
    } else {
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
  // kill the pending jobs
  QValueListIterator<QGuardedPtr<KMFolder> > fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if (!(*fit))
      continue;
    KMFolder *folder = *fit;
    KMFolderImap *imapFolder = dynamic_cast<KMFolderImap*>(folder);
    if (imapFolder && imapFolder->account()) {
      imapFolder->account()->killAllJobs();
      imapFolder->account()->setIdle(true);
    }
  }

  KMCommand::mCountJobs = 0;
  mCountMsgs = 0;
  // unget the transfered messages
  QPtrListIterator<KMMessage> it( mRetrievedMsgs );
  KMMessage* msg;
  while ( (msg = it.current()) != 0 )
  {
    KMFolder *folder = msg->parent();
    ++it;
    if (!folder)
      continue;
    msg->setTransferInProgress(false);
    int idx = folder->find(msg);
    if (idx > 0) folder->unGetMsg(idx);
  }
  mRetrievedMsgs.clear();
  emit messagesTransfered(false);
}

KMMailtoComposeCommand::KMMailtoComposeCommand( const KURL &url,
                                                KMMessage *msg )
  :mUrl( url ), mMessage( msg )
{
}

void KMMailtoComposeCommand::execute()
{
  KMComposeWin *win;
  KMMessage *msg = new KMMessage;
  uint id = 0;

  if ( mMessage && mMessage->parent() )
    id = mMessage->parent()->identity();

  msg->initHeader(id);
  msg->setCharset("utf-8");
  msg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  win = new KMComposeWin(msg, id);
  win->setCharset("", TRUE);
  win->setFocusToSubject();
  win->show();
}


KMMailtoReplyCommand::KMMailtoReplyCommand( QWidget *parent,
   const KURL &url, KMMessage *msg, const QString &selection )
  :KMCommand( parent, msg ), mUrl( url ), mSelection( selection  )
{
}

void KMMailtoReplyCommand::execute()
{
  //TODO : consider factoring createReply into this method.
  KMMessage *msg = retrievedMessage();
  KMComposeWin *win;
  KMMessage *rmsg = msg->createReply( KMail::ReplyNone, mSelection );
  rmsg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  win = new KMComposeWin(rmsg, 0);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus();
  win->show();
}


KMMailtoForwardCommand::KMMailtoForwardCommand( QWidget *parent,
   const KURL &url, KMMessage *msg )
  :KMCommand( parent, msg ), mUrl( url )
{
}

void KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  KMMessage *msg = retrievedMessage();
  KMComposeWin *win;
  KMMessage *fmsg = msg->createForward();
  fmsg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  win = new KMComposeWin(fmsg);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
}


KMAddBookmarksCommand::KMAddBookmarksCommand( const KURL &url, QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMAddBookmarksCommand::execute()
{
  QString filename = locateLocal( "data", QString::fromLatin1("konqueror/bookmarks.xml") );
  KBookmarkManager *bookManager = KBookmarkManager::managerForFile( filename,
                                                                    false );
  KBookmarkGroup group = bookManager->root();
  group.addBookmark( bookManager, mUrl.path(), KURL( mUrl ) );
  bookManager->save();
}

KMMailtoAddAddrBookCommand::KMMailtoAddAddrBookCommand( const KURL &url,
   QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMMailtoAddAddrBookCommand::execute()
{
  KMAddrBookExternal::addEmail( KMMessage::decodeMailtoUrl( mUrl.path() ), mParent );
}


KMMailtoOpenAddrBookCommand::KMMailtoOpenAddrBookCommand( const KURL &url,
   QWidget *parent )
  :mUrl( url ), mParent( parent )
{
}

void KMMailtoOpenAddrBookCommand::execute()
{
  KMAddrBookExternal::openEmail( KMMessage::decodeMailtoUrl( mUrl.path() ), mParent );
}


KMUrlCopyCommand::KMUrlCopyCommand( const KURL &url, KMMainWidget *mainWidget )
  :mUrl( url ), mMainWidget( mainWidget )
{
}

void KMUrlCopyCommand::execute()
{
  QClipboard* clip = QApplication::clipboard();

  if (mUrl.protocol() == "mailto") {
    // put the url into the mouse selection and the clipboard
    QString address = KMMessage::decodeMailtoUrl( mUrl.path() );
    clip->setSelectionMode( true );
    clip->setText( address );
    clip->setSelectionMode( false );
    clip->setText( address );
    if (mMainWidget)
      mMainWidget->statusMsg( i18n( "Address copied to clipboard." ));
  } else {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode( true );
    clip->setText( mUrl.url() );
    clip->setSelectionMode( false );
    clip->setText( mUrl.url() );
    if ( mMainWidget )
      mMainWidget->statusMsg( i18n( "URL copied to clipboard." ));
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
  if (KIO::NetAccess::exists(saveUrl, false, mParent))
  {
    if (KMessageBox::warningContinueCancel(0,
        i18n("<qt>File <b>%1</b> exists.<br>Do you want to replace it?</qt>")
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


KMEditMsgCommand::KMEditMsgCommand( QWidget *parent, KMMessage *msg )
  :KMCommand( parent, msg )
{
}

void KMEditMsgCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  if (!msg || !msg->parent() ||
      !kmkernel->folderIsDraftOrOutbox( msg->parent() ))
    return;

  // Remember the old parent, we need it a bit further down to be able
  // to put the unchanged messsage back in the drafts folder if the nth
  // edit is discarded, for n > 1.
  KMFolder *parent = msg->parent();
  if ( parent )
    parent->take( parent->find( msg ) );
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
  msg->setTransferInProgress(false); // From here on on, the composer owns the message.
  win->setMsg(msg, FALSE, TRUE);
  win->setFolder( parent );
  win->show();
}


KMShowMsgSrcCommand::KMShowMsgSrcCommand( QWidget *parent,
  KMMessage *msg, bool fixedFont )
  :KMCommand( parent, msg ), mFixedFont( fixedFont )
{
}

void KMShowMsgSrcCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  QString str = QString::fromLatin1( msg->asString() );

  MailSourceViewer *viewer = new MailSourceViewer(); // deletes itself upon close
  viewer->setCaption( i18n("Message as Plain Text") );
  viewer->setText(str);
  if( mFixedFont )
    viewer->setFont(KGlobalSettings::fixedFont());

  // Well, there is no widget to be seen here, so we have to use QCursor::pos()
  // Update: (GS) I'm not going to make this code behave according to Xinerama
  //         configuration because this is quite the hack.
  if (QApplication::desktop()->isVirtualDesktop()) {
    int scnum = QApplication::desktop()->screenNumber(QCursor::pos());
    viewer->resize(QApplication::desktop()->screenGeometry(scnum).width()/2,
                  2*QApplication::desktop()->screenGeometry(scnum).height()/3);
  } else {
    viewer->resize(QApplication::desktop()->geometry().width()/2,
                  2*QApplication::desktop()->geometry().height()/3);
  }
  viewer->show();
}

namespace {
  KURL subjectToUrl( const QString & subject ) {
    return KFileDialog::getSaveURL( subject.mid( subject.findRev(':') + 1 )
                                            .stripWhiteSpace()
                                            .replace( QDir::separator(), '_' ),
                                    QString::null );
  }
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent, KMMessage * msg )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mOffset( 0 ),
    mTotalSize( msg ? msg->msgSize() : 0 )
{
  if ( !msg ) return;
  setDeletesItself( true );
  mMsgList.append( msg->getMsgSerNum() );
  mUrl = subjectToUrl( msg->subject() );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent,
                                    const QPtrList<KMMsgBase> &msgList )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mOffset( 0 ),
    mTotalSize( 0 )
{
  if (!msgList.getFirst())
    return;
  setDeletesItself( true );
  KMMsgBase *msgBase = msgList.getFirst();

  // We operate on serNums and not the KMMsgBase pointers, as those can
  // change, or become invalid when changing the current message, switching
  // folders, etc.
  QPtrListIterator<KMMsgBase> it(msgList);
  while ( it.current() ) {
    mMsgList.append( (*it)->getMsgSerNum() );
    mTotalSize += (*it)->msgSize();
    if ((*it)->parent() != 0)
      (*it)->parent()->open();
    ++it;
  }
  mMsgListIndex = 0;
  mUrl = subjectToUrl( msgBase->subject() );
}

KURL KMSaveMsgCommand::url()
{
  return mUrl;
}

void KMSaveMsgCommand::execute()
{
  mJob = KIO::put( mUrl, -1, false, false );
  mJob->slotTotalSize( mTotalSize );
  mJob->setAsyncDataEnabled( true );
  mJob->setReportDataSent( true );
  connect(mJob, SIGNAL(dataReq(KIO::Job*, QByteArray &)),
    SLOT(slotSaveDataReq()));
  connect(mJob, SIGNAL(result(KIO::Job*)),
    SLOT(slotSaveResult(KIO::Job*)));
}

void KMSaveMsgCommand::slotSaveDataReq()
{
  int remainingBytes = mData.size() - mOffset;
  if ( remainingBytes > 0 ) {
    // eat leftovers first
    if ( remainingBytes > MAX_CHUNK_SIZE )
      remainingBytes = MAX_CHUNK_SIZE;

    QByteArray data;
    data.duplicate( mData.data() + mOffset, remainingBytes );
    mJob->sendAsyncData( data );
    mOffset += remainingBytes;
    return;
  }
  // No leftovers, process next message.
  if ( mMsgListIndex < mMsgList.size() ) {
    KMMessage *msg = 0;
    int idx = -1;
    KMFolder * p = 0;
    kmkernel->msgDict()->getLocation( mMsgList[mMsgListIndex], &p, &idx );
    assert( p );
    assert( idx >= 0 );
    msg = p->getMsg(idx);

    if (msg->transferInProgress()) {
      QByteArray data = QByteArray();
      mJob->sendAsyncData( data );
    }
    msg->setTransferInProgress( true );
    if (msg->isComplete() ) {
      slotMessageRetrievedForSaving(msg);
    } else {
      // retrieve Message first
      if (msg->parent()  && !msg->isComplete() ) {
        FolderJob *job = msg->parent()->createJob(msg);
        connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            this, SLOT(slotMessageRetrievedForSaving(KMMessage*)));
        job->start();
      }
    }
  } else {
    // No more messages. Tell the putjob we are done.
    QByteArray data = QByteArray();
    mJob->sendAsyncData( data );
  }
}

void KMSaveMsgCommand::slotMessageRetrievedForSaving(KMMessage *msg)
{
  QCString str( msg->fromEmail() );
  if ( str.isEmpty() )
    str = "unknown@unknown.invalid";
  str = "From " + str + " " + msg->dateShortStr() + "\n";
  str += KMFolderMbox::escapeFrom( msg->asString() );
  str += "\n";
  msg->setTransferInProgress(false);

  mData = str;
  mData.resize(mData.size() - 1);
  mOffset = 0;
  QByteArray data;
  int size;
  // Unless it is great than 64 k send the whole message. kio buffers for us.
  if( mData.size() > (unsigned int) MAX_CHUNK_SIZE )
    size = MAX_CHUNK_SIZE;
  else
    size = mData.size();

  data.duplicate( mData, size );
  mJob->sendAsyncData( data );
  mOffset += size;
  ++mMsgListIndex;
  // Get rid of the message.
  if (msg->parent()) {
    int idx = -1;
    KMFolder * p = 0;
    kmkernel->msgDict()->getLocation( msg, &p, &idx );
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
    p->close();
  }
}

void KMSaveMsgCommand::slotSaveResult(KIO::Job *job)
{
  if (job->error())
  {
    if (job->error() == KIO::ERR_FILE_ALREADY_EXIST)
    {
      if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?")
        .arg(mUrl.prettyURL()), i18n("Save to File"), i18n("&Replace"))
        == KMessageBox::Continue) {
        mOffset = 0;

        mJob = KIO::put( mUrl, -1, true, false );
        mJob->slotTotalSize( mTotalSize );
        mJob->setAsyncDataEnabled( true );
        mJob->setReportDataSent( true );
        connect(mJob, SIGNAL(dataReq(KIO::Job*, QByteArray &)),
            SLOT(slotSaveDataReq()));
        connect(mJob, SIGNAL(result(KIO::Job*)),
            SLOT(slotSaveResult(KIO::Job*)));
      }
    }
    else
    {
      job->showErrorDialog();
      delete this;
    }
  } else {
    delete this;
  }
}

//TODO: ReplyTo, NoQuoteReplyTo, ReplyList, ReplyToAll, ReplyAuthor
//      are all similar and should be factored
KMReplyToCommand::KMReplyToCommand( QWidget *parent, KMMessage *msg,
                                    const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

void KMReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplySmart, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();
}


KMNoQuoteReplyToCommand::KMNoQuoteReplyToCommand( QWidget *parent,
                                                  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

void KMNoQuoteReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplySmart, "", TRUE);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();
}


KMReplyListCommand::KMReplyListCommand( QWidget *parent,
  KMMessage *msg, const QString &selection )
 : KMCommand( parent, msg ), mSelection( selection )
{
}

void KMReplyListCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyList, mSelection);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();
}


KMReplyToAllCommand::KMReplyToAllCommand( QWidget *parent,
  KMMessage *msg, const QString &selection )
  :KMCommand( parent, msg ), mSelection( selection )
{
}

void KMReplyToAllCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyAll, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();
}


KMReplyAuthorCommand::KMReplyAuthorCommand( QWidget *parent, KMMessage *msg,
                                            const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

void KMReplyAuthorCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyAuthor, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();
}


KMForwardCommand::KMForwardCommand( QWidget *parent,
  const QPtrList<KMMsgBase> &msgList, uint identity )
  : KMCommand( parent, msgList ),
    mParent( parent ),
    mIdentity( identity )
{
}

KMForwardCommand::KMForwardCommand( QWidget *parent, KMMessage *msg,
                                    uint identity )
  : KMCommand( parent, msg ),
    mParent( parent ),
    mIdentity( identity )
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
      uint id = 0;
      KMMessage *fwdMsg = new KMMessage;
      KMMessagePart *msgPart = new KMMessagePart;
      QString msgPartText;
      int msgCnt = 0; // incase there are some we can't forward for some reason

      // dummy header initialization; initialization with the correct identity
      // is done below
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->mMsg->Headers().ContentType().CreateBoundary(1);
      QCString boundary( fwdMsg->mMsg->Headers().ContentType().Boundary().c_str() );
      msgPartText = i18n("\nThis is a MIME digest forward. The content of the"
                         " message is contained in the attachment(s).\n\n\n");
      // iterate through all the messages to be forwarded
      for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
        // set the identity
        if (id == 0)
          id = msg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();
        // set the part header
        msgPartText += "--";
        msgPartText += QString::fromLatin1( boundary );
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
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      fwdMsg->initHeader(id);
      msgPartText += "--";
      msgPartText += QString::fromLatin1( boundary );
      msgPartText += "--\n";
      QCString tmp;
      msgPart->setTypeStr("MULTIPART");
      tmp.sprintf( "Digest; boundary=\"%s\"", boundary.data() );
      msgPart->setSubtypeStr( tmp );
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(QCString(msgPartText.ascii()));
      KCursorSaver busy(KBusyPtr::busy());
      win = new KMComposeWin(fwdMsg, id);
      win->addAttach(msgPart);
      win->show();
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
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      KMMessage *fwdMsg = new KMMessage;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");
      fwdMsg->setBody(msgText);

      for (KMMessage *msg = linklist.first(); msg; msg = linklist.next())
        fwdMsg->link(msg, KMMsgStatusForwarded);

      KCursorSaver busy(KBusyPtr::busy());
      win = new KMComposeWin(fwdMsg, id);
      win->setCharset("");
      win->show();
      return;
    }
  }

  // forward a single message at most.

  KMMessage *msg = msgList.getFirst();
  if (!msg || !msg->codec()) return;

  KCursorSaver busy(KBusyPtr::busy());
  win = new KMComposeWin(msg->createForward());
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
}


KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
  const QPtrList<KMMsgBase> &msgList, uint identity, KMComposeWin *win )
  : KMCommand( parent, msgList ), mIdentity( identity ),
    mWin( QGuardedPtr< KMComposeWin >( win ))
{
}

KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
  KMMessage * msg, uint identity, KMComposeWin *win )
  : KMCommand( parent, msg ), mIdentity( identity ),
    mWin( QGuardedPtr< KMComposeWin >( win ))
{
}

void KMForwardAttachedCommand::execute()
{
  QPtrList<KMMessage> msgList = retrievedMsgs();
  KMMessage *fwdMsg = new KMMessage;

  if (msgList.count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    fwdMsg->initHeader(mIdentity);
  }
  else if (msgList.count() == 1) {
    KMMessage *msg = msgList.getFirst();
    fwdMsg->initFromMessage(msg);
    fwdMsg->setSubject( msg->forwardSubject() );
  }

  fwdMsg->setAutomaticFields(true);

  KCursorSaver busy(KBusyPtr::busy());
  if (!mWin)
    mWin = new KMComposeWin(fwdMsg, mIdentity);

  // iterate through all the messages to be forwarded
  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
    // set the part
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setTypeStr("message");
    msgPart->setSubtypeStr("rfc822");
    msgPart->setCharset(msg->charset());
    msgPart->setName("forwarded message");
    msgPart->setContentDescription(msg->from()+": "+msg->subject());
    msgPart->setContentDisposition( "inline" );
    // THIS HAS TO BE AFTER setCte()!!!!
    QValueList<int> dummy;
    msgPart->setBodyAndGuessCte(msg->asString(), dummy, true);
    msgPart->setCharset("");

    fwdMsg->link(msg, KMMsgStatusForwarded);
    mWin->addAttach(msgPart);
  }

  mWin->show();
}


KMRedirectCommand::KMRedirectCommand( QWidget *parent,
  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

void KMRedirectCommand::execute()
{
  //TODO: move KMMessage::createRedirect to here
  KMComposeWin *win;
  KMMessage *msg = retrievedMessage();
  if (!msg || !msg->codec()) return;

  KCursorSaver busy(KBusyPtr::busy());
  win = new KMComposeWin();
  win->setMsg(msg->createRedirect(), FALSE);
  win->setCharset(msg->codec()->mimeName());
  win->show();
}


KMBounceCommand::KMBounceCommand( QWidget *parent,
  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

void KMBounceCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  KMMessage *newMsg = msg->createBounce( TRUE /* with UI */);
  if (newMsg)
    kmkernel->msgSender()->send(newMsg, kmkernel->msgSender()->sendImmediate());
}


KMPrintCommand::KMPrintCommand( QWidget *parent,
  KMMessage *msg, bool htmlOverride )
  : KMCommand( parent, msg ), mHtmlOverride( htmlOverride )
{
}

void KMPrintCommand::execute()
{
  KMReaderWin printWin( 0, 0, 0 );
  printWin.setPrinting(TRUE);
  printWin.readConfig();
  printWin.setHtmlOverride( mHtmlOverride );
  printWin.setMsg(retrievedMessage(), TRUE);
  printWin.printMsg();
}


KMSetStatusCommand::KMSetStatusCommand( KMMsgStatus status,
  const QValueList<Q_UINT32> &serNums, bool toggle )
  : mStatus( status ), mSerNums( serNums ), mToggle( toggle )
{
}

void KMSetStatusCommand::execute()
{
  QValueListIterator<Q_UINT32> it;
  int idx = -1;
  KMFolder *folder = 0;
  bool parentStatus = false;

  // Toggle actions on threads toggle the whole thread
  // depending on the state of the parent.
  if (mToggle) {
    KMMsgBase *msg;
    kmkernel->msgDict()->getLocation( *mSerNums.begin(), &folder, &idx );
    if (folder) {
      msg = folder->getMsgBase(idx);
      if (msg && (msg->status()&mStatus))
        parentStatus = true;
      else
        parentStatus = false;
    }
  }
  QMap< KMFolder*, QValueList<int> > folderMap;
  for ( it = mSerNums.begin(); it != mSerNums.end(); ++it ) {
    kmkernel->msgDict()->getLocation( *it, &folder, &idx );
    if (folder) {
      if (mToggle) {
        KMMsgBase *msg = folder->getMsgBase(idx);
        // check if we are already at the target toggle state
        if (msg) {
          bool myStatus;
          if (msg->status()&mStatus)
            myStatus = true;
          else
            myStatus = false;
          if (myStatus != parentStatus)
            continue;
        }
      }
      /* Collect the ids for each folder in a separate list and
         send them off in one go at the end. */
      folderMap[folder].append(idx);
    }
  }
  QMapIterator< KMFolder*, QValueList<int> > it2 = folderMap.begin();
  while ( it2 != folderMap.end() ) {
     KMFolder *f = it2.key();
     f->setStatus( (*it2), mStatus, mToggle );
     ++it2;
  }
}


KMFilterCommand::KMFilterCommand( const QCString &field, const QString &value )
  : mField( field ), mValue( value )
{
}

void KMFilterCommand::execute()
{
  kmkernel->filterMgr()->createFilter( mField, mValue );
}


KMMailingListFilterCommand::KMMailingListFilterCommand( QWidget *parent,
  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMFilterActionCommand::KMFilterActionCommand( QWidget *parent,
                                              const QPtrList<KMMsgBase> &msgList,
                                              KMFilter *filter )
  : KMCommand( parent, msgList ), mFilter( filter  )
{
}

void KMFilterActionCommand::execute()
{
  QPtrList<KMMessage> msgList = retrievedMsgs();

  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next())
    kmkernel->filterMgr()->tempOpenFolder(msg->parent());

  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next()) {
    msg->setTransferInProgress(false);

    int filterResult = kmkernel->filterMgr()->process(msg, mFilter);
    if (filterResult == 2) {
      // something went horribly wrong (out of space?)
      perror("Critical error");
      kmkernel->emergencyExit( i18n("Not enough free disk space?" ));
    }
    msg->setTransferInProgress(true);
  }
}


KMMetaFilterActionCommand::KMMetaFilterActionCommand( KMFilter *filter,
                                                      KMHeaders *headers,
                                                      KMMainWidget *main )
    : QObject( main ),
      mFilter( filter ), mHeaders( headers ), mMainWidget( main )
{
}

void KMMetaFilterActionCommand::start()
{
#if 0 // use action scheduler
  KMFilterMgr::FilterSet set = KMFilterMgr::All;
  QPtrList<KMFilter> filters;
  filters.append( mFilter );
  ActionScheduler *scheduler = new ActionScheduler( set, filters, mHeaders );
  scheduler->setAlwaysMatch( true );
  scheduler->setAutoDestruct( true );

  int contentX, contentY;
  KMHeaderItem *nextItem = mHeaders->prepareMove( &contentX, &contentY );
  QPtrList<KMMsgBase> msgList = *mHeaders->selectedMsgs(true);
  mHeaders->finalizeMove( nextItem, contentX, contentY );


  for (KMMsgBase *msg = msgList.first(); msg; msg = msgList.next())
    scheduler->execFilters( msg );
#else
  KMCommand *filterCommand = new KMFilterActionCommand( mMainWidget,
  *mHeaders->selectedMsgs(), mFilter);
  filterCommand->start();
#endif
}


void KMMailingListFilterCommand::execute()
{
  QCString name;
  QString value;
  KMMessage *msg = retrievedMessage();
  if (!msg)
    return;

  if (!KMMLInfo::name( msg, name, value ).isNull())
    kmkernel->filterMgr()->createFilter( name, value );
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

  if (!kmkernel->imapFolderMgr()->dir().first()) {
    KMMenuCommand::makeFolderMenu(  &kmkernel->folderMgr()->dir(), move,
      receiver, aMenuToFolder, menu );
  } else {
    // operate on top-level items
    QPopupMenu* subMenu = new QPopupMenu(menu);
    subMenu = KMMenuCommand::makeFolderMenu(  &kmkernel->folderMgr()->dir(),
        move, receiver, aMenuToFolder, subMenu );
    menu->insertItem( i18n( "Local Folders" ), subMenu );
    KMFolderDir* fdir = &kmkernel->imapFolderMgr()->dir();
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
      menuId = menu->insertItem(i18n("Move to This Folder"));
    else
      menuId = menu->insertItem(i18n("Copy to This Folder"));
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
    label.replace("&","&&");
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


KMCopyCommand::KMCopyCommand( KMFolder* destFolder,
                              const QPtrList<KMMsgBase> &msgList )
  :mDestFolder( destFolder ), mMsgList( msgList )
{
}

KMCopyCommand::KMCopyCommand( KMFolder* destFolder, KMMessage * msg )
  :mDestFolder( destFolder )
{
  mMsgList.append( &msg->toMsgBase() );
}

void KMCopyCommand::execute()
{
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int idx = -1;
  bool isMessage;
  QPtrList<KMMessage> list;

  KCursorSaver busy(KBusyPtr::busy());

  for (msgBase = mMsgList.first(); msgBase; msgBase = mMsgList.next() )
  {
    KMFolder *srcFolder = msgBase->parent();
    if (isMessage = msgBase->isMessage())
    {
      msg = static_cast<KMMessage*>(msgBase);
    } else {
      idx = srcFolder->find(msgBase);
      assert(idx != -1);
      msg = srcFolder->getMsg(idx);
    }

    if (srcFolder &&
        (srcFolder->folderType()== KMFolderTypeImap) &&
        (mDestFolder->folderType() == KMFolderTypeImap) &&
        (static_cast<KMFolderImap*>(srcFolder)->account() ==
         static_cast<KMFolderImap*>(mDestFolder)->account()))
    {
      list.append(msg);
    } else {
      newMsg = new KMMessage;
      newMsg->fromString(msg->asString());
      newMsg->setStatus(msg->status());
      newMsg->setComplete(msg->isComplete());

      if (srcFolder && !newMsg->isComplete())
      {
        newMsg->setParent(msg->parent());
        FolderJob *job = srcFolder->createJob(newMsg);
        connect(job, SIGNAL(messageRetrieved(KMMessage*)),
                mDestFolder, SLOT(reallyAddCopyOfMsg(KMMessage*)));
        // msg musn't be deleted
        newMsg->setTransferInProgress(true);
        job->start();
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
      srcFolder->unGetMsg( idx );
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

}


KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              const QPtrList<KMMsgBase> &msgList)
  :mDestFolder( destFolder ), mMsgList( msgList )
{
  setDeletesItself( true );
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMessage *msg )
  :mDestFolder( destFolder )
{
  setDeletesItself( true );
  mMsgList.append( &msg->toMsgBase() );
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMsgBase *msgBase )
  :mDestFolder( destFolder )
{
  setDeletesItself( true );
  mMsgList.append( msgBase );
}

void KMMoveCommand::execute()
{
  typedef QMap< KMFolder*, QPtrList<KMMessage>* > FolderToMessageListMap;
  FolderToMessageListMap folderDeleteList;

  if (mDestFolder && mDestFolder->open() != 0)
    return;
  KCursorSaver busy(KBusyPtr::busy());

  KMMessage *msg;
  KMMsgBase *msgBase;
  int rc = 0;
  int index;
  QPtrList<KMMessage> list;
  int undoId = -1;

  if (mDestFolder) {
    connect (mDestFolder, SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
             this, SLOT(slotMsgAddedToDestFolder(KMFolder*, Q_UINT32)));

  }

  for (msgBase=mMsgList.first(); msgBase && !rc; msgBase=mMsgList.next()) {
    KMFolder *srcFolder = msgBase->parent();
    if (srcFolder == mDestFolder)
      continue;
    bool undo = msgBase->enableUndo();
    int idx = srcFolder->find(msgBase);
    assert(idx != -1);
    if ( msgBase->isMessage() )
      msg = static_cast<KMMessage*>(msgBase);
    else
      msg = srcFolder->getMsg(idx);

    if ( msg->transferInProgress() &&
         srcFolder->folderType() == KMFolderTypeImap )
    {
      // cancel the download
      msg->setTransferInProgress( false, true );
      static_cast<KMFolderImap*>(srcFolder)->ignoreJobsForMessage( msg );
    }

    if (mDestFolder) {
      mLostBoys.append(msg->getMsgSerNum());
      if (mDestFolder->folderType() == KMFolderTypeImap) {
        /* If we are moving to an imap folder, connect to it's completed
         * siganl so we notice when all the mails should have showed up in it
         * but haven't for some reason. */
        connect (mDestFolder, SIGNAL(folderComplete( KMFolderImap*, bool )),
            this, SLOT(slotImapFolderCompleted( KMFolderImap*, bool )));
        list.append(msg);
      } else {
        // We are moving to a local folder.
        rc = mDestFolder->moveMsg(msg, &index);
        if (rc == 0 && index != -1) {
          KMMsgBase *mb = mDestFolder->unGetMsg( mDestFolder->count() - 1 );
          if (undo && mb)
          {
            if ( undoId == -1 )
              undoId = kmkernel->undoStack()->newUndoAction( srcFolder, mDestFolder );
            kmkernel->undoStack()->addMsgToAction( undoId, mb->getMsgSerNum() );
          }
        } else if (rc != 0) {
          // Something  went wrong. Stop processing here, it is likely that the
          // other moves would fail as well.
          emit completed( false);
          deleteLater();
          return;
        }
      }
    } else {
      // really delete messages that are already in the trash folder or if
      // we are really, really deleting, not just moving to trash
      if (srcFolder->folderType() == KMFolderTypeImap) {
        if (!folderDeleteList[srcFolder])
          folderDeleteList[srcFolder] = new QPtrList<KMMessage>;
        folderDeleteList[srcFolder]->append( msg );
      } else {
        srcFolder->removeMsg(idx);
        delete msg;
      }
    }
  }
  if (!list.isEmpty() && mDestFolder) {
       mDestFolder->moveMsg(list, &index);
  } else {
    FolderToMessageListMap::Iterator it;
    for ( it = folderDeleteList.begin(); it != folderDeleteList.end(); ++it ) {
      it.key()->removeMsg(*it.data());
      delete it.data();
    }
    /* The list is empty, which means that either all messages were to be
     * deleted, which is done above, or all of them were already in this folder.
     * In both cases make sure a completed() signal is emitted nonetheless. */
    KMFolder *srcFolder = 0;
    if ( mMsgList.first() ) {
      srcFolder = mMsgList.first()->parent();
      if ( mDestFolder && mDestFolder == srcFolder ) {
        emit completed( true );
        deleteLater();
      }
    }
    if ( !mDestFolder ) {
      emit completed( true );
      deleteLater();
    }
  }
}

void KMMoveCommand::slotImapFolderCompleted(KMFolderImap *, bool success)
{
  if ( success ) {
    // the folder was checked successfully but we were still called, so check
    // if we are still waiting for messages to show up. If so, uidValidity
    // changed, or something else went wrong. Clean up.

    /* Unfortunately older UW imap servers change uid validity for each put job.
     * Yes, it is really that broken. *sigh* So we cannot report error here, I guess. */
    if ( !mLostBoys.isEmpty() ) {
      kdDebug(5006) <<  "### Not all moved messages reported back that they were " << endl
                    <<  "### added to the target folder. Did uidValidity change? " << endl;
    }
  } else {
    // Should we inform the user here or leave that to the caller?
  }
  emit completed( success );
  deleteLater();
}

void KMMoveCommand::slotMsgAddedToDestFolder(KMFolder *folder, Q_UINT32 serNum)
{
  if (folder != mDestFolder) {
    kdDebug(5006) << "KMMoveCommand::msgAddedToDestFolder different "
                     "folder or invalid serial number." << endl;
    return;
  }
  mLostBoys.remove(serNum);
  if ( mLostBoys.isEmpty() ) {
    // we are done. All messages transferred to the host succesfully
    if (mDestFolder && mDestFolder->folderType() != KMFolderTypeImap) {
      mDestFolder->sync();
    }
    emit completed( true );
    deleteLater();
  }
}

// srcFolder doesn't make much sense for searchFolders
KMDeleteMsgCommand::KMDeleteMsgCommand( KMFolder* srcFolder,
  const QPtrList<KMMsgBase> &msgList )
:KMMoveCommand(findTrashFolder( srcFolder ), msgList)
{
}

KMDeleteMsgCommand::KMDeleteMsgCommand( KMFolder* srcFolder, KMMessage * msg )
:KMMoveCommand(findTrashFolder( srcFolder ), msg)
{
}


KMFolder * KMDeleteMsgCommand::findTrashFolder( KMFolder * folder )
{
  if (folder->folderType()== KMFolderTypeImap)
  {
    KMFolderImap* fi = static_cast<KMFolderImap*> (folder);
    QString trashStr = fi->account()->trash();
    KMFolder* trash = kmkernel->imapFolderMgr()->findIdString( trashStr );
    if (!trash) trash = kmkernel->trashFolder();
    if (folder != trash)
      return trash;
  } else {
    if (folder != kmkernel->trashFolder())
      // move to trash folder
      return kmkernel->trashFolder();
  }
  return 0;
}

KMUrlClickedCommand::KMUrlClickedCommand( const KURL &url, uint identity,
  KMReaderWin *readerWin, bool htmlPref, KMMainWidget *mainWidget )
  :mUrl( url ), mIdentity( identity ), mReaderWin( readerWin ),
   mHtmlPref( htmlPref ), mMainWidget( mainWidget )
{
}

void KMUrlClickedCommand::execute()
{
  KMComposeWin *win;
  KMMessage* msg;

  if (mUrl.protocol() == "mailto")
  {
    msg = new KMMessage;
    msg->initHeader(mIdentity);
    msg->setCharset("utf-8");
    msg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );
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

    win = new KMComposeWin(msg, mIdentity);
    win->setCharset("", TRUE);
    win->show();
  }
  else if ((mUrl.protocol() == "http") || (mUrl.protocol() == "https") ||
           (mUrl.protocol() == "ftp") || (mUrl.protocol() == "file") ||
           (mUrl.protocol() == "ftps") || (mUrl.protocol() == "sftp" ) ||
           (mUrl.protocol() == "help") || (mUrl.protocol() == "vnc") ||
           (mUrl.protocol() == "smb"))
  {
    if (mMainWidget)
      mMainWidget->statusMsg( i18n("Opening URL..."));
    KMimeType::Ptr mime = KMimeType::findByURL( mUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-msdos-program" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18n( "<qt>Do you really want to execute <b>%1</b>?</qt>" )
        .arg( mUrl.prettyURL() ) ) != KMessageBox::Yes) return;
    }
    (void) new KRun( mUrl );
  }
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, KMMessage *msg )
  : KMCommand( parent, msg ), mParent( parent ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const QPtrList<KMMsgBase>& msgs )
  : KMCommand( parent, msgs ), mParent( parent ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, QPtrList<partNode>& attachments,
                                                    KMMessage *msg, bool encoded )
  : KMCommand( parent, msg ), mParent( parent ), mAttachments( attachments ), mEncoded( encoded )
{
  // do not load the complete message but only parts
  msg->setComplete( true );
  setDeletesItself( true );
}

void KMSaveAttachmentsCommand::execute()
{
  if ( mAttachments.count() > 0 )
  {
    saveAll( mAttachments );
    return;
  }
  KMMessage *msg = 0;
  QPtrList<KMMessage> lst = retrievedMsgs();
  QPtrListIterator<KMMessage> itr( lst );

  while ( itr.current() ) {
    msg = itr.current();
    ++itr;
    QCString type = msg->typeStr();

    int mainType    = msg->type();
    int mainSubType = msg->subtype();
    DwBodyPart* mainBody = 0;
    DwBodyPart* firstBodyPart = msg->getFirstDwBodyPart();
    if( !firstBodyPart ) {
      // ATTENTION: This definitely /should/ be optimized.
      //            Copying the message text into a new body part
      //            surely is not the most efficient way to go.
      //            I decided to do so for being able to get a
      //            solution working for old style (== non MIME)
      //            mails without spending much time on implementing.
      //            During code revisal when switching to KMime
      //            all this will probably disappear anyway (or it
      //            will be optimized, resp.).       (khz, 6.12.2001)
      kdDebug(5006) << "*no* first body part found, creating one from Message" << endl;
      mainBody = new DwBodyPart( msg->asDwString(), 0 );
      mainBody->Parse();
    }
    partNode *rootNode = new partNode( mainBody, mainType, mainSubType, true );
    rootNode->setFromAddress( msg->from() );

    if ( firstBodyPart ) {
      partNode* curNode = new partNode(firstBodyPart);
      rootNode->setFirstChild( curNode );
      curNode->buildObjectTree();
    }
    parse( rootNode );
  }
}

void KMSaveAttachmentsCommand::parse( partNode *rootNode )
{
  QPtrList<partNode> attachments;
  for( partNode *child = rootNode; child; child = child->firstChild() ) {
    for( partNode *tmp = child; tmp; tmp = tmp->nextSibling() ) {
      attachments.append( tmp );
    }
  }
  saveAll( attachments );
}

void KMSaveAttachmentsCommand::saveAll( const QPtrList<partNode>& attachments )
{
  if ( attachments.isEmpty() ) {
    KMessageBox::information( 0, i18n("Found no attachments to save.") );
    return;
  }
  mAttachments = attachments;
  // load all parts
  KMLoadPartsCommand *command = new KMLoadPartsCommand( mAttachments, retrievedMessage() );
  connect( command, SIGNAL( partsRetrieved() ),
      this, SLOT( slotSaveAll() ) );
  command->start();
}

void KMSaveAttachmentsCommand::slotSaveAll()
{
  QPtrListIterator<partNode> itr( mAttachments );

  QString dir, file;
  if ( mAttachments.count() > 1 )
  {
    // get the dir
    KFileDialog fdlg( QString::null, QString::null, mParent, 0, true );
    fdlg.setMode( (unsigned int) KFile::Directory );
    if ( !fdlg.exec() ) return;
    dir = fdlg.selectedURL().path();
  }
  else {
    // only one item, get the desired filename
    // replace all ':' with '_' because ':' isn't allowed on FAT volumes
    QString s =
      (*itr)->msgPart().fileName().stripWhiteSpace().replace( ':', '_' );
    if ( s.isEmpty() )
      s = (*itr)->msgPart().name().stripWhiteSpace().replace( ':', '_' );
    if ( s.isEmpty() )
      s = "unnamed"; // ### this should probably be i18n'ed
    file = KFileDialog::getSaveFileName( s, QString::null, mParent,
                                         QString::null );
  }

  while ( itr.current() ) {
    QString s;
    QString filename;
    if ( !dir.isEmpty() ) {
      s = (*itr)->msgPart().fileName().stripWhiteSpace().replace( ':', '_' );
      if ( s.isEmpty() )
        s = (*itr)->msgPart().name().stripWhiteSpace().replace( ':', '_' );
      // Check if it has the Content-Disposition... filename: header
      // to make sure it's an actual attachment
      // we can't do the check earlier as we first need to load the mimeheader
      // for imap attachments to do this check
      if ( s.isEmpty() ) {
        ++itr;
        continue;
      }
      filename = dir + "/" + s;
    }
    else
      filename = file;

    if( !filename.isEmpty() ) {
      if( QFile::exists( filename ) ) {
        if( KMessageBox::warningYesNo( mParent,
                                       i18n( "A file named %1 already exists. Do you want to overwrite it?" ).arg( s.isEmpty() ? filename : s ),
                                       i18n( "KMail Warning" ) ) ==
            KMessageBox::No ) {
          ++itr;
          continue;
        }
      }
      saveItem( itr.current(), filename );
    }
    ++itr;
  }
}

void KMSaveAttachmentsCommand::saveItem( partNode *node, const QString& filename )
{
  if ( node && !filename.isEmpty() ) {
    bool bSaveEncrypted = false;
    bool bEncryptedParts = node->encryptionState() != KMMsgNotEncrypted;
    if( bEncryptedParts )
      if( KMessageBox::questionYesNo( mParent,
                                      i18n( "This part of the message is encrypted. Do you want to keep the encryption when saving?" ),
                                      i18n( "KMail Question" ) ) ==
          KMessageBox::Yes )
        bSaveEncrypted = true;

    bool bSaveWithSig = true;
    if( node->signatureState() != KMMsgNotSigned )
      if( KMessageBox::questionYesNo( mParent,
                                      i18n( "This part of the message is signed. Do you want to keep the signature when saving?" ),
                                      i18n( "KMail Question" ) ) !=
          KMessageBox::Yes )
        bSaveWithSig = false;

    QFile file( filename );
    if( file.open( IO_WriteOnly ) ) {
      if ( mEncoded )
      {
        // This does not decode the Message Content-Transfer-Encoding
        // but saves the _original_ content of the message part
        QDataStream ds( &file );
        QCString cstr( node->msgPart().body() );
        ds.writeRawBytes( cstr, cstr.size() );
      }
      else
      {
        QDataStream ds( &file );
        if( (bSaveEncrypted || !bEncryptedParts) && bSaveWithSig ) {
          QByteArray cstr = node->msgPart().bodyDecodedBinary();
          size_t size = cstr.size();
          if ( node->msgPart().type() == DwMime::kTypeText ) {
            // convert CRLF to LF before writing text attachments to disk
            size = KMFolder::crlf2lf( cstr.data(), size );
          }
          ds.writeRawBytes( cstr.data(), size );
        }
      }
      file.close();
    } else
      // FIXME: After string freeze is over:
      // KMessageBox::error( mParent,
      //                     i18n( "%1 is detailed error description",
      //                           "Could not write the file:\n%1" )
      //                      .arg( QString::fromLocal8Bit( strerror( errno ) ) ),
      //                     i18n( "KMail Error" ) );
      KMessageBox::error( mParent,
                          i18n( "Could not write the file." ) + "\n"
                          + QString::fromLocal8Bit( strerror( errno ) ),
                          i18n( "KMail Error" ) );
  }
}

KMLoadPartsCommand::KMLoadPartsCommand( QPtrList<partNode>& parts, KMMessage *msg )
    : mParts( parts ), mNeedsRetrieval( 0 ), mMsg( msg )
{
}

KMLoadPartsCommand::KMLoadPartsCommand( partNode* node, KMMessage *msg )
    : mNeedsRetrieval( 0 ), mMsg( msg )
{
  mParts.append( node );
}

void KMLoadPartsCommand::start()
{
  QPtrListIterator<partNode> it( mParts );
  while ( it.current() )
  {
    if ( !it.current()->msgPart().isComplete() &&
         !it.current()->msgPart().partSpecifier().isEmpty() )
    {
      // incomplete part so retrieve it first
      ++mNeedsRetrieval;
      KMFolder* curFolder = mMsg->parent();
      if ( curFolder )
      {
        FolderJob *job = curFolder->createJob( mMsg, FolderJob::tGetMessage,
            0, it.current()->msgPart().partSpecifier() );
        connect( job, SIGNAL(messageUpdated(KMMessage*, QString)),
            this, SLOT(slotPartRetrieved(KMMessage*, QString)) );
        job->start();
      }
    }
    ++it;
  }
  if ( mNeedsRetrieval == 0 )
    execute();
}

void KMLoadPartsCommand::slotPartRetrieved( KMMessage* msg, QString partSpecifier )
{
  DwBodyPart* part = msg->findDwBodyPart( msg->getFirstDwBodyPart(), partSpecifier );
  if ( part )
  {
    // update the DwBodyPart in the partNode
    QPtrListIterator<partNode> it( mParts );
    while ( it.current() )
    {
      if ( it.current()->dwPart() == part )
        it.current()->setDwPart( part );
      ++it;
    }
  } else
    kdWarning(5006) << "KMLoadPartsCommand::slotPartRetrieved - could not find bodypart!" << endl;
  --mNeedsRetrieval;
  if ( mNeedsRetrieval == 0 )
    execute();
}

void KMLoadPartsCommand::execute()
{
  emit partsRetrieved();
  delete this;
}

