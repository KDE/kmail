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

#include <unistd.h> // link()
#include <errno.h>
#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>
#include <mimelib/string.h>

#include <kprogressdialog.h>
#include <kpimutils/email.h>
#include <kdbusservicestarter.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kabc/stdaddressbook.h>
#include <kabc/addresseelist.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kparts/browserextension.h>
#include <krun.h>
#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
// KIO headers
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>

#include <kmime/kmime_message.h>

#include <kpimidentities/identitymanager.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemfetchjob.h>


#include "foldercollection.h"

#include "actionscheduler.h"
using KMail::ActionScheduler;
#include "mailinglist-magic.h"
#include "messageviewer/kmaddrbook.h"
#include "messageviewer/nodehelper.h"
#include <kaddrbookexternal.h>
#include "composer.h"
#include "kmfiltermgr.h"
//TODO port to akonadi #include "kmfoldermbox.h"
//TODO port to akonadi #include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "messagesender.h"
#include "undostack.h"
#include "messageviewer/kcursorsaver.h"
#include "messageviewer/objecttreeparser.h"
#include "messageviewer/csshelper.h"
using KMail::ObjectTreeParser;
using KMail::FolderJob;
#include "chiasmuskeyselector.h"
#include "messageviewer/mailsourceviewer.h"
using namespace MessageViewer;
#include "kmreadermainwin.h"
#include "secondarywindow.h"
using KMail::SecondaryWindow;
#include "redirectdialog.h"
using KMail::RedirectDialog;
#include "util.h"
#include "templateparser.h"
using KMail::TemplateParser;
#include "messageviewer/editorwatcher.h"
#include "korghelper.h"
#include "broadcaststatus.h"
#include "globalsettings.h"
#include "stringutil.h"
#include "messageviewer/autoqpointer.h"

#include "messagehelper.h"

#include <kpimutils/kfileio.h>
#include "calendarinterface.h"
#include "interfaces/htmlwriter.h"

#include <akonadi/itemmovejob.h>
#include <akonadi/itemcopyjob.h>

#include "progressmanager.h"
using KPIM::ProgressManager;
using KPIM::ProgressItem;
#include <kmime/kmime_mdn.h>
using namespace KMime;

#include "kleo/specialjob.h"
#include "kleo/cryptobackend.h"
#include "kleo/cryptobackendfactory.h"

#ifdef Nepomuk_FOUND
  #include <nepomuk/tag.h>
#endif

#include <gpgme++/error.h>

#include <QClipboard>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QMenu>
#include <QByteArray>
#include <QApplication>
#include <QDesktopWidget>
#include <QList>
#include <QTextCodec>
#include <QProgressBar>

#include <memory>

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

/// Small helper function to get the composer context from a reply
static KMail::Composer::TemplateContext replyContext( KMail::MessageHelper::MessageReply reply )
{
  if ( reply.replyAll )
    return KMail::Composer::ReplyToAll;
  else
    return KMail::Composer::Reply;
}

KMCommand::KMCommand( QWidget *parent )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
}



KMCommand::KMCommand( QWidget *parent, const Akonadi::Item &msg )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
  if ( msg.isValid() ) {
    mMsgList.append( msg );
  }
}

KMCommand::KMCommand( QWidget *parent, const QList<Akonadi::Item> &msgList )
  : mProgressDialog( 0 ), mResult( Undefined ), mDeletesItself( false ),
    mEmitsCompletedItself( false ), mParent( parent )
{
  mMsgList = msgList;
}



KMCommand::~KMCommand()
{
  QList<QPointer<KMFolder> >::Iterator fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if ( !(*fit) ) {
      continue;
    }
    (*fit)->close( "kmcommand" );
  }
}

KMime::Message *KMCommand::message( const Akonadi::Item & item )
{
  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Payload is not a MessagePtr!";
    return 0;
  }
  KMime::Message *msg = new KMime::Message;
  msg->setContent( item.payloadData() );
  msg->parse();

  return msg;
}

KMCommand::Result KMCommand::result() const
{
  if ( mResult == Undefined ) {
    kDebug() << "mResult is Undefined";
  }
  return mResult;
}

void KMCommand::start()
{
  QTimer::singleShot( 0, this, SLOT( slotStart() ) );
}


const QList<Akonadi::Item> KMCommand::retrievedMsgs() const
{
  return mRetrievedMsgs;
}

Akonadi::Item KMCommand::retrievedMessage() const
{
  if ( mRetrievedMsgs.isEmpty() )
    return Akonadi::Item();
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

  if ( mMsgList.contains(Akonadi::Item()) ) {
      emit messagesTransfered( Failed );
      return;
  }

  Akonadi::Item mb;
  if ( !mMsgList.isEmpty() )
    mb = *(mMsgList.begin());
  if ( ( mb.isValid() ) && ( mMsgList.count() == 1 ) &&
       ( !mb.parentCollection().isValid() ) )
  {
    // Special case of operating on message that isn't in a folder
    mRetrievedMsgs.append(mMsgList.takeFirst());
    emit messagesTransfered( OK );
    return;
  }
  QList<Akonadi::Item>::const_iterator it;
  for ( it = mMsgList.constBegin(); it != mMsgList.constEnd(); ++it )
    if ( !(*it).parentCollection().isValid()  ) {
      emit messagesTransfered( Failed );
      return;
    } else {
      //keepFolderOpen( (*it)->parent() );
    }

  // transfer the selected messages first
  transferSelectedMsgs();
}

void KMCommand::slotPostTransfer( KMCommand::Result result )
{
  disconnect( this, SIGNAL( messagesTransfered( KMCommand::Result ) ),
              this, SLOT( slotPostTransfer( KMCommand::Result ) ) );
  if ( result == OK ) {
    result = execute();
  }
  mResult = result;
  Akonadi::Item msg;
  foreach( msg, mRetrievedMsgs ) {
#if 0 //TODO port to akonadi
    if ( msg && msg->parent() )
      msg->setTransferInProgress(false);
#endif
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
        "Please wait while the %1 messages are transferred", mMsgList.count()));
    mProgressDialog->setModal(true);
    mProgressDialog->setMinimumDuration(1000);
  }
