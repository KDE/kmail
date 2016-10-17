/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>
    Copyright (C) 2013-2016 Laurent Montel <montel@kde.org>

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

#include "widgets/collectionpane.h"
#include "kmreadermainwin.h"
#include "secondarywindow.h"
#include "util.h"
#include "settings/kmailsettings.h"
#include "kmail_debug.h"

#include "editor/composer.h"
#include "kmmainwidget.h"
#include "undostack.h"

#include <KIdentityManagement/IdentityManager>

#include <KMime/Message>
#include <KMime/MDN>

#include <AkonadiCore/ItemModifyJob>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemMoveJob>
#include <AkonadiCore/ItemCopyJob>
#include <AkonadiCore/ItemDeleteJob>
#include <AkonadiCore/Tag>
#include <AkonadiCore/TagCreateJob>

#include <MailCommon/FolderCollection>
#include <MailCommon/FilterAction>
#include <MailCommon/FilterManager>
#include <MailCommon/MailFilter>
#include <MailCommon/RedirectDialog>
#include <MailCommon/MDNStateAttribute>
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <MessageCore/StringUtil>
#include <MessageCore/MessageCoreSettings>
#include <MessageCore/StringUtil>
#include <MessageCore/MessageHelpers>
#include <MessageCore/MailingList>

#include <MessageComposer/MessageSender>
#include <MessageComposer/MessageHelper>
#include <MessageComposer/MessageComposerSettings>
#include <MessageComposer/MessageFactory>
#include <MessageComposer/Util>

#include <MessageList/Pane>

#include <MessageViewer/CSSHelper>
#include <MessageViewer/HeaderStylePlugin>
#include <MessageViewer/MessageViewerSettings>
#include <MessageViewer/MessageViewerUtil>
#include <MessageViewer/ObjectTreeEmptySource>

#include <MimeTreeParser/NodeHelper>
#include <MimeTreeParser/ObjectTreeParser>

#include <MailTransport/TransportAttribute>
#include <MailTransport/SentBehaviourAttribute>
#include <MailTransport/TransportManager>

#include <Libkdepim/ProgressManager>
#include <Libkdepim/BroadcastStatus>
#ifndef QT_NO_CURSOR
#include <Libkdepim/KCursorSaver>
#endif

#include <gpgme++/error.h>

#include <algorithm>
#include <memory>
#include <unistd.h> // link()
#include <KBookmarkManager>

#include <KDBusServiceStarter>
#include <KEmailAddress>
#include <KFileWidget>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentDirs>

// KIO headers
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/StatJob>

#include <QApplication>
#include <QByteArray>
#include <QFileDialog>
#include <QFontDatabase>
#include <QList>
#include <QProgressDialog>
#include <QStandardPaths>

using KMail::SecondaryWindow;
using MailTransport::TransportManager;
using MessageComposer::MessageFactory;

using KPIM::ProgressManager;
using KPIM::ProgressItem;
using namespace KMime;

using namespace MailCommon;

/// Small helper function to get the composer context from a reply
static KMail::Composer::TemplateContext replyContext(MessageFactory::MessageReply reply)
{
    if (reply.replyAll) {
        return KMail::Composer::ReplyToAll;
    } else {
        return KMail::Composer::Reply;
    }
}

/// Helper to sanely show an error message for a job
static void showJobError(KJob *job)
{
    assert(job);
    // we can be called from the KJob::kill, where we are no longer a KIO::Job
    // so better safe than sorry
    KIO::Job *kiojob = qobject_cast<KIO::Job *>(job);
    if (kiojob && kiojob->ui()) {
        kiojob->ui()->showErrorMessage();
    } else {
        qCWarning(KMAIL_LOG) << "There is no GUI delegate set for a kjob, and it failed with error:" << job->errorString();
    }
}

KMCommand::KMCommand(QWidget *parent)
    : mCountMsgs(0), mResult(Undefined), mDeletesItself(false),
      mEmitsCompletedItself(false), mParent(parent)
{
}

KMCommand::KMCommand(QWidget *parent, const Akonadi::Item &msg)
    : mCountMsgs(0), mResult(Undefined), mDeletesItself(false),
      mEmitsCompletedItself(false), mParent(parent)
{
    if (msg.isValid() || msg.hasPayload<KMime::Message::Ptr>()) {
        mMsgList.append(msg);
    }
}

KMCommand::KMCommand(QWidget *parent, const Akonadi::Item::List &msgList)
    : mCountMsgs(0), mResult(Undefined), mDeletesItself(false),
      mEmitsCompletedItself(false), mParent(parent)
{
    mMsgList = msgList;
}

KMCommand::~KMCommand()
{
}

KMCommand::Result KMCommand::result() const
{
    if (mResult == Undefined) {
        qCDebug(KMAIL_LOG) << "mResult is Undefined";
    }
    return mResult;
}

const Akonadi::Item::List KMCommand::retrievedMsgs() const
{
    return mRetrievedMsgs;
}

Akonadi::Item KMCommand::retrievedMessage() const
{
    if (mRetrievedMsgs.isEmpty()) {
        return Akonadi::Item();
    }
    return *(mRetrievedMsgs.begin());
}

QWidget *KMCommand::parentWidget() const
{
    return mParent;
}

bool KMCommand::deletesItself() const
{
    return mDeletesItself;
}

void KMCommand::setDeletesItself(bool deletesItself)
{
    mDeletesItself = deletesItself;
}

bool KMCommand::emitsCompletedItself() const
{
    return mEmitsCompletedItself;
}

void KMCommand::setEmitsCompletedItself(bool emitsCompletedItself)
{
    mEmitsCompletedItself = emitsCompletedItself;
}

void KMCommand::setResult(KMCommand::Result result)
{
    mResult = result;
}

int KMCommand::mCountJobs = 0;

void KMCommand::start()
{
    connect(this, &KMCommand::messagesTransfered,
            this, &KMCommand::slotPostTransfer);

    if (mMsgList.isEmpty()) {
        Q_EMIT messagesTransfered(OK);
        return;
    }

    // Special case of operating on message that isn't in a folder
    const Akonadi::Item mb = mMsgList.first();
    if ((mMsgList.count() == 1) && MessageCore::Util::isStandaloneMessage(mb)) {
        mRetrievedMsgs.append(mMsgList.takeFirst());
        Q_EMIT messagesTransfered(OK);
        return;
    }

    // we can only retrieve items with a valid id
    foreach (const Akonadi::Item &item, mMsgList) {
        if (!item.isValid()) {
            Q_EMIT messagesTransfered(Failed);
            return;
        }
    }

    // transfer the selected messages first
    transferSelectedMsgs();
}

