/* -*- mode: C++; c-file-style: "gnu" -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//
// This file implements various "command" classes. These command classes
// are based on the command design pattern.
//
// Historically various operations were implemented as slots of KMMainWin.
// This proved inadequate as KMail has multiple top level windows
// (KMMainWin, KMReaderMainWin, SearchWindow, KMComposeWin) that may
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

//Added by qt3to4:
#include <QMenu>
#include <Q3CString>
#include <kprogressdialog.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QList>
#include <QTextCodec>
#include <QProgressBar>

#include <emailfunctions/email.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kabc/stdaddressbook.h>
#include <kabc/addresseelist.h>
#include <kdirselectdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kparts/browserextension.h>
#include <krun.h>
#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kimproxy.h>
// KIO headers
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>

#include "actionscheduler.h"
using KMail::ActionScheduler;
#include "mailinglist-magic.h"
#include "kmaddrbook.h"
#include <kaddrbook.h>
#include "composer.h"
#include "kmfiltermgr.h"
#include "kmfoldermbox.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "headeritem.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "messagesender.h"
#include "kmmsgpartdlg.h"
#include "undostack.h"
#include "kcursorsaver.h"
#include "partNode.h"
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
using KMail::FolderJob;
#include "chiasmuskeyselector.h"
#include "mailsourceviewer.h"
using KMail::MailSourceViewer;
#include "kmreadermainwin.h"
#include "secondarywindow.h"
using KMail::SecondaryWindow;
#include "redirectdialog.h"
using KMail::RedirectDialog;
#include "util.h"
#include "templateparser.h"
using KMail::TemplateParser;

#include "broadcaststatus.h"
#include "globalsettings.h"

#include <libkdepim/kfileio.h>

#include "progressmanager.h"
using KPIM::ProgressManager;
using KPIM::ProgressItem;
#include <kmime/kmime_mdn.h>
using namespace KMime;

#include <kleo/specialjob.h>
#include <kleo/cryptobackend.h>
#include <kleo/cryptobackendfactory.h>

#include <QClipboard>


class LaterDeleterWithCommandCompletion : public KMail::Util::LaterDeleter
{
public:
  LaterDeleterWithCommandCompletion( KMCommand* command )
    :LaterDeleter( command ), m_result( KMCommand::Failed )
  {
  }
  ~LaterDeleterWithCommandCompletion()
  {
    setResult( m_result );
    KMCommand *command = static_cast<KMCommand*>( m_object );
    emit command->completed( command );
  }
  void setResult( KMCommand::Result v ) { m_result = v; }
private:
  KMCommand::Result m_result;
};


KMCommand::KMCommand( QWidget *parent )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
}

KMCommand::KMCommand( QWidget *parent, const QList<KMMsgBase*> &msgList )
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
  if ( msg )
    mMsgList.append( &msg->toMsgBase() );
}

KMCommand::~KMCommand()
{
  QList<QPointer<KMFolder> >::Iterator fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if (!(*fit))
      continue;
    (*fit)->close();
  }
}

KMCommand::Result KMCommand::result()
{
  if ( mResult == Undefined )
    kDebug(5006) << k_funcinfo << "mResult is Undefined" << endl;
  return mResult;
}

void KMCommand::start()
{
  QTimer::singleShot( 0, this, SLOT( slotStart() ) );
}


const QList<KMMessage*> KMCommand::retrievedMsgs() const
{
  return mRetrievedMsgs;
}

KMMessage *KMCommand::retrievedMessage() const
{
  return *(mRetrievedMsgs.begin());
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

  if ( mMsgList.contains(0) ) {
      emit messagesTransfered( Failed );
      return;
  }

  KMMsgBase *mb = *(mMsgList.begin());
  if ( (mMsgList.count() == 1) && ( mb->isMessage() ) &&
      ( mb->parent() == 0 ) )
  {
    // Special case of operating on message that isn't in a folder
    mRetrievedMsgs.append((KMMessage*)mMsgList.takeFirst());
    emit messagesTransfered( OK );
    return;
  }

  QList<KMMsgBase*>::const_iterator it;
  for ( it = mMsgList.begin(); it != mMsgList.end(); it++ )
    if ( !( (*it)->parent() ) ) {
      emit messagesTransfered( Failed );
      return;
    } else {
      keepFolderOpen( (*it)->parent() );
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
  KMMessage* msg;
  foreach( msg, mRetrievedMsgs ) {
    if ( msg && msg->parent() )
      msg->setTransferInProgress(false);
  }
  kmkernel->filterMgr()->deref();
  if ( !emitsCompletedItself() )
    emit completed( this );
  if ( !deletesItself() )
    deleteLater();
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
    mProgressDialog = new KProgressDialog(mParent,
      i18n("Please wait"),
      i18np("Please wait while the message is transferred",
        "Please wait while the %1 messages are transferred", mMsgList.count()),
      true);
    mProgressDialog->setMinimumDuration(1000);
  }
  QList<KMMsgBase*>::const_iterator it;
  for ( it = mMsgList.begin(); it != mMsgList.end(); it++ )
  {
    KMMsgBase *mb = (*it);
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
      kDebug(5006)<<"### INCOMPLETE\n";
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
    mProgressDialog = 0;
    emit messagesTransfered( OK );
  } else {
    // wait for the transfer and tell the progressBar the necessary steps
    if ( mProgressDialog ) {
      connect(mProgressDialog, SIGNAL(cancelClicked()),
              this, SLOT(slotTransferCancelled()));
      mProgressDialog->progressBar()->setMaximum(totalSize);
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
  mProgressDialog->progressBar()->setValue( done );
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
    mProgressDialog->setLabel(i18np("Please wait while the message is transferred",
          "Please wait while the %1 messages are transferred", KMCommand::mCountJobs));
  }
  if (KMCommand::mCountJobs == 0)
  {
    // all done
    delete mProgressDialog;
    mProgressDialog = 0;
    emit messagesTransfered( OK );
  }
}

void KMCommand::slotTransferCancelled()
{
  // kill the pending jobs
  QList<QPointer<KMFolder> >::Iterator fit;
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
  QList<KMMessage*>::const_iterator it;
  for ( it = mRetrievedMsgs.constBegin(); it != mRetrievedMsgs.constEnd(); ++it ) {
    KMMessage* msg = (*it);
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

KMMailtoComposeCommand::KMMailtoComposeCommand( const KUrl &url,
                                                KMMessage *msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMMailtoComposeCommand::execute()
{
  KMMessage *msg = new KMMessage;
  uint id = 0;

  if ( mMessage && mMessage->parent() )
    id = mMessage->parent()->identity();

  msg->initHeader(id);
  msg->setCharset("utf-8");
  msg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  KMail::Composer * win = KMail::makeComposer( msg, id );
  win->setCharset("", true);
  win->setFocusToSubject();
  win->show();

  return OK;
}


KMMailtoReplyCommand::KMMailtoReplyCommand( QWidget *parent,
   const KUrl &url, KMMessage *msg, const QString &selection )
  :KMCommand( parent, msg ), mUrl( url ), mSelection( selection  )
{
}

KMCommand::Result KMMailtoReplyCommand::execute()
{
  //TODO : consider factoring createReply into this method.
  KMMessage *msg = retrievedMessage();
  KMMessage *rmsg = msg->createReply( KMail::ReplyNone, mSelection );
  rmsg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  KMail::Composer * win = KMail::makeComposer( rmsg, 0 );
  win->setCharset(msg->codec()->name(), true);
  win->setReplyFocus();
  win->show();

  return OK;
}


KMMailtoForwardCommand::KMMailtoForwardCommand( QWidget *parent,
   const KUrl &url, KMMessage *msg )
  :KMCommand( parent, msg ), mUrl( url )
{
}

KMCommand::Result KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  KMMessage *msg = retrievedMessage();
  KMMessage *fmsg = msg->createForward();
  fmsg->setTo( KMMessage::decodeMailtoUrl( mUrl.path() ) );

  KMail::Composer * win = KMail::makeComposer( fmsg );
  win->setCharset(msg->codec()->name(), true);
  win->show();

  return OK;
}


KMAddBookmarksCommand::KMAddBookmarksCommand( const KUrl &url, QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMAddBookmarksCommand::execute()
{
  QString filename = KStandardDirs::locateLocal( "data", QString::fromLatin1("konqueror/bookmarks.xml") );
  KBookmarkManager *bookManager = KBookmarkManager::managerForFile( filename, "konqueror",
                                                                    false );
  KBookmarkGroup group = bookManager->root();
  group.addBookmark( bookManager, mUrl.path(), KUrl( mUrl ) );
  if( bookManager->save() ) {
    bookManager->emitChanged( group );
  }

  return OK;
}

KMMailtoAddAddrBookCommand::KMMailtoAddAddrBookCommand( const KUrl &url,
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


KMMailtoOpenAddrBookCommand::KMMailtoOpenAddrBookCommand( const KUrl &url,
   QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMMailtoOpenAddrBookCommand::execute()
{
  QString addr = KMMessage::decodeMailtoUrl( mUrl.path() );
  KAddrBookExternal::openEmail( EmailAddressTools::extractEmailAddress(addr), addr ,
                                parentWidget() );

  return OK;
}


KMUrlCopyCommand::KMUrlCopyCommand( const KUrl &url, KMMainWidget *mainWidget )
  :mUrl( url ), mMainWidget( mainWidget )
{
}

KMCommand::Result KMUrlCopyCommand::execute()
{
  QClipboard* clip = QApplication::clipboard();

  if (mUrl.protocol() == "mailto") {
    // put the url into the mouse selection and the clipboard
    QString address = KMMessage::decodeMailtoUrl( mUrl.path() );
    clip->setText( address, QClipboard::Clipboard );
    clip->setText( address, QClipboard::Selection );
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "Address copied to clipboard." ));
  } else {
    // put the url into the mouse selection and the clipboard
    clip->setText( mUrl.url(), QClipboard::Clipboard );
    clip->setText( mUrl.url(), QClipboard::Selection );
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "URL copied to clipboard." ));
  }

  return OK;
}


KMUrlOpenCommand::KMUrlOpenCommand( const KUrl &url, KMReaderWin *readerWin )
  :mUrl( url ), mReaderWin( readerWin )
{
}

KMCommand::Result KMUrlOpenCommand::execute()
{
  if ( !mUrl.isEmpty() )
    mReaderWin->slotUrlOpen( mUrl, KParts::URLArgs() );

  return OK;
}


KMUrlSaveCommand::KMUrlSaveCommand( const KUrl &url, QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMUrlSaveCommand::execute()
{
  if ( mUrl.isEmpty() )
    return OK;
  KUrl saveUrl = KFileDialog::getSaveUrl(mUrl.fileName(), QString(),
                                         parentWidget() );
  if ( saveUrl.isEmpty() )
    return Canceled;
  if ( KIO::NetAccess::exists( saveUrl, false, parentWidget() ) )
  {
    if (KMessageBox::warningContinueCancel(0,
        i18n("<qt>File <b>%1</b> exists.<br>Do you want to replace it?</qt>",
         saveUrl.prettyUrl()), i18n("Save to File"), KGuiItem(i18n("&Replace")))
        != KMessageBox::Continue)
      return Canceled;
  }
  KIO::Job *job = KIO::file_copy(mUrl, saveUrl, -1, true);
  connect(job, SIGNAL(result(KJob*)), SLOT(slotUrlSaveResult(KJob*)));
  setEmitsCompletedItself( true );
  return OK;
}

void KMUrlSaveCommand::slotUrlSaveResult( KJob *job )
{
  if ( job->error() ) {
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
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
      ( !kmkernel->folderIsDraftOrOutbox( msg->parent() ) &&
	!kmkernel->folderIsTemplates( msg->parent() ) ) )
    return Failed;

  // Remember the old parent, we need it a bit further down to be able
  // to put the unchanged messsage back in the original folder if the nth
  // edit is discarded, for n > 1.
  KMFolder *parent = msg->parent();
  if ( parent )
    parent->take( parent->find( msg ) );

  KMail::Composer * win = KMail::makeComposer();
  msg->setTransferInProgress(false); // From here on on, the composer owns the message.
  win->setMsg(msg, false, true);
  win->setFolder( parent );
  win->show();

  return OK;
}


KMUseTemplateCommand::KMUseTemplateCommand( QWidget *parent, KMMessage *msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMUseTemplateCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  if ( !msg || !msg->parent() ||
       !kmkernel->folderIsTemplates( msg->parent() ) )
    return Failed;

  // Take a copy of the original message, which remains unchanged.
  KMMessage *newMsg = new KMMessage;
  newMsg->setComplete( msg->isComplete() );
  newMsg->fromString( msg->asString() );

  KMail::Composer *win = KMail::makeComposer();
  newMsg->setTransferInProgress( false ); // From here on on, the composer owns the message.
  win->setMsg( newMsg, false, true );
  win->show();

  return OK;
}


KMShowMsgSrcCommand::KMShowMsgSrcCommand( QWidget *parent,
  KMMessage *msg, bool fixedFont )
  :KMCommand( parent, msg ), mFixedFont( fixedFont )
{
  // remember complete state
  mMsgWasComplete = msg->isComplete();
}

KMCommand::Result KMShowMsgSrcCommand::execute()
{
  KMMessage *msg = retrievedMessage();
  if ( msg->isComplete() && !mMsgWasComplete )
    msg->notify(); // notify observers as msg was transfered
  QString str = msg->codec()->toUnicode( msg->asString() );

  MailSourceViewer *viewer = new MailSourceViewer(); // deletes itself upon close
  viewer->setWindowTitle( i18n("Message as Plain Text") );
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

static KUrl subjectToUrl( const QString & subject ) {
    return KFileDialog::getSaveUrl( subject.trimmed()
                                           .replace( QDir::separator(), '_' ),
                                    QString() );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent, KMMessage * msg )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mStandAloneMessage( 0 ),
    mOffset( 0 ),
    mTotalSize( msg ? msg->msgSize() : 0 )
{
  if ( !msg ) return;
  setDeletesItself( true );
  // If the mail has a serial number, operate on sernums, if it does not
  // we need to work with the pointer, but can be reasonably sure it won't
  // go away, since it'll be an encapsulated message or one that was opened
  // from an .eml file.
  if ( msg->getMsgSerNum() != 0 ) {
    mMsgList.append( msg->getMsgSerNum() );
  } else {
    mStandAloneMessage = msg;
  }
  mUrl = subjectToUrl( msg->cleanSubject() );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent,
                                    const QList<KMMsgBase*> &msgList )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mStandAloneMessage( 0 ),
    mOffset( 0 ),
    mTotalSize( 0 )
{
  if ( msgList.empty() )
    return;
  setDeletesItself( true );
  // We operate on serNums and not the KMMsgBase pointers, as those can
  // change, or become invalid when changing the current message, switching
  // folders, etc.
  QList<KMMsgBase*>::const_iterator it;
  for ( it = msgList.begin(); it != msgList.end(); it++ ) {
    mMsgList.append( (*it)->getMsgSerNum() );
    mTotalSize += (*it)->msgSize();
    if ((*it)->parent() != 0)
      (*it)->parent()->open();
    ++it;
  }
  mMsgListIndex = 0;
  KMMsgBase *msgBase = *(msgList.begin());
  mUrl = subjectToUrl( msgBase->cleanSubject() );
}

KUrl KMSaveMsgCommand::url()
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
  connect(mJob, SIGNAL(result(KJob*)),
    SLOT(slotSaveResult(KJob*)));
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
    data = QByteArray( mData.data() + mOffset, remainingBytes );
    mJob->sendAsyncData( data );
    mOffset += remainingBytes;
    return;
  }
  // No leftovers, process next message.
  if ( mMsgListIndex < mMsgList.size() ) {
    KMMessage *msg = 0;
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( mMsgList[mMsgListIndex], &p, &idx );
    assert( p );
    assert( idx >= 0 );
    msg = p->getMsg(idx);

    if ( msg ) {
      if ( msg->transferInProgress() ) {
        QByteArray data = QByteArray();
        mJob->sendAsyncData( data );
      }
      msg->setTransferInProgress( true );
      if ( msg->isComplete() ) {
        slotMessageRetrievedForSaving( msg );
      } else {
        // retrieve Message first
        if ( msg->parent()  && !msg->isComplete() ) {
          FolderJob *job = msg->parent()->createJob( msg );
          job->setCancellable( false );
          connect(job, SIGNAL( messageRetrieved( KMMessage* ) ),
                  this, SLOT( slotMessageRetrievedForSaving( KMMessage* ) ) );
          job->start();
        }
      }
    } else {
      mJob->slotError( KIO::ERR_ABORTED,
                       i18n("The message was removed while saving it. "
                            "It has not been saved.") );
    }
  } else {
    if ( mStandAloneMessage ) {
      // do the special case of a standalone message
      slotMessageRetrievedForSaving( mStandAloneMessage );
      mStandAloneMessage = 0;
    } else {
      // No more messages. Tell the putjob we are done.
      QByteArray data = QByteArray();
      mJob->sendAsyncData( data );
    }
  }
}

void KMSaveMsgCommand::slotMessageRetrievedForSaving(KMMessage *msg)
{
  if ( msg ) {
    QByteArray str( msg->mboxMessageSeparator() );
    str += KMFolderMbox::escapeFrom( msg->asString() );
    str += '\n';
    msg->setTransferInProgress(false);

    mData = str;
    mData.resize(mData.size() - 1);
    mOffset = 0;
    QByteArray data;
    int size;
    // Unless it is great than 64 k send the whole message. kio buffers for us.
    if( mData.size() >  MAX_CHUNK_SIZE )
      size = MAX_CHUNK_SIZE;
    else
      size = mData.size();

    data = QByteArray( mData, size );
    mJob->sendAsyncData( data );
    mOffset += size;
  }
  ++mMsgListIndex;
  // Get rid of the message.
  if ( msg && msg->parent() && msg->getMsgSerNum() ) {
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( msg, &p, &idx );
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
    p->close();
  }
}

void KMSaveMsgCommand::slotSaveResult(KJob *job)
{
  if (job->error())
  {
    if (job->error() == KIO::ERR_FILE_ALREADY_EXIST)
    {
      if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?",
         mUrl.prettyUrl()), i18n("Save to File"), KGuiItem(i18n("&Replace")))
        == KMessageBox::Continue) {
        mOffset = 0;

        mJob = KIO::put( mUrl, S_IRUSR|S_IWUSR, true, false );
        mJob->slotTotalSize( mTotalSize );
        mJob->setAsyncDataEnabled( true );
        mJob->setReportDataSent( true );
        connect(mJob, SIGNAL(dataReq(KIO::Job*, QByteArray &)),
            SLOT(slotSaveDataReq()));
        connect(mJob, SIGNAL(result(KJob*)),
            SLOT(slotSaveResult(KJob*)));
      }
    }
    else
    {
      static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
      setResult( Failed );
      emit completed( this );
      deleteLater();
    }
  } else {
    setResult( OK );
    emit completed( this );
    deleteLater();
  }
}

//-----------------------------------------------------------------------------

KMOpenMsgCommand::KMOpenMsgCommand( QWidget *parent, const KUrl & url,
                                    const QString & encoding )
  : KMCommand( parent ),
    mUrl( url ),
    mEncoding( encoding )
{
  setDeletesItself( true );
}

KMCommand::Result KMOpenMsgCommand::execute()
{
  if ( mUrl.isEmpty() ) {
    mUrl = KFileDialog::getOpenUrl( KUrl(":OpenMessage"), "message/rfc822",
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
  connect( mJob, SIGNAL( result( KJob * ) ),
           SLOT( slotResult( KJob * ) ) );
  setEmitsCompletedItself( true );
  return OK;
}

void KMOpenMsgCommand::slotDataArrived( KIO::Job *, const QByteArray & data )
{
  if ( data.isEmpty() )
    return;

  mMsgString.append( data.data(), data.size() );
}

void KMOpenMsgCommand::slotResult( KJob *job )
{
  if ( job->error() ) {
    // handle errors
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    setResult( Failed );
    emit completed( this );
  }
  else {
    int startOfMessage = 0;
    if ( mMsgString.compare( 0, 5, "From ", 5 ) == 0 ) {
      startOfMessage = mMsgString.find( '\n' );
      if ( startOfMessage == -1 ) {
        KMessageBox::sorry( parentWidget(),
                            i18n( "The file does not contain a message." ) );
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
                          i18n( "The file does not contain a message." ) );
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
    win->showMsg( mEncoding, msg );
    win->show();
    if ( multipleMessages )
      KMessageBox::information( win,
                                i18n( "The file contains multiple messages. "
                                      "Only the first message is shown." ) );
    setResult( OK );
    emit completed( this );
  }
  deleteLater();
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
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset( msg->codec()->name(), true );
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
  KMMessage *reply = msg->createReply( KMail::ReplySmart, "", true);
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset(msg->codec()->name(), true);
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
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset(msg->codec()->name(), true);
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
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset( msg->codec()->name(), true );
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
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset( msg->codec()->name(), true );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMForwardCommand::KMForwardCommand( QWidget *parent,
  const QList<KMMsgBase*> &msgList, uint identity )
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
  QList<KMMessage*> msgList = retrievedMsgs();

  if (msgList.count() >= 2) {
    // ask if they want a mime digest forward

    if (KMessageBox::questionYesNo( parentWidget(),
                                    i18n("Forward selected messages as "
                                         "a MIME digest?"), QString(), KGuiItem(i18n("Send Digest")), KGuiItem(i18n("Send")) )
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
      QByteArray boundary( fwdMsg->mMsg->Headers().ContentType().Boundary().c_str() );
      msgPartText = i18n("\nThis is a MIME digest forward. The content of the"
                         " message is contained in the attachment(s).\n\n\n");
      // iterate through all the messages to be forwarded
      KMMessage *msg;
      foreach ( msg, msgList ) {
        // set the identity
        if (id == 0)
          id = msg->headerField("X-KMail-Identity").trimmed().toUInt();
        // set the part header
        msgPartText += "--";
        msgPartText += QString::fromLatin1( boundary );
        msgPartText += "\nContent-Type: MESSAGE/RFC822";
        msgPartText += QString("; CHARSET=%1").arg( QString::fromLatin1( msg->charset() ) );
        msgPartText += '\n';
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
        msgPartText += '\n';
        msgPartText += msg->body();
        msgPartText += '\n';     // eot
        msgCnt++;
        fwdMsg->link( msg, MessageStatus::statusForwarded() );
      }
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      fwdMsg->initHeader(id);
      msgPartText += "--";
      msgPartText += QString::fromLatin1( boundary );
      msgPartText += "--\n";
      Q3CString tmp;
      msgPart->setTypeStr("MULTIPART");
      tmp.sprintf( "Digest; boundary=\"%s\"", boundary.data() );
      msgPart->setSubtypeStr( tmp );
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(msgPartText.toAscii());
      KCursorSaver busy(KBusyPtr::busy());
      KMail::Composer * win = KMail::makeComposer( fwdMsg, id );
      win->addAttach(msgPart);
      win->show();
      return OK;
    } else {            // NO MIME DIGEST, Multiple forward
      uint id = 0;
      QList<KMMessage*> linklist;
      QList<KMMessage*>::const_iterator it;
      for ( it = msgList.begin(); it != msgList.end(); it++ ) {
        // set the identity
        if (id == 0)
          id = (*it)->headerField("X-KMail-Identity").trimmed().toUInt();

        // msgText += (*it)->createForwardBody();
        linklist.append( (*it) );
      }
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      KMMessage *fwdMsg = new KMMessage;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");
      // fwdMsg->setBody(msgText);

      for ( QList<KMMessage*>::const_iterator it2 = linklist.begin();
	    it2 != linklist.end(); ++it2 )
      {
	KMMessage *msg = *it;
	TemplateParser parser( fwdMsg, TemplateParser::Forward,
			       msg->body(), false, false, false, false);
        parser.process( msg, 0, true );

        fwdMsg->link( (*it), MessageStatus::statusForwarded() );
      }

      KCursorSaver busy(KBusyPtr::busy());
      KMail::Composer * win = KMail::makeComposer( fwdMsg, id );
      win->setCharset("");
      win->show();
      return OK;
    }
  }

  // forward a single message at most.
  KMMessage *msg = msgList.first();
  if ( !msg || !msg->codec() )
    return Failed;

  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *fwdMsg = msg->createForward();

  uint id = msg->headerField("X-KMail-Identity").trimmed().toUInt();
  if ( id == 0 )
    id = mIdentity;
  {
    KMail::Composer * win = KMail::makeComposer( fwdMsg, id );
    win->setCharset( fwdMsg->codec()->name(), true );
    // win->setBody( QString::fromUtf8( msg->createForwardBody() ) );
    win->show();
  }

  return OK;
}


KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
           const QList<KMMsgBase*> &msgList, uint identity, KMail::Composer *win )
  : KMCommand( parent, msgList ), mIdentity( identity ),
    mWin( QPointer<KMail::Composer>( win ))
{
}

KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
           KMMessage * msg, uint identity, KMail::Composer *win )
  : KMCommand( parent, msg ), mIdentity( identity ),
    mWin( QPointer< KMail::Composer >( win ))
{
}

KMCommand::Result KMForwardAttachedCommand::execute()
{
  QList<KMMessage*> msgList = retrievedMsgs();
  KMMessage *fwdMsg = new KMMessage;

  if (msgList.count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    fwdMsg->initHeader(mIdentity);
  }
  else if (msgList.count() == 1) {
    KMMessage *msg = msgList.first();
    fwdMsg->initFromMessage(msg);
    fwdMsg->setSubject( msg->forwardSubject() );
  }

  fwdMsg->setAutomaticFields(true);

  KCursorSaver busy(KBusyPtr::busy());
  if (!mWin)
    mWin = KMail::makeComposer(fwdMsg, mIdentity);

  // iterate through all the messages to be forwarded
  KMMessage *msg;
  foreach ( msg, msgList ) {
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
    QList<int> dummy;
    msgPart->setBodyAndGuessCte(msg->asString(), dummy, true);
    msgPart->setCharset("");

    fwdMsg->link( msg, MessageStatus::statusForwarded() );
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
  KMMessage *msg = retrievedMessage();
  if ( !msg || !msg->codec() )
    return Failed;

  RedirectDialog dlg( parentWidget(), kmkernel->msgSender()->sendImmediate() );
  dlg.setObjectName( "redirect" );
  if (dlg.exec()==QDialog::Rejected) return Failed;

  KMMessage *newMsg = msg->createRedirect( dlg.to() );
  KMFilterAction::sendMDN( msg, KMime::MDN::Dispatched );

  const KMail::MessageSender::SendMethod method = dlg.sendImmediate()
    ? KMail::MessageSender::SendImmediate
    : KMail::MessageSender::SendLater;
  if ( !kmkernel->msgSender()->send( newMsg, method ) ) {
    kDebug(5006) << "KMRedirectCommand: could not redirect message (sending failed)" << endl;
    return Failed; // error: couldn't send
  }
  return OK;
}


KMCustomReplyToCommand::KMCustomReplyToCommand( QWidget *parent, KMMessage *msg,
                                                const QString &selection,
                                                const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplySmart, mSelection,
                                       false, true, false, mTemplate );
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset( msg->codec()->name(), true );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMCustomReplyAllToCommand::KMCustomReplyAllToCommand( QWidget *parent, KMMessage *msg,
                                                      const QString &selection,
                                                      const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomReplyAllToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  KMMessage *msg = retrievedMessage();
  KMMessage *reply = msg->createReply( KMail::ReplyAll, mSelection,
                                       false, true, false, mTemplate );
  KMail::Composer * win = KMail::makeComposer( reply );
  win->setCharset( msg->codec()->name(), true );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
  const QList<KMMsgBase*> &msgList, uint identity, const QString &tmpl )
  : KMCommand( parent, msgList ),
    mIdentity( identity ), mTemplate( tmpl )
{
}

KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
  KMMessage *msg, uint identity, const QString &tmpl )
  : KMCommand( parent, msg ),
    mIdentity( identity ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomForwardCommand::execute()
{
  QList<KMMessage*> msgList = retrievedMsgs();

  if (msgList.count() >= 2) { // Multiple forward

    uint id = 0;
    // QCString msgText = "";
    QList<KMMessage*> linklist;
    for ( QList<KMMessage*>::const_iterator it = msgList.begin(); it != msgList.end(); ++it )
    {
      KMMessage *msg = *it;
      // set the identity
      if (id == 0)
        id = msg->headerField( "X-KMail-Identity" ).trimmed().toUInt();

      // msgText += msg->createForwardBody();
      linklist.append( msg );
    }
    if ( id == 0 )
      id = mIdentity; // use folder identity if no message had an id set
    KMMessage *fwdMsg = new KMMessage;
    fwdMsg->initHeader( id );
    fwdMsg->setAutomaticFields( true );
    fwdMsg->setCharset( "utf-8" );
    // fwdMsg->setBody( msgText );

    for ( QList<KMMessage*>::const_iterator it = linklist.begin(); it != linklist.end(); ++it )
    {
      KMMessage *msg = *it;
      TemplateParser parser( fwdMsg, TemplateParser::Forward,
        msg->body(), false, false, false, false);
        parser.process( msg, 0, true );

      fwdMsg->link( msg, MessageStatus::statusForwarded() );
    }

    KCursorSaver busy( KBusyPtr::busy() );
    KMail::Composer * win = KMail::makeComposer( fwdMsg, id );
    win->setCharset("");
    win->show();

  } else { // forward a single message at most

    KMMessage *msg = msgList.first();
    if ( !msg || !msg->codec() )
      return Failed;

    KCursorSaver busy( KBusyPtr::busy() );
    KMMessage *fwdMsg = msg->createForward( mTemplate );

    uint id = msg->headerField( "X-KMail-Identity" ).trimmed().toUInt();
    if ( id == 0 )
      id = mIdentity;
    {
      KMail::Composer * win = KMail::makeComposer( fwdMsg, id );
      win->setCharset( fwdMsg->codec()->name(), true );
      win->show();
    }
  }
  return OK;
}


KMPrintCommand::KMPrintCommand( QWidget *parent,
  KMMessage *msg, bool htmlOverride, bool htmlLoadExtOverride,
  bool useFixedFont, const QString & encoding )
  : KMCommand( parent, msg ), mHtmlOverride( htmlOverride ),
    mHtmlLoadExtOverride( htmlLoadExtOverride ),
    mUseFixedFont( useFixedFont ), mEncoding( encoding )
{
}

KMCommand::Result KMPrintCommand::execute()
{
  KMReaderWin printWin( 0, 0, 0 );
  printWin.setPrinting( true );
  printWin.readConfig();
  printWin.setHtmlOverride( mHtmlOverride );
  printWin.setHtmlLoadExtOverride( mHtmlLoadExtOverride );
  printWin.setUseFixedFont( mUseFixedFont );
  printWin.setOverrideEncoding( mEncoding );
  printWin.setMsg( retrievedMessage(), true );
  printWin.printMsg();

  return OK;
}


KMSetStatusCommand::KMSetStatusCommand( const MessageStatus& status,
  const QList<quint32> &serNums, bool toggle )
  : mStatus( status ), mSerNums( serNums ), mToggle( toggle )
{
}

KMCommand::Result KMSetStatusCommand::execute()
{
  QList<quint32>::Iterator it;
  int idx = -1;
  KMFolder *folder = 0;
  bool parentStatus = false;

  // Toggle actions on threads toggle the whole thread
  // depending on the state of the parent.
  if (mToggle) {
    KMMsgBase *msg;
    KMMsgDict::instance()->getLocation( *mSerNums.begin(), &folder, &idx );
    if (folder) {
      msg = folder->getMsgBase(idx);
      if ( msg && ( msg->status() & mStatus ))
        parentStatus = true;
      else
        parentStatus = false;
    }
  }
  QMap< KMFolder*, QList<int> > folderMap;
  for ( it = mSerNums.begin(); it != mSerNums.end(); ++it ) {
    KMMsgDict::instance()->getLocation( *it, &folder, &idx );
    if (folder) {
      if (mToggle) {
        KMMsgBase *msg = folder->getMsgBase(idx);
        // check if we are already at the target toggle state
        if (msg) {
          bool myStatus;
          if ( msg->status() & mStatus)
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
  QMap< KMFolder*, QList<int> >::Iterator it2 = folderMap.begin();
  while ( it2 != folderMap.end() ) {
     KMFolder *f = it2.key();
     f->setStatus( (*it2), mStatus, mToggle );
     ++it2;
  }
  //kapp->dcopClient()->emitDCOPSignal( "unreadCountChanged()", QByteArray() );

  return OK;
}


KMFilterCommand::KMFilterCommand( const QByteArray &field, const QString &value )
  : mField( field ), mValue( value )
{
}

KMCommand::Result KMFilterCommand::execute()
{
  kmkernel->filterMgr()->createFilter( mField, mValue );

  return OK;
}


KMFilterActionCommand::KMFilterActionCommand( QWidget *parent,
                                              const QList<KMMsgBase*> &msgList,
                                              KMFilter *filter )
  : KMCommand( parent, msgList ), mFilter( filter  )
{
  QList<KMMsgBase*>::const_iterator it;
  for ( it = msgList.begin(); it != msgList.end(); it++ )
    serNumList.append( (*it)->getMsgSerNum() );
}

KMCommand::Result KMFilterActionCommand::execute()
{
  KCursorSaver busy( KBusyPtr::busy() );

  int msgCount = 0;
  int msgCountToFilter = serNumList.count();
  ProgressItem* progressItem =
     ProgressManager::createProgressItem (
         "filter"+ProgressManager::getUniqueID(),
         i18n( "Filtering messages" ) );
  progressItem->setTotalItems( msgCountToFilter );

  QList<quint32>::const_iterator it;
  for ( it = serNumList.begin(); it != serNumList.end(); it++ ) {
    quint32 serNum = *it;
    int diff = msgCountToFilter - ++msgCount;
    if ( diff < 10 || !( msgCount % 10 ) || msgCount <= 10 ) {
      progressItem->updateProgress();
      QString statusMsg = i18n( "Filtering message %1 of %2",
                                msgCount, msgCountToFilter );
      KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );
      qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 50 );
    }

    int filterResult = kmkernel->filterMgr()->process( serNum, mFilter );
    if (filterResult == 2) {
      // something went horribly wrong (out of space?)
      perror("Critical error");
      kmkernel->emergencyExit( i18n("Not enough free disk space?" ));
    }
    progressItem->incCompletedItems();
  }

  progressItem->setComplete();
  progressItem = 0;
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
  if ( ActionScheduler::isEnabled() && false ) { // don't use it for now
    // use action scheduler
    KMFilterMgr::FilterSet set = KMFilterMgr::All;
    QList<KMFilter*> filters;
    filters.append( mFilter );
    ActionScheduler *scheduler = new ActionScheduler( set, filters, mHeaders );
    scheduler->setAlwaysMatch( true );
    scheduler->setAutoDestruct( true );

    int contentX, contentY;
    HeaderItem *nextItem = mHeaders->prepareMove( &contentX, &contentY );
#ifdef __GNUC__
#warning Port me!
#endif
    QList<KMMsgBase*> msgList = *mHeaders->selectedMsgs(true);
    mHeaders->finalizeMove( nextItem, contentX, contentY );

    KMMsgBase *msg;
    foreach( msg, msgList )
      scheduler->execFilters( msg );
  } else {
    KMCommand *filterCommand = new KMFilterActionCommand( mMainWidget,
        *mHeaders->selectedMsgs(), mFilter);
    filterCommand->start();
    int contentX, contentY;
    HeaderItem *item = mHeaders->prepareMove( &contentX, &contentY );
    mHeaders->finalizeMove( item, contentX, contentY );
  }
}

FolderShortcutCommand::FolderShortcutCommand( KMMainWidget *mainwidget,
                                              KMFolder *folder )
    : mMainWidget( mainwidget ), mFolder( folder ), mAction( 0 )
{
}


FolderShortcutCommand::~FolderShortcutCommand()
{
  delete mAction;
}

void FolderShortcutCommand::start()
{
  mMainWidget->slotSelectFolder( mFolder );
}

void FolderShortcutCommand::setAction( QAction* action )
{
  mAction = action;
}

KMMailingListFilterCommand::KMMailingListFilterCommand( QWidget *parent,
                                                        KMMessage *msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMMailingListFilterCommand::execute()
{
  Q3CString name;
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
  QObject *receiver, KMMenuToFolder *aMenuToFolder, QMenu *menu )
{
#ifdef __GNUC__
#warning Port me!
#endif
/*  while ( menu->count() )
  {
    QMenu *popup = menu->findItem( menu->idAt( 0 ) )->popup();
    if (popup)
      delete popup;
    else
      menu->removeItemAt( 0 );
  }*/

  if (!kmkernel->imapFolderMgr()->dir().first() &&
      !kmkernel->dimapFolderMgr()->dir().first())
  { // only local folders
    makeFolderMenu( &kmkernel->folderMgr()->dir(), move,
                    receiver, aMenuToFolder, menu );
  } else {
    // operate on top-level items
    QMenu *subMenu = menu->addMenu( i18n( "Local Folders" ) );
    makeFolderMenu( &kmkernel->folderMgr()->dir(),
                    move, receiver, aMenuToFolder, subMenu );
    KMFolderDir* fdir = &kmkernel->imapFolderMgr()->dir();
    KMFolderNode *node;
    QList<KMFolderNode*>::const_iterator it;
    for ( it = fdir->begin(); ( node = *it ) && it != fdir->end(); ++it ) {
      if (node->isDir())
        continue;
      subMenu = menu->addMenu( node->label() );
      makeFolderMenu( node, move, receiver, aMenuToFolder, subMenu );
    }
    fdir = &kmkernel->dimapFolderMgr()->dir();
    for ( it = fdir->begin(); ( node = *it ) && it != fdir->end(); ++it ) {
      if (node->isDir())
        continue;
      subMenu = menu->addMenu( node->label() );
      makeFolderMenu( node, move, receiver, aMenuToFolder, subMenu );
    }
  }
}

