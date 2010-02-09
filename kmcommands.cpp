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
#include "messageviewer/nodehelper.h"
#include <kaddrbookexternal.h>
#include "composer.h"
#include "kmfiltermgr.h"
#include "kmmainwidget.h"
#include "messagesender.h"
#include "undostack.h"
#include "messageviewer/kcursorsaver.h"
#include "messageviewer/objecttreeparser.h"
#include "messageviewer/csshelper.h"
//using KMail::FolderJob;
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
#include <akonadi/itemdeletejob.h>

#include <messagelist/pane.h>
#include "messagecore/stringutil.h"
#include "messageviewer/nodehelper.h"
#include "messageviewer/objecttreeemptysource.h"

#include "progressmanager.h"
using KPIM::ProgressManager;
using KPIM::ProgressItem;
#include <kmime/kmime_mdn.h>
using namespace KMime;

#include "kleo/specialjob.h"
#include "kleo/cryptobackend.h"
#include "kleo/cryptobackendfactory.h"

#include <nepomuk/tag.h>

#include <gpgme++/error.h>

#include <QClipboard>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QMenu>
#include <QByteArray>
#include <QApplication>
#include <QDesktopWidget>
#include <QList>
#include <QProgressBar>

#include <boost/bind.hpp>
#include <algorithm>
#include <memory>

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

  // TODO once the message list is based on ETM and we get the more advanced caching we need to make that check a bit more clever
  if ( !mFetchScope.isEmpty() ) {
#if 0 //TODO port to akonadi
    if ( thisMsg->parent() && !thisMsg->isComplete() &&
         ( !mProgressDialog || !mProgressDialog->wasCancelled() ) )
    {
      totalSize += thisMsg->msgSizeServer();
      // emitted when the message was transferred successfully
      connect(job, SIGNAL(messageRetrieved(KMime::Message*)),
              this, SLOT(slotMsgTransfered(KMime::Message*)));
    }
#endif
    complete = false;
    KMCommand::mCountJobs++;
    Akonadi::ItemFetchJob *fetch = new Akonadi::ItemFetchJob( mMsgList, this );
    fetch->setFetchScope( mFetchScope );
    connect( fetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(slotMsgTransfered(Akonadi::Item::List)) );
    connect( fetch, SIGNAL(result(KJob*)), SLOT(slotJobFinished()) );
  } else {
    // no need to fetch anything
    mRetrievedMsgs = mMsgList;
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

void KMCommand::slotMsgTransfered(const Akonadi::Item::List& msgs)
{
  if ( mProgressDialog && mProgressDialog->wasCancelled() ) {
    emit messagesTransfered( Canceled );
    return;
  }
  // save the complete messages
  mRetrievedMsgs.append( msgs );
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

  if ( mCountMsgs > mRetrievedMsgs.count() )
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  KMCommand::mCountJobs = 0;
  mCountMsgs = 0;
  mRetrievedMsgs.clear();
  emit messagesTransfered( Canceled );
}


KMMailtoComposeCommand::KMMailtoComposeCommand( const KUrl &url,
                                                const Akonadi::Item &msg )
  :mUrl( url ), mMessage( msg )
{
}

KMCommand::Result KMMailtoComposeCommand::execute()
{
  KMime::Message::Ptr msg( new KMime::Message );
  uint id = 0;

  if ( mMessage.isValid() && mMessage.parentCollection().isValid() ) {
    QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( mMessage.parentCollection() );
    id = fd->identity();
  }

  KMail::MessageHelper::initHeader( msg, id );
  msg->contentType()->setCharset("utf-8");
  msg->to()->fromUnicodeString( MessageCore::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" );

  KMail::Composer * win = KMail::makeComposer( msg, KMail::Composer::New, id );
  win->setFocusToSubject();
  win->show();
  return OK;
}


KMMailtoReplyCommand::KMMailtoReplyCommand( QWidget *parent,
                                            const KUrl &url, const Akonadi::Item &msg, const QString &selection )
  :KMCommand( parent, msg ), mUrl( url ), mSelection( selection  )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMMailtoReplyCommand::execute()
{
  //TODO : consider factoring createReply into this method.
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMime::Message::Ptr rmsg( KMail::MessageHelper::createReply( item, msg, KMail::ReplyNone, mSelection ).msg );
  rmsg->to()->fromUnicodeString( MessageCore::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" ); //TODO Check the UTF-8

  KMail::Composer * win = KMail::makeComposer( rmsg, KMail::Composer::Reply, 0, mSelection );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMMailtoForwardCommand::KMMailtoForwardCommand( QWidget *parent,
                                                const KUrl &url, const Akonadi::Item &msg )
  :KMCommand( parent, msg ), mUrl( url )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMMailtoForwardCommand::execute()
{
  //TODO : consider factoring createForward into this method.
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMime::Message::Ptr fmsg( KMail::MessageHelper::createForward( item, msg ) );
  fmsg->to()->fromUnicodeString( MessageCore::StringUtil::decodeMailtoUrl( mUrl.path() ), "utf-8" ); //TODO check the utf-8

  KMail::Composer * win = KMail::makeComposer( fmsg, KMail::Composer::Forward );
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
  KPIM::KAddrBookExternal::addEmail( MessageCore::StringUtil::decodeMailtoUrl( mUrl.path() ),
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
  QString addr = MessageCore::StringUtil::decodeMailtoUrl( mUrl.path() );
  KPIM::KAddrBookExternal::openEmail( KPIMUtils::extractEmailAddress(addr),
                                      addr, parentWidget() );

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
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
  setDeletesItself( true );
}

KMCommand::Result KMEditMsgCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if (!item.isValid() || !item.parentCollection().isValid() ||
      ( !kmkernel->folderIsDraftOrOutbox( item.parentCollection() ) &&
        !kmkernel->folderIsTemplates( item.parentCollection() ) ) ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( item );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotDeleteItem( KJob* ) ) );
  KMail::Composer *win = KMail::makeComposer();
  win->setMsg( msg, false, true );
  win->setFolder( item.parentCollection() );
  win->show();

  return OK;
}

void KMEditMsgCommand::slotDeleteItem( KJob *job )
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
  deleteLater( );
}

KMUseTemplateCommand::KMUseTemplateCommand( QWidget *parent, const Akonadi::Item  &msg )
  :KMCommand( parent, msg )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMUseTemplateCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid()
       || !item.parentCollection().isValid() ||
       !kmkernel->folderIsTemplates( item.parentCollection() )
       ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;

  KMime::Message::Ptr newMsg(new KMime::Message);
  newMsg->setContent( msg->encodedContent() );
  // these fields need to be regenerated for the new message
  newMsg->removeHeader("Date");
  newMsg->removeHeader("Message-ID");

  KMail::Composer *win = KMail::makeComposer();
  win->setMsg( newMsg, false, true );
  win->show();
#if 0
  newMsg->setComplete( msg->isComplete() );
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

  const QString filter = i18n( "*.mbox|email messages (*.mbox)\n*|all files (*)" );
  return KFileDialog::getSaveUrl( KUrl::fromPath( fileName ), filter );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent, const Akonadi::Item& msg )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mStandAloneMessage( 0 ),
    mOffset( 0 ),
    mTotalSize( msg.size() )
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
  fetchScope().fetchFullPayload( true ); // ### unless we call the corresponding KMCommand ctor, this has no effect
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
  fetchScope().fetchFullPayload( true ); // ### unless we call the corresponding KMCommand ctor, this has no effect
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

  mMsgString.append( data.data() );
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
    if ( mMsgString.startsWith("From ") ) {
      startOfMessage = mMsgString.indexOf( '\n' );
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
    int endOfMessage = mMsgString.indexOf( "\nFrom " );
    if ( endOfMessage == -1 ) {
      endOfMessage = mMsgString.length();
      multipleMessages = false;
    }
    KMime::Message *msg = new KMime::Message;
    msg->setContent( mMsgString.mid( startOfMessage,endOfMessage - startOfMessage ).toUtf8() );
    msg->parse();
    if ( !msg->hasContent() ) {
      KMessageBox::sorry( parentWidget(),
                          i18n( "The file does not contain a message." ) );
      delete msg; msg = 0;
      setResult( Failed );
      emit completed( this );
      // Emulate closing of a secondary window (see above).
      SecondaryWindow *win = new SecondaryWindow();
      win->close();
      win->deleteLater();
      deleteLater();
      return;
    }
    KMReaderMainWin *win = new KMReaderMainWin();
    Akonadi::Item item;
    KMime::Message::Ptr mMsg( msg );
    item.setPayload( mMsg );
    item.setMimeType( "message/rfc822" );
    win->showMessage( mEncoding, item );
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
KMReplyToCommand::KMReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                                    const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMReplyToCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply( item, msg, KMail::ReplySmart, mSelection );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ), 0, mSelection );
  win->setReplyFocus();
  win->show();


  return OK;
}