void KMCommand::slotPostTransfer(KMCommand::Result result)
{
    disconnect(this, &KMCommand::messagesTransfered,
               this, &KMCommand::slotPostTransfer);
    if (result == OK) {
        result = execute();
    }
    mResult = result;
    if (!emitsCompletedItself()) {
        Q_EMIT completed(this);
    }
    if (!deletesItself()) {
        deleteLater();
    }
}

Akonadi::ItemFetchJob *KMCommand::createFetchJob(const Akonadi::Item::List &items)
{
    return new Akonadi::ItemFetchJob(items, this);
}

void KMCommand::transferSelectedMsgs()
{
    // make sure no other transfer is active
    if (KMCommand::mCountJobs > 0) {
        Q_EMIT messagesTransfered(Failed);
        return;
    }

    bool complete = true;
    KMCommand::mCountJobs = 0;
    mCountMsgs = 0;
    mRetrievedMsgs.clear();
    mCountMsgs = mMsgList.count();
    uint totalSize = 0;
    // the QProgressDialog for the user-feedback. Only enable it if it's needed.
    // For some commands like KMSetStatusCommand it's not needed. Note, that
    // for some reason the QProgressDialog eats the MouseReleaseEvent (if a
    // command is executed after the MousePressEvent), cf. bug #71761.
    if (mCountMsgs > 0) {
        mProgressDialog = new QProgressDialog(mParent);
        mProgressDialog.data()->setWindowTitle(i18n("Please wait"));

        mProgressDialog.data()->setLabelText(i18np("Please wait while the message is transferred", "Please wait while the %1 messages are transferred", mMsgList.count()));
        mProgressDialog.data()->setModal(true);
        mProgressDialog.data()->setMinimumDuration(1000);
    }

    // TODO once the message list is based on ETM and we get the more advanced caching we need to make that check a bit more clever
    if (!mFetchScope.isEmpty()) {
        complete = false;
        ++KMCommand::mCountJobs;
        Akonadi::ItemFetchJob *fetch = createFetchJob(mMsgList);
        mFetchScope.fetchAttribute< MailCommon::MDNStateAttribute >();
        fetch->setFetchScope(mFetchScope);
        connect(fetch, &Akonadi::ItemFetchJob::itemsReceived, this, &KMCommand::slotMsgTransfered);
        connect(fetch, &Akonadi::ItemFetchJob::result, this, &KMCommand::slotJobFinished);
    } else {
        // no need to fetch anything
        if (!mMsgList.isEmpty()) {
            mRetrievedMsgs = mMsgList;
        }
    }

    if (complete) {
        delete mProgressDialog.data();
        mProgressDialog.clear();
        Q_EMIT messagesTransfered(OK);
    } else {
        // wait for the transfer and tell the progressBar the necessary steps
        if (mProgressDialog.data()) {
            connect(mProgressDialog.data(), &QProgressDialog::canceled,
                    this, &KMCommand::slotTransferCancelled);
            mProgressDialog.data()->setMaximum(totalSize);
        }
    }
}

void KMCommand::slotMsgTransfered(const Akonadi::Item::List &msgs)
{
    if (mProgressDialog.data() && mProgressDialog.data()->wasCanceled()) {
        Q_EMIT messagesTransfered(Canceled);
        return;
    }
    // save the complete messages
    mRetrievedMsgs.append(msgs);
}

void KMCommand::slotJobFinished()
{
    // the job is finished (with / without error)
    KMCommand::mCountJobs--;

    if (mProgressDialog.data() && mProgressDialog.data()->wasCanceled()) {
        return;
    }

    if (mCountMsgs > mRetrievedMsgs.count()) {
        // the message wasn't retrieved before => error
        if (mProgressDialog.data()) {
            mProgressDialog.data()->hide();
        }
        slotTransferCancelled();
        return;
    }
    // update the progressbar
    if (mProgressDialog.data()) {
        mProgressDialog.data()->setLabelText(i18np("Please wait while the message is transferred",
                                             "Please wait while the %1 messages are transferred", KMCommand::mCountJobs));
    }
    if (KMCommand::mCountJobs == 0) {
        // all done
        delete mProgressDialog.data();
        mProgressDialog.clear();
        Q_EMIT messagesTransfered(OK);
    }
}

void KMCommand::slotTransferCancelled()
{
    KMCommand::mCountJobs = 0;
    mCountMsgs = 0;
    mRetrievedMsgs.clear();
    Q_EMIT messagesTransfered(Canceled);
}

KMMailtoComposeCommand::KMMailtoComposeCommand(const QUrl &url,
        const Akonadi::Item &msg)
    : mUrl(url), mMessage(msg)
{
}

KMCommand::Result KMMailtoComposeCommand::execute()
{
    KMime::Message::Ptr msg(new KMime::Message);
    uint id = 0;

    if (mMessage.isValid() && mMessage.parentCollection().isValid()) {
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(mMessage.parentCollection(), false);
        id = fd->identity();
    }

    MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), id);
    msg->contentType()->setCharset("utf-8");
    msg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mUrl), "utf-8");

    KMail::Composer *win = KMail::makeComposer(msg, false, false, KMail::Composer::New, id);
    win->setFocusToSubject();
    win->show();
    return OK;
}

KMMailtoReplyCommand::KMMailtoReplyCommand(QWidget *parent,
        const QUrl &url, const Akonadi::Item &msg, const QString &selection)
    : KMCommand(parent, msg), mUrl(url), mSelection(selection)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMMailtoReplyCommand::execute()
{
    Akonadi::Item item = retrievedMessage();
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }
    MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));
    factory.setMailingListAddresses(KMail::Util::mailingListsFromMessage(item));
    factory.putRepliesInSameFolder(KMail::Util::putRepliesInSameFolder(item));
    factory.setReplyStrategy(MessageComposer::ReplyNone);
    factory.setSelection(mSelection);
    KMime::Message::Ptr rmsg = factory.createReply().msg;
    rmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mUrl), "utf-8");
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);

    KMail::Composer *win = KMail::makeComposer(rmsg, lastSign, lastEncrypt, KMail::Composer::Reply, 0, mSelection);
    win->setFocusToEditor();
    win->show();

    return OK;
}