void KMMenuCommand::makeFolderMenu(KMFolderNode* node, bool move,
  QObject *receiver, KMMenuToFolder *aMenuToFolder, QMenu *menu )
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
    QAction* act;
    if (move)
      act = menu->addAction(i18n("Move to This Folder"));
    else
      act = menu->addAction(i18n("Copy to This Folder"));
    aMenuToFolder->insert( act, folder );
    act->setEnabled( !folder->isReadOnly() );
    menu->addSeparator();
  }

  if (!folderDir)
    return;

  QList<KMFolderNode*>::const_iterator it;
  for ( it = folderDir->begin(); (*it) && it != folderDir->end(); ++it ) {
    if ( (*it)->isDir() )
      continue;
    KMFolder *child = static_cast<KMFolder*>(*it);
    QString label = child->label();
    label.replace("&","&&");
    if (child->child() && child->child()->first()) {
      // descend
      QMenu *subMenu = menu->addMenu( QString::null );
      makeFolderMenu( child, move, receiver,
                      aMenuToFolder, subMenu );
    } else {
      // insert an item
      QAction* act = menu->addAction( label );
      aMenuToFolder->insert( act, child );
      act->setEnabled( !child->isReadOnly() );
    }
  }
  return;
}


KMCopyCommand::KMCopyCommand( KMFolder* destFolder,
                              const QList<KMMsgBase*> &msgList )
