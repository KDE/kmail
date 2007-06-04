// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMCommands_h
#define KMCommands_h

#include <mimelib/string.h>
#include <messagestatus.h>
using KPIM::MessageStatus;
#include <kdemacros.h>
#include <kservice.h>
#include <kio/job.h>

#include <QPointer>
#include <QList>

#include <QMenu>

class KProgressDialog;
class KMFilter;
class KMFolder;
class KMFolderImap;
class KMHeaders;
class KMMainWidget;
class KMMessage;
class KMMsgBase;
class KMReaderWin;
class partNode;

namespace KIO { class Job; }
namespace KMail {
  class Composer;
  class FolderJob;
}
namespace GpgME { class Error; }
namespace Kleo { class SpecialJob; }

typedef QMap<QAction*,KMFolder*> KMMenuToFolder;
typedef QMap<partNode*, KMMessage*> PartNodeMessageMap;

class KDE_EXPORT KMCommand : public QObject
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
  Result result();

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

  bool deletesItself() { return mDeletesItself; }
  /** Specify whether the subclass takes care of the deletion of the object.
      By default the base class will delete the object.
      @param deletesItself true if the subclass takes care of deletion, false
                           if the base class should take care of deletion
  */
  void setDeletesItself( bool deletesItself )
  { mDeletesItself = deletesItself; }

  bool emitsCompletedItself() { return mEmitsCompletedItself; }
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

class KDE_EXPORT KMMailtoComposeCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoComposeCommand( const KUrl &url, KMMessage *msg=0 );

private:
  virtual Result execute();

  KUrl mUrl;
  KMMessage *mMessage;
};

class KDE_EXPORT KMMailtoReplyCommand : public KMCommand
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

class KDE_EXPORT KMMailtoForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoForwardCommand( QWidget *parent, const KUrl &url,
			  KMMessage *msg );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KDE_EXPORT KMMailtoAddAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoAddAddrBookCommand( const KUrl &url, QWidget *parent );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KDE_EXPORT KMAddBookmarksCommand : public KMCommand
{
  Q_OBJECT

public:
  KMAddBookmarksCommand( const KUrl &url, QWidget *parent );

private:
  virtual Result execute();

  KUrl mUrl;
};


class KDE_EXPORT KMMailtoOpenAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoOpenAddrBookCommand( const KUrl &url, QWidget *parent );

private:
  virtual Result execute();

  KUrl mUrl;
};

class KDE_EXPORT KMUrlCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlCopyCommand( const KUrl &url, KMMainWidget *mainWidget = 0 );

private:
  virtual Result execute();

  KUrl mUrl;
  KMMainWidget *mMainWidget;
};

class KDE_EXPORT KMUrlOpenCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlOpenCommand( const KUrl &url, KMReaderWin *readerWin );

private:
  virtual Result execute();

  KUrl mUrl;
  KMReaderWin *mReaderWin;
};

class KDE_EXPORT KMUrlSaveCommand : public KMCommand
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

class KDE_EXPORT KMEditMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMEditMsgCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KDE_EXPORT KMUseTemplateCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUseTemplateCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KDE_EXPORT KMShowMsgSrcCommand : public KMCommand
{
  Q_OBJECT

public:
  KMShowMsgSrcCommand( QWidget *parent, KMMessage *msg,
		       bool fixedFont );
  virtual Result execute();

private:
  bool mFixedFont;
  bool mMsgWasComplete;
};

class KDE_EXPORT KMSaveMsgCommand : public KMCommand
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

class KDE_EXPORT KMOpenMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMOpenMsgCommand( QWidget *parent, const KUrl & url = KUrl(),
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

class KDE_EXPORT KMSaveAttachmentsCommand : public KMCommand
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

class KDE_EXPORT KMReplyToCommand : public KMCommand
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

class KDE_EXPORT KMNoQuoteReplyToCommand : public KMCommand
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

class KDE_EXPORT KMReplyToAllCommand : public KMCommand
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

class KDE_EXPORT KMReplyAuthorCommand : public KMCommand
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

class KDE_EXPORT KMForwardCommand : public KMCommand
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

class KDE_EXPORT KMForwardAttachedCommand : public KMCommand
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

class KDE_EXPORT KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KDE_EXPORT KMCustomReplyToCommand : public KMCommand
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

class KDE_EXPORT KMCustomReplyAllToCommand : public KMCommand
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

class KDE_EXPORT KMCustomForwardCommand : public KMCommand
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

class KDE_EXPORT KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( QWidget *parent, KMMessage *msg,
                  bool htmlOverride=false,
                  bool htmlLoadExtOverride=false,
                  bool useFixedFont = false,
                  const QString & encoding = QString() );

private:
  virtual Result execute();

  bool mHtmlOverride;
  bool mHtmlLoadExtOverride;
  bool mUseFixedFont;
  QString mEncoding;
};

class KDE_EXPORT KMSetStatusCommand : public KMCommand
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


/* This command is used to create a filter based on the user's
    decision, e.g. filter by From header */