KMNoQuoteReplyToCommand::KMNoQuoteReplyToCommand( QWidget *parent,
                                                  const Akonadi::Item &msg )
  : KMCommand( parent, msg )
{
}

KMCommand::Result KMNoQuoteReplyToCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply( item, msg, KMail::ReplySmart, "", true);
  KMail::Composer *win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ) );
  win->setReplyFocus( false );
  win->show();

  return OK;
}


KMReplyListCommand::KMReplyListCommand( QWidget *parent,
                                        const Akonadi::Item &msg, const QString &selection )
 : KMCommand( parent, msg ), mSelection( selection )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMReplyListCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply( item, msg, KMail::ReplyList, mSelection );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ),
                                               0, mSelection );
  win->setReplyFocus( false );
  win->show();

  return OK;
}


KMReplyToAllCommand::KMReplyToAllCommand( QWidget *parent,
                                          const Akonadi::Item &msg, const QString &selection )
  :KMCommand( parent, msg ), mSelection( selection )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMReplyToAllCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }

  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply( item, msg, KMail::ReplyAll, mSelection );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ), 0,
                                               mSelection );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMReplyAuthorCommand::KMReplyAuthorCommand( QWidget *parent, const Akonadi::Item &msg,
                                            const QString &selection )
  : KMCommand( parent, msg ), mSelection( selection )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMReplyAuthorCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply = KMail::MessageHelper::createReply( item, msg, KMail::ReplyAuthor, mSelection );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ), 0,
                                               mSelection );
  win->setReplyFocus();
  win->show();

  return OK;
}


