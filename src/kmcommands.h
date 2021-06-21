//

#pragma once

#include "kmail_private_export.h"

#include <MailCommon/SearchPattern>
#include <MessageComposer/MessageFactoryNG>
#include <MessageList/View>
#include <MessageViewer/Viewer>

#include <Akonadi/KMime/MessageStatus>
#include <kio/job.h>
#include <kmime/kmime_message.h>

#include <AkonadiCore/collection.h>
#include <AkonadiCore/item.h>
#include <AkonadiCore/itemfetchscope.h>
#include <QList>
#include <QPointer>
#include <QUrl>
namespace Akonadi
{
class Tag;
}

namespace KPIM
{
class ProgressItem;
}

using Akonadi::MessageStatus;

class QProgressDialog;
class KMMainWidget;
class KMReaderMainWin;

template<typename T> class QSharedPointer;

namespace MessageViewer
{
class HeaderStyle;
class AttachmentStrategy;
}

namespace KIO
{
class Job;
}
namespace KMail
{
class Composer;
}
using PartNodeMessageMap = QMap<KMime::Content *, Akonadi::Item>;
/// Small helper structure which encapsulates the KMMessage created when creating a reply, and

class KMAILTESTS_TESTS_EXPORT KMCommand : public QObject
{
    Q_OBJECT

public:
    enum Result {
        Undefined,
        OK,
        Canceled,
        Failed,
    };

    // Trivial constructor, don't retrieve any messages
    explicit KMCommand(QWidget *parent = nullptr);
    KMCommand(QWidget *parent, const Akonadi::Item &);
    // Retrieve all messages in msgList when start is called.
    KMCommand(QWidget *parent, const Akonadi::Item::List &msgList);
    // Retrieve the single message msgBase when start is called.
    ~KMCommand() override;

    /** Returns the result of the command. Only call this method from the slot
      connected to completed().
    */
    Result result() const;

public Q_SLOTS:
    // Retrieve messages then calls execute
    void start();

Q_SIGNALS:

    /// @param result The status of the command.
    void messagesTransfered(KMCommand::Result result);

    /// Emitted when the command has completed.
    void completed(KMCommand *command);

protected:
    virtual Akonadi::ItemFetchJob *createFetchJob(const Akonadi::Item::List &items);

    /** Allows to configure how much data should be retrieved of the messages. */
    Akonadi::ItemFetchScope &fetchScope()
    {
        return mFetchScope;
    }

    // Returns list of messages retrieved
    const Akonadi::Item::List retrievedMsgs() const;
    // Returns the single message retrieved
    Akonadi::Item retrievedMessage() const;
    // Returns the parent widget
    QWidget *parentWidget() const;

    bool deletesItself() const;
    /** Specify whether the subclass takes care of the deletion of the object.
      By default the base class will delete the object.
      @param deletesItself true if the subclass takes care of deletion, false
                           if the base class should take care of deletion
    */
    void setDeletesItself(bool deletesItself);

    bool emitsCompletedItself() const;
    /** Specify whether the subclass takes care of emitting the completed()
      signal. By default the base class will Q_EMIT this signal.
      @param emitsCompletedItself true if the subclass emits the completed
                                  signal, false if the base class should Q_EMIT
                                  the signal
    */
    void setEmitsCompletedItself(bool emitsCompletedItself);

    /** Use this to set the result of the command.
      @param result The result of the command.
    */
    void setResult(Result result);

private:
    Q_DISABLE_COPY(KMCommand)
    // execute should be implemented by derived classes
    virtual Result execute() = 0;

