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

#include "kmcommands.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>
#include <mimelib/string.h>
#include <kapplication.h>
#include <dcopclient.h>

#include <qtextcodec.h>

#include <libkdepim/email.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <kabc/stdaddressbook.h>
#include <kabc/addresseelist.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/browserextension.h>
#include <kprogress.h>
#include <krun.h>
#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include "actionscheduler.h"
using KMail::ActionScheduler;
#include "mailinglist-magic.h"
#include "kmaddrbook.h"
#include <kaddrbook.h>
#include "kmcomposewin.h"
#include "kmfiltermgr.h"
#include "kmfoldermbox.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "kmsender.h"
#include "undostack.h"
#include "kcursorsaver.h"
#include "partNode.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
using KMail::FolderJob;
#include "mailsourceviewer.h"
using KMail::MailSourceViewer;
#include "kmreadermainwin.h"
#include "secondarywindow.h"
using KMail::SecondaryWindow;
#include "kimproxy.h"

#include "progressmanager.h"
using KPIM::ProgressManager;
using KPIM::ProgressItem;

#include "broadcaststatus.h"

#include "kmcommands.moc"

KMCommand::KMCommand( QWidget *parent )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
}

KMCommand::KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent ), mMsgList( msgList )
{
}

KMCommand::KMCommand( QWidget *parent, KMMsgBase *msgBase )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
  mMsgList.append( msgBase );
}

KMCommand::KMCommand( QWidget *parent, KMMessage *msg )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
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

KMCommand::Result KMCommand::result()
{
  if ( mResult == Undefined )
    kdDebug(5006) << k_funcinfo << "mResult is Undefined" << endl;
  return mResult;
}

void KMCommand::start()
{
  QTimer::singleShot( 0, this, SLOT( slotStart() ) );
}


const QPtrList<KMMessage> KMCommand::retrievedMsgs() const
{
  return mRetrievedMsgs;
}

KMMessage *KMCommand::retrievedMessage() const
{
  return mRetrievedMsgs.getFirst();
}

QWidget *KMCommand::parentWidget() const
{
  return mParent;
}

int KMCommand::mCountJobs = 0;

void KMCommand::slotStart()
{
  connect( this, SIGNAL( messagesTransfered( KMCommand::Result ) ),
           this, SLOT( slotPostTransfer( KMCommand::Result ) ) );
  kmkernel->filterMgr()->ref();

  if (mMsgList.find(0) != -1) {
      emit messagesTransfered( Failed );
      return;
  }

  if ((mMsgList.count() == 1) &&
      (mMsgList.getFirst()->isMessage()) &&
      (mMsgList.getFirst()->parent() == 0))
  {
    // Special case of operating on message that isn't in a folder
    mRetrievedMsgs.append((KMMessage*)mMsgList.getFirst());
    emit messagesTransfered( OK );
    return;
  }

  for (KMMsgBase *mb = mMsgList.first(); mb; mb = mMsgList.next())
    if (!mb->parent()) {
      emit messagesTransfered( Failed );
      return;
    } else {
      mFolders.append( mb->parent() );
      mb->parent()->open();
    }

  // transfer the selected messages first
  transferSelectedMsgs();
}

void KMCommand::slotPostTransfer( KMCommand::Result result )
{
  disconnect( this, SIGNAL( messagesTransfered( KMCommand::Result ) ),
              this, SLOT( slotPostTransfer( KMCommand::Result ) ) );
  if ( result == OK )
    result = execute();
  mResult = result;
  QPtrListIterator<KMMessage> it( mRetrievedMsgs );
  KMMessage* msg;
  while ( (msg = it.current()) != 0 )
  {
    ++it;
    if (msg->parent())
      msg->setTransferInProgress(false);
  }
  kmkernel->filterMgr()->deref();
  if ( !emitsCompletedItself() )
    emit completed( this );
  if ( !deletesItself() )
    delete this;
}

void KMCommand::transferSelectedMsgs()
{
  // make sure no other transfer is active
  if (KMCommand::mCountJobs > 0) {
    emit messagesTransfered( Failed );
    return;
  }

  bool complete = true;
  KMCommand::mCountJobs = 0;
  mCountMsgs = 0;
  mRetrievedMsgs.clear();
  mCountMsgs = mMsgList.count();
  uint totalSize = 0;
  // the KProgressDialog for the user-feedback. Only enable it if it's needed.
  // For some commands like KMSetStatusCommand it's not needed. Note, that
  // for some reason the KProgressDialog eats the MouseReleaseEvent (if a
  // command is executed after the MousePressEvent), cf. bug #71761.
  if ( mCountMsgs > 0 ) {
    mProgressDialog = new KProgressDialog(mParent, "transferProgress",
      i18n("Please wait"),
      i18n("Please wait while the message is transferred",
        "Please wait while the %n messages are transferred", mMsgList.count()),
      true);
    mProgressDialog->setMinimumDuration(1000);
  }
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

    if ( thisMsg->parent() && !thisMsg->isComplete() &&
         ( !mProgressDialog || !mProgressDialog->wasCancelled() ) )
    {
      kdDebug(5006)<<"### INCOMPLETE\n";
      // the message needs to be transferred first
      complete = false;
      KMCommand::mCountJobs++;
      FolderJob *job = thisMsg->parent()->createJob(thisMsg);
      job->setCancellable( false );
      totalSize += thisMsg->msgSizeServer();
      // emitted when the message was transferred successfully
      connect(job, SIGNAL(messageRetrieved(KMMessage*)),
              this, SLOT(slotMsgTransfered(KMMessage*)));
      // emitted when the job is destroyed
      connect(job, SIGNAL(finished()),
              this, SLOT(slotJobFinished()));
      connect(job, SIGNAL(progress(unsigned long, unsigned long)),
              this, SLOT(slotProgress(unsigned long, unsigned long)));
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
    emit messagesTransfered( OK );
  } else {
    // wait for the transfer and tell the progressBar the necessary steps
    if ( mProgressDialog ) {
      connect(mProgressDialog, SIGNAL(cancelClicked()),
              this, SLOT(slotTransferCancelled()));
      mProgressDialog->progressBar()->setTotalSteps(totalSize);
    }
  }
}

