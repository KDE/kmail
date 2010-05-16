// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMCommands_h
#define KMCommands_h

#include "kmail_export.h"
#include <kmime/kmime_message.h>
#include "messageviewer/viewer.h"
#include "messageviewer/editorwatcher.h"
#include "messageviewer/headerstrategy.h"
#include "messageviewer/headerstyle.h"
using MessageViewer::EditorWatcher;
#include <messagecore/messagestatus.h>
#include <messagelist/core/view.h>
using KPIM::MessageStatus;
#include <kservice.h>
#include <ktemporaryfile.h>
#include <kio/job.h>

#include <QPointer>
#include <QList>
#include <QMenu>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collection.h>
class KProgressDialog;
class KMFilter;
class KMMainWidget;
class KMReaderWin;
class FolderCollection;
template <typename T> class QSharedPointer;

namespace KIO { class Job; }
namespace KMail {
  class Composer;
}
namespace GpgME { class Error; }
namespace Kleo { class SpecialJob; }

typedef QMap<KMime::Content*, Akonadi::Item> PartNodeMessageMap;
  /// Small helper structure which encapsulates the KMMessage created when creating a reply, and

class KMAIL_EXPORT KMCommand : public QObject
{
  Q_OBJECT

public:
  enum Result { Undefined, OK, Canceled, Failed };

  // Trival constructor, don't retrieve any messages
  KMCommand( QWidget *parent = 0 );
  KMCommand( QWidget *parent, const Akonadi::Item & );
  // Retrieve all messages in msgList when start is called.
  KMCommand( QWidget *parent, const QList<Akonadi::Item> &msgList );
  // Retrieve the single message msgBase when start is called.
  virtual ~KMCommand();


  /** Returns the result of the command. Only call this method from the slot
      connected to completed().
  */
  Result result() const;

public slots:
  // Retrieve messages then calls execute
  void start();

  // advance the progressbar, emitted by the folderjob
  void slotProgress( unsigned long done, unsigned long total );

signals:

  /// @param result The status of the command.
  void messagesTransfered( KMCommand::Result result );

  /// Emitted when the command has completed.
  void completed( KMCommand *command );

protected:
  /** Allows to configure how much data should be retrieved of the messages. */
  Akonadi::ItemFetchScope& fetchScope() { return mFetchScope; }

  // Returns list of messages retrieved
  const QList<Akonadi::Item> retrievedMsgs() const;
  // Returns the single message retrieved
  Akonadi::Item retrievedMessage() const;
  // Returns the parent widget
  QWidget *parentWidget() const;

  bool deletesItself() const { return mDeletesItself; }
  /** Specify whether the subclass takes care of the deletion of the object.
      By default the base class will delete the object.
      @param deletesItself true if the subclass takes care of deletion, false
                           if the base class should take care of deletion
  */
  void setDeletesItself( bool deletesItself )
  { mDeletesItself = deletesItself; }

  bool emitsCompletedItself() const { return mEmitsCompletedItself; }
  /** Specify whether the subclass takes care of emitting the completed()
      signal. By default the base class will emit this signal.
      @param emitsCompletedItself true if the subclass emits the completed
                                  signal, false if the base class should emit
                                  the signal
  */
  void setEmitsCompletedItself( bool emitsCompletedItself )
  { mEmitsCompletedItself = emitsCompletedItself; }

  /** Use this to set the result of the command.
      @param result The result of the command.
   */
  void setResult( Result result )
  { mResult = result; }

private:
  // execute should be implemented by derived classes
  virtual Result execute() = 0;

  /** transfers the list of (imap)-messages
   *  this is a necessary preparation for e.g. forwarding */
  void transferSelectedMsgs();

private slots:
  /** Called from start() via a single shot timer. */
  virtual void slotStart();

  void slotPostTransfer( KMCommand::Result result );
  /** the msg has been transferred */
  void slotMsgTransfered(const Akonadi::Item::List& msgs);
  /** the KMImapJob is finished */
  void slotJobFinished();
  /** the transfer was canceled */
  void slotTransferCancelled();

protected:  
  QList<Akonadi::Item> mRetrievedMsgs;

private:
  // ProgressDialog for transferring messages
  KProgressDialog* mProgressDialog;
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
  explicit KMMailtoComposeCommand( const KUrl &url, const Akonadi::Item &msg=Akonadi::Item() );

private:
  virtual Result execute();