    /** transfers the list of (imap)-messages
     *  this is a necessary preparation for e.g. forwarding */
    void transferSelectedMsgs();

private Q_SLOTS:
    void slotPostTransfer(KMCommand::Result result);
    /** the msg has been transferred */
    void slotMsgTransfered(const Akonadi::Item::List &msgs);
    /** the KMImapJob is finished */
    void slotJobFinished();
    /** the transfer was canceled */
    void slotTransferCancelled();

protected:
    Akonadi::Item::List mRetrievedMsgs;

private:
    // ProgressDialog for transferring messages
    QPointer<QProgressDialog> mProgressDialog;
    // Currently only one async command allowed at a time
    static int mCountJobs;
    int mCountMsgs = 0;
    Result mResult = Undefined;
    bool mDeletesItself : 1;
    bool mEmitsCompletedItself : 1;

    QWidget *const mParent;
    Akonadi::Item::List mMsgList;
    Akonadi::ItemFetchScope mFetchScope;
};

class KMAILTESTS_TESTS_EXPORT KMMailtoComposeCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMMailtoComposeCommand(const QUrl &url, const Akonadi::Item &msg = Akonadi::Item());

private:
    Result execute() override;

    QUrl mUrl;
    Akonadi::Item mMessage;
};

class KMAILTESTS_TESTS_EXPORT KMMailtoReplyCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailtoReplyCommand(QWidget *parent, const QUrl &url, const Akonadi::Item &msg, const QString &selection);

    Q_REQUIRED_RESULT bool replyAsHtml() const;
    void setReplyAsHtml(bool replyAsHtml);

private:
    Result execute() override;

    QUrl mUrl;
    QString mSelection;
    bool mReplyAsHtml = false;
};

class KMAILTESTS_TESTS_EXPORT KMMailtoForwardCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailtoForwardCommand(QWidget *parent, const QUrl &url, const Akonadi::Item &msg);

private:
    Result execute() override;
    QUrl mUrl;
};

class KMAILTESTS_TESTS_EXPORT KMAddBookmarksCommand : public KMCommand
{
    Q_OBJECT

public:
    KMAddBookmarksCommand(const QUrl &url, QWidget *parent);

private:
    Result execute() override;

    QUrl mUrl;
};

class KMAILTESTS_TESTS_EXPORT KMUrlSaveCommand : public KMCommand
{
    Q_OBJECT

public:
    KMUrlSaveCommand(const QUrl &url, QWidget *parent);

private Q_SLOTS:
    void slotUrlSaveResult(KJob *job);

private:
    Result execute() override;

    QUrl mUrl;
};

class KMAILTESTS_TESTS_EXPORT KMEditItemCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMEditItemCommand(QWidget *parent, const Akonadi::Item &msg, bool deleteFromSource = true);
    ~KMEditItemCommand() override;
private Q_SLOTS:
    void slotDeleteItem(KJob *job);

private:
    Result execute() override;
    bool mDeleteFromSource = false;
};

class KMAILTESTS_TESTS_EXPORT KMEditMessageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMEditMessageCommand(QWidget *parent, const KMime::Message::Ptr &msg);

private:
    Result execute() override;
    KMime::Message::Ptr mMessage;
};

class KMAILTESTS_TESTS_EXPORT KMUseTemplateCommand : public KMCommand
{
    Q_OBJECT

public:
    KMUseTemplateCommand(QWidget *parent, const Akonadi::Item &msg);

private:
    Result execute() override;
};

class KMAILTESTS_TESTS_EXPORT KMSaveMsgCommand : public KMCommand
{
    Q_OBJECT

public:
    KMSaveMsgCommand(QWidget *parent, const Akonadi::Item::List &msgList);

private:
    Result execute() override;
};

class KMAILTESTS_TESTS_EXPORT KMOpenMsgCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMOpenMsgCommand(QWidget *parent, const QUrl &url = QUrl(), const QString &encoding = QString(), KMMainWidget *main = nullptr);

private:
    Result execute() override;

private Q_SLOTS:
    void slotDataArrived(KIO::Job *job, const QByteArray &data);
    void slotResult(KJob *job);