#if 0 //TODO port to akonadi
  QList<KMime::Message*>::const_iterator it;
  for ( it = mMsgList.constBegin(); it != mMsgList.constEnd(); ++it )
  {
    KMime::Message *mb = (*it);
    // check if all messages are complete
    KMime::Message *thisMsg = 0;
    if ( mb->isMessage() )
      thisMsg = static_cast<KMime::Message*>(mb);
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
      kDebug()<<"### INCOMPLETE";
      // the message needs to be transferred first
      complete = false;
      KMCommand::mCountJobs++;
      FolderJob *job = thisMsg->parent()->createJob(thisMsg);
      job->setCancellable( false );
      totalSize += thisMsg->msgSizeServer();
      // emitted when the message was transferred successfully
      connect(job, SIGNAL(messageRetrieved(KMime::Message*)),
              this, SLOT(slotMsgTransfered(KMime::Message*)));
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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

void KMCommand::slotMsgTransfered(const Akonadi::Item & msg)
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
    mProgressDialog->setLabelText(i18np("Please wait while the message is transferred",
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
#if 0 //TODO port to akonadi
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
  // unget the transferred messages
  QList<KMime::Message*>::const_iterator it;
  for ( it = mRetrievedMsgs.constBegin(); it != mRetrievedMsgs.constEnd(); ++it ) {
    KMime::Message* msg = (*it);
    KMFolder *folder = msg->parent();
    ++it;
    if (!folder)
      continue;
    msg->setTransferInProgress(false);
    int idx = folder->find(msg);
    if (idx > 0) folder->unGetMsg(idx);
  }
  mRetrievedMsgs.clear();
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  emit messagesTransfered( Canceled );
}

void KMCommand::keepFolderOpen( KMFolder *folder )
{
  folder->open( "kmcommand" );
  mFolders.append( folder );
}

KMMailtoComposeCommand::KMMailtoComposeCommand( const KUrl &url,
                                                const Akonadi::Item &msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMMailtoComposeCommand::execute()
{
  KMime::Message *msg = new KMime::Message;
  uint id = 0;

  if ( mMessage.isValid() && mMessage.parentCollection().isValid() ) {
    FolderCollection fd( mMessage.parentCollection() );
    id = fd.identity();
  }

  KMail::MessageHelper::initHeader(msg, id);
  msg->contentType()->setCharset("utf-8");
  msg->to()->fromUnicodeString( KMail::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" );

  KMail::Composer * win = KMail::makeComposer( msg, KMail::Composer::New, id );
  win->setCharset("", true);
  win->setFocusToSubject();
  win->show();
  return OK;
}


KMMailtoReplyCommand::KMMailtoReplyCommand( QWidget *parent,
                                            const KUrl &url, const Akonadi::Item &msg, const QString &selection )
  :KMCommand( parent, msg ), mUrl( url ), mSelection( selection  )
{
}

KMCommand::Result KMMailtoReplyCommand::execute()
{
  //TODO : consider factoring createReply into this method.
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*TODO Port || !msg->codec() */) { //TODO Reuse codec() from libmessageviewer/nodehelper
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMime::Message *rmsg = KMail::MessageHelper::createReply( msg, KMail::ReplyNone, mSelection );
  rmsg->to()->fromUnicodeString( KMail::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" ); //TODO Check the UTF-8

  KMail::Composer * win = KMail::makeComposer( rmsg, KMail::Composer::Reply, 0, mSelection );
  /** TODO Port it to KMime
  win->setCharset( msg->codec()->name(), true );
  */
  win->setReplyFocus();
  win->show();


  delete msg;

  return OK;
}


KMMailtoForwardCommand::KMMailtoForwardCommand( QWidget *parent,
                                                const KUrl &url, const Akonadi::Item &msg )
  :KMCommand( parent, msg ), mUrl( url )
{
}

KMCommand::Result KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*|| !msg->codec() Port to KMime::message*/) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMime::Message *fmsg = KMail::MessageHelper::createForward( msg );
  fmsg->to()->fromUnicodeString( KMail::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" ); //TODO check the utf-8

  KMail::Composer * win = KMail::makeComposer( fmsg, KMail::Composer::Forward );
  /** TODO Port to KMime
  win->setCharset( msg->codec()->name(), true );
  */
  win->show();
  delete msg;

  return OK;
}


KMAddBookmarksCommand::KMAddBookmarksCommand( const KUrl &url, QWidget *parent )
  : KMCommand( parent ), mUrl( url )
{
}

KMCommand::Result KMAddBookmarksCommand::execute()
{
  QString filename = KStandardDirs::locateLocal( "data", QString::fromLatin1("konqueror/bookmarks.xml") );
  KBookmarkManager *bookManager = KBookmarkManager::managerForFile( filename, "konqueror" );
  KBookmarkGroup group = bookManager->root();
  group.addBookmark( mUrl.path(), KUrl( mUrl ) );
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
  KPIM::KAddrBookExternal::addEmail( KMail::StringUtil::decodeMailtoUrl( mUrl.path() ),
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
  QString addr = KMail::StringUtil::decodeMailtoUrl( mUrl.path() );
  KPIM::KAddrBookExternal::openEmail( KPIMUtils::extractEmailAddress(addr),
                                      addr, parentWidget() );

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
    QString address = KMail::StringUtil::decodeMailtoUrl( mUrl.path() );
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
  if ( KIO::NetAccess::exists( saveUrl, KIO::NetAccess::DestinationSide, parentWidget() ) )
  {
    if (KMessageBox::warningContinueCancel(0,
        i18nc("@info", "File <filename>%1</filename> exists.<nl/>Do you want to replace it?",
         saveUrl.pathOrUrl()), i18n("Save to File"), KGuiItem(i18n("&Replace")))
        != KMessageBox::Continue)
      return Canceled;
  }
  KIO::Job *job = KIO::file_copy(mUrl, saveUrl, -1, KIO::Overwrite);
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


KMEditMsgCommand::KMEditMsgCommand( QWidget *parent, const Akonadi::Item&msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMEditMsgCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if (!item.isValid() || !item.parentCollection().isValid() ||
      ( !kmkernel->folderIsDraftOrOutbox( item.parentCollection() ) &&
        !kmkernel->folderIsTemplates( item.parentCollection() ) ) ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
#if 0
  // Remember the old parent, we need it a bit further down to be able
  // to put the unchanged messsage back in the original folder if the nth
  // edit is discarded, for n > 1.
  KMFolder *parent = msg->parent();
  if ( parent ) {
    parent->take( parent->find( msg ) );
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  KMail::Composer *win = KMail::makeComposer();
#if 0
  msg->setTransferInProgress( false ); // From here on on, the composer owns the message.
#endif
  win->setMsg( msg, false, true );
  win->setFolder( item.parentCollection() );
  win->show();

  return OK;
}

KMUseTemplateCommand::KMUseTemplateCommand( QWidget *parent, const Akonadi::Item  &msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMUseTemplateCommand::execute()
{
#if 0 //TODO port to akonadi
  Akonadi::Item msg = retrievedMessage();
  if ( !msg || !msg->parent() ||
       !kmkernel->folderIsTemplates( msg->parent() ) ) {
    return Failed;
  }

  // Take a copy of the original message, which remains unchanged.
  KMime::Message *newMsg = new KMime::Message( new DwMessage( *msg->asDwMessage() ) );
  newMsg->setComplete( msg->isComplete() );

  // these fields need to be regenerated for the new message
  newMsg->removeHeaderField("Date");
  newMsg->removeHeaderField("Message-ID");

  KMail::Composer *win = KMail::makeComposer();
  newMsg->setTransferInProgress( false ); // From here on on, the composer owns the message.
  win->setMsg( newMsg, false, true );
  win->show();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

static KUrl subjectToUrl( const QString &subject )
{
  QString fileName = KMCommand::cleanFileName( subject.trimmed() );

  // avoid stripping off the last part of the subject after a "."
  // by KFileDialog, which thinks it's an extension
  if ( !fileName.endsWith( ".mbox" ) )
    fileName += ".mbox";

  return KFileDialog::getSaveUrl( KUrl::fromPath( fileName ), "*.mbox" );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent, const Akonadi::Item& msg )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mStandAloneMessage( 0 ),
    mOffset( 0 ),
    mTotalSize( /* TODO port to akonadi msg ? msg->msgSize() :*/ 0 )
{
  if ( !msg.isValid() ) {
    return;
  }
  setDeletesItself( true );
  // If the mail has a serial number, operate on sernums, if it does not
  // we need to work with the pointer, but can be reasonably sure it won't
  // go away, since it'll be an encapsulated message or one that was opened
  // from an .eml file.
#if 0 //TODO port to akonadi
  if ( msg->getMsgSerNum() != 0 ) {
    mMsgList.append( msg->getMsgSerNum() );
  } else {
    mStandAloneMessage = msg;
  }

  mUrl = subjectToUrl( NodeHelper::cleanSubject( msg ) );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent,
                                    const QList<Akonadi::Item> &msgList )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mStandAloneMessage( 0 ),
    mOffset( 0 ),
    mTotalSize( 0 )
{
  if ( msgList.empty() ) {
    return;
  }
  setDeletesItself( true );
  // We operate on serNums and not the KMime::Message pointers, as those can
  // change, or become invalid when changing the current message, switching
  // folders, etc.
#if 0 //TODO port to akonadi
  QList<KMime::Message*>::const_iterator it;
  for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
    mMsgList.append( (*it)->getMsgSerNum() );
    mTotalSize += (*it)->msgSize();
    if ( (*it)->parent() != 0 ) {
      (*it)->parent()->open( "kmcommand" );
    }
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  mMsgListIndex = 0;
#if 0
  KMime::Message *msgBase = *(msgList.begin());
  mUrl = subjectToUrl( MessageViewer::NodeHelper::cleanSubject( msgBase ) );
#endif
}

KUrl KMSaveMsgCommand::url()
{
  return mUrl;
}

KMCommand::Result KMSaveMsgCommand::execute()
{
  mJob = KIO::put( mUrl, S_IRUSR|S_IWUSR );
  mJob->setTotalSize( mTotalSize );
  mJob->setAsyncDataEnabled( true );
  connect(mJob, SIGNAL(dataReq(KIO::Job*, QByteArray &)),
    SLOT(slotSaveDataReq()));
  connect(mJob, SIGNAL(result(KJob*)),
    SLOT(slotSaveResult(KJob*)));
  setEmitsCompletedItself( true );
  return OK;
}

void KMSaveMsgCommand::slotSaveDataReq()
{
#if 0 //TODO port to akonadi
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
  if ( mMsgListIndex < static_cast<unsigned int>( mMsgList.size() ) ) {
    KMime::Message *msg = 0;
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
          connect(job, SIGNAL( messageRetrieved( KMime::Message* ) ),
                  this, SLOT( slotMessageRetrievedForSaving( KMime::Message* ) ) );
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMSaveMsgCommand::slotMessageRetrievedForSaving(const Akonadi::Item &msg)
{
#if 0 //TODO port to akonadi
  if ( msg ) {
    QByteArray str( msg->mboxMessageSeparator() );
    str += KMFolderMbox::escapeFrom( msg->asDwString() );
    str += '\n';
#if 0  //TODO port to akonadi
    msg->setTransferInProgress(false);
#endif
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
    p->close( "kmcommand" );
  }
#endif
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

        mJob = KIO::put( mUrl, S_IRUSR|S_IWUSR, KIO::Overwrite );
        mJob->setTotalSize( mTotalSize );
        mJob->setAsyncDataEnabled( true );
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
    mUrl = KFileDialog::getOpenUrl( KUrl( ":OpenMessage" ),
                                    "message/rfc822 application/mbox",
                                    parentWidget(), i18n("Open Message") );
  }
  if ( mUrl.isEmpty() ) {
    setDeletesItself( false );
    return Canceled;
  }
  mJob = KIO::get( mUrl, KIO::NoReload, KIO::HideProgressInfo );
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
#if 0 //TODO port to akonadi
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
    KMime::Message *msg = new KMime::Message( dwMsg );
    msg->setReadyToShow( true );
    KMReaderMainWin *win = new KMReaderMainWin();
    win->showMessage( mEncoding, msg );
    win->show();
    if ( multipleMessages )
      KMessageBox::information( win,
                                i18n( "The file contains multiple messages. "
                                      "Only the first message is shown." ) );
    setResult( OK );
    emit completed( this );
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  deleteLater();
}

//-----------------------------------------------------------------------------

//TODO: ReplyTo, NoQuoteReplyTo, ReplyList, ReplyToAll, ReplyAuthor
//      are all similar and should be factored
KMReplyToCommand::KMReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                                    const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplySmart, mSelection );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ), 0, mSelection );
#if 0  //Port to kmime::message
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus();
  win->show();

  delete msg;

  return OK;
}


KMNoQuoteReplyToCommand::KMNoQuoteReplyToCommand( QWidget *parent,
                                                  const Akonadi::Item &msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMNoQuoteReplyToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplySmart, "", true);
  KMail::Composer *win = KMail::makeComposer( reply.msg, replyContext( reply ) );
#if 0 //Port to akonadi
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus( false );
  win->show();
  delete msg;

  return OK;
}


KMReplyListCommand::KMReplyListCommand( QWidget *parent,
                                        const Akonadi::Item &msg, const QString &selection )
 : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyListCommand::execute()
{
  KCursorSaver busy( KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplyList, mSelection );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ),
                                               0, mSelection );
#if 0
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus( false );
  win->show();

  delete msg;
  return OK;
}


KMReplyToAllCommand::KMReplyToAllCommand( QWidget *parent,
                                          const Akonadi::Item &msg, const QString &selection )
  :KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyToAllCommand::execute()
{
  KCursorSaver busy( KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }

  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplyAll, mSelection );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ), 0,
                                               mSelection );
#if 0
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus();
  win->show();

  delete msg;
  return OK;
}


KMReplyAuthorCommand::KMReplyAuthorCommand( QWidget *parent, const Akonadi::Item &msg,
                                            const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
}

KMCommand::Result KMReplyAuthorCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplyAuthor, mSelection );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ), 0,
                                               mSelection );