KMForwardCommand::KMForwardCommand( QWidget *parent,
  const QList<Akonadi::Item> &msgList, uint identity )
  : KMCommand( parent, msgList ),
    mIdentity( identity )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMForwardCommand::KMForwardCommand( QWidget *parent, const Akonadi::Item &msg,
                                    uint identity )
  : KMCommand( parent, msg ),
    mIdentity( identity )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMForwardCommand::execute()
{
  QList<Akonadi::Item> msgList = retrievedMsgs();

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
#if 0
      uint id = 0;
      KMime::Message *fwdMsg = new KMime::Message;
      KMime::MessagePart *msgPart = new KMime::MessagePart;
      QString msgPartText;
      int msgCnt = 0; // incase there are some we can't forward for some reason

      // dummy header initialization; initialization with the correct identity
      // is done below
      KMail::MessageHelper::initHeader( fwdMsg, id );
      KMail::MessageHelper::setAutomaticFields( fwdMsg, true );
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
      KMail::MessageHelper::initHeader( fwdMsg, id );
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
      MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::NoTemplate, id );
      win->addAttach(msgPart);
      win->show();
#endif
      return OK;
    } else if ( answer == KMessageBox::No ) {// NO MIME DIGEST, Multiple forward
      uint id = 0;
      QList<Akonadi::Item>::const_iterator it;
      for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
        // set the identity
        KMime::Message::Ptr msg = KMail::Util::message( *it );
        if ( msg ) {
          if (id == 0)
            id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
        }

      }
      if ( id == 0 )
        id = mIdentity; // use folder identity if no message had an id set
      KMime::Message::Ptr fwdMsg( new KMime::Message );
      KMail::MessageHelper::initHeader( fwdMsg, id );
      KMail::MessageHelper::setAutomaticFields( fwdMsg, true );
      fwdMsg->contentType()->setCharset("utf-8");

      for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
        KMime::Message::Ptr msg = KMail::Util::message( *it );
        if ( msg ) {

          TemplateParser parser( fwdMsg, TemplateParser::Forward );
          parser.setSelection( msg->body() ); // FIXME: Why is this needed?
          parser.process( msg, ( *it ).parentCollection(), true );

          KMail::MessageHelper::link( msg, *it, KPIM::MessageStatus::statusForwarded() );
        }
      }

      MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::NoTemplate, id );
      win->show();
      return OK;
    } else {
      // user cancelled
      return OK;
    }
  }

  // forward a single message at most.
  Akonadi::Item item = msgList.first();
  if ( !item.isValid() )
    return Failed;

  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  KMime::Message::Ptr fwdMsg( KMail::MessageHelper::createForward(item, msg) );

  uint id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
  if ( id == 0 )
    id = mIdentity;
  {
    KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id );
    win->show();
  }
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return OK;
}


KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
           const QList<Akonadi::Item> &msgList, uint identity, KMail::Composer *win )
  : KMCommand( parent, msgList ), mIdentity( identity ),
    mWin( QPointer<KMail::Composer>( win ))
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMForwardAttachedCommand::KMForwardAttachedCommand( QWidget *parent,
                                                    const Akonadi::Item & msg, uint identity, KMail::Composer *win )
  : KMCommand( parent, msg ), mIdentity( identity ),
    mWin( QPointer< KMail::Composer >( win ))
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMForwardAttachedCommand::execute()
{
  QList<Akonadi::Item> msgList = retrievedMsgs();
  KMime::Message::Ptr fwdMsg( new KMime::Message );

  if (msgList.count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    KMail::MessageHelper::initHeader(fwdMsg, mIdentity);
  }
  else if (msgList.count() == 1) {
    Akonadi::Item item = msgList.first();
    KMime::Message::Ptr msg = KMail::Util::message( item );
    if ( !msg )
      return Failed;
    KMail::MessageHelper::initFromMessage(item, fwdMsg, msg);
    fwdMsg->subject()->fromUnicodeString(  KMail::MessageHelper::forwardSubject(msg),"utf-8" );
  }
  KMail::MessageHelper::setAutomaticFields(fwdMsg, true);
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  if (!mWin)
    mWin = KMail::makeComposer(fwdMsg, KMail::Composer::Forward, mIdentity);
  // iterate through all the messages to be forwarded
  Akonadi::Item itemMsg;
  foreach ( itemMsg, msgList ) {
    KMime::Message::Ptr msg = KMail::Util::message( itemMsg );
    if ( !msg )
      return Failed;
    // remove headers that shouldn't be forwarded
    KMail::MessageHelper::removePrivateHeaderFields(msg);
    msg->removeHeader("BCC");
    // set the part
    KMime::Content *msgPart = new KMime::Content( msg.get() );
    msgPart->contentType()->setMimeType( "message/rfc822" );
    msgPart->contentType()->setCharset( NodeHelper::charset( msg.get() ) );


    msgPart->contentDisposition()->setFilename( "forwarded message" );
    msgPart->contentDisposition()->setDisposition( KMime::Headers::CDinline );
    msgPart->contentDescription()->fromUnicodeString( msg->from()->asUnicodeString() + ": " + msg->subject()->asUnicodeString(), "utf-8" );
    msgPart->fromUnicodeString( msg->encodedContent() );
    msgPart->assemble();
#if 0
    // THIS HAS TO BE AFTER setCte()!!!!
    msgPart->setCharset( "" );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    KMail::MessageHelper::link( msg, itemMsg, KPIM::MessageStatus::statusForwarded() );
    mWin->addAttach( msgPart );
  }

  mWin->show();
  return OK;
}