  KUrl mUrl;
  Akonadi::Item mMessage;
};

class KMAIL_EXPORT KMMailtoReplyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoReplyCommand( QWidget *parent, const KUrl &url,
                        const Akonadi::Item &msg, const QString &selection );

private:
  virtual Result execute();

  KUrl mUrl;
  QString mSelection;
};

class KMAIL_EXPORT KMMailtoForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoForwardCommand( QWidget *parent, const KUrl &url,const Akonadi::Item& msg );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KMAIL_EXPORT KMAddBookmarksCommand : public KMCommand
{
  Q_OBJECT

public:
  KMAddBookmarksCommand( const KUrl &url, QWidget *parent );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KMAIL_EXPORT KMUrlSaveCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlSaveCommand( const KUrl &url, QWidget *parent );

private slots:
  void slotUrlSaveResult( KJob *job );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KMAIL_EXPORT KMEditMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMEditMsgCommand( QWidget *parent, const Akonadi::Item &msg );
private slots:
  void slotDeleteItem( KJob *job );
private:
  virtual Result execute();
};

class KMAIL_EXPORT KMUseTemplateCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUseTemplateCommand( QWidget *parent, const Akonadi::Item &msg );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMSaveMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSaveMsgCommand( QWidget *parent, const QList<Akonadi::Item> &msgList );
  KMSaveMsgCommand( QWidget *parent, const Akonadi::Item & msg );
  KUrl url();

private:
  virtual Result execute();

private slots:
  void slotSaveDataReq();
  void slotSaveResult(KJob *job);
  /** the message has been transferred for saving */
  void slotMessageRetrievedForSaving(const Akonadi::Item &msg);

private:
  static const int MAX_CHUNK_SIZE = 64*1024;
  KUrl mUrl;
  QList<unsigned long> mMsgList;
  unsigned int mMsgListIndex;
  QByteArray mData;
  int mOffset;
  size_t mTotalSize;
  KIO::TransferJob *mJob;
};

class KMAIL_EXPORT KMOpenMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  explicit KMOpenMsgCommand( QWidget *parent, const KUrl & url = KUrl(),
                             const QString & encoding = QString() );

private:
  virtual Result execute();

private slots:
  void slotDataArrived( KIO::Job *job, const QByteArray & data );
  void slotResult( KJob *job );

private:
  static const int MAX_CHUNK_SIZE = 64*1024;
  KUrl mUrl;
  QString mMsgString;
  KIO::TransferJob *mJob;
  const QString mEncoding;
};

class KMAIL_EXPORT KMSaveAttachmentsCommand : public KMCommand
{
  Q_OBJECT
public:
  /** Use this to save all attachments of the given message.
      @param parent  The parent widget of the command used for message boxes.
      @param msg     The message of which the attachments should be saved.
   */
  KMSaveAttachmentsCommand( QWidget *parent, const Akonadi::Item &msg  );
  /** Use this to save all attachments of the given messages.
      @param parent  The parent widget of the command used for message boxes.
      @param msgs    The messages of which the attachments should be saved.
   */
  KMSaveAttachmentsCommand( QWidget *parent, const QList<Akonadi::Item>& msgs );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                    const QString &selection = QString() );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMAIL_EXPORT KMNoQuoteReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMNoQuoteReplyToCommand( QWidget *parent, const Akonadi::Item &msg );

private:
  virtual Result execute();
};

class KMReplyListCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyListCommand( QWidget *parent, const Akonadi::Item &msg,
                      const QString &selection = QString() );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMAIL_EXPORT KMReplyToAllCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToAllCommand( QWidget *parent, const Akonadi::Item &msg,
                       const QString &selection = QString() );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMAIL_EXPORT KMReplyAuthorCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyAuthorCommand( QWidget *parent, const Akonadi::Item &msg,
                        const QString &selection = QString() );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMAIL_EXPORT KMForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardCommand( QWidget *parent, const QList<Akonadi::Item> &msgList,
                    uint identity = 0 );
  KMForwardCommand( QWidget *parent, const Akonadi::Item& msg,
                    uint identity = 0 );

private:
  virtual Result execute();

private:
  uint mIdentity;
};