#if 0
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus();
  win->show();

  delete msg;
  return OK;
}


KMForwardCommand::KMForwardCommand( QWidget *parent,
  const QList<Akonadi::Item> &msgList, uint identity )
  : KMCommand( parent, msgList ),
    mIdentity( identity )
{
}

KMForwardCommand::KMForwardCommand( QWidget *parent, const Akonadi::Item &msg,
                                    uint identity )
  : KMCommand( parent, msg ),
    mIdentity( identity )
{
}

KMCommand::Result KMForwardCommand::execute()
{
#if 0 //TODO port to akonadi
  QList<KMime::Message*> msgList = retrievedMsgs();

  if (msgList.count() >= 2) {
    // ask if they want a mime digest forward

    int answer = KMessageBox::questionYesNoCancel(
                   parentWidget(),
                   i18n("Do you want to forward the selected messages as "
                        "attachments in one message (as a MIME digest) or as "
                        "individual messages?"), QString(),
                   KGuiItem(i18n("Send As Digest")),
                   KGuiItem(i18n("Send Individually")) );

    if ( answer == KMessageBox::Yes ) {
      uint id = 0;
      KMime::Message *fwdMsg = new KMime::Message;
      KMime::MessagePart *msgPart = new KMime::MessagePart;
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
      KMime::Message *msg;
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
      QString tmp;
      msgPart->setTypeStr("MULTIPART");
      tmp.sprintf( "Digest; boundary=\"%s\"", boundary.data() );
      msgPart->setSubtypeStr( tmp.toAscii() );
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(msgPartText.toAscii());
      KCursorSaver busy(KBusyPtr::busy());
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::NoTemplate, id );
      win->addAttach(msgPart);
      win->show();
      return OK;
    } else if ( answer == KMessageBox::No ) {// NO MIME DIGEST, Multiple forward
      uint id = 0;
      QList<KMime::Message*> linklist;
      QList<KMime::Message*>::const_iterator it;
      for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
        // set the identity
        if (id == 0)
          id = (*it)->headerField("X-KMail-Identity").trimmed().toUInt();

        linklist.append( (*it) );
      }
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      KMime::Message *fwdMsg = new KMime::Message;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");

      foreach( KMime::Message *msg, linklist ) {
        TemplateParser parser( fwdMsg, TemplateParser::Forward,
                               msg->body(), false, false, false );
        parser.process( msg, 0, true );

        fwdMsg->link( msg, MessageStatus::statusForwarded() );
      }

      KCursorSaver busy(KBusyPtr::busy());
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::NoTemplate, id );
      win->setCharset("");
      win->show();
      return OK;
    } else {
      // user cancelled
      return OK;
    }
  }

  // forward a single message at most.
  KMime::Message *msg = msgList.first();
  if ( !msg || !msg->codec() )
    return Failed;

  KCursorSaver busy(KBusyPtr::busy());
  KMime::Message *fwdMsg = msg->createForward();

  uint id = msg->headerField("X-KMail-Identity").trimmed().toUInt();
  if ( id == 0 )
    id = mIdentity;
  {
    KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id );
    win->setCharset( fwdMsg->codec()->name(), true );
    win->show();
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}


KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
           const QList<Akonadi::Item> &msgList, uint identity, KMail::Composer *win )
  : KMCommand( parent, msgList ), mIdentity( identity ),
    mWin( QPointer<KMail::Composer>( win ))
{
}

KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
                                                    const Akonadi::Item & msg, uint identity, KMail::Composer *win )
  : KMCommand( parent, msg ), mIdentity( identity ),
    mWin( QPointer< KMail::Composer >( win ))
{
}

KMCommand::Result KMForwardAttachedCommand::execute()
{
#if 0 //TODO port to akonadi
  QList<KMime::Message*> msgList = retrievedMsgs();
  KMime::Message *fwdMsg = new KMime::Message;

  if (msgList.count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    fwdMsg->initHeader(mIdentity);
  }
  else if (msgList.count() == 1) {
    KMime::Message *msg = msgList.first();
    fwdMsg->initFromMessage(msg);
    fwdMsg->setSubject( msg->forwardSubject() );
  }

  fwdMsg->setAutomaticFields(true);

  KCursorSaver busy(KBusyPtr::busy());
  if (!mWin)
    mWin = KMail::makeComposer(fwdMsg, KMail::Composer::Forward, mIdentity);

  // iterate through all the messages to be forwarded
  KMime::Message *msg;
  foreach ( msg, msgList ) {
    // remove headers that shouldn't be forwarded
    msg->removePrivateHeaderFields();
    msg->removeHeaderField("BCC");
    // set the part
    KMime::MessagePart *msgPart = new KMime::MessagePart;
    msgPart->setTypeStr("message");
    msgPart->setSubtypeStr("rfc822");
    msgPart->setCharset(msg->charset());
    msgPart->setName("forwarded message");
    msgPart->setContentDescription(msg->from()+": "+msg->subject());
    msgPart->setContentDisposition( "inline" );
    // THIS HAS TO BE AFTER setCte()!!!!
    msgPart->setMessageBody( KMail::Util::ByteArray( msg->asDwString() ) );
    msgPart->setCharset( "" );

    fwdMsg->link( msg, MessageStatus::statusForwarded() );
    mWin->addAttach( msgPart );
  }

  mWin->show();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}