KMRedirectCommand::KMRedirectCommand( QWidget *parent,
                                      const Akonadi::Item &msg )
  : KMCommand( parent, msg )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMRedirectCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  MessageViewer::AutoQPointer<RedirectDialog> dlg(
      new RedirectDialog( parentWidget(), kmkernel->msgSender()->sendImmediate() ) );
  dlg->setObjectName( "redirect" );
  if ( dlg->exec() == QDialog::Rejected || !dlg ) {
    return Failed;
  }

  KMime::Message::Ptr newMsg = KMail::MessageHelper::createRedirect( item, dlg->to() );
  if ( !newMsg )
    return Failed;

  KMFilterAction::sendMDN( KMail::Util::message( item ), KMime::MDN::Dispatched );

  const KMail::MessageSender::SendMethod method = dlg->sendImmediate()
    ? KMail::MessageSender::SendImmediate
    : KMail::MessageSender::SendLater;
  if ( !kmkernel->msgSender()->send( newMsg, method ) ) {
    kDebug() << "KMRedirectCommand: could not redirect message (sending failed)";
    return Failed; // error: couldn't send
  }
  return OK;
}


KMCustomReplyToCommand::KMCustomReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                                                const QString &selection,
                                                const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMCustomReplyToCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply =
      KMail::MessageHelper::createReply( item, msg, KMail::ReplySmart, mSelection,
                                         false, true, mTemplate );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ), 0,
                                               mSelection, mTemplate );
  win->setReplyFocus();
  win->show();


  return OK;
}


KMCustomReplyAllToCommand::KMCustomReplyAllToCommand( QWidget *parent, const Akonadi::Item &msg,
                                                      const QString &selection,
                                                      const QString &tmpl )
  : KMCommand( parent, msg ), mSelection( selection ), mTemplate( tmpl )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMCustomReplyAllToCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  KMail::MessageHelper::MessageReply reply =
      KMail::MessageHelper::createReply( item, msg, KMail::ReplyAll, mSelection,
                                         false, true, mTemplate );
  KMail::Composer * win = KMail::makeComposer( KMime::Message::Ptr( reply.msg ), replyContext( reply ), 0,
                                               mSelection, mTemplate );
  win->setReplyFocus();
  win->show();


  return OK;
}


KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
  const QList<Akonadi::Item> &msgList, uint identity, const QString &tmpl )
  : KMCommand( parent, msgList ),
    mIdentity( identity ), mTemplate( tmpl )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCustomForwardCommand::KMCustomForwardCommand( QWidget *parent,
                                                const Akonadi::Item &msg, uint identity, const QString &tmpl )
  : KMCommand( parent, msg ),
    mIdentity( identity ), mTemplate( tmpl )
{
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMCustomForwardCommand::execute()
{
  QList<Akonadi::Item> msgList = retrievedMsgs();

  if (msgList.count() >= 2) { // Multiple forward

    uint id = 0;
    // QCString msgText = "";
    for ( QList<Akonadi::Item>::const_iterator it = msgList.constBegin(); it != msgList.constEnd(); ++it )
    {
      KMime::Message::Ptr msg = KMail::Util::message( *it );
      if ( msg ) {
        // set the identity
        if (id == 0)
          id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType( "X-KMail-Identity" )->asUnicodeString().trimmed().toUInt() : 0;
      }
    }
    if ( id == 0 )
      id = mIdentity; // use folder identity if no message had an id set
    KMime::Message::Ptr fwdMsg( new KMime::Message );
    KMail::MessageHelper::initHeader( fwdMsg, id );
    KMail::MessageHelper::setAutomaticFields( fwdMsg, true );

    fwdMsg->contentType()->setCharset("utf-8");
    // fwdMsg->setBody( msgText );
    for ( QList<Akonadi::Item>::const_iterator it = msgList.constBegin(); it != msgList.constEnd(); ++it )
    {
      KMime::Message::Ptr msg = KMail::Util::message( *it );
      if ( msg ) {

        TemplateParser parser( fwdMsg, TemplateParser::Forward );
        parser.setSelection( msg->body() ); // FIXME: Why is this needed?
        parser.process( msg, ( *it ).parentCollection(), true );

        KMail::MessageHelper::link( msg, *it, MessageStatus::statusForwarded() );
      }
    }

    MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
    KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id,
                                                 QString(), mTemplate );
    win->show();
  } else { // forward a single message at most

    Akonadi::Item item = msgList.first();
    if ( !item.isValid() ) {
      return Failed;
    }
    KMime::Message::Ptr msg = KMail::Util::message( item );
    if ( !msg )
      return Failed;
    MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
    KMime::Message::Ptr fwdMsg = KMail::MessageHelper::createForward( item, msg, mTemplate );

    uint id = 0;
    QString strId = msg->headerByType( "X-KMail-Identity" ) ? msg->headerByType( "X-KMail-Identity" )->asUnicodeString().trimmed() : "";
    if ( !strId.isEmpty())
      id = strId.toUInt();
    if ( id == 0 )
      id = mIdentity;
    {
      KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id,
                                                   QString(), mTemplate );
      win->show();
    }
  }
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
  fetchScope().fetchFullPayload( true );
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
  printerWin->viewer()->printMessage( retrievedMessage() );
  return OK;
}