private:
    void doesNotContainMessage();
    static const int MAX_CHUNK_SIZE = 64 * 1024;
    QUrl mUrl;
    QString mMsgString;
    KIO::TransferJob *mJob = nullptr;
    const QString mEncoding;
    KMMainWidget *mMainWidget = nullptr;
};

class KMAILTESTS_TESTS_EXPORT KMSaveAttachmentsCommand : public KMCommand
{
    Q_OBJECT
public:
    /** Use this to save all attachments of the given message.
      @param parent  The parent widget of the command used for message boxes.
      @param msg     The message of which the attachments should be saved.
      @param viewer  The message viewer.
    */
    KMSaveAttachmentsCommand(QWidget *parent, const Akonadi::Item &msg, MessageViewer::Viewer *viewer);
    /** Use this to save all attachments of the given messages.
      @param parent  The parent widget of the command used for message boxes.
      @param msgs    The messages of which the attachments should be saved.
    */
    KMSaveAttachmentsCommand(QWidget *parent, const Akonadi::Item::List &msgs, MessageViewer::Viewer *viewer);

private:
    Result execute() override;
    MessageViewer::Viewer *mViewer = nullptr;
};

class KMAILTESTS_TESTS_EXPORT KMReplyCommand : public KMCommand
{
    Q_OBJECT
public:
    KMReplyCommand(QWidget *parent,
                   const Akonadi::Item &msg,
                   MessageComposer::ReplyStrategy replyStrategy,
                   const QString &selection = QString(),
                   bool noquote = false,
                   const QString &templateName = QString());
    Q_REQUIRED_RESULT bool replyAsHtml() const;
    void setReplyAsHtml(bool replyAsHtml);

private:
    Result execute() override;

private:
    QString mSelection;
    QString mTemplate;
    MessageComposer::ReplyStrategy m_replyStrategy;
    bool mNoQuote = false;
    bool mReplyAsHtml = false;
};

class KMAILTESTS_TESTS_EXPORT KMForwardCommand : public KMCommand
{
    Q_OBJECT

public:
    KMForwardCommand(QWidget *parent,
                     const Akonadi::Item::List &msgList,
                     uint identity = 0,
                     const QString &templateName = QString(),
                     const QString &selection = QString());
    KMForwardCommand(QWidget *parent,
                     const Akonadi::Item &msg,
                     uint identity = 0,
                     const QString &templateName = QString(),
                     const QString &selection = QString());

private:
    KMCommand::Result createComposer(const Akonadi::Item &item);
    Result execute() override;

private:
    uint mIdentity;
    QString mTemplate;
    QString mSelection;
};

class KMAILTESTS_TESTS_EXPORT KMForwardAttachedCommand : public KMCommand
{
    Q_OBJECT

public:
    KMForwardAttachedCommand(QWidget *parent, const Akonadi::Item::List &msgList, uint identity = 0, KMail::Composer *win = nullptr);
    KMForwardAttachedCommand(QWidget *parent, const Akonadi::Item &msg, uint identity = 0, KMail::Composer *win = nullptr);

private:
    Result execute() override;

    uint mIdentity;
    QPointer<KMail::Composer> mWin;
};

class KMAILTESTS_TESTS_EXPORT KMRedirectCommand : public KMCommand
{
    Q_OBJECT

public:
    KMRedirectCommand(QWidget *parent, const Akonadi::Item &msg);
    KMRedirectCommand(QWidget *parent, const Akonadi::Item::List &msgList);

private:
    Result execute() override;
};

struct KMAILTESTS_TESTS_EXPORT KMPrintCommandInfo {
    Akonadi::Item mMsg;
    QFont mOverrideFont;
    QString mEncoding;
    MessageViewer::Viewer::DisplayFormatMessage mFormat = MessageViewer::Viewer::UseGlobalSetting;
    const MessageViewer::AttachmentStrategy *mAttachmentStrategy = nullptr;
    MessageViewer::HeaderStylePlugin *mHeaderStylePlugin = nullptr;
    bool mHtmlLoadExtOverride = false;
    bool mUseFixedFont = false;
    bool mPrintPreview = false;
    bool mShowSignatureDetails = false;
    bool mShowEncryptionDetails = false;
};