KMRedirectCommand::KMRedirectCommand( QWidget *parent,
                                      const Akonadi::Item &msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMRedirectCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*|| !msg->codec()*/ ) {
    return Failed;
  }
  AutoQPointer<RedirectDialog> dlg( new RedirectDialog( parentWidget(),
                                                        kmkernel->msgSender()->sendImmediate() ) );
  dlg->setObjectName( "redirect" );
  if ( dlg->exec() == QDialog::Rejected || !dlg ) {
    return Failed;
  }
  KMime::Message *msg = message( item );

  KMime::Message *newMsg = KMail::MessageHelper::createRedirect( msg, dlg->to() );
  KMFilterAction::sendMDN( msg, KMime::MDN::Dispatched );

  const KMail::MessageSender::SendMethod method = dlg->sendImmediate()
    ? KMail::MessageSender::SendImmediate
    : KMail::MessageSender::SendLater;
  if ( !kmkernel->msgSender()->send( newMsg, method ) ) {
    kDebug() << "KMRedirectCommand: could not redirect message (sending failed)";
    return Failed; // error: couldn't send
  }
  delete msg;
  return OK;
}


KMCustomReplyToCommand::KMCustomReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                                                const QString &selection,
                                                const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomReplyToCommand::execute()
{
  KCursorSaver busy( KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*|| !msg->codec()*/ /*TODO port it */ ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplySmart, mSelection,
                                                    false, true, false, mTemplate );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ), 0,
                                               mSelection, mTemplate );
#if 0
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus();
  win->show();

  delete msg;

  return OK;
}


KMCustomReplyAllToCommand::KMCustomReplyAllToCommand( QWidget *parent, const Akonadi::Item &msg,
                                                      const QString &selection,
                                                      const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomReplyAllToCommand::execute()
{
  KCursorSaver busy(KBusyPtr::busy());
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*|| !msg->codec() Port to kmime*/ ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply2( msg, KMail::ReplyAll, mSelection,
                                                    false, true, false, mTemplate );
  KMail::Composer * win = KMail::makeComposer( reply.msg, replyContext( reply ), 0,
                                               mSelection, mTemplate );
#if 0
  win->setCharset( msg->codec()->name(), true );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  win->setReplyFocus();
  win->show();

  delete msg;

  return OK;
}


KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
  const QList<Akonadi::Item> &msgList, uint identity, const QString &tmpl )
  : KMCommand( parent, msgList ),
    mIdentity( identity ), mTemplate( tmpl )
{
}

KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
                                                const Akonadi::Item &msg, uint identity, const QString &tmpl )
  : KMCommand( parent, msg ),
    mIdentity( identity ), mTemplate( tmpl )
{
}

KMCommand::Result KMCustomForwardCommand::execute()
{
#if 0 //TODO port to akonadi
  QList<Akonadi::Item> msgList = retrievedMsgs();

  if (msgList.count() >= 2) { // Multiple forward

    uint id = 0;
    // QCString msgText = "";
    QList<Akonadi::Item*> linklist;
    for ( QList<KMime::Message*>::const_iterator it = msgList.constBegin(); it != msgList.constEnd(); ++it )
    {
      KMime::Message *msg = *it;
      // set the identity
      if (id == 0)
        id = msg->headerField( "X-KMail-Identity" ).trimmed().toUInt();

      linklist.append( msg );
    }
    if ( id == 0 )
      id = mIdentity; // use folder identity if no message had an id set
    KMime::Message *fwdMsg = new KMime::Message;
    fwdMsg->initHeader( id );
    fwdMsg->setAutomaticFields( true );
    fwdMsg->setCharset( "utf-8" );
    // fwdMsg->setBody( msgText );

    for ( QList<KMime::Message*>::const_iterator it = linklist.constBegin(); it != linklist.constEnd(); ++it )
    {
      KMime::Message *msg = *it;
      TemplateParser parser( fwdMsg, TemplateParser::Forward,
        msg->body(), false, false, false );
        parser.process( msg, 0, true );

      fwdMsg->link( msg, MessageStatus::statusForwarded() );
    }

    KCursorSaver busy( KBusyPtr::busy() );
    KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id,
                                                 QString(), mTemplate );
    win->setCharset("");
    win->show();

  } else { // forward a single message at most

    KMime::Message *msg = msgList.first();
    if ( !msg || !msg->codec() ) {
      return Failed;
    }

    KCursorSaver busy( KBusyPtr::busy() );
    KMime::Message *fwdMsg = msg->createForward( mTemplate );

    uint id = msg->headerField( "X-KMail-Identity" ).trimmed().toUInt();
    if ( id == 0 )
      id = mIdentity;
    {
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id,
                                                   QString(), mTemplate );
      win->setCharset( fwdMsg->codec()->name(), true );
      win->show();
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}


KMPrintCommand::KMPrintCommand( QWidget *parent, const Akonadi::Item &msg,
                                const HeaderStyle *headerStyle,
                                const HeaderStrategy *headerStrategy,
                                bool htmlOverride, bool htmlLoadExtOverride,
                                bool useFixedFont, const QString & encoding )
  : KMCommand( parent, msg ),
    mHeaderStyle( headerStyle ), mHeaderStrategy( headerStrategy ),
    mHtmlOverride( htmlOverride ),
    mHtmlLoadExtOverride( htmlLoadExtOverride ),
    mUseFixedFont( useFixedFont ), mEncoding( encoding )
{
  if ( GlobalSettings::useDefaultFonts() )
    mOverrideFont = KGlobalSettings::generalFont();
  else {
    KConfigGroup fonts( KMKernel::config(), "Fonts" );
    mOverrideFont = fonts.readEntry( "print-font", KGlobalSettings::generalFont() );
  }
}


void KMPrintCommand::setOverrideFont( const QFont& font )
{
  mOverrideFont = font;
}


KMCommand::Result KMPrintCommand::execute()
{
  // the window will be deleted after printout is performed, in KMReaderWin::slotPrintMsg()
  KMReaderWin *printerWin = new KMReaderWin( kmkernel->mainWin(), 0, 0 );
  printerWin->setPrinting( true );
  printerWin->readConfig();
  if ( mHeaderStyle != 0 && mHeaderStrategy != 0 )
    printerWin->setHeaderStyleAndStrategy( mHeaderStyle, mHeaderStrategy );
  printerWin->setHtmlOverride( mHtmlOverride );
  printerWin->setHtmlLoadExtOverride( mHtmlLoadExtOverride );
  printerWin->setUseFixedFont( mUseFixedFont );
  printerWin->setOverrideEncoding( mEncoding );
  printerWin->cssHelper()->setPrintFont( mOverrideFont );
  printerWin->setDecryptMessageOverwrite( true );
#if 0 //TODO port to akonadi
  printerWin->printMsg( retrievedMessage() );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}


KMSetStatusCommand::KMSetStatusCommand( const MessageStatus& status,
  const QList<quint32> &serNums, bool toggle )
  : mStatus( status ), mSerNums( serNums ), mToggle( toggle ), messageStatusChanged( 0 )
{
  setDeletesItself(true);
}

KMCommand::Result KMSetStatusCommand::execute()
{
  bool parentStatus = false;
  // Toggle actions on threads toggle the whole thread
  // depending on the state of the parent.
  if (mToggle) {
    Akonadi::Item first( *mSerNums.begin() );
    MessageStatus pStatus;
    pStatus.setStatusFromFlags( first.flags() );
    if ( pStatus & mStatus )
      parentStatus = true;
    else
      parentStatus = false;
  }

  QList<qint32> itemMap;
  QList<quint32>::Iterator it;
  for ( it = mSerNums.begin(); it != mSerNums.end(); ++it ) {
    if (mToggle) {
      Akonadi::Item tmpItem( *it );
      //kDebug()<<" item ::"<<tmpItem;
      if (tmpItem.isValid()) {
        bool myStatus;
        MessageStatus itemStatus;
        itemStatus.setStatusFromFlags( tmpItem.flags() );
        if ( itemStatus & mStatus)
          myStatus = true;
        else
          myStatus = false;
        if (myStatus != parentStatus)
          continue;
      }
    }
    itemMap.append( *it );
  }

  for ( int i = 0; i < itemMap.size(); ++i ) {
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob( Akonadi::Item( itemMap[i] ),this );
    connect( fetchJob, SIGNAL( result( KJob* ) ), this, SLOT( slotItemFetchDone( KJob* ) ) );
    fetchJob->start();
  }

  return OK;
}

void KMSetStatusCommand::slotItemFetchDone( KJob* job)
{
  if ( job->error() ) {
    kDebug()<<" Error in slotItemFetchDone :"<<job->errorText();
    deleteLater();
    return;
  }
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  if( fjob->items().count() != 1 ) {
    kError() << "Fetched" << fjob->items().count() << "items, expected 1.";
    deleteLater();
    return;
  }
  Akonadi::Item item = fjob->items().first();
  // Set a custom flag
  item.setFlags( mStatus.getStatusFlags() );
  // Store back modified item
  Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item, this );
  connect( modifyJob, SIGNAL( result( KJob* ) ), this, SLOT( slotModifyItemDone( KJob* ) ) );
}