:mDestFolder( destFolder ), mMsgList( msgList )
{
  setDeletesItself( true );
}

KMCopyCommand::KMCopyCommand( KMFolder* destFolder, KMMessage * msg )
  :mDestFolder( destFolder )
{
  setDeletesItself( true );
  mMsgList.append( &msg->toMsgBase() );
}

KMCommand::Result KMCopyCommand::execute()
{
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int idx = -1;
  bool isMessage;
  QList<KMMessage*> list;
  QList<KMMessage*> localList;

  if (mDestFolder && mDestFolder->open() != 0)
  {
    deleteLater();
    return Failed;
  }

  setEmitsCompletedItself( true );
  KCursorSaver busy(KBusyPtr::busy());

  QList<KMMsgBase*>::const_iterator it;
  for ( it = mMsgList.begin(); it != mMsgList.end(); it++ )
  {
    msgBase = (*it);
    KMFolder *srcFolder = msgBase->parent();
    if (isMessage = msgBase->isMessage())
    {
      msg = static_cast<KMMessage*>(msgBase);
    } else {
      idx = srcFolder->find(msgBase);
      assert(idx != -1);
      msg = srcFolder->getMsg(idx);
    }

    if (srcFolder && mDestFolder &&
        (srcFolder->folderType()== KMFolderTypeImap) &&
        (mDestFolder->folderType() == KMFolderTypeImap) &&
        (static_cast<KMFolderImap*>(srcFolder->storage())->account() ==
         static_cast<KMFolderImap*>(mDestFolder->storage())->account()))
    {
      // imap => imap with same account
      list.append(msg);
    } else {
      newMsg = new KMMessage;
      newMsg->setComplete(msg->isComplete());
      // make sure the attachment state is only calculated when it's complete
      if (!newMsg->isComplete())
        newMsg->setReadyToShow(false);
      newMsg->fromString(msg->asString());
      newMsg->setStatus( msg->messageStatus() );

      if (srcFolder && !newMsg->isComplete())
      {
        // imap => others
        newMsg->setParent(msg->parent());
        FolderJob *job = srcFolder->createJob(newMsg);
        job->setCancellable( false );
        mPendingJobs << job;
        connect(job, SIGNAL(messageRetrieved(KMMessage*)),
                mDestFolder, SLOT(reallyAddCopyOfMsg(KMMessage*)));
        connect( job, SIGNAL(result(KMail::FolderJob*)),
                 this, SLOT(slotJobFinished(KMail::FolderJob*)) );
        job->start();
      } else {
        // local => others
        localList.append(newMsg);
      }
    }

    if (srcFolder && !isMessage && list.isEmpty())
    {
      assert(idx != -1);
      srcFolder->unGetMsg( idx );
    }

  } // end for

  bool deleteNow = false;
  if (!localList.isEmpty())
  {
    QList<int> index;
    mDestFolder->addMsg( localList, index );
    for ( QList<int>::Iterator it = index.begin(); it != index.end(); ++it ) {
      mDestFolder->unGetMsg( *it );
    }
    if ( mDestFolder->folderType() == KMFolderTypeImap ) {
      if ( mPendingJobs.isEmpty() ) {
        // wait for the end of the copy before closing the folder
        KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
        connect( imapDestFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
            this, SLOT( slotFolderComplete( KMFolderImap*, bool ) ) );
      }
    } else {
      deleteNow = list.isEmpty() && mPendingJobs.isEmpty(); // we're done if there are no other mails we need to fetch
    }
  }

//TODO: Get rid of the other cases just use this one for all types of folder
//TODO: requires adding copyMsg and getFolder methods to KMFolder.h
  if (!list.isEmpty())
  {
    // copy the message(s); note: the list is empty afterwards!
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
    connect( imapDestFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
        this, SLOT( slotFolderComplete( KMFolderImap*, bool ) ) );
    imapDestFolder->copyMsg(list);
    imapDestFolder->getFolder();
  }

  // only close the folder and delete the job if we're done
  // otherwise this is done in slotMsgAdded or slotFolderComplete
  if ( deleteNow )
  {
    mDestFolder->close();
    setResult( OK );
    emit completed( this );
    deleteLater();
  }

  return OK;
}