KMSetStatusCommand::KMSetStatusCommand( const MessageStatus& status,
  const Akonadi::Item::List &items, bool toggle )
  : KMCommand( 0, items ), mStatus( status ), messageStatusChanged( 0 ), mToggle( toggle )
{
  setDeletesItself(true);
}

KMCommand::Result KMSetStatusCommand::execute()
{
  bool parentStatus = false;
  // Toggle actions on threads toggle the whole thread
  // depending on the state of the parent.
  if ( mToggle ) {
    const Akonadi::Item first = retrievedMsgs().first();
    MessageStatus pStatus;
    pStatus.setStatusFromFlags( first.flags() );
    if ( pStatus & mStatus )
      parentStatus = true;
    else
      parentStatus = false;
  }

  foreach( const Akonadi::Item &it, retrievedMsgs() ) {
    if ( mToggle ) {
      //kDebug()<<" item ::"<<tmpItem;
      if ( it.isValid() ) {
        bool myStatus;
        MessageStatus itemStatus;
        itemStatus.setStatusFromFlags( it.flags() );
        if ( itemStatus & mStatus )
          myStatus = true;
        else
          myStatus = false;
        if ( myStatus != parentStatus )
          continue;
      }
    }

    Akonadi::Item item( it );
    // Set a custom flag
    MessageStatus itemStatus;
    itemStatus.setStatusFromFlags( item.flags() );

//     MessageStatus oldStatus = itemStatus;
    if ( mToggle ) {
      itemStatus.toggle( mStatus );
    } else {
      itemStatus.set( mStatus );
    }
    /*if ( itemStatus != oldStatus )*/ {
      item.setFlags( itemStatus.getStatusFlags() );
      // Store back modified item
      Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item, this );
      ++messageStatusChanged;
      connect( modifyJob, SIGNAL( result( KJob* ) ), this, SLOT( slotModifyItemDone( KJob* ) ) );
    }
  }
  if ( messageStatusChanged == 0 )
    deleteLater();
  return OK;
}

void KMSetStatusCommand::slotModifyItemDone( KJob * job )
{
  if ( job->error() ) {
    kDebug()<<" Error in void KMSetStatusCommand::slotModifyItemDone( KJob * job ) :"<<job->errorText();
  }
  --messageStatusChanged;
  if ( messageStatusChanged == 0 ) {
    QDBusMessage message =
      QDBusMessage::createSignal( "/KMail", "org.kde.kmail.kmail", "unreadCountChanged" );
    QDBusConnection::sessionBus().send( message );
    deleteLater();
  }
}

KMSetTagCommand::KMSetTagCommand( const QString &tagLabel, const QList<Akonadi::Item> &item,
    SetTagMode mode )
  : mTagLabel( tagLabel )
  , mItem( item )
  , mMode( mode )
{
}

KMCommand::Result KMSetTagCommand::execute()
{
  //Set the visible name for the tag
  const Nepomuk::Tag n_tag( mTagLabel );
  Q_FOREACH( const Akonadi::Item item, mItem ) {
    Nepomuk::Resource n_resource( item.url() );
    const QList<Nepomuk::Tag> tagList = n_resource.tags();

    const int tagPosition = tagList.indexOf( mTagLabel );
    if ( tagPosition == -1 ) {
      n_resource.addTag( n_tag );
    } else if ( mMode == Toggle ) {
      QList< Nepomuk::Tag > n_tag_list = n_resource.tags();
      for (int i = 0; i < n_tag_list.count(); ++i ) {
        if ( n_tag_list[i].resourceUri() == mTagLabel ) {
          n_tag_list.removeAt( i );
          break;
        }
      }
      n_resource.setTags( n_tag_list );
    }
  }
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
  fetchScope().fetchFullPayload();
}