class KMAIL_EXPORT KMForwardAttachedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardAttachedCommand( QWidget *parent, const QList<Akonadi::Item> &msgList,
                            uint identity = 0, KMail::Composer *win = 0 );
  KMForwardAttachedCommand( QWidget *parent, const Akonadi::Item & msg,
                            uint identity = 0, KMail::Composer *win = 0 );

private:
  virtual Result execute();

  uint mIdentity;
  QPointer<KMail::Composer> mWin;
};

class KMAIL_EXPORT KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( QWidget *parent, const Akonadi::Item &msg );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMCustomReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCustomReplyToCommand( QWidget *parent, const Akonadi::Item &msg,
                          const QString &selection,
                          const QString &tmpl );

private:
  virtual Result execute();

private:
  QString mSelection;
  QString mTemplate;
};

class KMAIL_EXPORT KMCustomReplyAllToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCustomReplyAllToCommand( QWidget *parent, const Akonadi::Item &msg,
                          const QString &selection,
                          const QString &tmpl );

private:
  virtual Result execute();

private:
  QString mSelection;
  QString mTemplate;
};

class KMAIL_EXPORT KMCustomForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCustomForwardCommand( QWidget *parent, const QList<Akonadi::Item> &msgList,
                          uint identity, const QString &tmpl );
  KMCustomForwardCommand( QWidget *parent, const Akonadi::Item& msg,
                          uint identity, const QString &tmpl );

private:
  virtual Result execute();

  uint mIdentity;
  QString mTemplate;
};

class KMAIL_EXPORT KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( QWidget *parent, const Akonadi::Item &msg,
                  MessageViewer::HeaderStyle *headerStyle = 0,
                  const MessageViewer::HeaderStrategy *headerStrategy = 0,
                  bool htmlOverride = false,
                  bool htmlLoadExtOverride = false,
                  bool useFixedFont = false,
                  const QString & encoding = QString() );

  void setOverrideFont( const QFont& );
  void setAttachmentStrategy( const MessageViewer::AttachmentStrategy *strategy );

private:
  virtual Result execute();

  MessageViewer::HeaderStyle *mHeaderStyle;
  const MessageViewer::HeaderStrategy *mHeaderStrategy;
  const MessageViewer::AttachmentStrategy *mAttachmentStrategy;
  bool mHtmlOverride;
  bool mHtmlLoadExtOverride;
  bool mUseFixedFont;
  QFont mOverrideFont;
  QString mEncoding;
};

class KMAIL_EXPORT KMSetStatusCommand : public KMCommand
{
  Q_OBJECT

public:
  // Serial numbers
  KMSetStatusCommand( const MessageStatus& status, const Akonadi::Item::List &items,
                      bool toggle=false );

protected slots:
  void slotModifyItemDone( KJob * job );

private:
  virtual Result execute();

  MessageStatus mStatus;
  int messageStatusChanged;
  bool mToggle;
};

/** This command is used to set or toggle a tag for a list of messages. If toggle is
    true then the tag is deleted if it is already applied.
 */
class KMAIL_EXPORT KMSetTagCommand : public KMCommand
{
  Q_OBJECT

public:
  enum SetTagMode { AddIfNotExisting, Toggle };

  KMSetTagCommand( const QString &tagLabel, const QList<Akonadi::Item> &item,
                   SetTagMode mode=AddIfNotExisting );

private:
  virtual Result execute();

  QString mTagLabel;
  QList<Akonadi::Item> mItem;
  SetTagMode mMode;
};

/* This command is used to create a filter based on the user's
    decision, e.g. filter by From header */
class KMAIL_EXPORT KMFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMFilterCommand( const QByteArray &field, const QString &value );

private:
  virtual Result execute();

  QByteArray mField;
  QString mValue;
};


/* This command is used to apply a single filter (AKA ad-hoc filter)
    to a set of messages */
class KMAIL_EXPORT KMFilterActionCommand : public KMCommand
{
  Q_OBJECT

public:
  KMFilterActionCommand( QWidget *parent,
                         const QList<Akonadi::Item> &msgList, KMFilter *filter );

private:
  virtual Result execute();
  KMFilter *mFilter;
};


class KMAIL_EXPORT KMMetaFilterActionCommand : public QObject
{
  Q_OBJECT

public:
  KMMetaFilterActionCommand( KMFilter *filter, KMMainWidget *main );

public slots:
  void start();

private:
  KMFilter *mFilter;
  KMMainWidget *mMainWidget;
};