void KMSetStatusCommand::slotModifyItemDone( KJob * job )
{
  if ( job->error() ) {
    kDebug()<<" Error in void KMSetStatusCommand::slotModifyItemDone( KJob * job ) :"<<job->errorText();
  }
  messageStatusChanged++;
  if ( messageStatusChanged == mSerNums.size() ) {
    QDBusMessage message =
      QDBusMessage::createSignal( "/KMail", "org.kde.kmail.kmail", "unreadCountChanged" );
    QDBusConnection::sessionBus().send( message );
    deleteLater();
  }
}

KMSetTagCommand::KMSetTagCommand( const QString &tagLabel, const QList< unsigned long > &serNums,
    SetTagMode mode )
  : mTagLabel( tagLabel ), mSerNums( serNums ), mMode( mode )
{
}

KMCommand::Result KMSetTagCommand::execute()
{
#if 0 //TODO port to akonadi
#ifdef Nepomuk_FOUND
  //Set the visible name for the tag
  const KMime::MessageTagDescription *tagDesc = kmkernel->msgTagMgr()->find( mTagLabel );
  Nepomuk::Tag n_tag( mTagLabel );
  if ( tagDesc )
    n_tag.setLabel( tagDesc->name() );
#endif

  QList< KMime::Message * > msgList;
  foreach ( unsigned long serNum, mSerNums ) {
    KMFolder * folder;
    int idx;
    KMMsgDict::instance()->getLocation( serNum, &folder, &idx );
    if ( folder ) {
      KMime::Message *msg = folder->getMsgBase( idx );
#ifdef Nepomuk_FOUND
      Nepomuk::Resource n_resource( QString("kmail-email-%1").arg( msgBase->getMsgSerNum() ) );
#endif

      KMime::MessageTagList tagList;
      if ( msg->tagList() )
        tagList = * msg->tagList();

      int tagPosition = tagList.indexOf( mTagLabel );
      if ( tagPosition == -1 ) {
        tagList.append( mTagLabel );
#ifdef Nepomuk_FOUND
        n_resource.addTag( n_tag );
#endif
      } else if ( mMode == Toggle ) {
#ifdef Nepomuk_FOUND
        QList< Nepomuk::Tag > n_tag_list = n_resource.tags();
        for (int i = 0; i < n_tag_list.count(); ++i ) {
          if ( n_tag_list[i].identifiers()[0] == mTagLabel ) {
            n_tag_list.removeAt( i );
            break;
          }
        }
        n_resource.setTags( n_tag_list );
#endif
        tagList.removeAt( tagPosition );
      }
      msg->setTagList( tagList );
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
                                              const QList<Akonadi::Item> &msgList,
                                              KMFilter *filter )
  : KMCommand( parent, msgList ), mFilter( filter  )
{
  QList<Akonadi::Item>::const_iterator it;
  for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it )
    serNumList.append( (*it).id() );
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
  for ( it = serNumList.constBegin(); it != serNumList.constEnd(); ++it ) {
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
      kError() << "Critical error";
      kmkernel->emergencyExit( i18n("Not enough free disk space?" ));
    }
    progressItem->incCompletedItems();
  }

  progressItem->setComplete();
  progressItem = 0;
  return OK;
}


KMMetaFilterActionCommand::KMMetaFilterActionCommand( KMFilter *filter,
                                                      KMMainWidget *main )
    : QObject( main ),
      mFilter( filter ), mMainWidget( main )
{
}

void KMMetaFilterActionCommand::start()
{
  if ( ActionScheduler::isEnabled() ||
       kmkernel->filterMgr()->atLeastOneOnlineImapFolderTarget() )
  {
    // use action scheduler
    KMFilterMgr::FilterSet set = KMFilterMgr::All;
    QList<KMFilter*> filters;
    filters.append( mFilter );
    ActionScheduler *scheduler = new ActionScheduler( set, filters );
    scheduler->setAlwaysMatch( true );
    scheduler->setAutoDestruct( true );
    scheduler->setIgnoreFilterSet( true );
#ifdef OLD_MESSAGELIST
    QList<KMime::Message*> msgList = mMainWidget->messageListView()->selectionAsMsgBaseList();

    KMime::Message *msg;
    foreach( msg, msgList )
      scheduler->execFilters( msg );
#endif
  } else {
#ifdef OLD_MESSAGELIST
    KMCommand *filterCommand = new KMFilterActionCommand(
        mMainWidget, mMainWidget->messageListView()->selectionAsMsgBaseList(), mFilter
      );
    filterCommand->start();
#endif
  }
}

FolderShortcutCommand::FolderShortcutCommand( KMMainWidget *mainwidget,
                                              const Akonadi::Collection&col  )
    : mMainWidget( mainwidget ), mCollectionFolder( col ), mAction( 0 )
{
}


FolderShortcutCommand::~FolderShortcutCommand()
{
  if ( mAction && mAction->parentWidget() )
    mAction->parentWidget()->removeAction( mAction );
  delete mAction;
}

void FolderShortcutCommand::start()
{
  mMainWidget->selectCollectionFolder( mCollectionFolder );
}

void FolderShortcutCommand::setAction( QAction* action )
{
  mAction = action;
}

KMMailingListFilterCommand::KMMailingListFilterCommand( QWidget *parent,
                                                        const Akonadi::Item &msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMMailingListFilterCommand::execute()
{
  QByteArray name;
  QString value;
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
  if ( !MailingList::name( msg, name, value ).isEmpty() ) {
    kmkernel->filterMgr()->createFilter( name, value );
    delete msg;
    return OK;
  } else {
    delete msg;
    return Failed;
  }
}

KMCopyCommand::KMCopyCommand( const Akonadi::Collection& destFolder,
                              const QList<Akonadi::Item> &msgList )
  :mDestFolder( destFolder ), mMsgList( msgList )
{
  setDeletesItself( true );
}

KMCopyCommand::KMCopyCommand( const Akonadi::Collection& destFolder, const Akonadi::Item& msg )
  :mDestFolder( destFolder )
{
  setDeletesItself( true );
  mMsgList.append( msg );
}

KMCommand::Result KMCopyCommand::execute()
{
  Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob( mMsgList, mDestFolder,this );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCopyResult(KJob*)) );

  return OK;
}

void KMCopyCommand::slotCopyResult( KJob * job )
{
  if ( job->error() ) {
    // handle errors
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    setResult( Failed );
  }
  deleteLater();
}

KMMoveCommand::KMMoveCommand( const Akonadi::Collection& destFolder,
                                const QList<Akonadi::Item> &msgList)
    : mDestFolder( destFolder ), mProgressItem( 0 )
{
  mItem = msgList;
}

KMMoveCommand::KMMoveCommand( const Akonadi::Collection& destFolder,
                              const Akonadi::Item& msg )
  : mDestFolder( destFolder ), mProgressItem( 0 )
{
  mItem.append( msg );
}


KMMoveCommand::KMMoveCommand( quint32 )
  :  mDestFolder( 0 ), mProgressItem( 0 )
{
}

void KMMoveCommand::slotMoveResult( KJob * job )
{
  if ( job->error() ) {
    // handle errors
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    setResult( Failed );
  }
}

KMCommand::Result KMMoveCommand::execute()
{

  Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob( mItem, mDestFolder,this );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(slotMoveResult(KJob*)) );
#if 0 //TODO port to akonadi
  setEmitsCompletedItself( true );
  setDeletesItself( true );
  typedef QMap< KMFolder*, QList<KMime::Message*>* > FolderToMessageListMap;
  FolderToMessageListMap folderDeleteList;

  if ( mDestFolder && mDestFolder->open( "kmcommand" ) != 0 ) {
    completeMove( Failed );
    return Failed;
  }
  KCursorSaver busy( KBusyPtr::busy() );

  // TODO set SSL state according to source and destfolder connection?
  Q_ASSERT( !mProgressItem );
  mProgressItem =
    ProgressManager::createProgressItem ("move"+ProgressManager::getUniqueID(),
         mDestFolder ? i18n( "Moving messages" ) : i18n( "Deleting messages" ) );
  connect( mProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotMoveCanceled() ) );

  KMime::Message *msg;
  int rc = 0;
  int index;
  QList<KMime::Message*> list;
  int undoId = -1;
  mCompleteWithAddedMsg = false;

  if (mDestFolder) {
    connect (mDestFolder, SIGNAL(msgAdded(KMFolder*, quint32)),
             this, SLOT(slotMsgAddedToDestFolder(KMFolder*, quint32)));
    mLostBoys = mSerNumList;
  }
  mProgressItem->setTotalItems( mSerNumList.count() );

  for ( QList<quint32>::ConstIterator it = mSerNumList.constBegin(); it != mSerNumList.constEnd(); ++it ) {
    if ( *it == 0 ) {
      kDebug() << "serial number == 0!";
      continue; // invalid message
    }
    KMFolder *srcFolder = 0;
    int idx = -1;
    KMMsgDict::instance()->getLocation( *it, &srcFolder, &idx );
    if (srcFolder == mDestFolder)
      continue;
    assert(srcFolder);
    assert(idx != -1);
    if ( !srcFolder->isOpened() ) {
      srcFolder->open( "kmcommand" );
      mOpenedFolders.append( srcFolder );
    }
    msg = srcFolder->getMsg(idx);
    if ( !msg ) {
      kDebug() << "No message found for serial number" << *it;
      continue;
    }
    bool undo = msg->enableUndo();

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
          KMime::Message *mb = mDestFolder->unGetMsg( mDestFolder->count() - 1 );
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
          folderDeleteList[srcFolder] = new QList<KMime::Message*>;
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
      it.key()->removeMessages(*it.value());
      delete it.value();
    }
    if ( !mCompleteWithAddedMsg ) {
      // imap folders will be completed in slotMsgAddedToDestFolder
      completeMove( OK );
    }
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