void KMCopyCommand::slotJobFinished(KMail::FolderJob * job)
{
  mPendingJobs.remove( job );
  if ( job->error() ) {
    kDebug(5006) << k_funcinfo << "folder job failed: " << job->error() << endl;
    // kill all pending jobs
    for ( QList<KMail::FolderJob*>::Iterator it = mPendingJobs.begin(); it != mPendingJobs.end(); ++it ) {
      disconnect( (*it), SIGNAL(result(KMail::FolderJob*)),
                  this, SLOT(slotJobFinished(KMail::FolderJob*)) );
      (*it)->kill();
    }
    mPendingJobs.clear();
    setResult( Failed );
  }

  if ( mPendingJobs.isEmpty() )
  {
    mDestFolder->close();
    emit completed( this );
    deleteLater();
  }
}

void KMCopyCommand::slotFolderComplete( KMFolderImap*, bool success )
{
  kDebug(5006) << k_funcinfo << success << endl;
  if ( !success )
    setResult( Failed );
  mDestFolder->close();
  emit completed( this );
  deleteLater();
}


KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              const QList<KMMsgBase*> &msgList)
  : mDestFolder( destFolder ), mMsgList( msgList ), mProgressItem( 0 )
{
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMessage *msg )
  : mDestFolder( destFolder ), mProgressItem( 0 )
{
  mMsgList.append( &msg->toMsgBase() );
}