class KMAIL_EXPORT FolderShortcutCommand : public QObject
{
  Q_OBJECT

public:
  FolderShortcutCommand( KMMainWidget* mainwidget, const Akonadi::Collection & col );
  ~FolderShortcutCommand();

public slots:
  void start();
  /** Assign a KActio to the command which is used to trigger it. This
   * action will be deleted along with the command, so you don't need to
   * keep track of it separately. */
  void setAction( QAction* );

private:
  KMMainWidget *mMainWidget;
  Akonadi::Collection mCollectionFolder;
  QAction *mAction;
};


class KMAIL_EXPORT KMMailingListFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailingListFilterCommand( QWidget *parent, const Akonadi::Item &msg );

private:
  virtual Result execute();
};


class KMAIL_EXPORT KMCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCopyCommand( const Akonadi::Collection &destFolder, const QList<Akonadi::Item> &msgList );
  KMCopyCommand( const Akonadi::Collection& destFolder, const Akonadi::Item &msg );

protected slots:
  void slotCopyResult( KJob * job );
private:
  virtual Result execute();

  Akonadi::Collection mDestFolder;
  QList<Akonadi::Item> mMsgList;
};

namespace KPIM {
  class ProgressItem;
}
class KMAIL_EXPORT KMMoveCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMoveCommand( const Akonadi::Collection& destFolder, const QList<Akonadi::Item> &msgList, MessageList::Core::MessageItemSetReference ref );
  KMMoveCommand( const Akonadi::Collection& destFolder, const Akonadi::Item & msg, MessageList::Core::MessageItemSetReference ref = MessageList::Core::MessageItemSetReference() );
  Akonadi::Collection destFolder() const { return mDestFolder; }

  MessageList::Core::MessageItemSetReference refSet() const { return mRef; }

public slots:
  void slotMoveCanceled();
  void slotMoveResult( KJob * job );
protected:
  void setDestFolder( const Akonadi::Collection& folder ) { mDestFolder = folder; }

signals:
  void moveDone( KMMoveCommand* );

private:
  virtual Result execute();
  void completeMove( Result result );

  Akonadi::Collection mDestFolder;
  KPIM::ProgressItem *mProgressItem;
  MessageList::Core::MessageItemSetReference mRef;
  bool mCompleteWithAddedMsg;
};

class KMAIL_EXPORT KMTrashMsgCommand : public KMMoveCommand
{
  Q_OBJECT

public:
  KMTrashMsgCommand( const Akonadi::Collection& srcFolder, const QList<Akonadi::Item> &msgList,MessageList::Core::MessageItemSetReference ref );
  KMTrashMsgCommand( const Akonadi::Collection& srcFolder, const Akonadi::Item& msg,MessageList::Core::MessageItemSetReference ref );

private:
  static Akonadi::Collection findTrashFolder( const Akonadi::Collection& srcFolder );

};

class KMAIL_EXPORT KMUrlClickedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlClickedCommand( const KUrl &url, uint identity,
    KMReaderWin *readerWin, bool mHtmlPref, KMMainWidget *mainWidget = 0 );

private:
  virtual Result execute();

  KUrl mUrl;
  uint mIdentity;
  KMReaderWin *mReaderWin;
  bool mHtmlPref;
  KMMainWidget *mMainWidget;
};

class KMAIL_EXPORT KMResendMessageCommand : public KMCommand
{
  Q_OBJECT

public:
  explicit KMResendMessageCommand( QWidget *parent, const Akonadi::Item & msg= Akonadi::Item() );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMMailingListCommand : public KMCommand
{
  Q_OBJECT
public:
  KMMailingListCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
private:
  virtual Result execute();
private slots:
  void commandCompleted( KMCommand *command );
protected:
  virtual KUrl::List urls() const =0;
protected:
  QSharedPointer<FolderCollection> mFolder;
};

class KMAIL_EXPORT KMMailingListPostCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListPostCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListSubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListSubscribeCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListUnsubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListUnsubscribeCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListArchivesCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListArchivesCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListHelpCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListHelpCommand( QWidget *parent, const QSharedPointer<FolderCollection> &parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT CreateTodoCommand : public KMCommand
{
  Q_OBJECT
  public:
  CreateTodoCommand( QWidget *parent, const Akonadi::Item &msg );

  private:
    Result execute();
};

#endif /*KMCommands_h*/