#if 0 //TODO port to akonadi
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
      kDebug() << "### Not all moved messages reported back that they were";
      kDebug() << "### added to the target folder. Did uidValidity change?";
      completeMove( Failed );
    }
    else
      completeMove( OK );
  } else {
    // Should we inform the user here or leave that to the caller?
    completeMove( Failed );
  }
}
#endif

void KMMoveCommand::slotMsgAddedToDestFolder(KMFolder *folder, quint32 serNum)
{
#if 0 //Port to akonadi
  if ( folder != mDestFolder || !mLostBoys.contains( serNum )  ) {
    //kDebug() << "Different folder or invalid serial number.";
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
#endif
}

void KMMoveCommand::completeMove( Result result )
{
#if 0
  if ( mDestFolder )
    mDestFolder->close( "kmcommand" );
  while ( !mOpenedFolders.empty() ) {
    KMFolder *folder = mOpenedFolders.back();
    mOpenedFolders.pop_back();
    folder->close( "kmcommand" );
  }
  if ( mProgressItem ) {
    mProgressItem->setComplete();
    mProgressItem = 0;
  }
  setResult( result );
  emit completed( this );
  deleteLater();
#endif
}

void KMMoveCommand::slotMoveCanceled()
{
  completeMove( Canceled );
}

// srcFolder doesn't make much sense for searchFolders
KMTrashMsgCommand::KMTrashMsgCommand( const Akonadi::Collection& srcFolder,
  const QList<Akonadi::Item> &msgList )
:KMMoveCommand( findTrashFolder( srcFolder ), msgList)
{
  //srcFolder->open( "kmcommand" );
  //mOpenedFolders.push_back( srcFolder );
}

KMTrashMsgCommand::KMTrashMsgCommand( const Akonadi::Collection& srcFolder, const Akonadi::Item & msg )
:KMMoveCommand( findTrashFolder( srcFolder ), msg)
{
  //srcFolder->open( "kmcommand" );
  //mOpenedFolders.push_back( srcFolder );
}

KMTrashMsgCommand::KMTrashMsgCommand( quint32 sernum )
:KMMoveCommand( sernum )
{
#if 0 //Port to akonadi
  KMFolder *srcFolder = 0;
  int idx;
  KMMsgDict::instance()->getLocation( sernum, &srcFolder, &idx );

  if ( srcFolder ) {
    KMime::Message *msg = srcFolder->getMsgBase( idx );
    srcFolder->open( "kmcommand" );
    mOpenedFolders.push_back( srcFolder );
    addMsg( msg );
    setDestFolder( findTrashFolder( srcFolder ) );
  } else {
    kWarning() << "Failed to find a source folder for serial number: " << sernum;
  }
#endif
}

Akonadi::Collection KMTrashMsgCommand::findTrashFolder( const Akonadi::Collection& folder )
{
#if 0 //port to akonadi
  KMFolder* trash = folder->trashFolder();
  if( !trash )
    trash = kmkernel->trashFolder();
  if( trash != folder )
    return trash;
  return 0;
#endif
  return Akonadi::Collection();
}


KMUrlClickedCommand::KMUrlClickedCommand( const KUrl &url, uint identity,
  KMReaderWin *readerWin, bool htmlPref, KMMainWidget *mainWidget )
  :mUrl( url ), mIdentity( identity ), mReaderWin( readerWin ),
   mHtmlPref( htmlPref ), mMainWidget( mainWidget )
{
}

KMCommand::Result KMUrlClickedCommand::execute()
{
#if 0 //TODO port to akonadi
  KMime::Message* msg;

  if (mUrl.protocol() == "mailto")
  {
    msg = new KMime::Message;
    msg->initHeader(mIdentity);
    msg->setCharset("utf-8");

    QString to, body, subject, cc;
    KMail::StringUtil::parseMailtoUrl( mUrl, to, cc, subject, body );

    msg->setTo( to );
    if ( !subject.isEmpty() )
      msg->setSubject( subject );
    if ( !body.isEmpty() )
      msg->setBodyFromUnicode( body );
    if ( !cc.isEmpty() )
      msg->setCc( cc );

    KMail::Composer * win = KMail::makeComposer( msg, KMail::Composer::New, mIdentity );
    win->setCharset("", true);
    win->setFocusToSubject();
    win->show();
  }
  else if ((mUrl.protocol() == "http") || (mUrl.protocol() == "https") ||
           (mUrl.protocol() == "ftp")  || (mUrl.protocol() == "file")  ||
           (mUrl.protocol() == "ftps") || (mUrl.protocol() == "sftp" ) ||
           (mUrl.protocol() == "help") || (mUrl.protocol() == "vnc")   ||
           (mUrl.protocol() == "smb")  || (mUrl.protocol() == "fish")  ||
           (mUrl.protocol() == "news"))
  {
    KPIM::BroadcastStatus::instance()->setTransientStatusMsg( i18n("Opening URL..."));
    QTimer::singleShot( 2000, KPIM::BroadcastStatus::instance(), SLOT( reset() ) );

    KMimeType::Ptr mime = KMimeType::findByUrl( mUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-ms-dos-executable" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18nc( "@info", "Do you really want to execute <filename>%1</filename>?",
          mUrl.pathOrUrl() ), QString(), KGuiItem(i18n("Execute")), KStandardGuiItem::cancel() ) != KMessageBox::Yes)
        return Canceled;
    }
    if ( !KMail::Util::handleUrlOnMac( mUrl.pathOrUrl() ) ) {
      KRun *runner = new KRun( mUrl, mMainWidget ); // will delete itself
      runner->setRunExecutables( false );
    }
  }
  else
    return Failed;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const Akonadi::Item& msg )
  : KMCommand( parent, msg ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const QList<Akonadi::Item>& msgs )
  : KMCommand( parent, msgs ), mImplicitAttachments( true ), mEncoded( false )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, QList<partNode*>& attachments,
                                                    const Akonadi::Item &msg, bool encoded )
  : KMCommand( parent ), mImplicitAttachments( false ), mEncoded( encoded )
{
#if 0
  QList<partNode*>::const_iterator it;
  for ( it = attachments.constBegin(); it != attachments.constEnd(); ++it ) {
    mAttachmentMap.insert( (*it), msg );
  }
#endif
}

