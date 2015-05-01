//

#ifndef KMCommands_h
#define KMCommands_h

#include "kmail_export.h"
#include "messagecomposer/helper/messagefactory.h"
#include "messagelist/core/view.h"
#include "search/searchpattern.h"
#include "messageviewer/viewer/viewer.h"

#include <Akonadi/KMime/MessageStatus>
#include <kio/job.h>
#include <kmime/kmime_message.h>

#include <QPointer>
#include <QList>
#include <AkonadiCore/item.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/collection.h>
#include <QUrl>
namespace Akonadi
{
class Tag;
}

using Akonadi::MessageStatus;

class QProgressDialog;
class KMMainWidget;

template <typename T> class QSharedPointer;

namespace MessageViewer
{
class HeaderStyle;
class HeaderStrategy;
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
namespace GpgME
{
class Error;
}

typedef QMap<KMime::Content *, Akonadi::Item> PartNodeMessageMap;
/// Small helper structure which encapsulates the KMMessage created when creating a reply, and

class KMAIL_EXPORT KMCommand : public QObject
{
    Q_OBJECT

public:
    enum Result { Undefined, OK, Canceled, Failed };

    // Trival constructor, don't retrieve any messages
    explicit KMCommand(QWidget *parent = Q_NULLPTR);
    KMCommand(QWidget *parent, const Akonadi::Item &);
    // Retrieve all messages in msgList when start is called.
    KMCommand(QWidget *parent, const QList<Akonadi::Item> &msgList);
    // Retrieve the single message msgBase when start is called.
    virtual ~KMCommand();

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
    const QList<Akonadi::Item> retrievedMsgs() const;
    // Returns the single message retrieved
    Akonadi::Item retrievedMessage() const;
    // Returns the parent widget
    QWidget *parentWidget() const;

    bool deletesItself() const
    {
        return mDeletesItself;
    }
    /** Specify whether the subclass takes care of the deletion of the object.
      By default the base class will delete the object.
      @param deletesItself true if the subclass takes care of deletion, false
                           if the base class should take care of deletion
    */
    void setDeletesItself(bool deletesItself)
    {
        mDeletesItself = deletesItself;
    }

    bool emitsCompletedItself() const
    {
        return mEmitsCompletedItself;
    }
    /** Specify whether the subclass takes care of emitting the completed()
      signal. By default the base class will Q_EMIT this signal.
      @param emitsCompletedItself true if the subclass emits the completed
                                  signal, false if the base class should emit
                                  the signal
    */
    void setEmitsCompletedItself(bool emitsCompletedItself)
    {
        mEmitsCompletedItself = emitsCompletedItself;
    }

    /** Use this to set the result of the command.
      @param result The result of the command.
    */
    void setResult(Result result)
    {
        mResult = result;
    }

private:
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
    QList<Akonadi::Item> mRetrievedMsgs;

private:
    // ProgressDialog for transferring messages
    QWeakPointer<QProgressDialog> mProgressDialog;
    //Currently only one async command allowed at a time
    static int mCountJobs;
    int mCountMsgs;
    Result mResult;
    bool mDeletesItself : 1;
    bool mEmitsCompletedItself : 1;

    QWidget *mParent;
    QList<Akonadi::Item> mMsgList;
    Akonadi::ItemFetchScope mFetchScope;
};

class KMAIL_EXPORT KMMailtoComposeCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMMailtoComposeCommand(const QUrl &url, const Akonadi::Item &msg = Akonadi::Item());

private:
    Result execute() Q_DECL_OVERRIDE;

    QUrl mUrl;
    Akonadi::Item mMessage;
};

class KMAIL_EXPORT KMMailtoReplyCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailtoReplyCommand(QWidget *parent, const QUrl &url,
                         const Akonadi::Item &msg, const QString &selection);

private:
    Result execute() Q_DECL_OVERRIDE;

    QUrl mUrl;
    QString mSelection;
};