void KMCommand::slotMsgTransfered(KMMessage* msg)
{
  if ( mProgressDialog && mProgressDialog->wasCancelled() ) {
    emit messagesTransfered( Canceled );
    return;
  }

  // save the complete messages
  mRetrievedMsgs.append(msg);
}

void KMCommand::slotProgress( unsigned long done, unsigned long /*total*/ )
{
  mProgressDialog->progressBar()->setProgress( done );
}

void KMCommand::slotJobFinished()
{
  // the job is finished (with / without error)
  KMCommand::mCountJobs--;

  if ( mProgressDialog && mProgressDialog->wasCancelled() ) return;

  if ( (mCountMsgs - static_cast<int>(mRetrievedMsgs.count())) > KMCommand::mCountJobs )
  {
    // the message wasn't retrieved before => error
    if ( mProgressDialog )
      mProgressDialog->hide();
    slotTransferCancelled();
    return;
  }
  // update the progressbar
  if ( mProgressDialog ) {
    mProgressDialog->setLabel(i18n("Please wait while the message is transferred",
          "Please wait while the %n messages are transferred", KMCommand::mCountJobs));
  }
  if (KMCommand::mCountJobs == 0)
  {
    // all done
    delete mProgressDialog;
    emit messagesTransfered( OK );
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
  emit messagesTransfered( Canceled );
}

void KMCommand::keepFolderOpen( KMFolder *folder )
{
  folder->open();
  mFolders.append( folder );
}

KMMailtoComposeCommand::KMMailtoComposeCommand( const KURL &url,
                                                KMMessage *msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMMailtoComposeCommand::execute()
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

  return OK;
}


KMMailtoReplyCommand::KMMailtoReplyCommand( QWidget *parent,
   const KURL &url, KMMessage *msg, const QString &selection )
  :KMCommand( parent, msg ), mUrl( url ), mSelection( selection  )
{
}

KMCommand::Result KMMailtoReplyCommand::execute()
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

  return OK;
}


KMMailtoForwardCommand::KMMailtoForwardCommand( QWidget *parent,
   const KURL &url, KMMessage *msg )
  :KMCommand( parent, msg ), mUrl( url )
{
}

KMCommand::Result KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  KMMessage *msg = retrievedMessage();
  KMComposeWin *win;
  KMMessage *fmsg = msg->createForward();
  fmsg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  win = new KMComposeWin(fmsg);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();

  return OK;
}


KMAddBookmarksCommand::KMAddBookmarksCommand( const KURL &url, QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMAddBookmarksCommand::execute()
{
  QString filename = locateLocal( "data", QString::fromLatin1("konqueror/bookmarks.xml") );
  KBookmarkManager *bookManager = KBookmarkManager::managerForFile( filename,
                                                                    false );
  KBookmarkGroup group = bookManager->root();
  group.addBookmark( bookManager, mUrl.path(), KURL( mUrl ) );
  if( bookManager->save() ) {
    bookManager->emitChanged( group );
  }

  return OK;
}

KMMailtoAddAddrBookCommand::KMMailtoAddAddrBookCommand( const KURL &url,
   QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMMailtoAddAddrBookCommand::execute()
{
  KAddrBookExternal::addEmail( KMMessage::decodeMailtoUrl( mUrl.path() ),
                               parentWidget() );

  return OK;
}


KMMailtoOpenAddrBookCommand::KMMailtoOpenAddrBookCommand( const KURL &url,
   QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMMailtoOpenAddrBookCommand::execute()
{
  QString addr = KMMessage::decodeMailtoUrl( mUrl.path() );
  KAddrBookExternal::openEmail( KPIM::getEmailAddr(addr), addr ,
                                parentWidget() );

  return OK;
}


KMUrlCopyCommand::KMUrlCopyCommand( const KURL &url, KMMainWidget *mainWidget )
  :mUrl( url ), mMainWidget( mainWidget )
{
}

KMCommand::Result KMUrlCopyCommand::execute()
{
  QClipboard* clip = QApplication::clipboard();

  if (mUrl.protocol() == "mailto") {
    // put the url into the mouse selection and the clipboard
    QString address = KMMessage::decodeMailtoUrl( mUrl.path() );
    clip->setSelectionMode( true );
    clip->setText( address );
    clip->setSelectionMode( false );
    clip->setText( address );
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "Address copied to clipboard." ));
  } else {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode( true );
    clip->setText( mUrl.url() );
    clip->setSelectionMode( false );
    clip->setText( mUrl.url() );
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "URL copied to clipboard." ));
  }

  return OK;
}


KMUrlOpenCommand::KMUrlOpenCommand( const KURL &url, KMReaderWin *readerWin )
  :mUrl( url ), mReaderWin( readerWin )
{
}

KMCommand::Result KMUrlOpenCommand::execute()
{
  if ( !mUrl.isEmpty() )
    mReaderWin->slotUrlOpen( mUrl, KParts::URLArgs() );

  return OK;
}


KMUrlSaveCommand::KMUrlSaveCommand( const KURL &url, QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMUrlSaveCommand::execute()
{
  if ( mUrl.isEmpty() )
    return OK;
  KURL saveUrl = KFileDialog::getSaveURL(mUrl.fileName(), QString::null,
                                         parentWidget() );
  if ( saveUrl.isEmpty() )
    return Canceled;
  if ( KIO::NetAccess::exists( saveUrl, false, parentWidget() ) )
  {
    if (KMessageBox::warningContinueCancel(0,
        i18n("<qt>File <b>%1</b> exists.<br>Do you want to replace it?</qt>")
        .arg(saveUrl.prettyURL()), i18n("Save to File"), i18n("&Replace"))
        != KMessageBox::Continue)
      return Canceled;
  }
  KIO::Job *job = KIO::file_copy(mUrl, saveUrl, -1, true);
  connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotUrlSaveResult(KIO::Job*)));
  setEmitsCompletedItself( true );
  return OK;
}

void KMUrlSaveCommand::slotUrlSaveResult( KIO::Job *job )
{
  if ( job->error() ) {
    job->showErrorDialog();
    setResult( Failed );
    emit completed( this );
  }
  else {
    setResult( OK );
    emit completed( this );
  }
}