KMCommand::Result KMSaveAttachmentsCommand::execute()
{
#if 0 //TODO port to akonadi
  setEmitsCompletedItself( true );
  if ( mImplicitAttachments ) {
    QList<KMime::Message*> msgList = retrievedMsgs();
    QList<KMime::Message*>::const_iterator it;
    for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
      KMime::Message *msg = (*it);
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
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

void KMSaveAttachmentsCommand::slotSaveAll()
{
#if 0 //TODO port to akonadi
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
    dirUrl = KFileDialog::getExistingDirectoryUrl( KUrl( "kfiledialog:///saveAttachment" ),
                                                   parentWidget(),
                                                   i18n( "Save Attachments To" ) );
    if ( !dirUrl.isValid() ) {
      setResult( Canceled );
      emit completed( this );
      deleteLater();
      return;
    }

    // we may not get a slash-terminated url out of KFileDialog
    dirUrl.adjustPath( KUrl::AddTrailingSlash );
  }
  else {
    // only one item, get the desired filename
    partNode *node = mAttachmentMap.begin().key();
    QString s = node->msgPart().fileName().trimmed();
    if ( s.isEmpty() )
      s = node->msgPart().name().trimmed();
    if ( s.isEmpty() )
      s = i18nc("filename for an unnamed attachment", "attachment.1");
    else
      s = cleanFileName( s );

    url = KFileDialog::getSaveUrl( KUrl( "kfiledialog:///saveAttachment/" + s ),
                                   QString(),
                                   parentWidget(),
                                   i18n( "Save Attachment" ) );
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
  bool overwriteAll = false;
  for ( PartNodeMessageMap::const_iterator it = mAttachmentMap.constBegin();
        it != mAttachmentMap.constEnd();
        ++it ) {
    KUrl curUrl;
    if ( !dirUrl.isEmpty() ) {
      curUrl = dirUrl;
      QString s = it.key()->msgPart().fileName().trimmed();
      if ( s.isEmpty() )
        s = it.key()->msgPart().name().trimmed();
      if ( s.isEmpty() ) {
        ++unnamedAtmCount;
        s = i18nc("filename for the %1-th unnamed attachment",
                 "attachment.%1",
              unnamedAtmCount );
      }
      else
        s = cleanFileName( s );

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


      if ( !overwriteAll && KIO::NetAccess::exists( curUrl, KIO::NetAccess::DestinationSide, parentWidget() ) ) {
        if ( mAttachmentMap.count() == 1 ) {
          if ( KMessageBox::warningContinueCancel( parentWidget(),
                i18n( "A file named <br><filename>%1</filename><br>already exists.<br><br>Do you want to overwrite it?",
                  curUrl.fileName() ),
                i18n( "File Already Exists" ), KGuiItem(i18n("&Overwrite")) ) == KMessageBox::Cancel) {
            continue;
          }
        }
        else {
          int button = KMessageBox::warningYesNoCancel(
                parentWidget(),
                i18n( "A file named <br><filename>%1</filename><br>already exists.<br><br>Do you want to overwrite it?",
                  curUrl.fileName() ),
                i18n( "File Already Exists" ), KGuiItem(i18n("&Overwrite")),
                KGuiItem(i18n("Overwrite &All")) );
          if ( button == KMessageBox::Cancel )
            continue;
          else if ( button == KMessageBox::No )
            overwriteAll = true;
        }
      }
      // save
      const Result result = saveItem( it.key(), curUrl );
      if ( result != OK )
        globalResult = result;
    }
  }
  setResult( globalResult );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  emit completed( this );
  deleteLater();
}

QString KMCommand::cleanFileName( const QString &name )
{
  QString fileName = name.trimmed();
  // replace all ':' with '_' because ':' isn't allowed on FAT volumes
  fileName.replace( ':', '_' );
  // better not use a dir-delimiter in a filename
  fileName.replace( '/', '_' );
  fileName.replace( '\\', '_' );

  return fileName;
}

KMCommand::Result KMSaveAttachmentsCommand::saveItem( partNode *node,
                                                      const KUrl& url )
{
#if 0 //TODO port to akonadi
  bool bSaveEncrypted = false;
  bool bEncryptedParts = node->encryptionState() != KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?",
           url.fileName() ),
          i18n( "KMail Question" ), KGuiItem(i18n("Keep Encryption")), KGuiItem(i18n("Do Not Keep")) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if( node->signatureState() != KMMsgNotSigned )
    if( KMessageBox::questionYesNo( parentWidget(),
          i18n( "The part %1 of the message is signed. Do you want to keep the signature when saving?",
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
      if ( !bSaveWithSig ) {
        if ( DwMime::kTypeMultipart == node->type() &&
             DwMime::kSubtypeSigned == node->subType() ) {
          // carefully look for the part that is *not* the signature part:
          if ( node->findType( DwMime::kTypeApplication,
                               DwMime::kSubtypePgpSignature,
                               true, false ) ) {
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                                          DwMime::kSubtypePgpSignature,
                                          true, false );
          } else if ( node->findType( DwMime::kTypeApplication,
                                      DwMime::kSubtypePkcs7Mime,
                                      true, false ) ) {
            dataNode = node->findTypeNot( DwMime::kTypeApplication,
                                          DwMime::kSubtypePkcs7Mime,
                                          true, false );
          } else {
            dataNode = node->findTypeNot( DwMime::kTypeMultipart,
                                          DwMime::kSubtypeUnknown,
                                          true, false );
          }
        } else {
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
        // PENDING (romain) disable on Windows ?
        size = KMail::Util::crlf2lf( data.data(), size );
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
    file.setFileName( url.toLocalFile() );
    if ( !file.open( QIODevice::WriteOnly ) )
    {
      KMessageBox::error( parentWidget(),
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 file.fileName(),
                                 QString::fromLocal8Bit( strerror( errno ) ) ),
                          i18n( "Error saving attachment" ) );
      return Failed;
    }

    // #79685 by default use the umask the user defined, but let it be configurable
    if ( GlobalSettings::self()->disregardUmask() )
      fchmod( file.handle(), S_IRUSR | S_IWUSR );

    ds.setDevice( &file );
  } else
  {
    // tmp file for upload
    tf.open();
    ds.setDevice( &tf );
  }

  if ( ds.writeRawData( data.data(), data.size() ) == -1)
  {
    QFile *f = static_cast<QFile *>( ds.device() );
    KMessageBox::error( parentWidget(),
                        i18nc( "1 = file name, 2 = error string",
                               "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                               f->fileName(),
                               f->errorString() ),
                        i18n( "Error saving attachment" ) );
    return Failed;
  }

  if ( !url.isLocalFile() )
  {
    // QTemporaryFile::fileName() is only defined while the file is open
    QString tfName = tf.fileName();
    tf.close();
    if ( !KIO::NetAccess::upload( tfName, url, parentWidget() ) )
    {
      KMessageBox::error( parentWidget(),
                          i18nc( "1 = file name, 2 = error string",
                                 "<qt>Could not write to the file<br><filename>%1</filename><br><br>%2",
                                 url.prettyUrl(),
                                 KIO::NetAccess::lastErrorString() ),
                          i18n( "Error saving attachment" ) );
      return Failed;
    }
  } else
    file.close();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

KMLoadPartsCommand::KMLoadPartsCommand( QList<partNode*>& parts, const Akonadi::Item &msg )
  : mNeedsRetrieval( 0 )
{
#if 0
  QList<partNode*>::const_iterator it;
  for ( it = parts.constBegin(); it != parts.constEnd(); ++it ) {
    mPartMap.insert( (*it), msg );
  }
#endif
}

KMLoadPartsCommand::KMLoadPartsCommand( partNode *node, KMime::Message *msg )
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
#if 0 //TODO port to akonadi
  for ( PartNodeMessageMap::const_iterator it = mPartMap.constBegin();
        it != mPartMap.constEnd();
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
        connect( job, SIGNAL(messageUpdated(KMime::Message*, const QString&)),
                 this, SLOT(slotPartRetrieved(KMime::Message*, const QString&)) );
        job->start();
      } else
        kWarning() <<"KMLoadPartsCommand - msg has no parent";
    }
  }
  if ( mNeedsRetrieval == 0 )
    execute();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMLoadPartsCommand::slotPartRetrieved( KMime::Message *msg,
                                            const QString &partSpecifier )
{
#if 0 //TODO port to akonadi
  DwBodyPart *part =
    msg->findDwBodyPart( msg->getFirstDwBodyPart(), partSpecifier );
  if ( part ) {
    // update the DwBodyPart in the partNode
    for ( PartNodeMessageMap::const_iterator it = mPartMap.constBegin();
          it != mPartMap.constEnd();
          ++it ) {
      if ( it.key()->dwPart()->partId() == part->partId() )
        it.key()->setDwPart( part );
    }
  } else
    kWarning() << "Could not find bodypart!";
  --mNeedsRetrieval;
  if ( mNeedsRetrieval == 0 )
    execute();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
                                                const Akonadi::Item &msg )
  :KMCommand( parent, msg )
{
}

KMCommand::Result KMResendMessageCommand::execute()
{

  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() /*|| !msg->codec()*/ ) {
    return Failed;
  }
  KMime::Message *msg = message( item );
#if 0
  KMime::Message *newMsg = new KMime::Message( *msg );
  newMsg->setCharset( msg->codec()->name() );
  // the message needs a new Message-Id
  newMsg->removeHeader( "Message-Id" );
  newMsg->setParent( 0 );
  // Remember the identity for the message before removing the headers which
  // store the identity information.
  uint originalIdentity = newMsg->identityUoid();

  // Remove all unnecessary headers
  QStringList whiteList;
  whiteList << "To" << "Cc" << "Bcc" << "Subject";
  newMsg->sanitizeHeaders( whiteList );

  // Set the identity from above
  newMsg->setHeaderField( "X-KMail-Identity", QString::number( originalIdentity ) );
  newMsg->applyIdentity( originalIdentity );

  // Restore the original bcc field as this is overwritten in applyIdentity
  newMsg->setBcc( msg->bcc() );

  KMail::Composer * win = KMail::makeComposer();
  win->setMsg( newMsg, false, true );
  win->show();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

KMMailingListCommand::KMMailingListCommand( QWidget *parent, FolderCollection *folder )
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

KMMailingListPostCommand::KMMailingListPostCommand( QWidget *parent, FolderCollection *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListPostCommand::urls() const
{
  return mFolder->mailingList().postURLS();
}

KMMailingListSubscribeCommand::KMMailingListSubscribeCommand( QWidget *parent, FolderCollection *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListSubscribeCommand::urls() const
{
  return mFolder->mailingList().subscribeURLS();
}

KMMailingListUnsubscribeCommand::KMMailingListUnsubscribeCommand( QWidget *parent, FolderCollection *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListUnsubscribeCommand::urls() const
{
  return mFolder->mailingList().unsubscribeURLS();
}

KMMailingListArchivesCommand::KMMailingListArchivesCommand( QWidget *parent, FolderCollection *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListArchivesCommand::urls() const
{
  return mFolder->mailingList().archiveURLS();
}

KMMailingListHelpCommand::KMMailingListHelpCommand( QWidget *parent, FolderCollection *folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListHelpCommand::urls() const
{
  return mFolder->mailingList().helpURLS();
}

KMHandleAttachmentCommand::KMHandleAttachmentCommand( partNode* node,
                                                      const Akonadi::Item& msg, int atmId, const QString& atmName,
     AttachmentAction action, KService::Ptr offer, QWidget* parent )
: KMCommand( parent ), mNode( node ), mAtmId( atmId ), mAtmName( atmName ),
  mAction( action ), mOffer( offer ), mJob( 0 )
{
  mMsg = msg;
}

void KMHandleAttachmentCommand::slotStart()
{
#if 0 //TODO port to akonadi
  if ( !mNode->msgPart().isComplete() )
  {
    // load the part
    kDebug() << "load part";
    KMLoadPartsCommand *command = new KMLoadPartsCommand( mNode, mMsg );
    connect( command, SIGNAL( partsRetrieved() ),
        this, SLOT( slotPartComplete() ) );
    command->start();
  } else
  {
    execute();
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
      kDebug() << "unknown action" << mAction;
      break;
  }
  setResult( OK );
  emit completed( this );
  deleteLater();
  return OK;
}

QString KMHandleAttachmentCommand::createAtmFileLink() const
{
#if 0 //TODO port to akonadi
  QFileInfo atmFileInfo( mAtmName );

  if ( atmFileInfo.size() == 0 )
  {
    kDebug() << "rewriting attachment";
    // there is something wrong so write the file again
    QByteArray data = mNode->msgPart().bodyDecodedBinary();
    if ( mNode->msgPart().type() == DwMime::kTypeText && data.size() > 0 ) {
      // convert CRLF to LF before writing text attachments to disk
      const size_t newsize = KMail::Util::crlf2lf( data.data(), data.size() );
      data.truncate( newsize );
    }
    KPIMUtils::kByteArrayToFile( data, mAtmName, false, false, false );
  }

  KTemporaryFile *linkFile = new KTemporaryFile();
  linkFile->setPrefix(atmFileInfo.fileName() +"_[");
  linkFile->setSuffix( "]." + KMimeType::extractKnownExtension( atmFileInfo.fileName() ) );
  linkFile->open();
  QString linkName = linkFile->fileName();
  delete linkFile;

  if ( ::link(QFile::encodeName( mAtmName ), QFile::encodeName( linkName )) == 0 ) {
    return linkName; // success
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return QString();
}

KService::Ptr KMHandleAttachmentCommand::getServiceOffer()
{
#if 0 //TODO port to akonadi
  KMime::MessagePart& msgPart = mNode->msgPart();
  const QString contentTypeStr =
    ( msgPart.typeStr() + '/' + msgPart.subtypeStr() ).toLower();

  // determine the MIME type of the attachment
  KMimeType::Ptr mimetype;
  // prefer the value of the Content-Type header
  mimetype = KMimeType::mimeType( contentTypeStr, KMimeType::ResolveAliases );

  if ( !mimetype.isNull() && mimetype->is( KABC::Addressee::mimeType() ) ) {
    atmView();
    return KService::Ptr( 0 );
  }

  if ( mimetype.isNull() ) {
    // consider the filename if mimetype can not be found by content-type
    mimetype = KMimeType::findByPath( mAtmName, 0, true /* no disk access */ );
  }
  if ( ( mimetype->name() == "application/octet-stream" )
       && msgPart.isComplete() ) {
    // consider the attachment's contents if neither the Content-Type header
    // nor the filename give us a clue
    mimetype = KMimeType::findByFileContent( mAtmName );
  }
  return KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMHandleAttachmentCommand::atmOpen()
{
  if ( !mOffer )
    mOffer = getServiceOffer();
  if ( !mOffer ) {
    kDebug() << "got no offer";
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
  if ( (!KRun::run( *mOffer, lst, 0, autoDelete )) && autoDelete ) {
      QFile::remove(url.toLocalFile());
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
  if ( (! KRun::displayOpenWithDialog(lst, kmkernel->mainWin(), autoDelete)) && autoDelete ) {
    QFile::remove( url.toLocalFile() );
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
#if 0
  KMSaveAttachmentsCommand *command =
    new KMSaveAttachmentsCommand( 0, parts, mMsg, false );
  command->start();
#endif
}

void KMHandleAttachmentCommand::atmProperties()
{
  kDebug() << "port me to AttachmentPropertiesDialog";
#if 0
  KMMsgPartDialogCompat dlg( 0, true );
  KMime::MessagePart& msgPart = mNode->msgPart();
  //Port to KMime::Content
  dlg.setMsgPart( &msgPart );
  dlg.exec();
#endif
}

void KMHandleAttachmentCommand::atmEncryptWithChiasmus()
{
#if 0 //TODO port to akonadi
  const partNode * node = mNode;
  Q_ASSERT( node );
  if ( !node )
    return;

  // FIXME: better detection of mimetype??
  if ( !mAtmName.endsWith( QLatin1String(".xia"), Qt::CaseInsensitive ) )
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

  AutoQPointer<ChiasmusKeySelector> selectorDlg;
  selectorDlg = new ChiasmusKeySelector( parentWidget(),
                                         i18n( "Chiasmus Decryption Key Selection" ),
                                         keys, GlobalSettings::chiasmusDecryptionKey(),
                                         GlobalSettings::chiasmusDecryptionOptions() );
  if ( selectorDlg->exec() != QDialog::Accepted || !selectorDlg ) {
    return;
  }

  GlobalSettings::setChiasmusDecryptionOptions( selectorDlg->options() );
  GlobalSettings::setChiasmusDecryptionKey( selectorDlg->key() );
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
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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

  bool overwrite = KMail::Util::checkOverwrite( url, parentWidget() );
  if ( !overwrite )
    return;

  d.setDisabled( true ); // we got this far, don't delete yet
  KIO::Job * uploadJob = KIO::storedPut( result.toByteArray(), url, -1, KIO::Overwrite );
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

CreateTodoCommand::CreateTodoCommand(QWidget * parent, const Akonadi::Item &msg)
  : KMCommand( parent, msg )
{
}

KMCommand::Result CreateTodoCommand::execute()
{
  Akonadi::Item msg = retrievedMessage();
  if ( !msg.isValid() /*|| !msg->codec()*/ ) {
    return Failed;
  }

  KMail::KorgHelper::ensureRunning();
#if 0
  QString txt = i18n("From: %1\nTo: %2\nSubject: %3", msg->from(),
                     msg->to(), msg->subject() );

  KTemporaryFile tf;
  tf.setAutoRemove( true );
  if ( !tf.open() ) {
    kWarning() << "CreateTodoCommand: Unable to open temp file.";
    return Failed;
  }
  QString uri = "kmail:" + QString::number( msg->getMsgSerNum() ) + '/' + msg->msgId();
  tf.write( msg->asDwString().c_str(), msg->asDwString().length() );

  OrgKdeKorganizerCalendarInterface *iface =
      new OrgKdeKorganizerCalendarInterface( "org.kde.korganizer", "/Calendar",
                                             QDBusConnection::sessionBus(), this );
  iface->openTodoEditor( i18n("Mail: %1", msg->subject() ), txt, uri,
                         tf.fileName(), QStringList(), "message/rfc822", true );
  delete iface;
  tf.close();
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}

#include "kmcommands.moc"