class KMAIL_EXPORT KMMailtoForwardCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailtoForwardCommand(QWidget *parent, const QUrl &url, const Akonadi::Item &msg);

private:
    Result execute() Q_DECL_OVERRIDE;
    QUrl mUrl;
};

class KMAIL_EXPORT KMAddBookmarksCommand : public KMCommand
{
    Q_OBJECT

public:
    KMAddBookmarksCommand(const QUrl &url, QWidget *parent);

private:
    Result execute() Q_DECL_OVERRIDE;

    QUrl mUrl;
};

class KMAIL_EXPORT KMUrlSaveCommand : public KMCommand
{
    Q_OBJECT

public:
    KMUrlSaveCommand(const QUrl &url, QWidget *parent);

private Q_SLOTS:
    void slotUrlSaveResult(KJob *job);

private:
    Result execute() Q_DECL_OVERRIDE;

    QUrl mUrl;
};

class KMAIL_EXPORT KMEditItemCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMEditItemCommand(QWidget *parent, const Akonadi::Item &msg, bool deleteFromSource = true);
    ~KMEditItemCommand();
private Q_SLOTS:
    void slotDeleteItem(KJob *job);
private:
    Result execute() Q_DECL_OVERRIDE;
    bool mDeleteFromSource;
};

class KMAIL_EXPORT KMEditMessageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMEditMessageCommand(QWidget *parent, const KMime::Message::Ptr &msg);
private:
    Result execute() Q_DECL_OVERRIDE;
    KMime::Message::Ptr mMessage;
};

class KMAIL_EXPORT KMUseTemplateCommand : public KMCommand
{
    Q_OBJECT

public:
    KMUseTemplateCommand(QWidget *parent, const Akonadi::Item &msg);

private:
    Result execute() Q_DECL_OVERRIDE;
};

class KMAIL_EXPORT KMSaveMsgCommand : public KMCommand
{
    Q_OBJECT

public:
    KMSaveMsgCommand(QWidget *parent, const QList<Akonadi::Item> &msgList);

private:
    Result execute() Q_DECL_OVERRIDE;

};

class KMAIL_EXPORT KMOpenMsgCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMOpenMsgCommand(QWidget *parent, const QUrl &url = QUrl(),
                              const QString &encoding = QString() , KMMainWidget *main = Q_NULLPTR);

private:
    Result execute() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotDataArrived(KIO::Job *job, const QByteArray &data);
    void slotResult(KJob *job);

private:
    void doesNotContainMessage();
    static const int MAX_CHUNK_SIZE = 64 * 1024;
    QUrl mUrl;
    QString mMsgString;
    KIO::TransferJob *mJob;
    const QString mEncoding;
    KMMainWidget *mMainWidget;
};

class KMAIL_EXPORT KMSaveAttachmentsCommand : public KMCommand
{
    Q_OBJECT
public:
    /** Use this to save all attachments of the given message.
      @param parent  The parent widget of the command used for message boxes.
      @param msg     The message of which the attachments should be saved.
    */
    KMSaveAttachmentsCommand(QWidget *parent, const Akonadi::Item &msg, MessageViewer::Viewer *viewer);
    /** Use this to save all attachments of the given messages.
      @param parent  The parent widget of the command used for message boxes.
      @param msgs    The messages of which the attachments should be saved.
    */
    KMSaveAttachmentsCommand(QWidget *parent, const QList<Akonadi::Item> &msgs);

private:
    Result execute() Q_DECL_OVERRIDE;
    MessageViewer::Viewer *mViewer;
};

class KMAIL_EXPORT KMReplyCommand : public KMCommand
{
    Q_OBJECT
public:
    KMReplyCommand(QWidget *parent, const Akonadi::Item &msg,
                   MessageComposer::ReplyStrategy replyStrategy,
                   const QString &selection = QString(), bool noquote = false, const QString &templateName = QString());
private:
    Result execute() Q_DECL_OVERRIDE;

private:
    QString mSelection;
    QString mTemplate;
    MessageComposer::ReplyStrategy m_replyStrategy;
    bool mNoQuote;
};