KMEditMsgCommand::KMEditMsgCommand( QWidget *parent, KMMessage *msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMEditMsgCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  if (!msg || !msg->parent() ||
      !kmkernel->folderIsDraftOrOutbox( msg->parent() ))
    return Failed;

  // Remember the old parent, we need it a bit further down to be able
  // to put the unchanged messsage back in the drafts folder if the nth
  // edit is discarded, for n > 1.
  KMFolder *parent = msg->parent();
  if ( parent )
    parent->take( parent->find( msg ) );

  KMComposeWin *win = new KMComposeWin();
  msg->setTransferInProgress(false); // From here on on, the composer owns the message.
  win->setMsg(msg, FALSE, TRUE);
  win->setFolder( parent );
  win->show();

  return OK;
}


KMShowMsgSrcCommand::KMShowMsgSrcCommand( QWidget *parent,
  KMMessage *msg, bool fixedFont )
  :KMCommand( parent, msg ), mFixedFont( fixedFont )
{
}

KMCommand::Result KMShowMsgSrcCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  QString str = msg->codec()->toUnicode( msg->asString() );

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

  return OK;
}

static KURL subjectToUrl( const QString & subject ) {
    return KFileDialog::getSaveURL( subject.stripWhiteSpace()
                                           .replace( QDir::separator(), '_' ),
                                    QString::null );
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
  mUrl = subjectToUrl( msg->cleanSubject() );
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
  mUrl = subjectToUrl( msgBase->cleanSubject() );
}

KURL KMSaveMsgCommand::url()
{
  return mUrl;
}

KMCommand::Result KMSaveMsgCommand::execute()
{
  mJob = KIO::put( mUrl, S_IRUSR|S_IWUSR, false, false );
  mJob->slotTotalSize( mTotalSize );
  mJob->setAsyncDataEnabled( true );
  mJob->setReportDataSent( true );
  connect(mJob, SIGNAL(dataReq(KIO::Job*, QByteArray &)),
    SLOT(slotSaveDataReq()));
  connect(mJob, SIGNAL(result(KIO::Job*)),
    SLOT(slotSaveResult(KIO::Job*)));
  setEmitsCompletedItself( true );
  return OK;
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
        job->setCancellable( false );
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
  QCString str( msg->mboxMessageSeparator() );
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

        mJob = KIO::put( mUrl, S_IRUSR|S_IWUSR, true, false );
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
      setResult( Failed );
      emit completed( this );
      delete this;
    }
  } else {
    setResult( OK );
    emit completed( this );
    delete this;
  }
}

//-----------------------------------------------------------------------------

KMOpenMsgCommand::KMOpenMsgCommand( QWidget *parent, const KURL & url )
  : KMCommand( parent ),
    mUrl( url )
{
  setDeletesItself( true );
}

KMCommand::Result KMOpenMsgCommand::execute()
{
  if ( mUrl.isEmpty() ) {
    mUrl = KFileDialog::getOpenURL( ":OpenMessage", "message/rfc822",
                                    parentWidget(), i18n("Open Message") );
  }
  if ( mUrl.isEmpty() ) {
    setDeletesItself( false );
    return Canceled;
  }
  mJob = KIO::get( mUrl, false, false );
  mJob->setReportDataSent( true );
  connect( mJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
           this, SLOT( slotDataArrived( KIO::Job*, const QByteArray & ) ) );
  connect( mJob, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotResult( KIO::Job * ) ) );
  setEmitsCompletedItself( true );
  return OK;
}

void KMOpenMsgCommand::slotDataArrived( KIO::Job *, const QByteArray & data )
{
  if ( data.isEmpty() )
    return;

  mMsgString.append( data.data(), data.size() );
}

void KMOpenMsgCommand::slotResult( KIO::Job *job )
{
  if ( job->error() ) {
    // handle errors
    job->showErrorDialog();
    setResult( Failed );
    emit completed( this );
  }
  else {
    int startOfMessage = 0;
    if ( mMsgString.compare( 0, 5, "From ", 5 ) == 0 ) {
      startOfMessage = mMsgString.find( '\n' );
      if ( startOfMessage == -1 ) {
        KMessageBox::sorry( parentWidget(),
                            i18n( "The file doesn't contain a message." ) );
        setResult( Failed );
        emit completed( this );
        // Emulate closing of a secondary window so that KMail exits in case it
        // was started with the --view command line option. Otherwise an
        // invisible KMail would keep running.
        SecondaryWindow *win = new SecondaryWindow();
        win->close();
        win->deleteLater();
        deleteLater();
        return;
      }
      startOfMessage += 1; // the message starts after the '\n'
    }
    // check for multiple messages in the file
    bool multipleMessages = true;
    int endOfMessage = mMsgString.find( "\nFrom " );
    if ( endOfMessage == -1 ) {
      endOfMessage = mMsgString.length();
      multipleMessages = false;
    }
    DwMessage *dwMsg = new DwMessage;
    dwMsg->FromString( mMsgString.substr( startOfMessage,
                                          endOfMessage - startOfMessage ) );
    dwMsg->Parse();
    // check whether we have a message ( no headers => this isn't a message )
    if ( dwMsg->Headers().NumFields() == 0 ) {
      KMessageBox::sorry( parentWidget(),
                          i18n( "The file doesn't contain a message." ) );
      delete dwMsg; dwMsg = 0;
      setResult( Failed );
      emit completed( this );
      // Emulate closing of a secondary window (see above).
      SecondaryWindow *win = new SecondaryWindow();
      win->close();
      win->deleteLater();
      deleteLater();
      return;
    }
    KMMessage *msg = new KMMessage( dwMsg );
    msg->setReadyToShow( true );
    KMReaderMainWin *win = new KMReaderMainWin();
    win->showMsg( kmkernel->networkCodec(), msg );
    win->show();
    if ( multipleMessages )
      KMessageBox::information( win,
                                i18n( "The file contains multiple messages. "
                                      "Only the first message is shown." ) );
    setResult( OK );
    emit completed( this );
  }
  delete this;
}

//-----------------------------------------------------------------------------

//TODO: ReplyTo, NoQuoteReplyTo, ReplyList, ReplyToAll, ReplyAuthor
//      are all similar and should be factored
KMReplyToCommand::KMReplyToCommand( QWidget *parent, KMMessage *msg,
                                    const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplySmart, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMNoQuoteReplyToCommand::KMNoQuoteReplyToCommand( QWidget *parent,
                                                  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMNoQuoteReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplySmart, "", TRUE);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();

  return OK;
}