KMMoveCommand::KMMoveCommand( KMFolder* destFolder,
                              KMMsgBase *msgBase )
  : mDestFolder( destFolder ), mProgressItem( 0 )
{
  mMsgList.append( msgBase );
}

KMMoveCommand::KMMoveCommand( quint32 )
  : mProgressItem( 0 )
{
}

KMCommand::Result KMMoveCommand::execute()
{
  setEmitsCompletedItself( true );
  setDeletesItself( true );
  typedef QMap< KMFolder*, QList<KMMessage*>* > FolderToMessageListMap;
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
  connect( mProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotMoveCanceled() ) );

  KMMessage *msg;
  KMMsgBase *msgBase;
  int rc = 0;
  int index;
  QList<KMMessage*> list;
  int undoId = -1;
  mCompleteWithAddedMsg = false;

  if (mDestFolder) {
    connect (mDestFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
             this, SLOT(slotMsgAddedToDestFolder(KMFolder*, quint32)));
    foreach ( msgBase, mMsgList ) {
      mLostBoys.append( msgBase->getMsgSerNum() );
    }
  }
  mProgressItem->setTotalItems( mMsgList.count() );

  QList<KMMsgBase*>::const_iterator it;
  for ( it = mMsgList.begin(); (*it) && !rc && it != mMsgList.end(); it++ ) {
    msgBase = (*it);
    KMFolder *srcFolder = msgBase->parent();
    if (srcFolder == mDestFolder)
      continue;
    bool undo = msgBase->enableUndo();
    int idx = srcFolder->find(msgBase);
    assert(idx != -1);
    if ( msgBase->isMessage() ) {
      msg = static_cast<KMMessage*>(msgBase);
    } else {
      msg = srcFolder->getMsg(idx);
    }

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
        if ( srcFolder->folderType() == KMFolderTypeImap ) {
          // do not complete here but wait until all messages are transferred
          mCompleteWithAddedMsg = true;
        }
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
          folderDeleteList[srcFolder] = new QList<KMMessage*>;
        folderDeleteList[srcFolder]->append( msg );
      } else {
        srcFolder->removeMsg(idx);
        delete msg;
      }
    }
  }
  if (!list.isEmpty() && mDestFolder) {
    // will be completed with folderComplete signal
    mDestFolder->moveMsg(list, &index);
  } else {
    FolderToMessageListMap::Iterator it;
    for ( it = folderDeleteList.begin(); it != folderDeleteList.end(); ++it ) {
      it.key()->removeMsg(*it.value());
      delete it.value();
    }
    if ( !mCompleteWithAddedMsg ) {
      // imap folders will be completed in slotMsgAddedToDestFolder
      completeMove( OK );
    }
  }

  return OK;
}

