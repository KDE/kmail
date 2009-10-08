// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMCommands_h
#define KMCommands_h

#include "kmail_export.h"
#include "kmmsgbase.h"

#include <mimelib/string.h>
#include <messagecore/messagestatus.h>
using KPIM::MessageStatus;
#include <kservice.h>
#include <ktemporaryfile.h>
#include <kio/job.h>

#include <QPointer>
#include <QList>
#include <QMenu>

class KProgressDialog;
class KMFilter;
class KMFolder;
class KMFolderImap;
class KMMainWidget;
class KMMessage;
class KMMsgBase;
class KMReaderWin;
class partNode;
class DwBodyPart;
class DwEntity;

namespace KIO { class Job; }
namespace KMail {
  class Composer;
  class FolderJob;
  class EditorWatcher;
  class HeaderStyle;
  class HeaderStrategy;
}
namespace GpgME { class Error; }
namespace Kleo { class SpecialJob; }

typedef QMap<partNode*, KMMessage*> PartNodeMessageMap;

class KMAIL_EXPORT KMCommand : public QObject
{
  Q_OBJECT
    friend class LaterDeleterWithCommandCompletion;

public:
  enum Result { Undefined, OK, Canceled, Failed };

  // Trival constructor, don't retrieve any messages
  KMCommand( QWidget *parent = 0 );
  // Retrieve all messages in msgList when start is called.
  KMCommand( QWidget *parent, const QList<KMMsgBase*> &msgList );
  // Retrieve the single message msgBase when start is called.
  KMCommand( QWidget *parent, KMMsgBase *msgBase );
  // Retrieve the single message msgBase when start is called.
  KMCommand( QWidget *parent, KMMessage *message );
  virtual ~KMCommand();

  /** These folders will be closed by the dtor, handy, if you need to keep
      a folder open during the lifetime of the command, but don't want to
      care about closing it again.
   */
  void keepFolderOpen( KMFolder *folder );

  /** Returns the result of the command. Only call this method from the slot
      connected to completed().
  */
  Result result() const;

  /** cleans a filename by replacing characters not allowed or wanted on the filesystem
      e.g. ':', '/', '\' with '_'
  */
  static QString cleanFileName( const QString &fileName );

public slots:
  // Retrieve messages then calls execute
  void start();

  // advance the progressbar, emitted by the folderjob
  void slotProgress( unsigned long done, unsigned long total );

signals:
  void messagesTransfered( KMCommand::Result result );
  /** Emitted when the command has completed.
   * @param result The status of the command. */
  void completed( KMCommand *command );

protected:
  // Returns list of messages retrieved
  const QList<KMMessage*> retrievedMsgs() const;
  // Returns the single message retrieved
  KMMessage *retrievedMessage() const;
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
  void slotMsgTransfered(KMMessage* msg);
  /** the KMImapJob is finished */
  void slotJobFinished();
  /** the transfer was canceled */
  void slotTransferCancelled();

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
  QList<KMMessage*> mRetrievedMsgs;
  QList<KMMsgBase*> mMsgList;
  QList<QPointer<KMFolder> > mFolders;
};

class KMAIL_EXPORT KMMailtoComposeCommand : public KMCommand
{
  Q_OBJECT

public:
  explicit KMMailtoComposeCommand( const KUrl &url, KMMessage *msg=0 );

private:
  virtual Result execute();

  KUrl mUrl;
  KMMessage *mMessage;
};

class KMAIL_EXPORT KMMailtoReplyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoReplyCommand( QWidget *parent, const KUrl &url,
                        KMMessage *msg, const QString &selection );

private:
  virtual Result execute();

  KUrl mUrl;
  QString mSelection;
};

class KMAIL_EXPORT KMMailtoForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoForwardCommand( QWidget *parent, const KUrl &url, KMMessage *msg );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KMAIL_EXPORT KMMailtoAddAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoAddAddrBookCommand( const KUrl &url, QWidget *parent );

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


class KMAIL_EXPORT KMMailtoOpenAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoOpenAddrBookCommand( const KUrl &url, QWidget *parent );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KMAIL_EXPORT KMUrlCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  explicit KMUrlCopyCommand( const KUrl &url, KMMainWidget *mainWidget = 0 );

private:
  virtual Result execute();

  KUrl mUrl;
  KMMainWidget *mMainWidget;
};

class KMAIL_EXPORT KMUrlOpenCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlOpenCommand( const KUrl &url, KMReaderWin *readerWin );