KMReplyListCommand::KMReplyListCommand( QWidget *parent,
  KMMessage *msg, const QString &selection )
 : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyListCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyList, mSelection);
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();

  return OK;
}


KMReplyToAllCommand::KMReplyToAllCommand( QWidget *parent,
  KMMessage *msg, const QString &selection )
  :KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyToAllCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyAll, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMReplyAuthorCommand::KMReplyAuthorCommand( QWidget *parent, KMMessage *msg,
                                            const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyAuthorCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyAuthor, mSelection );
  KMComposeWin *win = new KMComposeWin( reply );
  win->setCharset( msg->codec()->mimeName(), TRUE );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMForwardCommand::KMForwardCommand( QWidget *parent,
  const QPtrList<KMMsgBase> &msgList, uint identity )
  : KMCommand( parent, msgList ),
    mIdentity( identity )
{
}

KMForwardCommand::KMForwardCommand( QWidget *parent, KMMessage *msg,
                                    uint identity )
  : KMCommand( parent, msg ),
    mIdentity( identity )
{
}

KMCommand::Result KMForwardCommand::execute()
{
  KMComposeWin *win;
  QPtrList<KMMessage> msgList = retrievedMsgs();

  if (msgList.count() >= 2) {
    // ask if they want a mime digest forward

    if (KMessageBox::questionYesNo( parentWidget(),
                                    i18n("Forward selected messages as "
                                         "a MIME digest?") )
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
        // remove headers that shouldn't be forwarded
        msg->removePrivateHeaderFields();
        msg->removeHeaderField("BCC");
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
      return OK;
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
      return OK;
    }
  }

  // forward a single message at most.

  KMMessage *msg = msgList.getFirst();
  if ( !msg || !msg->codec() )
    return Failed;

  KCursorSaver busy(KBusyPtr::busy());
  win = new KMComposeWin(msg->createForward());
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();

  return OK;
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

KMCommand::Result KMForwardAttachedCommand::execute()
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
    // remove headers that shouldn't be forwarded
    msg->removePrivateHeaderFields();
    msg->removeHeaderField("BCC");
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

  return OK;
}


KMRedirectCommand::KMRedirectCommand( QWidget *parent,
  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMRedirectCommand::execute()
{
  //TODO: move KMMessage::createRedirect to here
  KMComposeWin *win;
  KMMessage *msg = retrievedMessage();
  if ( !msg || !msg->codec() )
    return Failed;

  KCursorSaver busy(KBusyPtr::busy());
  win = new KMComposeWin();
  win->setMsg(msg->createRedirect(), FALSE);
  win->setCharset(msg->codec()->mimeName());
  win->show();

  return OK;
}


KMBounceCommand::KMBounceCommand( QWidget *parent,
  KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMBounceCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  KMMessage *newMsg = msg->createBounce( TRUE /* with UI */);
  if (newMsg)
    kmkernel->msgSender()->send(newMsg, kmkernel->msgSender()->sendImmediate());

  return OK;
}


KMPrintCommand::KMPrintCommand( QWidget *parent,
  KMMessage *msg, bool htmlOverride )
  : KMCommand( parent, msg ), mHtmlOverride( htmlOverride )
{
}

KMCommand::Result KMPrintCommand::execute()
{
  KMReaderWin printWin( 0, 0, 0 );
  printWin.setPrinting(TRUE);
  printWin.readConfig();
  printWin.setHtmlOverride( mHtmlOverride );
  printWin.setMsg(retrievedMessage(), TRUE);
  printWin.printMsg();

  return OK;
}


KMSetStatusCommand::KMSetStatusCommand( KMMsgStatus status,
  const QValueList<Q_UINT32> &serNums, bool toggle )
  : mStatus( status ), mSerNums( serNums ), mToggle( toggle )
{
}

KMCommand::Result KMSetStatusCommand::execute()
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
  //kapp->dcopClient()->emitDCOPSignal( "unreadCountChanged()", QByteArray() );

  return OK;
}


KMFilterCommand::KMFilterCommand( const QCString &field, const QString &value )
  : mField( field ), mValue( value )
{
}

KMCommand::Result KMFilterCommand::execute()
{
  kmkernel->filterMgr()->createFilter( mField, mValue );

  return OK;
}


KMFilterActionCommand::KMFilterActionCommand( QWidget *parent,
                                              const QPtrList<KMMsgBase> &msgList,
                                              KMFilter *filter )
  : KMCommand( parent, msgList ), mFilter( filter  )
{
}

KMCommand::Result KMFilterActionCommand::execute()
{
  QPtrList<KMMessage> msgList = retrievedMsgs();

  for (KMMessage *msg = msgList.first(); msg; msg = msgList.next())
    if( msg->parent() )
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

  return OK;
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
  int contentX, contentY;
  KMHeaderItem *item = mHeaders->prepareMove( &contentX, &contentY );
  mHeaders->finalizeMove( item, contentX, contentY );
#endif
}