QDebug operator<<(QDebug d, const KMPrintCommandInfo &t);

class KMAILTESTS_TESTS_EXPORT KMPrintCommand : public KMCommand
{
    Q_OBJECT

public:
    KMPrintCommand(QWidget *parent, const KMPrintCommandInfo &commandInfo);

private:
    Result execute() override;

    KMPrintCommandInfo mPrintCommandInfo;
};

class KMAILTESTS_TESTS_EXPORT KMSetStatusCommand : public KMCommand
{
    Q_OBJECT

public:
    // Serial numbers
    KMSetStatusCommand(const MessageStatus &status, const Akonadi::Item::List &items, bool invert = false);

protected Q_SLOTS:
    void slotModifyItemDone(KJob *job);

private:
    Result execute() override;
    MessageStatus mStatus;
    bool mInvertMark = false;
};

/** This command is used to set or toggle a tag for a list of messages. If toggle is
    true then the tag is deleted if it is already applied.
 */
class KMAILTESTS_TESTS_EXPORT KMSetTagCommand : public KMCommand
{
    Q_OBJECT

public:
    enum SetTagMode {
        AddIfNotExisting,
        Toggle,
        CleanExistingAndAddNew,
    };

    KMSetTagCommand(const Akonadi::Tag::List &tags, const Akonadi::Item::List &item, SetTagMode mode = AddIfNotExisting);

protected Q_SLOTS:
    void slotModifyItemDone(KJob *job);

private:
    Result execute() override;
    void setTags();

    Akonadi::Tag::List mTags;
    Akonadi::Tag::List mCreatedTags;
    Akonadi::Item::List mItem;
    SetTagMode mMode;
};

/* This command is used to apply a single filter (AKA ad-hoc filter)
    to a set of messages */
class KMAILTESTS_TESTS_EXPORT KMFilterActionCommand : public KMCommand
{
    Q_OBJECT

public:
    KMFilterActionCommand(QWidget *parent, const QVector<qlonglong> &msgListId, const QString &filterId);

private:
    Result execute() override;
    QVector<qlonglong> mMsgListId;
    QString mFilterId;
};

class KMAILTESTS_TESTS_EXPORT KMMetaFilterActionCommand : public QObject
{
    Q_OBJECT

public:
    KMMetaFilterActionCommand(const QString &filterId, KMMainWidget *main);

public Q_SLOTS:
    void start();

private:
    QString mFilterId;
    KMMainWidget *mMainWidget = nullptr;
};

class KMAILTESTS_TESTS_EXPORT KMMailingListFilterCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailingListFilterCommand(QWidget *parent, const Akonadi::Item &msg);

private:
    Result execute() override;
};

class KMAILTESTS_TESTS_EXPORT KMCopyCommand : public KMCommand
{
    Q_OBJECT

public:
    KMCopyCommand(const Akonadi::Collection &destFolder, const Akonadi::Item::List &msgList);
    KMCopyCommand(const Akonadi::Collection &destFolder, const Akonadi::Item &msg);

protected Q_SLOTS:
    void slotCopyResult(KJob *job);

private:
    Result execute() override;

    Akonadi::Collection mDestFolder;
};

class KMAILTESTS_TESTS_EXPORT KMCopyDecryptedCommand : public KMCommand
{
    Q_OBJECT
public:
    KMCopyDecryptedCommand(const Akonadi::Collection &destFolder, const Akonadi::Item::List &msgList);
    KMCopyDecryptedCommand(const Akonadi::Collection &destFolder, const Akonadi::Item &msg);

protected Q_SLOTS:
    void slotAppendResult(KJob *job);

private:
    Result execute() override;