private:
  virtual Result execute();

  KUrl mUrl;
  KMReaderWin *mReaderWin;
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
  KMEditMsgCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMUseTemplateCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUseTemplateCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMShowMsgSrcCommand : public KMCommand
{
  Q_OBJECT

public:
  KMShowMsgSrcCommand( QWidget *parent, KMMessage *msg, bool fixedFont );
  virtual Result execute();

private:
  bool mFixedFont;
  bool mMsgWasComplete;
};

class KMAIL_EXPORT KMSaveMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSaveMsgCommand( QWidget *parent, const QList<KMMsgBase*> &msgList );
  KMSaveMsgCommand( QWidget *parent, KMMessage * msg );
  KUrl url();

private:
  virtual Result execute();

private slots:
  void slotSaveDataReq();
  void slotSaveResult(KJob *job);
  /** the message has been transferred for saving */
  void slotMessageRetrievedForSaving(KMMessage *msg);

private:
  static const int MAX_CHUNK_SIZE = 64*1024;
  KUrl mUrl;
  QList<unsigned long> mMsgList;
  unsigned int mMsgListIndex;
  KMMessage *mStandAloneMessage;
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
  DwString mMsgString;
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
  KMSaveAttachmentsCommand( QWidget *parent, KMMessage *msg  );
  /** Use this to save all attachments of the given messages.
      @param parent  The parent widget of the command used for message boxes.
      @param msgs    The messages of which the attachments should be saved.
   */
  KMSaveAttachmentsCommand( QWidget *parent, const QList<KMMsgBase*>& msgs );
  /** Use this to save the specified attachments of the given message.
      @param parent       The parent widget of the command used for message
                          boxes.
      @param attachments  The attachments that should be saved.
      @param msg          The message that the attachments belong to.
      @param encoded      True if the transport encoding should not be removed
                          when the attachment is saved.
   */
  KMSaveAttachmentsCommand( QWidget *parent, QList<partNode*> &attachments,
                            KMMessage *msg, bool encoded = false  );

private slots:
  void slotSaveAll();

private:
  virtual Result execute();
  Result saveItem( partNode *node, const KUrl& url );

private:
  PartNodeMessageMap mAttachmentMap;
  bool mImplicitAttachments;
  bool mEncoded;
};