KMMailingListFilterCommand::KMMailingListFilterCommand( QWidget *parent,
                                                        KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMMailingListFilterCommand::execute()
{
  QCString name;
  QString value;
  KMMessage *msg = retrievedMessage();
  if (!msg)
    return Failed;

  if ( !MailingList::name( msg, name, value ).isEmpty() ) {
    kmkernel->filterMgr()->createFilter( name, value );
    return OK;
  }
  else
    return Failed;
}


void KMMenuCommand::folderToPopupMenu(bool move,
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

  if (!kmkernel->imapFolderMgr()->dir().first() &&
      !kmkernel->dimapFolderMgr()->dir().first())
  { // only local folders
    makeFolderMenu( &kmkernel->folderMgr()->dir(), move,
                    receiver, aMenuToFolder, menu );
  } else {
    // operate on top-level items
    QPopupMenu* subMenu = new QPopupMenu(menu);
    makeFolderMenu( &kmkernel->folderMgr()->dir(),
                    move, receiver, aMenuToFolder, subMenu );
    menu->insertItem( i18n( "Local Folders" ), subMenu );
    KMFolderDir* fdir = &kmkernel->imapFolderMgr()->dir();
    for (KMFolderNode *node = fdir->first(); node; node = fdir->next()) {
      if (node->isDir())
        continue;
      subMenu = new QPopupMenu(menu);
      makeFolderMenu( node, move, receiver, aMenuToFolder, subMenu );
      menu->insertItem( node->label(), subMenu );
    }
    fdir = &kmkernel->dimapFolderMgr()->dir();
    for (KMFolderNode *node = fdir->first(); node; node = fdir->next()) {
      if (node->isDir())
        continue;
      subMenu = new QPopupMenu(menu);
      makeFolderMenu( node, move, receiver, aMenuToFolder, subMenu );
      menu->insertItem( node->label(), subMenu );
    }
  }
}

void KMMenuCommand::makeFolderMenu(KMFolderNode* node, bool move,
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
    menu->setItemEnabled( menuId, !folder->isReadOnly() );
    menu->insertSeparator();
  }

  if (!folderDir)
    return;

  for (KMFolderNode *it = folderDir->first(); it; it = folderDir->next() ) {
    if (it->isDir())
      continue;
    KMFolder *child = static_cast<KMFolder*>(it);
    QString label = child->label();
    label.replace("&","&&");
    if (child->child() && child->child()->first()) {
      // descend
      QPopupMenu *subMenu = new QPopupMenu(menu, "subMenu");
      makeFolderMenu( child, move, receiver,
                      aMenuToFolder, subMenu );
      menu->insertItem( label, subMenu );
    } else {
      // insert an item
      int menuId = menu->insertItem( label );
      aMenuToFolder->insert( menuId, child );
      menu->setItemEnabled( menuId, !child->isReadOnly() );
    }
  }
  return;
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

KMCommand::Result KMCopyCommand::execute()
{
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int idx = -1;
  bool isMessage;
  QPtrList<KMMessage> list;

  if (mDestFolder && mDestFolder->open() != 0)
    return Failed;

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
        (static_cast<KMFolderImap*>(srcFolder->storage())->account() ==
         static_cast<KMFolderImap*>(mDestFolder->storage())->account()))
    {
      list.append(msg);
    } else {
      newMsg = new KMMessage;
      newMsg->setComplete(msg->isComplete());
      // make sure the attachment state is only calculated when it's complete
      if (!newMsg->isComplete())
        newMsg->setReadyToShow(false);
      newMsg->fromString(msg->asString());
      newMsg->setStatus(msg->status());

      if (srcFolder && !newMsg->isComplete())
      {
        newMsg->setParent(msg->parent());
        FolderJob *job = srcFolder->createJob(newMsg);
        job->setCancellable( false );
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
  mDestFolder->close();

//TODO: Get rid of the other cases just use this one for all types of folder
//TODO: requires adding copyMsg and getFolder methods to KMFolder.h

  if (!list.isEmpty())
  {
    // copy the message(s); note: the list is empty afterwards!
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
    imapDestFolder->copyMsg(list);
    imapDestFolder->getFolder();
  }

  return OK;
}


KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              const QPtrList<KMMsgBase> &msgList)
  :mDestFolder( destFolder ), mMsgList( msgList ), mProgressItem( 0 )
{
  setDeletesItself( true );
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMessage *msg )
  :mDestFolder( destFolder ), mProgressItem( 0 )
{
  setDeletesItself( true );
  mMsgList.append( &msg->toMsgBase() );
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMsgBase *msgBase )
  :mDestFolder( destFolder ), mProgressItem( 0 )
{
  setDeletesItself( true );
  mMsgList.append( msgBase );
}

KMCommand::Result KMMoveCommand::execute()
{
  setEmitsCompletedItself( true );
  typedef QMap< KMFolder*, QPtrList<KMMessage>* > FolderToMessageListMap;
  FolderToMessageListMap folderDeleteList;

  if (mDestFolder && mDestFolder->open() != 0) {
    completeMove( Failed );
    return Failed;
  }
  KCursorSaver busy(KBusyPtr::busy());

  // TODO set SSL state according to source and destfolder connection?
  Q_ASSERT( !mProgressItem );
  mProgressItem =
     ProgressManager::createProgressItem (
         "move"+ProgressManager::getUniqueID(),
         mDestFolder ? i18n( "Moving messages" ) : i18n( "Deleting messages" ) );
  connect( mProgressItem, SIGNAL( progressItemCanceled( ProgressItem* ) ),
           this, SLOT( slotMoveCanceled() ) );

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
  for ( msgBase=mMsgList.first(); msgBase; msgBase=mMsgList.next() ) {
    mLostBoys.append( msgBase->getMsgSerNum() );
  }
  mProgressItem->setTotalItems( mMsgList.count() );

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
      static_cast<KMFolderImap*>(srcFolder->storage())->ignoreJobsForMessage( msg );
    }

    if (mDestFolder) {
      if (mDestFolder->folderType() == KMFolderTypeImap) {
        /* If we are moving to an imap folder, connect to it's completed
         * signal so we notice when all the mails should have showed up in it
         * but haven't for some reason. */
        KMFolderImap *imapFolder = static_cast<KMFolderImap*> ( mDestFolder->storage() );
        disconnect (imapFolder, SIGNAL(folderComplete( KMFolderImap*, bool )),
                 this, SLOT(slotImapFolderCompleted( KMFolderImap*, bool )));

        connect (imapFolder, SIGNAL(folderComplete( KMFolderImap*, bool )),
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
          completeMove( Failed );
          return Failed;
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
        completeMove( OK );
      }
    }
    if ( !mDestFolder ) {
      completeMove( OK );
    }
  }

  return OK;
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
    completeMove( OK );
  } else {
    // Should we inform the user here or leave that to the caller?
    completeMove( Failed );
  }
}

void KMMoveCommand::slotMsgAddedToDestFolder(KMFolder *folder, Q_UINT32 serNum)
{
  if (folder != mDestFolder || !mLostBoys.contains( serNum ) ) {
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
    completeMove( OK );
  } else {
    mProgressItem->incCompletedItems();
    mProgressItem->updateProgress();
  }
}

void KMMoveCommand::completeMove( Result result )
{
  if ( mDestFolder )
    mDestFolder->close();
  if ( mProgressItem )
    mProgressItem->setComplete();
  setResult( result );
  emit completed( this );
  deleteLater();
}