void KMMoveCommand::slotImapFolderCompleted(KMFolderImap* imapFolder, bool success)
{
  disconnect (imapFolder, SIGNAL(folderComplete( KMFolderImap*, bool )),
      this, SLOT(slotImapFolderCompleted( KMFolderImap*, bool )));
  if ( success ) {
    // the folder was checked successfully but we were still called, so check
    // if we are still waiting for messages to show up. If so, uidValidity
    // changed, or something else went wrong. Clean up.

    /* Unfortunately older UW imap servers change uid validity for each put job.
     * Yes, it is really that broken. *sigh* So we cannot report error here, I guess. */
    if ( !mLostBoys.isEmpty() ) {
      kDebug(5006) <<  "### Not all moved messages reported back that they were " << endl
                    <<  "### added to the target folder. Did uidValidity change? " << endl;
    }
    completeMove( OK );
  } else {
    // Should we inform the user here or leave that to the caller?
    completeMove( Failed );
  }
}

void KMMoveCommand::slotMsgAddedToDestFolder(KMFolder *folder, quint32 serNum)
{
  if ( folder != mDestFolder || !mLostBoys.contains( serNum )  ) {
    //kDebug(5006) << "KMMoveCommand::msgAddedToDestFolder different "
    //                 "folder or invalid serial number." << endl;
    return;
  }
  mLostBoys.removeAll(serNum);
  if ( mLostBoys.isEmpty() ) {
    // we are done. All messages transferred to the host successfully
    disconnect (mDestFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
             this, SLOT(slotMsgAddedToDestFolder(KMFolder*, quint32)));
    if (mDestFolder && mDestFolder->folderType() != KMFolderTypeImap) {
      mDestFolder->sync();
    }
    if ( mCompleteWithAddedMsg ) {
      completeMove( OK );
    }
  } else {
    if ( mProgressItem ) {
      mProgressItem->incCompletedItems();
      mProgressItem->updateProgress();
    }
  }
}

void KMMoveCommand::completeMove( Result result )
{
  if ( mDestFolder )
    mDestFolder->close();
  while ( !mOpenedFolders.empty() ) {
    KMFolder *folder = mOpenedFolders.back();
    mOpenedFolders.pop_back();
    folder->close();
  }
  if ( mProgressItem ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
  }
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
  const QList<KMMsgBase*> &msgList )
:KMMoveCommand( findTrashFolder( srcFolder ), msgList)
{
  srcFolder->open();
  mOpenedFolders.push_back( srcFolder );
}

KMDeleteMsgCommand::KMDeleteMsgCommand( KMFolder* srcFolder, KMMessage * msg )
:KMMoveCommand( findTrashFolder( srcFolder ), msg)
{
  srcFolder->open();
  mOpenedFolders.push_back( srcFolder );
}

KMDeleteMsgCommand::KMDeleteMsgCommand( quint32 sernum )
:KMMoveCommand( sernum )
{
  KMFolder *srcFolder = 0;
  int idx;
  KMMsgDict::instance()->getLocation( sernum, &srcFolder, &idx );
  if ( srcFolder ) {
    KMMsgBase *msg = srcFolder->getMsgBase( idx );
    srcFolder->open();
    mOpenedFolders.push_back( srcFolder );
    addMsg( msg );
    setDestFolder( findTrashFolder( srcFolder ) );
  }
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


KMUrlClickedCommand::KMUrlClickedCommand( const KUrl &url, uint identity,
  KMReaderWin *readerWin, bool htmlPref, KMMainWidget *mainWidget )
  :mUrl( url ), mIdentity( identity ), mReaderWin( readerWin ),
   mHtmlPref( htmlPref ), mMainWidget( mainWidget )
{
}

KMCommand::Result KMUrlClickedCommand::execute()
{
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
      int secondQuery = query.indexOf('?',1);
      if (secondQuery != -1)
        queryPart = query.left(secondQuery);
      else
        queryPart = query;
      query = query.mid(queryPart.length());

      if (queryPart.left(9) == "?subject=")
        msg->setSubject( KUrl::fromPercentEncoding(queryPart.mid(9).toLatin1()) );
      else if (queryPart.left(6) == "?body=")
        // It is correct to convert to latin1() as URL should not contain
        // anything except ascii.
        msg->setBody( KUrl::fromPercentEncoding(queryPart.mid(6).toLatin1()).toLatin1() );
      else if (queryPart.left(4) == "?cc=")
        msg->setCc( KUrl::fromPercentEncoding(queryPart.mid(4).toLatin1()) );
    }

    KMail::Composer * win = KMail::makeComposer( msg, mIdentity );
    win->setCharset("", true);
    win->show();
  }
  else if ( mUrl.protocol() == "im" )
  {
#ifdef __GNUC__
#warning "port me"
#endif
    //kmkernel->imProxy()->chatWithContact( mUrl.path() );
  }
  else if ((mUrl.protocol() == "http") || (mUrl.protocol() == "https") ||
           (mUrl.protocol() == "ftp")  || (mUrl.protocol() == "file")  ||
           (mUrl.protocol() == "ftps") || (mUrl.protocol() == "sftp" ) ||
           (mUrl.protocol() == "help") || (mUrl.protocol() == "vnc")   ||
           (mUrl.protocol() == "smb")  || (mUrl.protocol() == "fish")  ||
           (mUrl.protocol() == "news"))
  {
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Opening URL..."));
    KMimeType::Ptr mime = KMimeType::findByUrl( mUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-msdos-program" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18n( "<qt>Do you really want to execute <b>%1</b>?</qt>" ,
          mUrl.prettyUrl() ), QString(), KGuiItem(i18n("Execute")), KStandardGuiItem::cancel() ) != KMessageBox::Yes)
        return Canceled;
    }
    (void) new KRun( mUrl, mMainWidget );
  }
  else
    return Failed;

  return OK;
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, KMMessage *msg )
  : KMCommand( parent, msg ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const QList<KMMsgBase*>& msgs )
  : KMCommand( parent, msgs ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, QList<partNode*>& attachments,
                                                    KMMessage *msg, bool encoded )
  : KMCommand( parent ), mImplicitAttachments( false ), mEncoded( encoded )
{
  QList<partNode*>::const_iterator it;
  for ( it = attachments.constBegin(); it != attachments.constEnd(); ++it ) {
    mAttachmentMap.insert( (*it), msg );
  }
}