class KMAIL_EXPORT KMReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToCommand( QWidget *parent, KMMessage *msg,
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
  KMNoQuoteReplyToCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMReplyListCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyListCommand( QWidget *parent, KMMessage *msg,
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
  KMReplyToAllCommand( QWidget *parent, KMMessage *msg,
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
  KMReplyAuthorCommand( QWidget *parent, KMMessage *msg,
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
  KMForwardCommand( QWidget *parent, const QList<KMMsgBase*> &msgList,
                    uint identity = 0 );
  KMForwardCommand( QWidget *parent, KMMessage * msg,
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
  KMForwardAttachedCommand( QWidget *parent, const QList<KMMsgBase*> &msgList,
                            uint identity = 0, KMail::Composer *win = 0 );
  KMForwardAttachedCommand( QWidget *parent, KMMessage * msg,
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
  KMRedirectCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMCustomReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCustomReplyToCommand( QWidget *parent, KMMessage *msg,
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
  KMCustomReplyAllToCommand( QWidget *parent, KMMessage *msg,
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
  KMCustomForwardCommand( QWidget *parent, const QList<KMMsgBase *> &msgList,
                          uint identity, const QString &tmpl );
  KMCustomForwardCommand( QWidget *parent, KMMessage * msg,
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
  KMPrintCommand( QWidget *parent, KMMessage *msg,
                  const KMail::HeaderStyle *headerStyle = 0,
                  const KMail::HeaderStrategy *headerStrategy = 0,
                  bool htmlOverride = false,
                  bool htmlLoadExtOverride = false,
                  bool useFixedFont = false,
                  const QString & encoding = QString() );

  void setOverrideFont( const QFont& );

private:
  virtual Result execute();

  const KMail::HeaderStyle *mHeaderStyle;
  const KMail::HeaderStrategy *mHeaderStrategy;
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
  KMSetStatusCommand( const MessageStatus& status, const QList<quint32> &,
                      bool toggle=false );

private:
  virtual Result execute();

  MessageStatus mStatus;
  QList<quint32> mSerNums;
  QList<int> mIds;
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

  KMSetTagCommand( const QString &tagLabel, const QList< unsigned long > &serNums,
                   SetTagMode mode=AddIfNotExisting );

private:
  virtual Result execute();

  QString mTagLabel;
  QList< unsigned long > mSerNums;
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
                         const QList<KMMsgBase*> &msgList, KMFilter *filter );

private:
  virtual Result execute();
  QList<quint32> serNumList;
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
  FolderShortcutCommand( KMMainWidget* mainwidget, KMFolder *folder );
  ~FolderShortcutCommand();

public slots:
  void start();
  /** Assign a KActio to the command which is used to trigger it. This
   * action will be deleted along with the command, so you don't need to
   * keep track of it separately. */
  void setAction( QAction* );

private:
  KMMainWidget *mMainWidget;
  KMFolder *mFolder;
  QAction *mAction;
};


class KMAIL_EXPORT KMMailingListFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailingListFilterCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};


class KMAIL_EXPORT KMCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCopyCommand( KMFolder* destFolder, const QList<KMMsgBase*> &msgList );
  KMCopyCommand( KMFolder* destFolder, KMMessage *msg );

protected slots:
  void slotJobFinished( KMail::FolderJob *job );

  void slotFolderComplete( KMFolderImap*, bool success );

private:
  virtual Result execute();

  KMFolder *mDestFolder;
  QList<KMMsgBase*> mMsgList;
  QList<KMail::FolderJob*> mPendingJobs;
};

namespace KPIM {
  class ProgressItem;
}
class KMAIL_EXPORT KMMoveCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMoveCommand( KMFolder* destFolder, const QList<KMMsgBase*> &msgList );
  KMMoveCommand( KMFolder* destFolder, KMMessage * msg );
  KMMoveCommand( KMFolder* destFolder, KMMsgBase * msgBase );
  KMFolder* destFolder() const { return mDestFolder; }

public slots:
  void slotImapFolderCompleted(KMFolderImap *folder, bool success);
  void slotMsgAddedToDestFolder(KMFolder *folder, quint32 serNum);
  void slotMoveCanceled();

protected:
  // Needed for KMDeleteCommand for "move to trash"
  KMMoveCommand( quint32 sernum );
  void setDestFolder( KMFolder* folder ) { mDestFolder = folder; }
  void addMsg( KMMsgBase *msg ) { mSerNumList.append( msg->getMsgSerNum() ); }
  QVector<KMFolder*> mOpenedFolders;

private:
  virtual Result execute();
  void completeMove( Result result );

  KMFolder *mDestFolder;
  QList<quint32> mSerNumList;
  // List of serial numbers that have to be transferred to a host.
  // Ticked off as they come in via msgAdded signals.
  QList<quint32> mLostBoys;
  KPIM::ProgressItem *mProgressItem;
  bool mCompleteWithAddedMsg;
};

class KMAIL_EXPORT KMTrashMsgCommand : public KMMoveCommand
{
  Q_OBJECT

public:
  KMTrashMsgCommand( KMFolder* srcFolder, const QList<KMMsgBase*> &msgList );
  KMTrashMsgCommand( KMFolder* srcFolder, KMMessage * msg );
  KMTrashMsgCommand( quint32 sernum );

private:
  static KMFolder * findTrashFolder( KMFolder * srcFolder );

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

class KMAIL_EXPORT KMLoadPartsCommand : public KMCommand
{
  Q_OBJECT

public:
  KMLoadPartsCommand( QList<partNode*>& parts, KMMessage* msg );
  KMLoadPartsCommand( partNode* node, KMMessage* msg );
  KMLoadPartsCommand( PartNodeMessageMap& partMap );

public slots:
  void slotPartRetrieved( KMMessage* msg, const QString &partSpecifier );

signals:
  void partsRetrieved();

private:
  // Retrieve parts then calls execute
  virtual void slotStart();

  virtual Result execute();

  int mNeedsRetrieval;
  PartNodeMessageMap mPartMap;
};

class KMAIL_EXPORT KMResendMessageCommand : public KMCommand
{
  Q_OBJECT

public:
  explicit KMResendMessageCommand( QWidget *parent, KMMessage *msg=0 );

private:
  virtual Result execute();
};

class KMAIL_EXPORT KMMailingListCommand : public KMCommand
{
  Q_OBJECT
public:
  KMMailingListCommand( QWidget *parent, KMFolder *parentFolder );
private:
  virtual Result execute();
private slots:
  void commandCompleted( KMCommand *command );
protected:
  virtual KUrl::List urls() const =0;
protected:
  KMFolder *mFolder;
};

class KMAIL_EXPORT KMMailingListPostCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListPostCommand( QWidget *parent, KMFolder *parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListSubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListSubscribeCommand( QWidget *parent, KMFolder *parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListUnsubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListUnsubscribeCommand( QWidget *parent, KMFolder *parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListArchivesCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListArchivesCommand( QWidget *parent, KMFolder *parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMMailingListHelpCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListHelpCommand( QWidget *parent, KMFolder *parentFolder );
protected:
  virtual KUrl::List urls() const;
};

class KMAIL_EXPORT KMHandleAttachmentCommand : public KMCommand
{
  Q_OBJECT

public:
  /** Supported types of attachment handling */
  enum AttachmentAction
  {
    Open = 1,
    OpenWith = 2,
    View = 3,
    Save = 4,
    Properties = 5,
    ChiasmusEncrypt = 6,
    Delete = 7,
    Edit = 8,
    Copy = 9,
    ScrollTo = 10
  };
  /**
   * Construct a new command
   * @param node the partNode
   * @param msg the KMMessage
   * @param atmId the ID of the attachment, the partNode must know this
   * @param atmName the name of the attachment
   * @param action what to do with the attachment
   * @param offer specify a KService that should handle the "open" action, 0 otherwise
   */
  KMHandleAttachmentCommand( partNode* node, KMMessage* msg, int atmId,
      const QString& atmName, AttachmentAction action, KService::Ptr offer, QWidget* parent );


signals:
  void showAttachment( int id, const QString& name );

private:
  virtual Result execute();

  QString createAtmFileLink() const;

  /** Get a KService if it was not specified */
  KService::Ptr getServiceOffer();

  /** Open needs a valid KService */
  void atmOpen();

  /** Display an open-with dialog */
  void atmOpenWith();

  /**
   * Viewing is not supported by this command
   * so it just emits showAttachment
   */
  void atmView();

  /** Save the attachment */
  void atmSave();

  /** Display the properties */
  void atmProperties();

  /** Encrypt using chiasmus */
  void atmEncryptWithChiasmus();

private slots:
  /** Called from start() via a single shot timer. */
  virtual void slotStart();

  /**
   * Called when the part was downloaded
   * Calls execute
   */
  void slotPartComplete();

  void slotAtmDecryptWithChiasmusResult( const GpgME::Error &, const QVariant & );
  void slotAtmDecryptWithChiasmusUploadResult( KJob * );

private:
  partNode* mNode;
  KMMessage* mMsg;
  int mAtmId;
  QString mAtmName;
  AttachmentAction mAction;
  KService::Ptr mOffer;
  Kleo::SpecialJob *mJob;

};


/** Base class for commands modifying attachements of existing messages. */
class KMAIL_EXPORT AttachmentModifyCommand : public KMCommand
{
  Q_OBJECT
  public:
    AttachmentModifyCommand( partNode *node, KMMessage *msg, QWidget *parent );
    AttachmentModifyCommand( int nodeId, KMMessage *msg, QWidget *parent );
    ~AttachmentModifyCommand();

  protected:
    void storeChangedMessage( KMMessage* msg );
    virtual Result doAttachmentModify() = 0;

  protected:
    int mPartIndex;
    quint32 mSernum;

  private:
    Result execute();

  private slots:
    void messageStoreResult( KMFolderImap* folder, bool success );
    void messageTrashResult( KMCommand *cmd );

  private:
    QPointer<KMFolder> mFolder;
};

class KMAIL_EXPORT KMDeleteAttachmentCommand : public AttachmentModifyCommand
{
  Q_OBJECT
  public:
    KMDeleteAttachmentCommand( partNode *node, KMMessage *msg, QWidget *parent );
    KMDeleteAttachmentCommand( int nodeId, KMMessage *msg, QWidget *parent );
    ~KMDeleteAttachmentCommand();

  protected:
    Result doAttachmentModify();
};


class KMAIL_EXPORT KMEditAttachmentCommand : public AttachmentModifyCommand
{
  Q_OBJECT
  public:
    KMEditAttachmentCommand( partNode *node, KMMessage *msg, QWidget *parent );
    KMEditAttachmentCommand( int nodeId, KMMessage *msg, QWidget *parent );
    ~KMEditAttachmentCommand();

  protected:
    Result doAttachmentModify();

  private slots:
    void editDone( KMail::EditorWatcher *watcher );

  private:
    KTemporaryFile mTempFile;
};

class KMAIL_EXPORT CreateTodoCommand : public KMCommand
{
  Q_OBJECT
  public:
    CreateTodoCommand( QWidget *parent, KMMessage *msg );

  private:
    Result execute();
};

#endif /*KMCommands_h*/