void KMMoveCommand::slotMoveCanceled()
{
  completeMove( Canceled );
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
  KMFolder* trash = folder->trashFolder();
  if( !trash )
    trash = kmkernel->trashFolder();
  if( trash != folder )
    return trash;
  return 0;
}

KMUrlClickedCommand::KMUrlClickedCommand( const KURL &url, uint identity,
  KMReaderWin *readerWin, bool htmlPref, KMMainWidget *mainWidget )
  :mUrl( url ), mIdentity( identity ), mReaderWin( readerWin ),
   mHtmlPref( htmlPref ), mMainWidget( mainWidget )
{
}

KMCommand::Result KMUrlClickedCommand::execute()
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
  else if ( mUrl.protocol() == "im" )
  {
    kmkernel->imProxy()->chatWithContact( mUrl.path() );
  }
  else if ((mUrl.protocol() == "http") || (mUrl.protocol() == "https") ||
           (mUrl.protocol() == "ftp") || (mUrl.protocol() == "file") ||
           (mUrl.protocol() == "ftps") || (mUrl.protocol() == "sftp" ) ||
           (mUrl.protocol() == "help") || (mUrl.protocol() == "vnc") ||
           (mUrl.protocol() == "smb"))
  {
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Opening URL..."));
    KMimeType::Ptr mime = KMimeType::findByURL( mUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-msdos-program" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18n( "<qt>Do you really want to execute <b>%1</b>?</qt>" )
        .arg( mUrl.prettyURL() ) ) != KMessageBox::Yes)
        return Canceled;
    }
    (void) new KRun( mUrl );
  }
  else
    return Failed;

  return OK;
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, KMMessage *msg )
  : KMCommand( parent, msg ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const QPtrList<KMMsgBase>& msgs )
  : KMCommand( parent, msgs ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, QPtrList<partNode>& attachments,
                                                    KMMessage *msg, bool encoded )
  : KMCommand( parent, msg ), mImplicitAttachments( false ),
    mEncoded( encoded )
{
  // do not load the complete message but only parts
  msg->setComplete( true );
  for ( QPtrListIterator<partNode> it( attachments ); it.current(); ++it ) {
    mAttachmentMap.insert( it.current(), msg );
  }
}

KMCommand::Result KMSaveAttachmentsCommand::execute()
{
  setEmitsCompletedItself( true );
  if ( mImplicitAttachments ) {
    QPtrList<KMMessage> msgList = retrievedMsgs();
    KMMessage *msg;
    for ( QPtrListIterator<KMMessage> itr( msgList );
          ( msg = itr.current() );
          ++itr ) {
      partNode *rootNode = partNode::fromMessage( msg );
      for ( partNode *child = rootNode; child;
            child = child->firstChild() ) {
        for ( partNode *node = child; node; node = node->nextSibling() ) {
          if ( node->type() != DwMime::kTypeMultipart )
            mAttachmentMap.insert( node, msg );
        }
      }
    }
  }
  setDeletesItself( true );
  // load all parts
  KMLoadPartsCommand *command = new KMLoadPartsCommand( mAttachmentMap );
  connect( command, SIGNAL( partsRetrieved() ),
           this, SLOT( slotSaveAll() ) );
  command->start();

  return OK;
}

void KMSaveAttachmentsCommand::slotSaveAll()
{
  // now that all message parts have been retrieved, remove all parts which
  // don't represent an attachment if they were not explicitely passed in the
  // c'tor
  if ( mImplicitAttachments ) {
    for ( PartNodeMessageMap::iterator it = mAttachmentMap.begin();
          it != mAttachmentMap.end(); ) {
      // only body parts which have a filename or a name parameter (except for
      // the root node for which name is set to the message's subject) are
      // considered attachments
      if ( it.key()->msgPart().fileName().stripWhiteSpace().isEmpty() &&
           ( it.key()->msgPart().name().stripWhiteSpace().isEmpty() ||
             !it.key()->parentNode() ) ) {
        PartNodeMessageMap::iterator delIt = it;
        ++it;
        mAttachmentMap.remove( delIt );
      }
      else
        ++it;
    }
    if ( mAttachmentMap.isEmpty() ) {
      KMessageBox::information( 0, i18n("Found no attachments to save.") );
      setResult( OK ); // The user has already been informed.
      emit completed( this );
      delete this;
      return;
    }
  }

  KURL url, dirUrl;
  if ( mAttachmentMap.count() > 1 ) {
    // get the dir
    KFileDialog fdlg( ":saveAttachments", QString::null, parentWidget(),
                      "save attachments dialog", true );
    fdlg.setCaption( i18n("Save Attachments To") );
    fdlg.setOperationMode( KFileDialog::Saving );
    fdlg.setMode( (unsigned int) KFile::Directory );
    if ( fdlg.exec() == QDialog::Rejected || !fdlg.selectedURL().isValid() ) {
      setResult( Canceled );
      emit completed( this );
      delete this;
      return;
    }
    dirUrl = fdlg.selectedURL();
  }
  else {
    // only one item, get the desired filename
    partNode *node = mAttachmentMap.begin().key();
    // replace all ':' with '_' because ':' isn't allowed on FAT volumes
    QString s =
      node->msgPart().fileName().stripWhiteSpace().replace( ':', '_' );
    if ( s.isEmpty() )
      s = node->msgPart().name().stripWhiteSpace().replace( ':', '_' );
    if ( s.isEmpty() )
      s = i18n("filename for an unnamed attachment", "attachment.1");
    url = KFileDialog::getSaveURL( s, QString::null, parentWidget(),
                                   QString::null );
    if ( url.isEmpty() ) {
      setResult( Canceled );
      emit completed( this );
      delete this;
      return;
    }
  }

  QMap< QString, int > renameNumbering;

  Result globalResult = OK;
  int unnamedAtmCount = 0;
  for ( PartNodeMessageMap::const_iterator it = mAttachmentMap.begin();
        it != mAttachmentMap.end();
        ++it ) {
    KURL curUrl;
    if ( !dirUrl.isEmpty() ) {
      curUrl = dirUrl;
      QString s =
        it.key()->msgPart().fileName().stripWhiteSpace().replace( ':', '_' );
      if ( s.isEmpty() )
        s = it.key()->msgPart().name().stripWhiteSpace().replace( ':', '_' );
      if ( s.isEmpty() ) {
        ++unnamedAtmCount;
        s = i18n("filename for the %1-th unnamed attachment",
                 "attachment.%1")
            .arg( unnamedAtmCount );
      }
      curUrl.setFileName( s );
    } else {
      curUrl = url;
    }

    if ( !curUrl.isEmpty() ) {

     // Rename the file if we have already saved one with the same name:
     // try appending a number before extension (e.g. "pic.jpg" => "pic_2.jpg")
     QString origFile = curUrl.fileName();
     QString file = origFile;

     while ( renameNumbering.contains(file) ) {
       file = origFile;
       int num = renameNumbering[file] + 1;
       int dotIdx = file.findRev('.');
       file = file.insert( (dotIdx>=0) ? dotIdx : file.length(), QString("_") + QString::number(num) );
     }
     curUrl.setFileName(file);

     // Increment the counter for both the old and the new filename
     if ( !renameNumbering.contains(origFile))
         renameNumbering[origFile] = 1;
     else
         renameNumbering[origFile]++;

     if ( file != origFile ) {
        if ( !renameNumbering.contains(file))
            renameNumbering[file] = 1;
        else
            renameNumbering[file]++;
     }


      if ( KIO::NetAccess::exists( curUrl, false, parentWidget() ) ) {
        if ( KMessageBox::warningContinueCancel( parentWidget(),
              i18n( "A file named %1 already exists. Do you want to overwrite it?" )
              .arg( curUrl.fileName() ),
              i18n( "File Already Exists" ), i18n("Overwrite") ) == KMessageBox::Cancel) {
          continue;
        }
      }
      // save
      const Result result = saveItem( it.key(), curUrl );
      if ( result != OK )
        globalResult = result;
    }
  }
  setResult( globalResult );
  emit completed( this );
  delete this;
}