KMCommand::Result KMSaveAttachmentsCommand::execute()
{
  setEmitsCompletedItself( true );
  if ( mImplicitAttachments ) {
    QList<KMMessage*> msgList = retrievedMsgs();
    QList<KMMessage*>::const_iterator it;
    for ( it = msgList.begin(); it != msgList.end(); ++it ) {
      KMMessage *msg = (*it);
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
  // don't represent an attachment if they were not explicitly passed in the
  // c'tor
  if ( mImplicitAttachments ) {
    for ( PartNodeMessageMap::iterator it = mAttachmentMap.begin();
          it != mAttachmentMap.end(); ) {
      // only body parts which have a filename or a name parameter (except for
      // the root node for which name is set to the message's subject) are
      // considered attachments
      if ( it.key()->msgPart().fileName().trimmed().isEmpty() &&
           ( it.key()->msgPart().name().trimmed().isEmpty() ||
             !it.key()->parentNode() ) ) {
        PartNodeMessageMap::iterator delIt = it;
        ++it;
        mAttachmentMap.erase( delIt );
      }
      else
        ++it;
    }
    if ( mAttachmentMap.isEmpty() ) {
      KMessageBox::information( 0, i18n("Found no attachments to save.") );
      setResult( OK ); // The user has already been informed.
      emit completed( this );
      deleteLater();
      return;
    }
  }

  KUrl url, dirUrl;
  if ( mAttachmentMap.count() > 1 ) {
    // get the dir
    dirUrl = KDirSelectDialog::selectDirectory( QString(), false,
                                                parentWidget(),
                                                i18n("Save Attachments To") );
    if ( !dirUrl.isValid() ) {
      setResult( Canceled );
      emit completed( this );
      deleteLater();
      return;
    }

    // we may not get a slash-terminated url out of KDirSelectDialog
    dirUrl.adjustPath( KUrl::AddTrailingSlash );
  }
  else {
    // only one item, get the desired filename
    partNode *node = mAttachmentMap.begin().key();
    // replace all ':' with '_' because ':' isn't allowed on FAT volumes
    QString s =
      node->msgPart().fileName().trimmed().replace( ':', '_' );
    if ( s.isEmpty() )
      s = node->msgPart().name().trimmed().replace( ':', '_' );
    if ( s.isEmpty() )
      s = i18nc("filename for an unnamed attachment", "attachment.1");
    url = KFileDialog::getSaveUrl( s, QString(), parentWidget(),
                                   QString() );
    if ( url.isEmpty() ) {
      setResult( Canceled );
      emit completed( this );
      deleteLater();
      return;
    }
  }

  QMap< QString, int > renameNumbering;

  Result globalResult = OK;
  int unnamedAtmCount = 0;
  for ( PartNodeMessageMap::const_iterator it = mAttachmentMap.begin();
        it != mAttachmentMap.end();
        ++it ) {
    KUrl curUrl;
    if ( !dirUrl.isEmpty() ) {
      curUrl = dirUrl;
      QString s =
        it.key()->msgPart().fileName().trimmed().replace( ':', '_' );
      if ( s.isEmpty() )
        s = it.key()->msgPart().name().trimmed().replace( ':', '_' );
      if ( s.isEmpty() ) {
        ++unnamedAtmCount;
        s = i18nc("filename for the %1-th unnamed attachment",
                 "attachment.%1",
              unnamedAtmCount );
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
       int dotIdx = file.lastIndexOf('.');
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
              i18n( "A file named %1 already exists. Do you want to overwrite it?" ,
                curUrl.fileName() ),
              i18n( "File Already Exists" ), KGuiItem(i18n("&Overwrite")) ) == KMessageBox::Cancel) {
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
  deleteLater();
}

KMCommand::Result KMSaveAttachmentsCommand::saveItem( partNode *node,
                                                      const KUrl& url )
{
  bool bSaveEncrypted = false;
  bool bEncryptedParts = node->encryptionState() != KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?" ,
           url.fileName() ),
          i18n( "KMail Question" ), KGuiItem(i18n("Keep Encryption")), KGuiItem(i18n("Do Not Keep")) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if( node->signatureState() != KMMsgNotSigned )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is signed. Do you want to keep the signature when saving?" ,
           url.fileName() ),
          i18n( "KMail Question" ), KGuiItem(i18n("Keep Signature")), KGuiItem(i18n("Do Not Keep")) ) !=
        KMessageBox::Yes )
      bSaveWithSig = false;

  QByteArray data;
  if ( mEncoded )
  {
    // This does not decode the Message Content-Transfer-Encoding
    // but saves the _original_ content of the message part
    data = node->msgPart().body();
  }
  else
  {
    if( bSaveEncrypted || !bEncryptedParts) {
      partNode *dataNode = node;
      QByteArray rawReplyString;
      bool gotRawReplyString = false;
      if( !bSaveWithSig ) {
        if( DwMime::kTypeMultipart == node->type() &&
            DwMime::kSubtypeSigned == node->subType() ){
          // carefully look for the part that is *not* the signature part:
          if( node->findType( DwMime::kTypeApplication,
                DwMime::kSubtypePgpSignature,
                true, false ) ){
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                DwMime::kSubtypePgpSignature,
                true, false );
          }else if( node->findType( DwMime::kTypeApplication,
                DwMime::kSubtypePkcs7Mime,
                true, false ) ){
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                DwMime::kSubtypePkcs7Mime,
                true, false );
          }else{
            dataNode = node->findTypeNot( DwMime::kTypeMultipart,
                DwMime::kSubtypeUnknown,
                true, false );
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
        size = KMail::Util::crlf2lf( cstr.data(), size );
      }
      data.resize( size );
    }
  }
  QDataStream ds;
  QFile file;
  KTemporaryFile tf;
  if ( url.isLocalFile() )
  {
    // save directly
    file.setFileName( url.path() );
    if ( !file.open( QIODevice::WriteOnly ) )
    {
      KMessageBox::error( parentWidget(),
          i18nc( "%2 is detailed error description",
            "Could not write the file %1:\n%2" ,
            file.fileName() ,
            QString::fromLocal8Bit( strerror( errno ) ) ),
          i18n( "KMail Error" ) );
      return Failed;
    }
    fchmod( file.handle(), S_IRUSR | S_IWUSR );
    ds.setDevice( &file );
  } else
  {
    // tmp file for upload
    tf.open();
    ds.setDevice( &tf );
  }

  ds.writeRawData( data.data(), data.size() );
  if ( !url.isLocalFile() )
  {
    tf.close();
    if ( !KIO::NetAccess::upload( tf.fileName(), url, parentWidget() ) )
    {
      KMessageBox::error( parentWidget(),
          i18n( "Could not write the file %1." ,
            url.path() ),
          i18n( "KMail Error" ) );
      return Failed;
    }
  } else
    file.close();
  return OK;
}

KMLoadPartsCommand::KMLoadPartsCommand( QList<partNode*>& parts, KMMessage *msg )
  : mNeedsRetrieval( 0 )
{
  QList<partNode*>::const_iterator it;
  for ( it = parts.constBegin(); it != parts.constEnd(); ++it ) {
    mPartMap.insert( (*it), msg );
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
      KMFolder* curFolder = it.value()->parent();
      if ( curFolder ) {
        FolderJob *job =
          curFolder->createJob( it.value(), FolderJob::tGetMessage,
                                0, it.key()->msgPart().partSpecifier() );
        job->setCancellable( false );
        connect( job, SIGNAL(messageUpdated(KMMessage*, QString)),
                 this, SLOT(slotPartRetrieved(KMMessage*, QString)) );
        job->start();
      } else
        kWarning(5006) << "KMLoadPartsCommand - msg has no parent" << endl;
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
      if ( it.key()->dwPart()->partId() == part->partId() )
        it.key()->setDwPart( part );
    }
  } else
    kWarning(5006) << "KMLoadPartsCommand::slotPartRetrieved - could not find bodypart!" << endl;
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
  KMMessage *msg = retrievedMessage();

  KMMessage *newMsg = new KMMessage(*msg);
  newMsg->setCharset(msg->codec()->name());
  // the message needs a new Message-Id
  newMsg->removeHeaderField( "Message-Id" );
  newMsg->setParent( 0 );

  // adds the new date to the message
  newMsg->removeHeaderField( "Date" );

  KMail::Composer * win = KMail::makeComposer();
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
  KUrl::List lst = urls();
  QString handler = ( mFolder->mailingList().handler() == MailingList::KMail )
    ? "mailto" : "https";

  KMCommand *command = 0;
  for ( KUrl::List::Iterator itr = lst.begin(); itr != lst.end(); ++itr ) {
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
  deleteLater();
}

KMMailingListPostCommand::KMMailingListPostCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListPostCommand::urls() const
{
  return mFolder->mailingList().postURLS();
}

KMMailingListSubscribeCommand::KMMailingListSubscribeCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListSubscribeCommand::urls() const
{
  return mFolder->mailingList().subscribeURLS();
}

KMMailingListUnsubscribeCommand::KMMailingListUnsubscribeCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListUnsubscribeCommand::urls() const
{
  return mFolder->mailingList().unsubscribeURLS();
}

KMMailingListArchivesCommand::KMMailingListArchivesCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListArchivesCommand::urls() const
{
  return mFolder->mailingList().archiveURLS();
}

