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

#include "mailinglist-magic.h"
#include "messageviewer/nodehelper.h"
#include "composer.h"
#include "kmfiltermgr.h"
#include "kmmainwidget.h"
#include "undostack.h"
#include "messageviewer/kcursorsaver.h"
#include "messageviewer/objecttreeparser.h"
#include "messageviewer/csshelper.h"
//using KMail::FolderJob;
#include "messageviewer/mailsourceviewer.h"
#include "kmreadermainwin.h"
#include "secondarywindow.h"
using KMail::SecondaryWindow;
#include "redirectdialog.h"
using KMail::RedirectDialog;
#include "util.h"
#include "messageviewer/editorwatcher.h"
#include "korghelper.h"
#include "broadcaststatus.h"
#include "globalsettings.h"
#include "stringutil.h"
#include "messageviewer/autoqpointer.h"
#include "messageviewer/globalsettings.h"
#include "messagecore/globalsettings.h"
#include "templateparser/templateparser.h"

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

#include "messagecomposer/messagesender.h"
#include "messagecomposer/messagehelper.h"
#include "messagecomposer/messagecomposersettings.h"
#include "messagecomposer/messagefactory.h"

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
static KMail::Composer::TemplateContext replyContext( MessageFactory::MessageReply reply )
{
  if ( reply.replyAll )
    return KMail::Composer::ReplyToAll;
  else
    return KMail::Composer::Reply;
}