KMCommand::Result KMSaveAttachmentsCommand::saveItem( partNode *node,
                                                      const KURL& url )
{
  bool bSaveEncrypted = false;
  bool bEncryptedParts = node->encryptionState() != KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?" ).
          arg( url.fileName() ),
          i18n( "KMail Question" ) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if( node->signatureState() != KMMsgNotSigned )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is signed. Do you want to keep the signature when saving?" ).
          arg( url.fileName() ),
          i18n( "KMail Question" ) ) !=
        KMessageBox::Yes )
      bSaveWithSig = false;

  QByteArray data;
  if ( mEncoded )
  {
    // This does not decode the Message Content-Transfer-Encoding
    // but saves the _original_ content of the message part
    QCString cstr( node->msgPart().body() );
    data = cstr;
    data.resize(data.size() - 1);
  }
  else
  {
    if( bSaveEncrypted || !bEncryptedParts) {
      partNode *dataNode = node;
      QCString rawReplyString;
      bool gotRawReplyString = false;
      if( !bSaveWithSig ) {
        if( DwMime::kTypeMultipart == node->type() &&
            DwMime::kSubtypeSigned == node->subType() ){
          // carefully look for the part that is *not* the signature part:
          if( node->findType( DwMime::kTypeApplication,
                DwMime::kSubtypePgpSignature,
                TRUE, false ) ){
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                DwMime::kSubtypePgpSignature,
                TRUE, false );
          }else if( node->findType( DwMime::kTypeApplication,
                DwMime::kSubtypePkcs7Mime,
                TRUE, false ) ){
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                DwMime::kSubtypePkcs7Mime,
                TRUE, false );
          }else{
            dataNode = node->findTypeNot( DwMime::kTypeMultipart,
                DwMime::kSubtypeUnknown,
                TRUE, false );
          }
	}else{
	  ObjectTreeParser otp( 0, 0, false, false, false );

	  // process this node and all it's siblings and descendants
	  dataNode->setProcessed( false, true );
	  otp.parseObjectTree( dataNode );

	  rawReplyString = otp.rawReplyString();
	  gotRawReplyString = true;
        }
      }
      QByteArray cstr = gotRawReplyString
                         ? rawReplyString
                         : dataNode->msgPart().bodyDecodedBinary();
      data = cstr;
      size_t size = cstr.size();
      if ( dataNode->msgPart().type() == DwMime::kTypeText ) {
        // convert CRLF to LF before writing text attachments to disk
        size = KMFolder::crlf2lf( cstr.data(), size );
      }
      data.resize( size );
    }
  }
  QDataStream ds;
  QFile file;
  KTempFile tf;
  tf.setAutoDelete( true );
  if ( url.isLocalFile() )
  {
    // save directly
    file.setName( url.path() );
    if ( !file.open( IO_WriteOnly ) )
    {
      KMessageBox::error( parentWidget(),
          i18n( "%2 is detailed error description",
            "Could not write the file %1:\n%2" )
          .arg( file.name() )
          .arg( QString::fromLocal8Bit( strerror( errno ) ) ),
          i18n( "KMail Error" ) );
      return Failed;
    }
    fchmod( file.handle(), S_IRUSR | S_IWUSR );
    ds.setDevice( &file );
  } else
  {
    // tmp file for upload
    ds.setDevice( tf.file() );
  }

  ds.writeRawBytes( data.data(), data.size() );
  if ( !url.isLocalFile() )
  {
    tf.close();
    if ( !KIO::NetAccess::upload( tf.name(), url, parentWidget() ) )
    {
      KMessageBox::error( parentWidget(),
          i18n( "Could not write the file %1." )
          .arg( url.path() ),
          i18n( "KMail Error" ) );
      return Failed;
    }
  } else
    file.close();
  return OK;
}

KMLoadPartsCommand::KMLoadPartsCommand( QPtrList<partNode>& parts, KMMessage *msg )
  : mNeedsRetrieval( 0 )
{
  for ( QPtrListIterator<partNode> it( parts ); it.current(); ++it ) {
    mPartMap.insert( it.current(), msg );
  }
}

KMLoadPartsCommand::KMLoadPartsCommand( partNode *node, KMMessage *msg )
  : mNeedsRetrieval( 0 )
{
  mPartMap.insert( node, msg );
}

KMLoadPartsCommand::KMLoadPartsCommand( PartNodeMessageMap& partMap )
  : mNeedsRetrieval( 0 ), mPartMap( partMap )
{
}