KMMailtoForwardCommand::KMMailtoForwardCommand(QWidget *parent,
        const QUrl &url, const Akonadi::Item &msg)
    : KMCommand(parent, msg), mUrl(url)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMMailtoForwardCommand::execute()
{
    //TODO : consider factoring createForward into this method.
    Akonadi::Item item = retrievedMessage();
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }
    MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));
    KMime::Message::Ptr fmsg = factory.createForward();
    fmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mUrl).toLower(), "utf-8");
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);

    KMail::Composer *win = KMail::makeComposer(fmsg, lastSign, lastEncrypt, KMail::Composer::Forward);
    win->show();

    return OK;
}

KMAddBookmarksCommand::KMAddBookmarksCommand(const QUrl &url, QWidget *parent)
    : KMCommand(parent), mUrl(url)
{
}

KMCommand::Result KMAddBookmarksCommand::execute()
{
    const QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");
    QFileInfo fileInfo(filename);
    QDir().mkpath(fileInfo.absolutePath());
    KBookmarkManager *bookManager = KBookmarkManager::managerForFile(filename, QStringLiteral("konqueror"));
    KBookmarkGroup group = bookManager->root();
    group.addBookmark(mUrl.path(), QUrl(mUrl), QString());
    if (bookManager->save()) {
        bookManager->emitChanged(group);
    }

    return OK;
}

KMUrlSaveCommand::KMUrlSaveCommand(const QUrl &url, QWidget *parent)
    : KMCommand(parent), mUrl(url)
{
}

KMCommand::Result KMUrlSaveCommand::execute()
{
    if (mUrl.isEmpty()) {
        return OK;
    }
    QString recentDirClass;
    QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///OpenMessage")), recentDirClass);
    startUrl.setPath(startUrl.path() + QLatin1Char('/') + mUrl.fileName());
    const QUrl saveUrl = QFileDialog::getSaveFileUrl(parentWidget(), i18n("Save To File"), startUrl);
    if (saveUrl.isEmpty()) {
        return Canceled;
    }

    if (!recentDirClass.isEmpty()) {
        KRecentDirs::add(recentDirClass, saveUrl.path());
    }

    bool fileExists = false;
    if (saveUrl.isLocalFile()) {
        fileExists = QFile::exists(saveUrl.toLocalFile());
    } else {
        auto job = KIO::stat(saveUrl, KIO::StatJob::DestinationSide, 0);
        KJobWidgets::setWindow(job, parentWidget());
        fileExists = job->exec();
    }

    if (fileExists) {
        if (KMessageBox::warningContinueCancel(Q_NULLPTR,
                                               xi18nc("@info", "File <filename>%1</filename> exists.<nl/>Do you want to replace it?",
                                                       saveUrl.toDisplayString()), i18n("Save to File"), KGuiItem(i18n("&Replace")))
                != KMessageBox::Continue) {
            return Canceled;
        }
    }
    KIO::Job *job = KIO::file_copy(mUrl, saveUrl, -1, KIO::Overwrite);
    connect(job, &KIO::Job::result, this, &KMUrlSaveCommand::slotUrlSaveResult);
    setEmitsCompletedItself(true);
    return OK;
}

void KMUrlSaveCommand::slotUrlSaveResult(KJob *job)
{
    if (job->error()) {
        showJobError(job);
        setResult(Failed);
    } else {
        setResult(OK);
    }
    Q_EMIT completed(this);
}

KMEditMessageCommand::KMEditMessageCommand(QWidget *parent, const KMime::Message::Ptr &msg)
    : KMCommand(parent), mMessage(msg)
{
}

KMCommand::Result KMEditMessageCommand::execute()
{
    if (!mMessage) {
        return Failed;
    }

    KMail::Composer *win = KMail::makeComposer();
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mMessage);
    win->setMessage(mMessage, lastSign, lastEncrypt, false, true);
    win->show();
    win->setModified(true);
    return OK;
}

KMEditItemCommand::KMEditItemCommand(QWidget *parent, const Akonadi::Item &msg, bool deleteFromSource)
    : KMCommand(parent, msg)
    , mDeleteFromSource(deleteFromSource)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().fetchAttribute<MailTransport::TransportAttribute>();
    fetchScope().fetchAttribute<MailTransport::SentBehaviourAttribute>();
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMEditItemCommand::~KMEditItemCommand()
{
}

KMCommand::Result KMEditItemCommand::execute()
{
    Akonadi::Item item = retrievedMessage();
    if (!item.isValid() || !item.parentCollection().isValid()) {
        return Failed;
    }
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }

    if (mDeleteFromSource) {
        setDeletesItself(true);
        Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(item);
        connect(job, &KIO::Job::result, this, &KMEditItemCommand::slotDeleteItem);
    }
    KMail::Composer *win = KMail::makeComposer();
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);
    win->setMessage(msg, lastSign, lastEncrypt, false, true);

    win->setFolder(item.parentCollection());

    const MailTransport::TransportAttribute *transportAttribute = item.attribute<MailTransport::TransportAttribute>();
    if (transportAttribute) {
        win->setCurrentTransport(transportAttribute->transportId());
    } else {
        int transportId = msg->headerByType("X-KMail-Transport") ? msg->headerByType("X-KMail-Transport")->asUnicodeString().toInt() : -1;
        if (transportId != -1) {
            win->setCurrentTransport(transportId);
        }
    }

    if (auto hdr = msg->replyTo(false)) {
        const QString replyTo = hdr->asUnicodeString();
        win->setCurrentReplyTo(replyTo);
    }

    const MailTransport::SentBehaviourAttribute *sentAttribute = item.attribute<MailTransport::SentBehaviourAttribute>();
    if (sentAttribute && (sentAttribute->sentBehaviour() == MailTransport::SentBehaviourAttribute::MoveToCollection)) {
        win->setFcc(QString::number(sentAttribute->moveToCollection().id()));
    }
    win->show();
    if (mDeleteFromSource) {
        win->setModified(true);
    }

    return OK;
}

void KMEditItemCommand::slotDeleteItem(KJob *job)
{
    if (job->error()) {
        showJobError(job);
        setResult(Failed);
    } else {
        setResult(OK);
    }
    Q_EMIT completed(this);
    deleteLater();
}

KMUseTemplateCommand::KMUseTemplateCommand(QWidget *parent, const Akonadi::Item  &msg)
    : KMCommand(parent, msg)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMUseTemplateCommand::execute()
{
    Akonadi::Item item = retrievedMessage();
    if (!item.isValid()
            || !item.parentCollection().isValid() ||
            !CommonKernel->folderIsTemplates(item.parentCollection())
       ) {
        return Failed;
    }
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }

    KMime::Message::Ptr newMsg(new KMime::Message);
    newMsg->setContent(msg->encodedContent());
    newMsg->parse();
    // these fields need to be regenerated for the new message
    newMsg->removeHeader<KMime::Headers::Date>();
    newMsg->removeHeader<KMime::Headers::MessageID>();

    KMail::Composer *win = KMail::makeComposer();

    win->setMessage(newMsg, false, false, false, true);
    win->show();
    return OK;
}