KMMailingListHelpCommand::KMMailingListHelpCommand( QWidget *parent, KMFolder *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListHelpCommand::urls() const
{
  return mFolder->mailingList().helpURLS();
}

KMIMChatCommand::KMIMChatCommand( const KUrl &url, KMMessage *msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMIMChatCommand::execute()
{
  kDebug( 5006 ) << k_funcinfo << " URL is: " << mUrl << endl;
  QString addr = KMMessage::decodeMailtoUrl( mUrl.path() );
  // find UID for mail address
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  KABC::Addressee::List addressees = addressBook->findByEmail( EmailAddressTools::extractEmailAddress( addr ) ) ;

  // start chat
  if( addressees.count() == 1 ) {
#ifdef __GNUC__
#warning "port me"
#endif
    //kmkernel->imProxy()->chatWithContact( addressees[0].uid() );
    return OK;
  }
  else
  {
    kDebug( 5006 ) << "Didn't find exactly one addressee, couldn't tell who to chat to for that email address.  Count = " << addressees.count() << endl;

    QString apology;
    if ( addressees.isEmpty() )
      apology = i18n( "There is no Address Book entry for this email address. Add them to the Address Book and then add instant messaging addresses using your preferred messaging client." );
    else
    {
      QStringList nameList;
      KABC::Addressee::List::const_iterator it = addressees.begin();
      KABC::Addressee::List::const_iterator end = addressees.end();
      for ( ; it != end; ++it )
      {
          nameList.append( (*it).realName() );
      }
      QString names = nameList.join( QString::fromLatin1( ",\n" ) );
      apology = i18n( "More than one Address Book entry uses this email address:\n %1\n it is not possible to determine who to chat with.", names );
    }

    KMessageBox::sorry( parentWidget(), apology );
    return Failed;
  }
}

KMHandleAttachmentCommand::KMHandleAttachmentCommand( partNode* node,
     KMMessage* msg, int atmId, const QString& atmName,
     AttachmentAction action, KService::Ptr offer, QWidget* parent )
: KMCommand( parent ), mNode( node ), mMsg( msg ), mAtmId( atmId ), mAtmName( atmName ),
  mAction( action ), mOffer( offer ), mJob( 0 )
{
}

void KMHandleAttachmentCommand::slotStart()
{
  if ( !mNode->msgPart().isComplete() )
  {
    // load the part
    kDebug(5006) << "load part" << endl;
    KMLoadPartsCommand *command = new KMLoadPartsCommand( mNode, mMsg );
    connect( command, SIGNAL( partsRetrieved() ),
        this, SLOT( slotPartComplete() ) );
    command->start();
  } else
  {
    execute();
  }
}

void KMHandleAttachmentCommand::slotPartComplete()
{
  execute();
}

KMCommand::Result KMHandleAttachmentCommand::execute()
{
  switch( mAction )
  {
    case Open:
      atmOpen();
      break;
    case OpenWith:
      atmOpenWith();
      break;
    case View:
      atmView();
      break;
    case Save:
      atmSave();
      break;
    case Properties:
      atmProperties();
      break;
    case ChiasmusEncrypt:
      atmEncryptWithChiasmus();
      return Undefined;
      break;
    default:
      kDebug(5006) << "unknown action " << mAction << endl;
      break;
  }
  setResult( OK );
  emit completed( this );
  deleteLater();
  return OK;
}

QString KMHandleAttachmentCommand::createAtmFileLink() const
{
  QFileInfo atmFileInfo( mAtmName );

  if ( atmFileInfo.size() == 0 )
  {
    kDebug(5006) << k_funcinfo << "rewriting attachment" << endl;
    // there is something wrong so write the file again
    QByteArray data = mNode->msgPart().bodyDecodedBinary();
    if ( mNode->msgPart().type() == DwMime::kTypeText && data.size() > 0 ) {
      // convert CRLF to LF before writing text attachments to disk
      const size_t newsize = KMail::Util::crlf2lf( data.data(), data.size() );
      data.truncate( newsize );
    }
    KPIM::kByteArrayToFile( data, mAtmName, false, false, false );
  }

  KTemporaryFile *linkFile = new KTemporaryFile();
  linkFile->setPrefix(atmFileInfo.fileName() +"_[");
  linkFile->setSuffix("]."+ atmFileInfo.suffix());
  linkFile->open();
  QString linkName = linkFile->fileName();
  delete linkFile;

  if ( ::link(QFile::encodeName( mAtmName ), QFile::encodeName( linkName )) == 0 ) {
    return linkName; // success
  }
  return QString();
}

KService::Ptr KMHandleAttachmentCommand::getServiceOffer()
{
  KMMessagePart& msgPart = mNode->msgPart();
  const QString contentTypeStr =
    ( msgPart.typeStr() + '/' + msgPart.subtypeStr() ).toLower();

  if ( contentTypeStr == "text/x-vcard" || contentTypeStr == "text/directory" ) {
    atmView();
    return KService::Ptr( 0 );
  }
  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( contentTypeStr );
  if ( mimetype->name() == "application/octet-stream" ) {
    // consider the filename if Content-Type is application/octet-stream
    mimetype = KMimeType::findByPath( mAtmName, 0, true /* no disk access */ );
  }
  if ( ( mimetype->name() == "application/octet-stream" )
       && msgPart.isComplete() ) {
    // consider the attachment's contents if neither the Content-Type header
    // nor the filename give us a clue
    mimetype = KMimeType::findByFileContent( mAtmName );
  }
  return KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );
}

void KMHandleAttachmentCommand::atmOpen()
{
  if ( !mOffer )
    mOffer = getServiceOffer();
  if ( !mOffer ) {
    kDebug(5006) << k_funcinfo << "got no offer" << endl;
    return;
  }

  KUrl::List lst;
  KUrl url;
  bool autoDelete = true;
  QString fname = createAtmFileLink();

  if ( fname.isNull() ) {
    autoDelete = false;
    fname = mAtmName;
  }

  url.setPath( fname );
  lst.append( url );
  if ( (KRun::run( *mOffer, lst, 0, autoDelete ) <= 0) && autoDelete ) {
      QFile::remove(url.path());
  }
}

void KMHandleAttachmentCommand::atmOpenWith()
{
  KUrl::List lst;
  KUrl url;
  bool autoDelete = true;
  QString fname = createAtmFileLink();

  if ( fname.isNull() ) {
    autoDelete = false;
    fname = mAtmName;
  }

  url.setPath( fname );
  lst.append( url );
  if ( (! KRun::displayOpenWithDialog(lst, 0, autoDelete)) && autoDelete ) {
    QFile::remove( url.path() );
  }
}

void KMHandleAttachmentCommand::atmView()
{
  // we do not handle this ourself
  emit showAttachment( mAtmId, mAtmName );
}

void KMHandleAttachmentCommand::atmSave()
{
  QList<partNode*> parts;
  parts.append( mNode );
  // save, do not leave encoded
  KMSaveAttachmentsCommand *command =
    new KMSaveAttachmentsCommand( 0, parts, mMsg, false );
  command->start();
}

void KMHandleAttachmentCommand::atmProperties()
{
  KMMsgPartDialogCompat dlg( 0, true );
  KMMessagePart& msgPart = mNode->msgPart();
  dlg.setMsgPart( &msgPart );
  dlg.exec();
}

void KMHandleAttachmentCommand::atmEncryptWithChiasmus()
{
  const partNode * node = mNode;
  Q_ASSERT( node );
  if ( !node )
    return;

  // FIXME: better detection of mimetype??
  if ( !mAtmName.endsWith( ".xia", Qt::CaseInsensitive ) )
    return;

  const Kleo::CryptoBackend::Protocol * chiasmus =
    Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" );
  Q_ASSERT( chiasmus );
  if ( !chiasmus )
    return;

  const std::auto_ptr<Kleo::SpecialJob> listjob( chiasmus->specialJob( "x-obtain-keys", QMap<QString,QVariant>() ) );
  if ( !listjob.get() ) {
    const QString msg = i18n( "Chiasmus backend does not offer the "
                              "\"x-obtain-keys\" function. Please report this bug." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  if ( listjob->exec() ) {
    listjob->showErrorDialog( parentWidget(), i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const QVariant result = listjob->property( "result" );
  if ( result.type() != QVariant::StringList ) {
    const QString msg = i18n( "Unexpected return value from Chiasmus backend: "
                              "The \"x-obtain-keys\" function did not return a "
                              "string list. Please report this bug." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const QStringList keys = result.toStringList();
  if ( keys.empty() ) {
    const QString msg = i18n( "No keys have been found. Please check that a "
                              "valid key path has been set in the Chiasmus "
                              "configuration." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  ChiasmusKeySelector selectorDlg( parentWidget(), i18n( "Chiasmus Decryption Key Selection" ),
                                   keys, GlobalSettings::chiasmusDecryptionKey(),
                                   GlobalSettings::chiasmusDecryptionOptions() );
  if ( selectorDlg.exec() != QDialog::Accepted )
    return;

  GlobalSettings::setChiasmusDecryptionOptions( selectorDlg.options() );
  GlobalSettings::setChiasmusDecryptionKey( selectorDlg.key() );
  assert( !GlobalSettings::chiasmusDecryptionKey().isEmpty() );

  Kleo::SpecialJob * job = chiasmus->specialJob( "x-decrypt", QMap<QString,QVariant>() );
  if ( !job ) {
    const QString msg = i18n( "Chiasmus backend does not offer the "
                              "\"x-decrypt\" function. Please report this bug." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const QByteArray input = node->msgPart().bodyDecodedBinary();

  if ( !job->setProperty( "key", GlobalSettings::chiasmusDecryptionKey() ) ||
       !job->setProperty( "options", GlobalSettings::chiasmusDecryptionOptions() ) ||
       !job->setProperty( "input", input ) ) {
    const QString msg = i18n( "The \"x-decrypt\" function does not accept "
                              "the expected parameters. Please report this bug." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  setDeletesItself( true ); // the job below is async, we have to cleanup ourselves
  if ( job->start() ) {
    job->showErrorDialog( parentWidget(), i18n( "Chiasmus Decryption Error" ) );
    return;
  }

  mJob = job;
  connect( job, SIGNAL(result(const GpgME::Error&,const QVariant&)),
           this, SLOT(slotAtmDecryptWithChiasmusResult(const GpgME::Error&,const QVariant&)) );
}

// return true if we should proceed, false if we should abort
static bool checkOverwrite( const KUrl& url, bool& overwrite, QWidget* w )
{
  if ( KIO::NetAccess::exists( url, false /*dest*/, w ) ) {
    if ( KMessageBox::Cancel ==
         KMessageBox::warningContinueCancel(
                                            w,
                                            i18n( "A file named \"%1\" already exists. "
                                                  "Are you sure you want to overwrite it?", url.prettyUrl() ),
                                            i18n( "Overwrite File?" ),
                                            KGuiItem(i18n( "&Overwrite" )) ) )
      return false;
    overwrite = true;
  }
  return true;
}

static const QString chomp( const QString & base, const QString & suffix, bool cs ) {
  return base.endsWith( suffix, cs?(Qt::CaseSensitive):(Qt::CaseInsensitive) ) ? base.left( base.length() - suffix.length() ) : base ;
}

void KMHandleAttachmentCommand::slotAtmDecryptWithChiasmusResult( const GpgME::Error & err, const QVariant & result )
{
  LaterDeleterWithCommandCompletion d( this );
  if ( !mJob )
    return;
  Q_ASSERT( mJob == sender() );
  if ( mJob != sender() )
    return;
  Kleo::Job * job = mJob;
  mJob = 0;
  if ( err.isCanceled() )
    return;
  if ( err ) {
    job->showErrorDialog( parentWidget(), i18n( "Chiasmus Decryption Error" ) );
    return;
  }

  if ( result.type() != QVariant::ByteArray ) {
    const QString msg = i18n( "Unexpected return value from Chiasmus backend: "
                              "The \"x-decrypt\" function did not return a "
                              "byte array. Please report this bug." );
    KMessageBox::error( parentWidget(), msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const KUrl url = KFileDialog::getSaveUrl( chomp( mAtmName, ".xia", false ), QString(), parentWidget() );
  if ( url.isEmpty() )
    return;

  bool overwrite = false;
  if ( !checkOverwrite( url, overwrite, parentWidget() ) )
    return;

  d.setDisabled( true ); // we got this far, don't delete yet
  KIO::Job * uploadJob = KIO::storedPut( result.toByteArray(), url, -1, overwrite, false /*resume*/ );
  uploadJob->ui()->setWindow( parentWidget() );
  connect( uploadJob, SIGNAL(result(KJob*)),
           this, SLOT(slotAtmDecryptWithChiasmusUploadResult(KJob*)) );
}

void KMHandleAttachmentCommand::slotAtmDecryptWithChiasmusUploadResult( KJob * job )
{
  if ( job->error() )
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
  LaterDeleterWithCommandCompletion d( this );
  d.setResult( OK );
}

#include "kmcommands.moc"