/// Helper to sanely show an error message for a job
static void showJobError( KJob* job )
{
    assert(job);
    // we can be called from the KJob::kill, where we are no longer a KIO::Job
    // so better safe than sorry
    KIO::Job* kiojob = dynamic_cast<KIO::Job*>(job);
    if( kiojob && kiojob->ui() )
      kiojob->ui()->showErrorMessage();
    else
      kWarning() << "There is no GUI delegate set for a kjob, and it failed with error:" << job->errorString();
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
  if ( msg.isValid() || msg.hasPayload<KMime::Message::Ptr>() ) {
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

  if ( mMsgList.isEmpty() ) {
      emit messagesTransfered( Failed );
      return;
  }

  const Akonadi::Item mb = mMsgList.first();
  if ( ( mMsgList.count() == 1 ) && !mb.parentCollection().isValid() ) {
    // Special case of operating on message that isn't in a folder
    Q_ASSERT( mb.isValid() || mb.hasPayload<KMime::Message::Ptr>() );

    mRetrievedMsgs.append(mMsgList.takeFirst());
    emit messagesTransfered( OK );
    return;
  }
  QList<Akonadi::Item>::const_iterator it;
  for ( it = mMsgList.constBegin(); it != mMsgList.constEnd(); ++it ) {
    if ( !(*it).parentCollection().isValid()  ) {
      emit messagesTransfered( Failed );
      return;
    }
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
    if ( !mMsgList.isEmpty() )
      mRetrievedMsgs = mMsgList;
  }

  if ( complete ) {
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

  MessageHelper::initHeader( msg, KMKernel::self()->identityManager(),id );
  msg->contentType()->setCharset("utf-8");
  msg->to()->fromUnicodeString( KPIMUtils::decodeMailtoUrl( mUrl ), "utf-8" );

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
  Akonadi::Item item = retrievedMessage();
  if ( !item.isValid() ) {
    return Failed;
  }
  KMime::Message::Ptr msg = KMail::Util::message( item );
  if ( !msg )
    return Failed;
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplyNone );
  factory.setSelection( mSelection );
  KMime::Message::Ptr rmsg = factory.createReply().msg;
  rmsg->to()->fromUnicodeString( KPIMUtils::decodeMailtoUrl( mUrl ), "utf-8" ); //TODO Check the UTF-8

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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  KMime::Message::Ptr fmsg = factory.createForward();
  fmsg->to()->fromUnicodeString( KPIMUtils::decodeMailtoUrl( mUrl ), "utf-8" ); //TODO check the utf-8

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
    showJobError(job);
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
    showJobError(job);
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
  newMsg->parse();
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
  QString fileName = MessageCore::StringUtil::cleanFileName( subject.trimmed() );

  // avoid stripping off the last part of the subject after a "."
  // by KFileDialog, which thinks it's an extension
  if ( !fileName.endsWith( QLatin1String( ".mbox" ) ) )
    fileName += ".mbox";

  const QString filter = i18n( "*.mbox|email messages (*.mbox)\n*|all files (*)" );
  return KFileDialog::getSaveUrl( KUrl::fromPath( fileName ), filter );
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent, const Akonadi::Item& msg )
  : KMCommand( parent ),
    mMsgListIndex( 0 ),
    mOffset( 0 ),
    mTotalSize( msg.size() )
{
  if ( !msg.isValid() ) {
    return;
  }
  setDeletesItself( true );
  mUrl = subjectToUrl( MessageViewer::NodeHelper::cleanSubject( msg.payload<KMime::Message::Ptr>().get() ) );
  fetchScope().fetchFullPayload( true ); // ### unless we call the corresponding KMCommand ctor, this has no effect
}

KMSaveMsgCommand::KMSaveMsgCommand( QWidget *parent,
                                    const QList<Akonadi::Item> &msgList )
  : KMCommand( parent, msgList ),
    mMsgListIndex( 0 ),
    mOffset( 0 ),
    mTotalSize( 0 )
{
  if ( msgList.empty() ) {
    return;
  }
  setDeletesItself( true );
  mMsgListIndex = 0;
  Akonadi::Item msgBase = msgList.at(0);
  mUrl = subjectToUrl( MessageViewer::NodeHelper::cleanSubject( msgBase.payload<KMime::Message::Ptr>().get() ) );
  kDebug() << mUrl;
  fetchScope().fetchFullPayload( true ); // ### unless we call the corresponding KMCommand ctor, this has no effect
}

KUrl KMSaveMsgCommand::url()
{
  return mUrl;
}

KMCommand::Result KMSaveMsgCommand::execute()
{ 
  mJob = KIO::put( mUrl, MessageViewer::Util::getWritePermissions() );
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
  if ( mMsgListIndex < static_cast<unsigned int>( mRetrievedMsgs.size() ) ) {
    slotMessageRetrievedForSaving( mRetrievedMsgs[mMsgListIndex] );
  } else {
    // No more messages. Tell the putjob we are done.
    QByteArray data = QByteArray();
    mJob->sendAsyncData( data );
  }
}

#define STRDIM(x) (sizeof(x)/sizeof(*x)-1)
//TODO: copied from runtime/resources/mbox/libmbox/mbox_p.cpp . Check if we can share it.
QByteArray escapeFrom( const QByteArray &str )
{
  const unsigned int strLen = str.length();
  if ( strLen <= STRDIM( "From " ) )
    return str;

  // worst case: \nFrom_\nFrom_\nFrom_... => grows to 7/6
  QByteArray result( int( strLen + 5 ) / 6 * 7 + 1, '\0');

  const char * s = str.data();
  const char * const e = s + strLen - STRDIM( "From ");
  char * d = result.data();

  bool onlyAnglesAfterLF = false; // dont' match ^From_
  while ( s < e ) {
    switch ( *s ) {
    case '\n':
      onlyAnglesAfterLF = true;
      break;
    case '>':
      break;
    case 'F':
      if ( onlyAnglesAfterLF && qstrncmp( s+1, "rom ", STRDIM("rom ") ) == 0 )
        *d++ = '>';
      // fall through
    default:
      onlyAnglesAfterLF = false;
      break;
    }
    *d++ = *s++;
  }
  while ( s < str.data() + strLen )
    *d++ = *s++;

  result.truncate( d - result.data() );
  return result;
}
#undef STRDIM

QByteArray mboxMessageSeparator( const QByteArray &msg )
{
  KMime::Message mail;
  mail.setHead( KMime::CRLFtoLF( msg ) );
  mail.parse();

  QByteArray separator = "From ";

  KMime::Headers::From *from = mail.from( false );
  if ( !from || from->addresses().isEmpty() )
    separator += "unknown@unknown.invalid";
  else
    separator += from->addresses().first() + ' ';

  KMime::Headers::Date *date = mail.date(false);
  if (!date || date->isEmpty())
    separator += QDateTime::currentDateTime().toString( Qt::TextDate ).toUtf8() + '\n';
  else
    separator += date->as7BitString(false) + '\n';

  return separator;
}

void KMSaveMsgCommand::slotMessageRetrievedForSaving(const Akonadi::Item &msg)
{
  //if ( msg )
  {
    QByteArray msgData = msg.payloadData();
    QByteArray str( mboxMessageSeparator( msgData ) );
    str += escapeFrom( msgData );
    str += '\n';
    mData = str;
    mData.resize( mData.size() - 1 );
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

        mJob = KIO::put( mUrl, MessageViewer::Util::getWritePermissions(), KIO::Overwrite );
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
      showJobError(job);
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
    showJobError(job);
    setResult( Failed );
    emit completed( this );
  }
  else {
    int startOfMessage = 0;
    if ( mMsgString.startsWith( QLatin1String( "From " ) ) ) {
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setReplyStrategy( MessageComposer::ReplySmart );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setSelection( mSelection );
  MessageFactory::MessageReply reply = factory.createReply();
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplySmart );
  MessageFactory::MessageReply reply = factory.createReply();
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplyList );
  factory.setSelection( mSelection );
  MessageFactory::MessageReply reply = factory.createReply();
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplyAll );
  factory.setSelection( mSelection );
  MessageFactory::MessageReply reply = factory.createReply();
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplyAuthor );
  factory.setSelection( mSelection );
  MessageFactory::MessageReply reply = factory.createReply();
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
        MessageFactory factory( KMime::Message::Ptr( new KMime::Message ), mIdentity );
        factory.setIdentityManager( KMKernel::self()->identityManager() );
        factory.setFolderIdentity( KMail::Util::folderIdentity( msgList.first() ) );
        // get a list of messages
        QList< KMime::Message::Ptr > msgs;
        foreach( const Akonadi::Item& item, msgList )
          msgs << KMail::Util::message( item );
        QPair< KMime::Message::Ptr, KMime::Content* > fwdMsg = factory.createForwardDigestMIME( msgs );
        
        {
          KMail::Composer * win = KMail::makeComposer( fwdMsg.first, KMail::Composer::Forward, mIdentity );
          win->addAttach( fwdMsg.second );
          win->show();
        }
      return OK;
    } else if ( answer == KMessageBox::No ) {// NO MIME DIGEST, Multiple forward
      QList<Akonadi::Item>::const_iterator it;
      for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
        if ( !it->isValid() )
          return Failed;

        KMime::Message::Ptr msg = KMail::Util::message( *it );
        
        if ( !msg )
          return Failed;
        MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
        MessageFactory factory( msg, it->id() );
        factory.setIdentityManager( KMKernel::self()->identityManager() );
        factory.setFolderIdentity( KMail::Util::folderIdentity( *it ) );
        KMime::Message::Ptr fwdMsg = factory.createForward();

        uint id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
        kDebug() << "mail" << msg->encodedContent();
        if ( id == 0 )
          id = mIdentity;
        {
          KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id );
          win->show();
        }

      }
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  KMime::Message::Ptr fwdMsg = factory.createForward();

  uint id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
  if ( id == 0 )
    id = mIdentity;
  {
    KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id );
    win->show();
  }
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
  MessageFactory factory( KMime::Message::Ptr( new KMime::Message ), mIdentity );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( msgList.first() ) );
  // get a list of messages
  QList< KMime::Message::Ptr > msgs;
  foreach( const Akonadi::Item& item, msgList )
    msgs << KMail::Util::message( item );
  QPair< KMime::Message::Ptr, QList< KMime::Content* > > fwdMsg = factory.createAttachedForward( msgs );
  {
    if ( !mWin ) {
      mWin = KMail::makeComposer( fwdMsg.first, KMail::Composer::Forward, mIdentity );
    }
    foreach( KMime::Content* attach, fwdMsg.second )
      mWin->addAttach( attach );
    mWin->show();
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
      new RedirectDialog( parentWidget(), MessageComposer::MessageComposerSettings::self()->sendImmediate() ) );
  dlg->setObjectName( "redirect" );
  if ( dlg->exec() == QDialog::Rejected || !dlg ) {
    return Failed;
  }

  MessageFactory factory( item.payload<KMime::Message::Ptr>(),  item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  KMime::Message::Ptr newMsg = factory.createRedirect( dlg->to() );
  if ( !newMsg )
    return Failed;

  KMFilterAction::sendMDN( KMail::Util::message( item ), KMime::MDN::Dispatched );

  const MessageSender::SendMethod method = dlg->sendImmediate()
    ? MessageSender::SendImmediate
    : MessageSender::SendLater;
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplySmart );
  factory.setSelection( mSelection );
  factory.setTemplate( mTemplate );
  MessageFactory::MessageReply reply = factory.createReply();
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
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setMailingListAddresses( KMail::Util::mailingListsFromMessage( item ) );
  factory.putRepliesInSameFolder( KMail::Util::putRepliesInSameFolder( item ) );
  factory.setReplyStrategy( MessageComposer::ReplyAll );
  factory.setSelection( mSelection );
  factory.setTemplate( mTemplate );
  MessageFactory::MessageReply reply = factory.createReply();
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
     QList<Akonadi::Item>::const_iterator it;
      for ( it = msgList.constBegin(); it != msgList.constEnd(); ++it ) {
        if ( !it->isValid() )
          return Failed;

        KMime::Message::Ptr msg = KMail::Util::message( *it );

        if ( !msg )
          return Failed;
        MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
        MessageFactory factory( msg, it->id() );
        factory.setIdentityManager( KMKernel::self()->identityManager() );
        factory.setFolderIdentity( KMail::Util::folderIdentity( *it ) );
        factory.setTemplate( mTemplate );
        KMime::Message::Ptr fwdMsg = factory.createForward();

        uint id = msg->headerByType( "X-KMail-Identity" ) ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
        if ( id == 0 )
          id = mIdentity;
        {
          KMail::Composer * win = KMail::makeComposer( fwdMsg, KMail::Composer::Forward, id );
          win->show();
        }

      }
  } else { // forward a single message at most

    Akonadi::Item item = msgList.first();
    if ( !item.isValid() ) {
      return Failed;
    }
    KMime::Message::Ptr msg = KMail::Util::message( item );
    if ( !msg )
      return Failed;
    MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
    MessageFactory factory( msg, item.id() );
    factory.setIdentityManager( KMKernel::self()->identityManager() );
    factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
    factory.setTemplate( mTemplate );
    KMime::Message::Ptr fwdMsg = factory.createForward();

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
                                MessageViewer::HeaderStyle *headerStyle,
                                const MessageViewer::HeaderStrategy *headerStrategy,
                                bool htmlOverride, bool htmlLoadExtOverride,
                                bool useFixedFont, const QString & encoding )
  : KMCommand( parent, msg ),
    mHeaderStyle( headerStyle ), mHeaderStrategy( headerStrategy ),
    mAttachmentStrategy( 0 ),
    mHtmlOverride( htmlOverride ),
    mHtmlLoadExtOverride( htmlLoadExtOverride ),
    mUseFixedFont( useFixedFont ), mEncoding( encoding )
{
  fetchScope().fetchFullPayload( true );
  if ( MessageCore::GlobalSettings::useDefaultFonts() )
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

void KMPrintCommand::setAttachmentStrategy( const MessageViewer::AttachmentStrategy *strategy )
{
  mAttachmentStrategy = strategy;
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
  if ( mAttachmentStrategy != 0 )
    printerWin->setAttachmentStrategy( mAttachmentStrategy );
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
    kDebug()<<" Error trying to set item status:" << job->errorText();
  }
  --messageStatusChanged;
  if ( messageStatusChanged == 0 ) {
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
  KMCommand *filterCommand = new KMFilterActionCommand(
      mMainWidget, mMainWidget->messageListPane()->selectionAsMessageItemList() , mFilter );
  filterCommand->start();
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
    showJobError(job);
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
    showJobError(job);
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
    MessageHelper::initHeader( msg, KMKernel::self()->identityManager(), mIdentity );
    msg->contentType()->setCharset("utf-8");

    QMap<QString, QString> fields =  MessageCore::StringUtil::parseMailtoUrl( mUrl );

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

// FIXME: This is blatant code duplication with ViewerPrivate::saveAttachments!!
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
    QString fileName = MessageViewer::NodeHelper::fileName( content );
    fileName = MessageCore::StringUtil::cleanFileName( fileName );
    if ( fileName.isEmpty() ) {
      fileName = i18nc( "filename for an unnamed attachment", "attachment.1" );
    }
    url = KFileDialog::getSaveUrl( KUrl( "kfiledialog:///saveAttachment/" + fileName ),
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
      QString fileName = MessageViewer::NodeHelper::fileName( content );
      fileName = MessageCore::StringUtil::cleanFileName( fileName );
      if ( fileName.isEmpty() ) {
        ++unnamedAtmCount;
        fileName = i18nc( "filename for the %1-th unnamed attachment",
                          "attachment.%1", unnamedAtmCount );
      }
      curUrl.setFileName( fileName );
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

KMCommand::Result KMSaveAttachmentsCommand::saveItem( KMime::Content *content,
                                                      const KUrl& url )
{
  KMime::Content *topContent  = content->topLevel();
  MessageViewer::NodeHelper *mNodeHelper = new MessageViewer::NodeHelper;
  bool bSaveEncrypted = false;
  bool bEncryptedParts = mNodeHelper->encryptionState( content ) != MessageViewer::KMMsgNotEncrypted;
  if( bEncryptedParts )
    if( KMessageBox::questionYesNo( parentWidget(),
                                    i18n( "The part %1 of the message is encrypted. Do you want to keep the encryption when saving?",
                                          url.fileName() ),
                                    i18n( "KMail Question" ), KGuiItem(i18n("Keep Encryption")), KGuiItem(i18n("Do Not Keep")) ) ==
        KMessageBox::Yes )
      bSaveEncrypted = true;

  bool bSaveWithSig = true;
  if(mNodeHelper->signatureState( content ) != MessageViewer::KMMsgNotSigned )
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
        otp.parseObjectTree( dataNode );

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

      const int permissions = MessageViewer::Util::getWritePermissions();
      if ( permissions >= 0 )
        fchmod( file.handle(), permissions );

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

  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  KMime::Message::Ptr newMsg = factory.createResend();
  newMsg->contentType()->setCharset( MessageViewer::NodeHelper::charset( msg.get() ) );

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
  QString uri = "kmail:" + QString::number( item.id() ) + '/' + MessageHelper::msgId(msg);
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