KMSaveMsgCommand::KMSaveMsgCommand(QWidget *parent, const Akonadi::Item::List &msgList)
    : KMCommand(parent, msgList)
{
    if (msgList.empty()) {
        return;
    }

    fetchScope().fetchFullPayload(true);   // ### unless we call the corresponding KMCommand ctor, this has no effect
}

KMCommand::Result KMSaveMsgCommand::execute()
{
    if (!MessageViewer::Util::saveMessageInMbox(retrievedMsgs(), parentWidget())) {
        return Failed;
    }
    return OK;
}

//-----------------------------------------------------------------------------

KMOpenMsgCommand::KMOpenMsgCommand(QWidget *parent, const QUrl &url,
                                   const QString &encoding, KMMainWidget *main)
    : KMCommand(parent),
      mUrl(url),
      mJob(Q_NULLPTR),
      mEncoding(encoding),
      mMainWidget(main)
{
    qCDebug(KMAIL_LOG) << "url :" << url;
}

KMCommand::Result KMOpenMsgCommand::execute()
{
    if (mUrl.isEmpty()) {
        mUrl = QFileDialog::getOpenFileUrl(parentWidget(), i18n("Open Message"), QUrl(),
                                           i18n("Message (*.mbox)")
                                          );
    }
    if (mUrl.isEmpty()) {
        return Canceled;
    }

    if (mMainWidget) {
        mMainWidget->addRecentFile(mUrl);
    }

    setDeletesItself(true);
    mJob = KIO::get(mUrl, KIO::NoReload, KIO::HideProgressInfo);
    connect(mJob, &KIO::TransferJob::data,
            this, &KMOpenMsgCommand::slotDataArrived);
    connect(mJob, &KJob::result,
            this, &KMOpenMsgCommand::slotResult);
    setEmitsCompletedItself(true);
    return OK;
}

void KMOpenMsgCommand::slotDataArrived(KIO::Job *, const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    mMsgString.append(QString::fromLatin1(data.data()));
}

void KMOpenMsgCommand::doesNotContainMessage()
{
    KMessageBox::sorry(parentWidget(),
                       i18n("The file does not contain a message."));
    setResult(Failed);
    Q_EMIT completed(this);
    // Emulate closing of a secondary window so that KMail exits in case it
    // was started with the --view command line option. Otherwise an
    // invisible KMail would keep running.
    SecondaryWindow *win = new SecondaryWindow();
    win->close();
    win->deleteLater();
    deleteLater();

}

void KMOpenMsgCommand::slotResult(KJob *job)
{
    if (job->error()) {
        // handle errors
        showJobError(job);
        setResult(Failed);
    } else {
        if (mMsgString.isEmpty()) {
            qCDebug(KMAIL_LOG) << " Message not found. There is a problem";
            doesNotContainMessage();
            return;
        }
        int startOfMessage = 0;
        if (mMsgString.startsWith(QStringLiteral("From "))) {
            startOfMessage = mMsgString.indexOf(QLatin1Char('\n'));
            if (startOfMessage == -1) {
                doesNotContainMessage();
                return;
            }
            startOfMessage += 1; // the message starts after the '\n'
        }
        // check for multiple messages in the file
        bool multipleMessages = true;
        int endOfMessage = mMsgString.indexOf(QLatin1String("\nFrom "));
        if (endOfMessage == -1) {
            endOfMessage = mMsgString.length();
            multipleMessages = false;
        }
        KMime::Message *msg = new KMime::Message;
        msg->setContent(KMime::CRLFtoLF(mMsgString.mid(startOfMessage, endOfMessage - startOfMessage).toUtf8()));
        msg->parse();
        if (!msg->hasContent()) {
            delete msg; msg = Q_NULLPTR;
            doesNotContainMessage();
            return;
        }
        KMReaderMainWin *win = new KMReaderMainWin();
        KMime::Message::Ptr mMsg(msg);
        win->showMessage(mEncoding, mMsg);
        win->show();
        if (multipleMessages)
            KMessageBox::information(win,
                                     i18n("The file contains multiple messages. "
                                          "Only the first message is shown."));
        setResult(OK);
    }
    Q_EMIT completed(this);
    deleteLater();
}

//-----------------------------------------------------------------------------
KMReplyCommand::KMReplyCommand(QWidget *parent, const Akonadi::Item &msg, MessageComposer::ReplyStrategy replyStrategy,
                               const QString &selection, bool noquote, const QString &templateName)
    : KMCommand(parent, msg),
      mSelection(selection),
      mTemplate(templateName),
      m_replyStrategy(replyStrategy),
      mNoQuote(noquote)

{
    if (!noquote) {
        fetchScope().fetchFullPayload(true);
    }

    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMReplyCommand::execute()
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    Akonadi::Item item = retrievedMessage();
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }

    MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));
    factory.setMailingListAddresses(KMail::Util::mailingListsFromMessage(item));
    factory.putRepliesInSameFolder(KMail::Util::putRepliesInSameFolder(item));
    factory.setReplyStrategy(m_replyStrategy);
    factory.setSelection(mSelection);
    if (!mTemplate.isEmpty()) {
        factory.setTemplate(mTemplate);
    }
    if (mNoQuote) {
        factory.setQuote(false);
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);

    MessageFactory::MessageReply reply = factory.createReply();
    KMail::Composer *win = KMail::makeComposer(KMime::Message::Ptr(reply.msg), lastSign, lastEncrypt, replyContext(reply), 0,
                           mSelection, mTemplate);
    win->setFocusToEditor();
    win->show();

    return OK;
}

KMForwardCommand::KMForwardCommand(QWidget *parent,
                                   const Akonadi::Item::List &msgList, uint identity, const QString &templateName)
    : KMCommand(parent, msgList),
      mIdentity(identity),
      mTemplate(templateName)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMForwardCommand::KMForwardCommand(QWidget *parent, const Akonadi::Item &msg,
                                   uint identity, const QString &templateName)
    : KMCommand(parent, msg),
      mIdentity(identity),
      mTemplate(templateName)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMForwardCommand::createComposer(const Akonadi::Item &item)
{
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));
    if (!mTemplate.isEmpty()) {
        factory.setTemplate(mTemplate);
    }
    KMime::Message::Ptr fwdMsg = factory.createForward();

    uint id = msg->headerByType("X-KMail-Identity") ?  msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed().toUInt() : 0;
    qCDebug(KMAIL_LOG) << "mail" << msg->encodedContent();
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);

    if (id == 0) {
        id = mIdentity;
    }
    {
        KMail::Composer *win = KMail::makeComposer(fwdMsg, lastSign, lastEncrypt, KMail::Composer::Forward, id, QString(), mTemplate);
        win->show();
    }
    return OK;
}