void KMLoadPartsCommand::slotStart()
{
  for ( PartNodeMessageMap::const_iterator it = mPartMap.begin();
        it != mPartMap.end();
        ++it ) {
    if ( !it.key()->msgPart().isComplete() &&
         !it.key()->msgPart().partSpecifier().isEmpty() ) {
      // incomplete part, so retrieve it first
      ++mNeedsRetrieval;
      KMFolder* curFolder = it.data()->parent();
      if ( curFolder ) {
        FolderJob *job =
          curFolder->createJob( it.data(), FolderJob::tGetMessage,
                                0, it.key()->msgPart().partSpecifier() );
        job->setCancellable( false );
        connect( job, SIGNAL(messageUpdated(KMMessage*, QString)),
                 this, SLOT(slotPartRetrieved(KMMessage*, QString)) );
        job->start();
      } else
        kdWarning(5006) << "KMLoadPartsCommand - msg has no parent" << endl;
    }
  }
  if ( mNeedsRetrieval == 0 )
    execute();
}

void KMLoadPartsCommand::slotPartRetrieved( KMMessage *msg,
                                            QString partSpecifier )
{
  DwBodyPart *part =
    msg->findDwBodyPart( msg->getFirstDwBodyPart(), partSpecifier );
  if ( part ) {
    // update the DwBodyPart in the partNode
    for ( PartNodeMessageMap::const_iterator it = mPartMap.begin();
          it != mPartMap.end();
          ++it ) {
      if ( it.key()->dwPart() == part )
        it.key()->setDwPart( part );
    }
  } else
    kdWarning(5006) << "KMLoadPartsCommand::slotPartRetrieved - could not find bodypart!" << endl;
  --mNeedsRetrieval;
  if ( mNeedsRetrieval == 0 )
    execute();
}

KMCommand::Result KMLoadPartsCommand::execute()
{
  emit partsRetrieved();
  setResult( OK );
  emit completed( this );
  deleteLater();
  return OK;
}

KMResendMessageCommand::KMResendMessageCommand( QWidget *parent,
   KMMessage *msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMResendMessageCommand::execute()
{
  KMComposeWin *win;
  KMMessage *msg = retrievedMessage();

  KMMessage *newMsg = new KMMessage(*msg);
  newMsg->setCharset(msg->codec()->mimeName());
  // the message needs a new Message-Id
  newMsg->removeHeaderField( "Message-Id" );
  newMsg->setParent( 0 );

  // adds the new date to the message
  newMsg->removeHeaderField( "Date" );

  win = new KMComposeWin();
  win->setMsg(newMsg, false, true);
  win->show();

  return OK;
}

KMMailingListCommand::KMMailingListCommand( QWidget *parent, KMFolder *folder )
  : KMCommand( parent ), mFolder( folder )
{
}

KMCommand::Result KMMailingListCommand::execute()
{
  KURL::List lst = urls();
  QString handler = ( mFolder->mailingList().handler() == MailingList::KMail )
    ? "mailto" : "https";

  KMCommand *command = 0;
  for ( KURL::List::Iterator itr = lst.begin(); itr != lst.end(); ++itr ) {
    if ( handler == (*itr).protocol() ) {
      command = new KMUrlClickedCommand( *itr, mFolder->identity(), 0, false );
    }
  }
  if ( !command && !lst.empty() ) {
    command =
      new KMUrlClickedCommand( lst.first(), mFolder->identity(), 0, false );
  }
  if ( command ) {
    connect( command, SIGNAL( completed( KMCommand * ) ),
             this, SLOT( commandCompleted( KMCommand * ) ) );
    setDeletesItself( true );
    setEmitsCompletedItself( true );
    command->start();
    return OK;
  }
  return Failed;
}

void KMMailingListCommand::commandCompleted( KMCommand *command )
{
  setResult( command->result() );
  emit completed( this );
  delete this;
}

KMMailingListPostCommand::KMMailingListPostCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KURL::List KMMailingListPostCommand::urls() const
{
  return mFolder->mailingList().postURLS();
}

KMMailingListSubscribeCommand::KMMailingListSubscribeCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KURL::List KMMailingListSubscribeCommand::urls() const
{
  return mFolder->mailingList().subscribeURLS();
}

KMMailingListUnsubscribeCommand::KMMailingListUnsubscribeCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KURL::List KMMailingListUnsubscribeCommand::urls() const
{
  return mFolder->mailingList().unsubscribeURLS();
}

KMMailingListArchivesCommand::KMMailingListArchivesCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KURL::List KMMailingListArchivesCommand::urls() const
{
  return mFolder->mailingList().archiveURLS();
}

KMMailingListHelpCommand::KMMailingListHelpCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KURL::List KMMailingListHelpCommand::urls() const
{
  return mFolder->mailingList().helpURLS();
}

KMIMChatCommand::KMIMChatCommand( const KURL &url, KMMessage *msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMIMChatCommand::execute()
{
  kdDebug( 5006 ) << k_funcinfo << " URL is: " << mUrl << endl;
  // find UID for mail address
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  KABC::AddresseeList addresses = addressBook->findByEmail( KPIM::getEmailAddr( mUrl.path() ) ) ;

  // start chat
  if( addresses.count() == 1 ) {
    kmkernel->imProxy()->chatWithContact( addresses[0].uid() );
    return OK;
  }
  else
  {
    kdDebug( 5006 ) << "Didn't find exactly one addressee, couldn't tell who to chat to for that email address.  Count = " << addresses.count() << endl;

    QString apology;
    if ( addresses.isEmpty() )
      apology = i18n( "There is no Address Book entry for this email address. Add them to the Address Book and then add instant messaging addresses using your preferred messaging client." );
    else
    {
      apology = i18n( "More than one Address Book entry uses this email address:\n %1\n it is not possible to determine who to chat with." );
      QStringList nameList;
      KABC::AddresseeList::const_iterator it = addresses.begin();
      KABC::AddresseeList::const_iterator end = addresses.end();
      for ( ; it != end; ++it )
      {
          nameList.append( (*it).realName() );
      }
      QString names = nameList.join( QString::fromLatin1( ",\n" ) );
      apology = apology.arg( names );
    }

    KMessageBox::sorry( parentWidget(), apology );
    return Failed;
  }
}