    Akonadi::Collection mDestFolder;
    QList<KJob *> mPendingJobs;
};

class KMAILTESTS_TESTS_EXPORT KMMoveCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMoveCommand(const Akonadi::Collection &destFolder, const Akonadi::Item::List &msgList, MessageList::Core::MessageItemSetReference ref);
    KMMoveCommand(const Akonadi::Collection &destFolder,
                  const Akonadi::Item &msg,
                  MessageList::Core::MessageItemSetReference ref = MessageList::Core::MessageItemSetReference());
    Akonadi::Collection destFolder() const
    {
        return mDestFolder;
    }

    MessageList::Core::MessageItemSetReference refSet() const
    {
        return mRef;
    }

public Q_SLOTS:
    void slotMoveCanceled();
    void slotMoveResult(KJob *job);

protected:
    void setDestFolder(const Akonadi::Collection &folder)
    {
        mDestFolder = folder;
    }

Q_SIGNALS:
    void moveDone(KMMoveCommand *);

private:
    Result execute() override;
    void completeMove(Result result);

    Akonadi::Collection mDestFolder;
    KPIM::ProgressItem *mProgressItem = nullptr;
    MessageList::Core::MessageItemSetReference mRef;
};

class KMAILTESTS_TESTS_EXPORT KMTrashMsgCommand final : public KMCommand
{
    Q_OBJECT

public:
    enum TrashOperation {
        Unknown,
        MoveToTrash,
        Delete,
        Both,
    };

    KMTrashMsgCommand(const Akonadi::Collection &srcFolder, const Akonadi::Item::List &msgList, MessageList::Core::MessageItemSetReference ref);
    KMTrashMsgCommand(const Akonadi::Collection &srcFolder, const Akonadi::Item &msg, MessageList::Core::MessageItemSetReference ref);
    MessageList::Core::MessageItemSetReference refSet() const
    {
        return mRef;
    }

    TrashOperation operation() const;

public Q_SLOTS:
    void slotMoveCanceled();

private Q_SLOTS:
    void slotMoveResult(KJob *job);
    void slotDeleteResult(KJob *job);
Q_SIGNALS:
    void moveDone(KMTrashMsgCommand *);

private:
    Result execute() override;
    void completeMove(Result result);

    static Akonadi::Collection findTrashFolder(const Akonadi::Collection &srcFolder);

    QMap<Akonadi::Collection, Akonadi::Item::List> mTrashFolders;
    KPIM::ProgressItem *mMoveProgress = nullptr;
    KPIM::ProgressItem *mDeleteProgress = nullptr;
    MessageList::Core::MessageItemSetReference mRef;
    QList<KJob *> mPendingMoves;
    QList<KJob *> mPendingDeletes;
};

class KMAILTESTS_TESTS_EXPORT KMResendMessageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMResendMessageCommand(QWidget *parent, const Akonadi::Item &msg = Akonadi::Item());

private:
    Result execute() override;
};

class KMAILTESTS_TESTS_EXPORT KMShareImageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMShareImageCommand(const QUrl &url, QWidget *parent);

private:
    Result execute() override;
    QUrl mUrl;
};

class KMAILTESTS_TESTS_EXPORT KMFetchMessageCommand : public KMCommand
{
    Q_OBJECT
public:
    explicit KMFetchMessageCommand(QWidget *parent, const Akonadi::Item &item, MessageViewer::Viewer *viewer, KMReaderMainWin *win = nullptr);

    Akonadi::Item item() const;

    KMReaderMainWin *readerMainWin() const;

private:
    Akonadi::ItemFetchJob *createFetchJob(const Akonadi::Item::List &items) override;
    Result execute() override;

    Akonadi::Item mItem;
    MessageViewer::Viewer *mViewer = nullptr;
    KMReaderMainWin *mReaderMainWin = nullptr;
};