class KMAIL_EXPORT KMForwardCommand : public KMCommand
{
    Q_OBJECT

public:
    KMForwardCommand(QWidget *parent, const QList<Akonadi::Item> &msgList,
                     uint identity = 0, const QString &templateName = QString());
    KMForwardCommand(QWidget *parent, const Akonadi::Item &msg,
                     uint identity = 0, const QString &templateName = QString());

private:
    KMCommand::Result createComposer(const Akonadi::Item &item);
    Result execute() Q_DECL_OVERRIDE;

private:
    uint mIdentity;
    QString mTemplate;
};

class KMAIL_EXPORT KMForwardAttachedCommand : public KMCommand
{
    Q_OBJECT

public:
    KMForwardAttachedCommand(QWidget *parent, const QList<Akonadi::Item> &msgList,
                             uint identity = 0, KMail::Composer *win = Q_NULLPTR);
    KMForwardAttachedCommand(QWidget *parent, const Akonadi::Item &msg,
                             uint identity = 0, KMail::Composer *win = Q_NULLPTR);

private:
    Result execute() Q_DECL_OVERRIDE;

    uint mIdentity;
    QPointer<KMail::Composer> mWin;
};

class KMAIL_EXPORT KMRedirectCommand : public KMCommand
{
    Q_OBJECT

public:
    KMRedirectCommand(QWidget *parent, const Akonadi::Item &msg);
    KMRedirectCommand(QWidget *parent, const QList<Akonadi::Item> &msgList);

private:
    Result execute() Q_DECL_OVERRIDE;
};

class KMAIL_EXPORT KMPrintCommand : public KMCommand
{
    Q_OBJECT

public:
    KMPrintCommand(QWidget *parent, const Akonadi::Item &msg,
                   MessageViewer::HeaderStyle *headerStyle = Q_NULLPTR,
                   MessageViewer::HeaderStrategy *headerStrategy = Q_NULLPTR,
                   MessageViewer::Viewer::DisplayFormatMessage format = MessageViewer::Viewer::UseGlobalSetting,
                   bool htmlLoadExtOverride = false,
                   bool useFixedFont = false,
                   const QString &encoding = QString());

    void setOverrideFont(const QFont &);
    void setAttachmentStrategy(const MessageViewer::AttachmentStrategy *strategy);
    void setPrintPreview(bool preview);

private:
    Result execute() Q_DECL_OVERRIDE;

    MessageViewer::HeaderStyle *mHeaderStyle;
    MessageViewer::HeaderStrategy *mHeaderStrategy;
    const MessageViewer::AttachmentStrategy *mAttachmentStrategy;
    QFont mOverrideFont;
    QString mEncoding;
    MessageViewer::Viewer::DisplayFormatMessage mFormat;
    bool mHtmlLoadExtOverride;
    bool mUseFixedFont;
    bool mPrintPreview;
};

class KMAIL_EXPORT KMSetStatusCommand : public KMCommand
{
    Q_OBJECT

public:
    // Serial numbers
    KMSetStatusCommand(const MessageStatus &status, const Akonadi::Item::List &items,
                       bool invert = false);

protected Q_SLOTS:
    void slotModifyItemDone(KJob *job);

private:
    Result execute() Q_DECL_OVERRIDE;
    MessageStatus mStatus;
    bool mInvertMark;
};

/** This command is used to set or toggle a tag for a list of messages. If toggle is
    true then the tag is deleted if it is already applied.
 */
class KMAIL_EXPORT KMSetTagCommand : public KMCommand
{
    Q_OBJECT

public:
    enum SetTagMode { AddIfNotExisting, Toggle, CleanExistingAndAddNew };

    KMSetTagCommand(const Akonadi::Tag::List &tags, const QList<Akonadi::Item> &item,
                    SetTagMode mode = AddIfNotExisting);

protected Q_SLOTS:
    void slotModifyItemDone(KJob *job);

private:
    Result execute() Q_DECL_OVERRIDE;
    void setTags();