KMCommand::Result KMForwardCommand::execute()
{
    Akonadi::Item::List msgList = retrievedMsgs();

    if (msgList.count() >= 2) {
        // ask if they want a mime digest forward

        int answer = KMessageBox::questionYesNoCancel(
                         parentWidget(),
                         i18n("Do you want to forward the selected messages as "
                              "attachments in one message (as a MIME digest) or as "
                              "individual messages?"), QString(),
                         KGuiItem(i18n("Send As Digest")),
                         KGuiItem(i18n("Send Individually")));

        if (answer == KMessageBox::Yes) {
            Akonadi::Item firstItem(msgList.first());
            MessageFactory factory(KMime::Message::Ptr(new KMime::Message), firstItem.id(), MailCommon::Util::updatedCollection(firstItem.parentCollection()));
            factory.setIdentityManager(KMKernel::self()->identityManager());
            factory.setFolderIdentity(MailCommon::Util::folderIdentity(firstItem));

            QPair< KMime::Message::Ptr, KMime::Content * > fwdMsg = factory.createForwardDigestMIME(msgList);
            KMail::Composer *win = KMail::makeComposer(fwdMsg.first, false, false, KMail::Composer::Forward, mIdentity);
            win->addAttach(fwdMsg.second);
            win->show();
            return OK;
        } else if (answer == KMessageBox::No) {  // NO MIME DIGEST, Multiple forward
            Akonadi::Item::List::const_iterator it;
            Akonadi::Item::List::const_iterator end(msgList.constEnd());

            for (it = msgList.constBegin(); it != end; ++it) {
                if (createComposer(*it) == Failed) {
                    return Failed;
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
    if (createComposer(item) == Failed) {
        return Failed;
    }
    return OK;
}

KMForwardAttachedCommand::KMForwardAttachedCommand(QWidget *parent,
        const Akonadi::Item::List &msgList, uint identity, KMail::Composer *win)
    : KMCommand(parent, msgList), mIdentity(identity),
      mWin(QPointer<KMail::Composer>(win))
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMForwardAttachedCommand::KMForwardAttachedCommand(QWidget *parent,
        const Akonadi::Item &msg, uint identity, KMail::Composer *win)
    : KMCommand(parent, msg), mIdentity(identity),
      mWin(QPointer< KMail::Composer >(win))
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMForwardAttachedCommand::execute()
{
    Akonadi::Item::List msgList = retrievedMsgs();
    Akonadi::Item firstItem(msgList.first());
    MessageFactory factory(KMime::Message::Ptr(new KMime::Message), firstItem.id(), MailCommon::Util::updatedCollection(firstItem.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(firstItem));

    QPair< KMime::Message::Ptr, QList< KMime::Content * > > fwdMsg = factory.createAttachedForward(msgList);
    if (!mWin) {
        mWin = KMail::makeComposer(fwdMsg.first, false, false, KMail::Composer::Forward, mIdentity);
    }
    foreach (KMime::Content *attach, fwdMsg.second) {
        mWin->addAttach(attach);
    }
    mWin->show();
    return OK;
}

KMRedirectCommand::KMRedirectCommand(QWidget *parent,
                                     const Akonadi::Item::List &msgList)
    : KMCommand(parent, msgList)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().fetchAttribute<MailTransport::SentBehaviourAttribute>();
    fetchScope().fetchAttribute<MailTransport::TransportAttribute>();

    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMRedirectCommand::KMRedirectCommand(QWidget *parent,
                                     const Akonadi::Item &msg)
    : KMCommand(parent, msg)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().fetchAttribute<MailTransport::SentBehaviourAttribute>();
    fetchScope().fetchAttribute<MailTransport::TransportAttribute>();

    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMRedirectCommand::execute()
{
    const MailCommon::RedirectDialog::SendMode sendMode = MessageComposer::MessageComposerSettings::self()->sendImmediate()
            ? MailCommon::RedirectDialog::SendNow
            : MailCommon::RedirectDialog::SendLater;

    QScopedPointer<MailCommon::RedirectDialog> dlg(new MailCommon::RedirectDialog(sendMode, parentWidget()));
    dlg->setObjectName(QStringLiteral("redirect"));
    if (dlg->exec() == QDialog::Rejected || !dlg) {
        return Failed;
    }
    if (!TransportManager::self()->showTransportCreationDialog(parentWidget(), TransportManager::IfNoTransportExists)) {
        return Failed;
    }

    //TODO use sendlateragent here too.
    const MessageComposer::MessageSender::SendMethod method = (dlg->sendMode() == MailCommon::RedirectDialog::SendNow)
            ? MessageComposer::MessageSender::SendImmediate
            : MessageComposer::MessageSender::SendLater;

    const int identity = dlg->identity();
    int transportId = dlg->transportId();
    const QString to = dlg->to();
    const QString cc = dlg->cc();
    const QString bcc = dlg->bcc();
    foreach (const Akonadi::Item &item, retrievedMsgs()) {
        const KMime::Message::Ptr msg = MessageCore::Util::message(item);
        if (!msg) {
            return Failed;
        }

        MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
        factory.setIdentityManager(KMKernel::self()->identityManager());
        factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));

        if (transportId == -1) {
            const MailTransport::TransportAttribute *transportAttribute = item.attribute<MailTransport::TransportAttribute>();
            if (transportAttribute) {
                transportId = transportAttribute->transportId();
                const MailTransport::Transport *transport = MailTransport::TransportManager::self()->transportById(transportId);
                if (!transport) {
                    transportId = -1;
                }
            }
        }

        const MailTransport::SentBehaviourAttribute *sentAttribute = item.attribute<MailTransport::SentBehaviourAttribute>();
        QString fcc;
        if (sentAttribute && (sentAttribute->sentBehaviour() == MailTransport::SentBehaviourAttribute::MoveToCollection)) {
            fcc =  QString::number(sentAttribute->moveToCollection().id());
        }

        const KMime::Message::Ptr newMsg = factory.createRedirect(to, cc, bcc, transportId, fcc, identity);
        if (!newMsg) {
            return Failed;
        }

        MessageStatus status;
        status.setStatusFromFlags(item.flags());
        if (!status.isRead()) {
            FilterAction::sendMDN(item, KMime::MDN::Dispatched);
        }

        if (!kmkernel->msgSender()->send(newMsg, method)) {
            qCDebug(KMAIL_LOG) << "KMRedirectCommand: could not redirect message (sending failed)";
            return Failed; // error: couldn't send
        }
    }

    return OK;
}

KMPrintCommand::KMPrintCommand(QWidget *parent, const Akonadi::Item &msg,
                               MessageViewer::HeaderStylePlugin *plugin,
                               MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtOverride,
                               bool useFixedFont, const QString &encoding)
    : KMCommand(parent, msg),
      mHeaderStylePlugin(plugin),
      mAttachmentStrategy(Q_NULLPTR),
      mEncoding(encoding),
      mFormat(format),
      mHtmlLoadExtOverride(htmlLoadExtOverride),
      mUseFixedFont(useFixedFont),
      mPrintPreview(false)
{
    fetchScope().fetchFullPayload(true);
    if (MessageCore::MessageCoreSettings::useDefaultFonts()) {
        mOverrideFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    } else {
        mOverrideFont = MessageViewer::MessageViewerSettings::self()->printFont();
    }
}

void KMPrintCommand::setOverrideFont(const QFont &font)
{
    mOverrideFont = font;
}

void KMPrintCommand::setAttachmentStrategy(const MimeTreeParser::AttachmentStrategy *strategy)
{
    mAttachmentStrategy = strategy;
}

void KMPrintCommand::setPrintPreview(bool preview)
{
    mPrintPreview = preview;
}

KMCommand::Result KMPrintCommand::execute()
{
    // the window will be deleted after printout is performed, in KMReaderWin::slotPrintMsg()
    KMReaderWin *printerWin = new KMReaderWin(Q_NULLPTR, kmkernel->mainWin(), Q_NULLPTR);
    printerWin->setPrinting(true);
    printerWin->readConfig();
    printerWin->setPrintElementBackground(MessageViewer::MessageViewerSettings::self()->printBackgroundColorImages());
    if (mHeaderStylePlugin) {
        printerWin->viewer()->setPluginName(mHeaderStylePlugin->name());
    }
    printerWin->setDisplayFormatMessageOverwrite(mFormat);
    printerWin->setHtmlLoadExtOverride(mHtmlLoadExtOverride);
    printerWin->setUseFixedFont(mUseFixedFont);
    printerWin->setOverrideEncoding(mEncoding);
    printerWin->cssHelper()->setPrintFont(mOverrideFont);
    printerWin->setDecryptMessageOverwrite(true);
    if (mAttachmentStrategy != Q_NULLPTR) {
        printerWin->setAttachmentStrategy(mAttachmentStrategy);
    }
    if (mPrintPreview) {
        printerWin->viewer()->printPreviewMessage(retrievedMessage());
    } else {
        printerWin->viewer()->printMessage(retrievedMessage());
    }
    return OK;
}

KMSetStatusCommand::KMSetStatusCommand(const MessageStatus &status,
                                       const Akonadi::Item::List &items, bool invert)
    : KMCommand(Q_NULLPTR, items), mStatus(status), mInvertMark(invert)
{
    setDeletesItself(true);
}

KMCommand::Result KMSetStatusCommand::execute()
{
    bool parentStatus = false;
    // Toggle actions on threads toggle the whole thread
    // depending on the state of the parent.
    if (mInvertMark) {
        const Akonadi::Item first = retrievedMsgs().first();
        MessageStatus pStatus;
        pStatus.setStatusFromFlags(first.flags());
        if (pStatus & mStatus) {
            parentStatus = true;
        } else {
            parentStatus = false;
        }
    }

    Akonadi::Item::List itemsToModify;
    foreach (const Akonadi::Item &it, retrievedMsgs()) {
        if (mInvertMark) {
            //qCDebug(KMAIL_LOG)<<" item ::"<<tmpItem;
            if (it.isValid()) {
                bool myStatus;
                MessageStatus itemStatus;
                itemStatus.setStatusFromFlags(it.flags());
                if (itemStatus & mStatus) {
                    myStatus = true;
                } else {
                    myStatus = false;
                }
                if (myStatus != parentStatus) {
                    continue;
                }
            }
        }
        Akonadi::Item item(it);
        const Akonadi::Item::Flag flag = *(mStatus.statusFlags().begin());
        if (mInvertMark) {
            if (item.hasFlag(flag)) {
                item.clearFlag(flag);
                itemsToModify.push_back(item);
            } else {
                item.setFlag(flag);
                itemsToModify.push_back(item);
            }
        } else {
            if (!item.hasFlag(flag)) {
                item.setFlag(flag);
                itemsToModify.push_back(item);
            }
        }
    }

    if (itemsToModify.isEmpty()) {
        slotModifyItemDone(Q_NULLPTR);   // pretend we did something
    } else {
        Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(itemsToModify, this);
        modifyJob->disableRevisionCheck();
        modifyJob->setIgnorePayload(true);
        connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &KMSetStatusCommand::slotModifyItemDone);
    }
    return OK;
}

void KMSetStatusCommand::slotModifyItemDone(KJob *job)
{
    if (job && job->error()) {
        qCWarning(KMAIL_LOG) << " Error trying to set item status:" << job->errorText();
    }
    deleteLater();
}

KMSetTagCommand::KMSetTagCommand(const Akonadi::Tag::List &tags, const Akonadi::Item::List &item,
                                 SetTagMode mode)
    : mTags(tags)
    , mItem(item)
    , mMode(mode)
{
    setDeletesItself(true);
}

KMCommand::Result KMSetTagCommand::execute()
{
    Q_FOREACH (const Akonadi::Tag &tag, mTags) {
        if (!tag.isValid()) {
            Akonadi::TagCreateJob *createJob = new Akonadi::TagCreateJob(tag, this);
            connect(createJob, &Akonadi::TagCreateJob::result, this, &KMSetTagCommand::slotModifyItemDone);
        } else {
            mCreatedTags << tag;
        }
    }

    if (mCreatedTags.size() == mTags.size()) {
        setTags();
    } else {
        deleteLater();
    }

    return OK;
}

void KMSetTagCommand::setTags()
{
    Akonadi::Item::List itemsToModify;
    itemsToModify.reserve(mItem.count());
    Q_FOREACH (const Akonadi::Item &i, mItem) {
        Akonadi::Item item(i);
        if (mMode == CleanExistingAndAddNew) {
            //WorkAround. ClearTags doesn't work.
            Q_FOREACH (const Akonadi::Tag &tag, item.tags()) {
                item.clearTag(tag);
            }
            //item.clearTags();
        }

        if (mMode == KMSetTagCommand::Toggle) {
            Q_FOREACH (const Akonadi::Tag &tag, mCreatedTags) {
                if (item.hasTag(tag)) {
                    item.clearTag(tag);
                } else {
                    item.setTag(tag);
                }
            }
        } else {
            if (!mCreatedTags.isEmpty()) {
                item.setTags(mCreatedTags);
            }
        }
        itemsToModify << item;
    }
    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(itemsToModify, this);
    modifyJob->disableRevisionCheck();
    modifyJob->setIgnorePayload(true);
    connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &KMSetTagCommand::slotModifyItemDone);

    if (!mCreatedTags.isEmpty()) {
        KConfigGroup tag(KMKernel::self()->config(), "MessageListView");
        const QString oldTagList = tag.readEntry("TagSelected");
        QStringList lst = oldTagList.split(QLatin1Char(','));
        Q_FOREACH (const Akonadi::Tag &tag, mCreatedTags) {
            const QString url = tag.url().url();
            if (!lst.contains(url)) {
                lst.append(url);
            }
        }
        tag.writeEntry("TagSelected", lst);
        KMKernel::self()->updatePaneTagComboBox();
    }
}

void KMSetTagCommand::slotModifyItemDone(KJob *job)
{
    if (job && job->error()) {
        qCWarning(KMAIL_LOG) << " Error trying to set item status:" << job->errorText();
    }
    deleteLater();
}

KMFilterActionCommand::KMFilterActionCommand(QWidget *parent,
        const QVector<qlonglong> &msgListId,
        const QString &filterId)
    : KMCommand(parent), mMsgListId(msgListId), mFilterId(filterId)
{
}

KMCommand::Result KMFilterActionCommand::execute()
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    int msgCount = 0;
    const int msgCountToFilter = mMsgListId.count();
    ProgressItem *progressItem =
        ProgressManager::createProgressItem(
            QLatin1String("filter") + ProgressManager::getUniqueID(),
            i18n("Filtering messages"), QString(), true, KPIM::ProgressItem::Unknown);
    progressItem->setTotalItems(msgCountToFilter);

    foreach (const qlonglong &id, mMsgListId) {
        int diff = msgCountToFilter - ++msgCount;
        if (diff < 10 || !(msgCount % 10) || msgCount <= 10) {
            progressItem->updateProgress();
            const QString statusMsg = i18n("Filtering message %1 of %2",
                                           msgCount, msgCountToFilter);
            KPIM::BroadcastStatus::instance()->setStatusMsg(statusMsg);
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        }

        MailCommon::FilterManager::instance()->filter(Akonadi::Item(id), mFilterId, QString());
        progressItem->incCompletedItems();
    }

    progressItem->setComplete();
    progressItem = Q_NULLPTR;
    return OK;
}