KMCommand::Result KMFilterActionCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );

  int msgCount = 0;
  int msgCountToFilter = retrievedMsgs().size();
  ProgressItem* progressItem =
     ProgressManager::createProgressItem (
         "filter"+ProgressManager::getUniqueID(),
         i18n( "Filtering messages" ) );
  progressItem->setTotalItems( msgCountToFilter );

  foreach ( const Akonadi::Item &item, retrievedMsgs() ) {
    int diff = msgCountToFilter - ++msgCount;
    if ( diff < 10 || !( msgCount % 10 ) || msgCount <= 10 ) {
      progressItem->updateProgress();
      QString statusMsg = i18n( "Filtering message %1 of %2",
                                msgCount, msgCountToFilter );
      KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );
      qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 50 );
    }

    int filterResult = kmkernel->filterMgr()->process( item, mFilter );
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
    QList<Akonadi::Item> msgList = mMainWidget->messageListPane()->selectionAsMessageItemList();

    foreach( const Akonadi::Item &item, msgList )
      scheduler->execFilters( item );
  } else {
    KMCommand *filterCommand = new KMFilterActionCommand(
        mMainWidget, mMainWidget->messageListPane()->selectionAsMessageItemList() , mFilter );
    filterCommand->start();
  }
}

FolderShortcutCommand::FolderShortcutCommand( KMMainWidget *mainwidget,
                                              const Akonadi::Collection&col  )
    : QObject( mainwidget ), mMainWidget( mainwidget ), mCollectionFolder( col ), mAction( 0 )
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
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  if ( !MailingList::name( msg, name, value ).isEmpty() ) {
    kmkernel->filterMgr()->createFilter( name, value );
    return OK;
  } else {
    return Failed;
  }
}

KMCopyCommand::KMCopyCommand( const Akonadi::Collection& destFolder,
                              const QList<Akonadi::Item> &msgList)
  :mDestFolder( destFolder ), mMsgList( msgList )
{
  setDeletesItself( true );
}

KMCopyCommand::KMCopyCommand( const Akonadi::Collection& destFolder, const Akonadi::Item& msg)
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
                              const QList<Akonadi::Item> &msgList,
                              MessageList::Core::MessageItemSetReference ref)
    : KMCommand( 0, msgList ), mDestFolder( destFolder ), mProgressItem( 0 ), mRef( ref )
{
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMMoveCommand::KMMoveCommand( const Akonadi::Collection& destFolder,
                              const Akonadi::Item& msg ,
                              MessageList::Core::MessageItemSetReference ref)
  : KMCommand( 0, msg ), mDestFolder( destFolder ), mProgressItem( 0 ), mRef( ref )
{
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

void KMMoveCommand::slotMoveResult( KJob * job )
{
  if ( job->error() ) {
    // handle errors
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    setResult( Failed );
  }
  else
    setResult( OK );
  deleteLater();
  emit moveDone(this);
}

KMCommand::Result KMMoveCommand::execute()
{
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );

  setEmitsCompletedItself( true );
  setDeletesItself( true );
  if ( mDestFolder.isValid() ) {
    Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob( retrievedMsgs(), mDestFolder, this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotMoveResult(KJob*)) );

    // group by source folder for undo
    Akonadi::Item::List items = retrievedMsgs();
    std::sort( items.begin(), items.end(), boost::bind( &Akonadi::Item::storageCollectionId, _1 ) <
                                           boost::bind( &Akonadi::Item::storageCollectionId, _2 ) );
    Akonadi::Collection parent;
    int undoId = -1;
    foreach ( const Akonadi::Item &item, items ) {
      if ( item.storageCollectionId() <= 0 )
        continue;
      if ( parent.id() != item.storageCollectionId() ) {
        parent = Akonadi::Collection( item.storageCollectionId() );
        undoId = kmkernel->undoStack()->newUndoAction( parent, mDestFolder );
      }
      kmkernel->undoStack()->addMsgToAction( undoId, item );
    }
  }
  else {
    Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( retrievedMsgs(), this );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotMoveResult( KJob* ) ) );
  }

#if 0 //TODO port to akonadi
  // TODO set SSL state according to source and destfolder connection?
  Q_ASSERT( !mProgressItem );
  mProgressItem =
    ProgressManager::createProgressItem ("move"+ProgressManager::getUniqueID(),
         mDestFolder ? i18n( "Moving messages" ) : i18n( "Deleting messages" ) );
  connect( mProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotMoveCanceled() ) );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return OK;
}