class KDE_EXPORT KMFilterCommand : public KMCommand
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
class KDE_EXPORT KMFilterActionCommand : public KMCommand
{
  Q_OBJECT

public:
  KMFilterActionCommand( QWidget *parent,
			 const QList<KMMsgBase*> &msgList,
			 KMFilter *filter );

private:
  virtual Result execute();
  QList<quint32> serNumList;
  KMFilter *mFilter;
};


class KDE_EXPORT KMMetaFilterActionCommand : public QObject
{
  Q_OBJECT

public:
  KMMetaFilterActionCommand( KMFilter *filter, KMHeaders *headers,
			     KMMainWidget *main );

public slots:
  void start();

private:
  KMFilter *mFilter;
  KMHeaders *mHeaders;
  KMMainWidget *mMainWidget;
};

class KDE_EXPORT FolderShortcutCommand : public QObject
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


class KDE_EXPORT KMMailingListFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailingListFilterCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};


class KDE_EXPORT KMCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMCopyCommand( KMFolder* destFolder,
		 const QList<KMMsgBase*> &msgList );
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
class KDE_EXPORT KMMoveCommand : public KMCommand
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
  void addMsg( KMMsgBase *msg ) { mMsgList.append( msg ); }
  QVector<KMFolder*> mOpenedFolders;

private:
  virtual Result execute();
  void completeMove( Result result );

  KMFolder *mDestFolder;
  QList<KMMsgBase*> mMsgList;
  // List of serial numbers that have to be transferred to a host.
  // Ticked off as they come in via msgAdded signals.
  QList<quint32> mLostBoys;
  KPIM::ProgressItem *mProgressItem;
  bool mCompleteWithAddedMsg;
};

class KDE_EXPORT KMDeleteMsgCommand : public KMMoveCommand
{
  Q_OBJECT

public:
  KMDeleteMsgCommand( KMFolder* srcFolder, const QList<KMMsgBase*> &msgList );
  KMDeleteMsgCommand( KMFolder* srcFolder, KMMessage * msg );
  KMDeleteMsgCommand( quint32 sernum );

private:
  static KMFolder * findTrashFolder( KMFolder * srcFolder );

};

class KDE_EXPORT KMUrlClickedCommand : public KMCommand
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

class KDE_EXPORT KMLoadPartsCommand : public KMCommand
{
  Q_OBJECT

public:
  KMLoadPartsCommand( QList<partNode*>& parts, KMMessage* msg );
  KMLoadPartsCommand( partNode* node, KMMessage* msg );
  KMLoadPartsCommand( PartNodeMessageMap& partMap );

public slots:
  void slotPartRetrieved( KMMessage* msg, QString partSpecifier );

signals:
  void partsRetrieved();

private:
  // Retrieve parts then calls execute
  virtual void slotStart();

  virtual Result execute();

  int mNeedsRetrieval;
  PartNodeMessageMap mPartMap;
};

class KDE_EXPORT KMResendMessageCommand : public KMCommand
{
  Q_OBJECT

public:
  KMResendMessageCommand( QWidget *parent, KMMessage *msg=0 );

private:
  virtual Result execute();
};

class KDE_EXPORT KMMailingListCommand : public KMCommand
{
  Q_OBJECT
public:
  KMMailingListCommand( QWidget *parent, KMFolder *parent );
private:
  virtual Result execute();
private slots:
  void commandCompleted( KMCommand *command );
protected:
  virtual KUrl::List urls() const =0;
protected:
  KMFolder *mFolder;
};

class KDE_EXPORT KMMailingListPostCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListPostCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KUrl::List urls() const;
};

class KDE_EXPORT KMMailingListSubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListSubscribeCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KUrl::List urls() const;
};

class KDE_EXPORT KMMailingListUnsubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListUnsubscribeCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KUrl::List urls() const;
};

class KDE_EXPORT KMMailingListArchivesCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListArchivesCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KUrl::List urls() const;
};

class KDE_EXPORT KMMailingListHelpCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListHelpCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KUrl::List urls() const;
};

class KDE_EXPORT KMIMChatCommand : public KMCommand
{
  Q_OBJECT

public:
  KMIMChatCommand( const KUrl &url, KMMessage *msg=0 );

private:
  /**
   * Try to start a chat with the addressee associated the mail address in <i>url</i>.
   * If there is no addressee for the email address, or more than one, a KMessageBox is shown to inform the user.
   * If the addressee does not have instant messaging address(es), this is currently handled by the instant messaging client
   * which handles the request, since we don't have a convenient API for extracting them using KABC.
   */
  virtual Result execute();

  KUrl mUrl;
  KMMessage *mMessage;
};

class KDE_EXPORT KMHandleAttachmentCommand : public KMCommand
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
    ChiasmusEncrypt = 6
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
  const QString& mAtmName;
  AttachmentAction mAction;
  KService::Ptr mOffer;
  Kleo::SpecialJob *mJob;

};

#endif /*KMCommands_h*/