    Akonadi::Tag::List mTags;
    Akonadi::Tag::List mCreatedTags;
    QList<Akonadi::Item> mItem;
    SetTagMode mMode;
};

/* This command is used to apply a single filter (AKA ad-hoc filter)
    to a set of messages */
class KMAIL_EXPORT KMFilterActionCommand : public KMCommand
{
    Q_OBJECT

public:
    KMFilterActionCommand(QWidget *parent,
                          const QVector<qlonglong> &msgListId, const QString &filterId);

private:
    Result execute() Q_DECL_OVERRIDE;
    QVector<qlonglong> mMsgListId;
    QString mFilterId;
};

class KMAIL_EXPORT KMMetaFilterActionCommand : public QObject
{
    Q_OBJECT

public:
    KMMetaFilterActionCommand(const QString &filterId, KMMainWidget *main);

public Q_SLOTS:
    void start();

private:
    QString mFilterId;
    KMMainWidget *mMainWidget;
};

class KMAIL_EXPORT KMMailingListFilterCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMailingListFilterCommand(QWidget *parent, const Akonadi::Item &msg);

private:
    Result execute() Q_DECL_OVERRIDE;
};

class KMAIL_EXPORT KMCopyCommand : public KMCommand
{
    Q_OBJECT

public:
    KMCopyCommand(const Akonadi::Collection &destFolder, const QList<Akonadi::Item> &msgList);
    KMCopyCommand(const Akonadi::Collection &destFolder, const Akonadi::Item &msg);

protected Q_SLOTS:
    void slotCopyResult(KJob *job);
private:
    Result execute() Q_DECL_OVERRIDE;

    Akonadi::Collection mDestFolder;
};

namespace KPIM
{
class ProgressItem;
}
class KMAIL_EXPORT KMMoveCommand : public KMCommand
{
    Q_OBJECT

public:
    KMMoveCommand(const Akonadi::Collection &destFolder, const QList<Akonadi::Item> &msgList, MessageList::Core::MessageItemSetReference ref);
    KMMoveCommand(const Akonadi::Collection &destFolder, const Akonadi::Item &msg, MessageList::Core::MessageItemSetReference ref = MessageList::Core::MessageItemSetReference());
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
    Result execute() Q_DECL_OVERRIDE;
    void completeMove(Result result);

    Akonadi::Collection mDestFolder;
    KPIM::ProgressItem *mProgressItem;
    MessageList::Core::MessageItemSetReference mRef;
};

class KMAIL_EXPORT KMTrashMsgCommand : public KMMoveCommand
{
    Q_OBJECT

public:
    KMTrashMsgCommand(const Akonadi::Collection &srcFolder, const QList<Akonadi::Item> &msgList, MessageList::Core::MessageItemSetReference ref);
    KMTrashMsgCommand(const Akonadi::Collection &srcFolder, const Akonadi::Item &msg, MessageList::Core::MessageItemSetReference ref);

private:
    static Akonadi::Collection findTrashFolder(const Akonadi::Collection &srcFolder);

};

class KMAIL_EXPORT KMResendMessageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMResendMessageCommand(QWidget *parent, const Akonadi::Item &msg = Akonadi::Item());

private:
    Result execute() Q_DECL_OVERRIDE;
};

class KMAIL_EXPORT KMShareImageCommand : public KMCommand
{
    Q_OBJECT

public:
    explicit KMShareImageCommand(const QUrl &url, QWidget *parent);

private:
    Result execute() Q_DECL_OVERRIDE;
    QUrl mUrl;
};

class KMAIL_EXPORT KMFetchMessageCommand : public KMCommand
{
    Q_OBJECT
public:
    explicit KMFetchMessageCommand(QWidget *parent, const Akonadi::Item &item);

    Akonadi::Item item() const;

private:
    Akonadi::ItemFetchJob *createFetchJob(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;
    Result execute() Q_DECL_OVERRIDE;

    Akonadi::Item mItem;
};

#endif /*KMCommands_h*/