KMMetaFilterActionCommand::KMMetaFilterActionCommand(const QString &filterId,
        KMMainWidget *main)
    : QObject(main),
      mFilterId(filterId), mMainWidget(main)
{
}

void KMMetaFilterActionCommand::start()
{
    KMCommand *filterCommand = new KMFilterActionCommand(
        mMainWidget, mMainWidget->messageListPane()->selectionAsMessageItemListId(), mFilterId);
    filterCommand->start();
}

KMMailingListFilterCommand::KMMailingListFilterCommand(QWidget *parent,
        const Akonadi::Item &msg)
    : KMCommand(parent, msg)
{
}

KMCommand::Result KMMailingListFilterCommand::execute()
{
    QByteArray name;
    QString value;
    Akonadi::Item item = retrievedMessage();
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }
    if (!MailingList::name(msg, name, value).isEmpty()) {
        FilterIf->openFilterDialog(false);
        FilterIf->createFilter(name, value);
        return OK;
    } else {
        return Failed;
    }
}

KMCopyCommand::KMCopyCommand(const Akonadi::Collection &destFolder,
                             const Akonadi::Item::List &msgList)
    : KMCommand(Q_NULLPTR, msgList), mDestFolder(destFolder)
{
}

KMCopyCommand::KMCopyCommand(const Akonadi::Collection &destFolder, const Akonadi::Item &msg)
    : KMCommand(Q_NULLPTR, msg), mDestFolder(destFolder)
{
}