void KMMoveCommand::completeMove( Result result )
{
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
KMTrashMsgCommand::KMTrashMsgCommand( const Akonadi::Collection& srcFolder,
  const QList<Akonadi::Item> &msgList,MessageList::Core::MessageItemSetReference ref )
:KMMoveCommand( findTrashFolder( srcFolder ), msgList, ref)
{
}

KMTrashMsgCommand::KMTrashMsgCommand( const Akonadi::Collection& srcFolder, const Akonadi::Item & msg,MessageList::Core::MessageItemSetReference ref )
:KMMoveCommand( findTrashFolder( srcFolder ), msg, ref)
{
}


Akonadi::Collection KMTrashMsgCommand::findTrashFolder( const Akonadi::Collection& folder )
{
  Akonadi::Collection col = kmkernel->trashCollectionFromResource( folder );
  if ( !col.isValid() ) {
    col = kmkernel->trashCollectionFolder();
  }
  if ( folder != col )
    return col;
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
  KMime::Message::Ptr msg;

  if (mUrl.protocol() == "mailto")
  {
    msg = KMime::Message::Ptr( new KMime::Message );
    KMail::MessageHelper::initHeader( msg, mIdentity );
    msg->contentType()->setCharset("utf-8");

    QMap<QString, QString> fields =  KMail::StringUtil::parseMailtoUrl( mUrl );

    msg->to()->fromUnicodeString( fields.value( "to" ),"utf-8" );
    if ( !fields.value( "subject" ).isEmpty() )
      msg->subject()->fromUnicodeString( fields.value( "subject" ),"utf-8" );
    if ( !fields.value( "body" ).isEmpty() )
      msg->setBody( fields.value( "body" ).toUtf8() );
    if ( !fields.value( "cc" ).isEmpty() )
      msg->cc()->fromUnicodeString( fields.value( "cc" ),"utf-8" );

    KMail::Composer * win = KMail::makeComposer( msg, KMail::Composer::New, mIdentity );
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
    if ( !MessageViewer::Util::handleUrlOnMac( mUrl.pathOrUrl() ) ) {
      KRun *runner = new KRun( mUrl, mMainWidget ); // will delete itself
      runner->setRunExecutables( false );
    }
  }
  else
    return Failed;
  return OK;
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const Akonadi::Item& msg )
  : KMCommand( parent, msg )
{
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand( QWidget *parent, const QList<Akonadi::Item>& msgs )
  : KMCommand( parent, msgs )
{
}

KMCommand::Result KMSaveAttachmentsCommand::execute()
{
  setEmitsCompletedItself( true );
  QList<Akonadi::Item> msgList = retrievedMsgs();
  QList<Akonadi::Item>::const_iterator it;
  for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
#if 0 //TODO port to akonadi
    partNode *rootNode = partNode::fromMessage( msg );
    for ( partNode *child = rootNode; child;
          child = child->firstChild() ) {
      for ( partNode *node = child; node; node = node->nextSibling() ) {
        if ( node->type() != DwMime::kTypeMultipart )
          mAttachmentMap.insert( node, msg );
      }
    }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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
    for ( PartNodeMessageMap::iterator it = mAttachmentMap.begin();
          it != mAttachmentMap.end(); ) {
      // only body parts which have a filename or a name parameter (except for
      // the root node for which name is set to the message's subject) are
      // considered attachments
      if ( it.key()->contentDisposition()->filename().trimmed().isEmpty() &&
           ( it.key()->contentType()->name().trimmed().isEmpty() ||
             !it.key()->topLevel() ) ) {
        PartNodeMessageMap::iterator delIt = it;
        ++it;
        mAttachmentMap.erase( delIt );
      }
      else
        ++it;
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
    KMime::Content *content = mAttachmentMap.begin().key();
    QString s = content->contentDisposition()->filename().trimmed();
    if ( s.isEmpty() )
      s = content->contentType()->name().trimmed();
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
    KMime::Content *content = it.key();
    if ( !dirUrl.isEmpty() ) {
      curUrl = dirUrl;
      QString s = content->contentDisposition()->filename().trimmed();
      if ( s.isEmpty() )
        s = content->contentType()->name().trimmed();
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
  emit completed( this );
  deleteLater();
}

QString KMCommand::cleanFileName( const QString &name )
{
  QString fileName = name.trimmed();

  // We need to replace colons with underscores since those cause problems with
  // KFileDialog (bug in KFileDialog though) and also on Windows filesystems.
  // We also look at the special case of ": ", since converting that to "_ "
  // would look strange, simply "_" looks better.
  // https://issues.kolab.org/issue3805
  fileName.replace( ": ", "_" );
  // replace all ':' with '_' because ':' isn't allowed on FAT volumes
  fileName.replace( ':', '_' );
  // better not use a dir-delimiter in a filename
  fileName.replace( '/', '_' );
  fileName.replace( '\\', '_' );

  // replace all '.' with '_', not just at the start of the filename
  fileName.replace( '.', '_' );

  // replace all '~' with '_', not just leading '~' either.
  fileName.replace( '~', '_' );

  return fileName;
}

KMCommand::Result KMSaveAttachmentsCommand::saveItem( KMime::Content *content,
                                                      const KUrl& url )
{
  KMime::Content *topContent  = content->topLevel();
  MessageViewer::NodeHelper *mNodeHelper = new MessageViewer::NodeHelper;
  bool bSaveEncrypted = false;
  bool bEncryptedParts = mNodeHelper->encryptionState( content ) != KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( parentWidget(),
                                    i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?",
                                          url.fileName() ),
                                    i18n( "KMail Question" ), KGuiItem(i18n("Keep Encryption")), KGuiItem(i18n("Do Not Keep")) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if(mNodeHelper->signatureState( content ) != KMMsgNotSigned )
    if( KMessageBox::questionYesNo( parentWidget(),
                                    i18n( "The part %1 of the message is signed. Do you want to keep the signature when saving?",
                                          url.fileName() ),
                                    i18n( "KMail Question" ), KGuiItem(i18n("Keep Signature")), KGuiItem(i18n("Do Not Keep")) ) !=
        KMessageBox::Yes )
      bSaveWithSig = false;

  QByteArray data;
  if( bSaveEncrypted || !bEncryptedParts) {
    KMime::Content *dataNode = content;
    QByteArray rawReplyString;
    bool gotRawReplyString = false;
    if ( !bSaveWithSig ) {
      if ( topContent->contentType()->mimeType() == "multipart/signed" )  {
        // carefully look for the part that is *not* the signature part:
        if ( MessageViewer::ObjectTreeParser::findType( topContent, "application/pgp-signature", true, false ) ) {
          dataNode = MessageViewer::ObjectTreeParser::findTypeNot( topContent, "application", "pgp-signature", true, false );
        } else if ( MessageViewer::ObjectTreeParser::findType( topContent, "application/pkcs7-mime" , true, false ) ) {
          dataNode = MessageViewer::ObjectTreeParser::findTypeNot( topContent, "application", "pkcs7-mime", true, false );
        } else {
          dataNode = MessageViewer::ObjectTreeParser::findTypeNot( topContent, "multipart", "", true, false );
        }
      } else {
        MessageViewer::EmptySource emptySource;
        MessageViewer::ObjectTreeParser otp( &emptySource, 0, 0,false, false, false );

        // process this node and all it's siblings and descendants
        mNodeHelper->setNodeUnprocessed( dataNode, true );
        otp.parseObjectTree( Akonadi::Item(), dataNode );

        rawReplyString = otp.rawReplyString();
        gotRawReplyString = true;
      }
    }
    QByteArray cstr = gotRawReplyString
      ? rawReplyString
      : dataNode->decodedContent();
    data = KMime::CRLFtoLF( cstr );
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
    }
  else
    file.close();
  mNodeHelper->removeTempFiles();
  delete mNodeHelper;
  return OK;
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
  fetchScope().fetchFullPayload( true );
  fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
}

KMCommand::Result KMResendMessageCommand::execute()
{

  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;

  KMime::Message::Ptr newMsg = KMail::MessageHelper::createResend( item, msg );
  newMsg->contentType()->setCharset( NodeHelper::charset( msg.get() ) );

  KMail::Composer * win = KMail::makeComposer();
  win->setMsg( newMsg, false, true );
  win->show();

  return OK;
}

KMMailingListCommand::KMMailingListCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
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

KMMailingListPostCommand::KMMailingListPostCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListPostCommand::urls() const
{
  return mFolder->mailingList().postURLS();
}

KMMailingListSubscribeCommand::KMMailingListSubscribeCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListSubscribeCommand::urls() const
{
  return mFolder->mailingList().subscribeURLS();
}

KMMailingListUnsubscribeCommand::KMMailingListUnsubscribeCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListUnsubscribeCommand::urls() const
{
  return mFolder->mailingList().unsubscribeURLS();
}

KMMailingListArchivesCommand::KMMailingListArchivesCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListArchivesCommand::urls() const
{
  return mFolder->mailingList().archiveURLS();
}

KMMailingListHelpCommand::KMMailingListHelpCommand( QWidget *parent, const QSharedPointer<FolderCollection> &folder )
  : KMMailingListCommand( parent, folder )
{
}
KUrl::List KMMailingListHelpCommand::urls() const
{
  return mFolder->mailingList().helpURLS();
}


CreateTodoCommand::CreateTodoCommand(QWidget * parent, const Akonadi::Item &msg)
  : KMCommand( parent, msg )
{
}

KMCommand::Result CreateTodoCommand::execute()
{
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;

  KMail::KorgHelper::ensureRunning();
  QString txt = i18n("From: %1\nTo: %2\nSubject: %3", msg->from()->asUnicodeString(),
                     msg->to()->asUnicodeString(), msg->subject()->asUnicodeString() );
  KTemporaryFile tf;
  tf.setAutoRemove( true );
  if ( !tf.open() ) {
    kWarning() << "CreateTodoCommand: Unable to open temp file.";
    return Failed;
  }
  QString uri = "kmail:" + QString::number( item.id() ) + '/' + KMail::MessageHelper::msgId(msg);
  tf.write( msg->encodedContent() );
  OrgKdeKorganizerCalendarInterface *iface =
      new OrgKdeKorganizerCalendarInterface( "org.kde.korganizer", "/Calendar",
                                             QDBusConnection::sessionBus(), this );
  iface->openTodoEditor( i18n("Mail: %1", msg->subject()->asUnicodeString() ), txt, uri,
                         tf.fileName(), QStringList(), "message/rfc822", true );
  delete iface;
  tf.close();
  return OK;
}

#include "kmcommands.moc"