KMCommand::Result KMCopyCommand::execute()
{
    setDeletesItself(true);

    Akonadi::Item::List listItem = retrievedMsgs();
    Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob(listItem, Akonadi::Collection(mDestFolder.id()), this);
    connect(job, &KIO::Job::result, this, &KMCopyCommand::slotCopyResult);

    return OK;
}

void KMCopyCommand::slotCopyResult(KJob *job)
{
    if (job->error()) {
        // handle errors
        showJobError(job);
        setResult(Failed);
    }
    Q_EMIT completed(this);
    deleteLater();
}

KMMoveCommand::KMMoveCommand(const Akonadi::Collection &destFolder,
                             const Akonadi::Item::List &msgList,
                             MessageList::Core::MessageItemSetReference ref)
    : KMCommand(Q_NULLPTR, msgList), mDestFolder(destFolder), mProgressItem(Q_NULLPTR), mRef(ref)
{
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMMoveCommand::KMMoveCommand(const Akonadi::Collection &destFolder,
                             const Akonadi::Item &msg,
                             MessageList::Core::MessageItemSetReference ref)
    : KMCommand(Q_NULLPTR, msg), mDestFolder(destFolder), mProgressItem(Q_NULLPTR), mRef(ref)
{
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

void KMMoveCommand::slotMoveResult(KJob *job)
{
    if (job->error()) {
        // handle errors
        showJobError(job);
        completeMove(Failed);
    } else {
        completeMove(OK);
    }
}

KMCommand::Result KMMoveCommand::execute()
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    setEmitsCompletedItself(true);
    setDeletesItself(true);
    Akonadi::Item::List retrievedList = retrievedMsgs();
    if (!retrievedList.isEmpty()) {
        if (mDestFolder.isValid()) {
            Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob(retrievedList, mDestFolder, this);
            connect(job, &KIO::Job::result, this, &KMMoveCommand::slotMoveResult);

            // group by source folder for undo
            std::sort(retrievedList.begin(), retrievedList.end(),
            [](const Akonadi::Item & lhs, const Akonadi::Item & rhs) {
                return lhs.storageCollectionId() < rhs.storageCollectionId();
            });
            Akonadi::Collection parent;
            int undoId = -1;
            foreach (const Akonadi::Item &item, retrievedList) {
                if (item.storageCollectionId() <= 0) {
                    continue;
                }
                if (parent.id() != item.storageCollectionId()) {
                    parent = Akonadi::Collection(item.storageCollectionId());
                    undoId = kmkernel->undoStack()->newUndoAction(parent, mDestFolder);
                }
                kmkernel->undoStack()->addMsgToAction(undoId, item);
            }
        } else {
            Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(retrievedList, this);
            connect(job, &KIO::Job::result, this, &KMMoveCommand::slotMoveResult);
        }
    } else {
        deleteLater();
        return Failed;
    }
    // TODO set SSL state according to source and destfolder connection?
    Q_ASSERT(!mProgressItem);
    mProgressItem =
        ProgressManager::createProgressItem(QLatin1String("move") + ProgressManager::getUniqueID(),
                                            mDestFolder.isValid() ? i18n("Moving messages") : i18n("Deleting messages"), QString(), true, KPIM::ProgressItem::Unknown);
    mProgressItem->setUsesBusyIndicator(true);
    connect(mProgressItem, &ProgressItem::progressItemCanceled,
            this, &KMMoveCommand::slotMoveCanceled);
    return OK;
}

void KMMoveCommand::completeMove(Result result)
{
    if (mProgressItem) {
        mProgressItem->setComplete();
        mProgressItem = Q_NULLPTR;
    }
    setResult(result);
    Q_EMIT moveDone(this);
    Q_EMIT completed(this);
    deleteLater();
}

void KMMoveCommand::slotMoveCanceled()
{
    completeMove(Canceled);
}

// srcFolder doesn't make much sense for searchFolders
KMTrashMsgCommand::KMTrashMsgCommand(const Akonadi::Collection &srcFolder,
                                     const Akonadi::Item::List &msgList, MessageList::Core::MessageItemSetReference ref)
    : KMMoveCommand(findTrashFolder(srcFolder), msgList, ref)
{
}

KMTrashMsgCommand::KMTrashMsgCommand(const Akonadi::Collection &srcFolder, const Akonadi::Item &msg, MessageList::Core::MessageItemSetReference ref)
    : KMMoveCommand(findTrashFolder(srcFolder), msg, ref)
{
}

Akonadi::Collection KMTrashMsgCommand::findTrashFolder(const Akonadi::Collection &folder)
{
    Akonadi::Collection col = CommonKernel->trashCollectionFromResource(folder);
    if (!col.isValid()) {
        col = CommonKernel->trashCollectionFolder();
    }
    if (folder != col) {
        return col;
    }
    return Akonadi::Collection();
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand(QWidget *parent, const Akonadi::Item &msg, MessageViewer::Viewer *viewer)
    : KMCommand(parent, msg),
      mViewer(viewer)
{
    fetchScope().fetchFullPayload(true);
}

KMSaveAttachmentsCommand::KMSaveAttachmentsCommand(QWidget *parent, const Akonadi::Item::List &msgs)
    : KMCommand(parent, msgs),
      mViewer(Q_NULLPTR)
{
    fetchScope().fetchFullPayload(true);
}

KMCommand::Result KMSaveAttachmentsCommand::execute()
{
    KMime::Content::List contentsToSave;
    foreach (const Akonadi::Item &item, retrievedMsgs()) {
        if (item.hasPayload<KMime::Message::Ptr>()) {
            contentsToSave += item.payload<KMime::Message::Ptr>()->attachments();
        } else {
            qCWarning(KMAIL_LOG) << "Retrieved item has no payload? Ignoring for saving the attachments";
        }
    }
    QUrl currentUrl;
    if (MessageViewer::Util::saveAttachments(contentsToSave, parentWidget(), currentUrl)) {
        if (mViewer) {
            mViewer->showOpenAttachmentFolderWidget(currentUrl);
        }
        return OK;
    }
    return Failed;
}

KMResendMessageCommand::KMResendMessageCommand(QWidget *parent,
        const Akonadi::Item &msg)
    : KMCommand(parent, msg)
{
    fetchScope().fetchFullPayload(true);
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
}

KMCommand::Result KMResendMessageCommand::execute()
{
    Akonadi::Item item = retrievedMessage();
    KMime::Message::Ptr msg = MessageCore::Util::message(item);
    if (!msg) {
        return Failed;
    }

    MessageFactory factory(msg, item.id(), MailCommon::Util::updatedCollection(item.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(item));
    KMime::Message::Ptr newMsg = factory.createResend();
    newMsg->contentType()->setCharset(MimeTreeParser::NodeHelper::charset(msg.data()));

    KMail::Composer *win = KMail::makeComposer();
    if (auto hdr = msg->replyTo(false)) {
        const QString replyTo = hdr->asUnicodeString();
        win->setCurrentReplyTo(replyTo);
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, msg);
    win->setMessage(newMsg, lastSign, lastEncrypt, false, true);

    win->show();

    return OK;
}

KMShareImageCommand::KMShareImageCommand(const QUrl &url, QWidget *parent)
    : KMCommand(parent),
      mUrl(url)
{
}

KMCommand::Result KMShareImageCommand::execute()
{
    KMime::Message::Ptr msg(new KMime::Message);
    uint id = 0;

    MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), id);
    msg->contentType()->setCharset("utf-8");

    KMail::Composer *win = KMail::makeComposer(msg, false, false, KMail::Composer::New, id);
    win->setFocusToSubject();
    win->addAttachment(mUrl, i18n("Image"));
    win->show();
    return OK;
}

KMFetchMessageCommand::KMFetchMessageCommand(QWidget *parent, const Akonadi::Item &item)
    : KMCommand(parent, item)
{
    // Workaround KMCommand::transferSelectedMsgs() expecting non-empty fetchscope
    fetchScope().fetchFullPayload(true);
}

Akonadi::ItemFetchJob *KMFetchMessageCommand::createFetchJob(const Akonadi::Item::List &items)
{
    Q_ASSERT(items.size() == 1);
    Akonadi::ItemFetchJob *fetch = MessageViewer::Viewer::createFetchJob(items.first());
    fetchScope() = fetch->fetchScope();
    return fetch;
}

KMCommand::Result KMFetchMessageCommand::execute()
{
    Akonadi::Item item = retrievedMessage();
    if (!item.isValid() || !item.hasPayload<KMime::Message::Ptr>()) {
        return Failed;
    }

    mItem = item;
    return OK;
}

Akonadi::Item KMFetchMessageCommand::item() const
{
    return mItem;
}
