// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMCommands_h
#define KMCommands_h

#include <qguardedptr.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <kio/job.h>
#include "kmmsgbase.h" // for KMMsgStatus
#include <mimelib/string.h>

class QPopupMenu;
class QTextCodec;
class KMainWindow;
class KProgressDialog;
class KMComposeWin;
class KMFilter;
class KMFolder;
class KMFolderImap;
class KMFolderNode;
class KMHeaders;
class KMMainWidget;
class KMMessage;
class KMMsgBase;
class KMReaderWin;
class partNode;
namespace KIO { class Job; }

typedef QMap<int,KMFolder*> KMMenuToFolder;
typedef QMap<partNode*, KMMessage*> PartNodeMessageMap;

class KMCommand : public QObject
{
  Q_OBJECT

public:
  enum Result { Undefined, OK, Canceled, Failed };

  // Trival constructor, don't retrieve any messages
  KMCommand( QWidget *parent = 0 );
  // Retrieve all messages in msgList when start is called.
  KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
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
  const QPtrList<KMMessage> retrievedMsgs() const;
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
  QPtrList<KMMessage> mRetrievedMsgs;
  QPtrList<KMMsgBase> mMsgList;
  QValueList<QGuardedPtr<KMFolder> > mFolders;
};

class KMMailtoComposeCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoComposeCommand( const KURL &url, KMMessage *msg=0 );

private:
  virtual Result execute();

  KURL mUrl;
  KMMessage *mMessage;
};

class KMMailtoReplyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoReplyCommand( QWidget *parent, const KURL &url,
			KMMessage *msg, const QString &selection );

private:
  virtual Result execute();

  KURL mUrl;
  QString mSelection;
};

class KMMailtoForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoForwardCommand( QWidget *parent, const KURL &url,
			  KMMessage *msg );

private:
  virtual Result execute();

  KURL mUrl;
};

class KMMailtoAddAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoAddAddrBookCommand( const KURL &url, QWidget *parent );

private:
  virtual Result execute();

  KURL mUrl;
};

class KMAddBookmarksCommand : public KMCommand
{
  Q_OBJECT

public:
  KMAddBookmarksCommand( const KURL &url, QWidget *parent );

private:
  virtual Result execute();

  KURL mUrl;
};


class KMMailtoOpenAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoOpenAddrBookCommand( const KURL &url, QWidget *parent );

private:
  virtual Result execute();

  KURL mUrl;
};

class KMUrlCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlCopyCommand( const KURL &url, KMMainWidget *mainWidget = 0 );

private:
  virtual Result execute();

  KURL mUrl;
  KMMainWidget *mMainWidget;
};

class KMUrlOpenCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlOpenCommand( const KURL &url, KMReaderWin *readerWin );

private:
  virtual Result execute();

  KURL mUrl;
  KMReaderWin *mReaderWin;
};

class KMUrlSaveCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlSaveCommand( const KURL &url, QWidget *parent );

private slots:
  void slotUrlSaveResult( KIO::Job *job );

private:
  virtual Result execute();

  KURL mUrl;
};

namespace KMail {
  class SaveTextAsCommand : public KMCommand {
    Q_OBJECT

  public:
    SaveTextAsCommand( const QString & txt, QWidget * parent );

  private slots:
    void slotResult( KIO::Job * );

  private:
    /*! \rimp */
    Result execute();

  private:
    QString mText;
  };
}

class KMEditMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMEditMsgCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMShowMsgSrcCommand
{
public:
  KMShowMsgSrcCommand( KMMessage *msg, bool fixedFont );
		       
  void start();

private:
  bool mFixedFont;
  KMMessage *mMsg;
};

class KMSaveMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSaveMsgCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
  KMSaveMsgCommand( QWidget *parent, KMMessage * msg );
  KURL url();

private:
  virtual Result execute();

private slots:
  void slotSaveDataReq();
  void slotSaveResult(KIO::Job *job);
  /** the message has been transferred for saving */
  void slotMessageRetrievedForSaving(KMMessage *msg);

private:
  static const int MAX_CHUNK_SIZE = 64*1024;
  KURL mUrl;
  QValueList<unsigned long> mMsgList;
  unsigned int mMsgListIndex;
  QByteArray mData;
  int mOffset;
  size_t mTotalSize;
  KIO::TransferJob *mJob;
};

class KMOpenMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMOpenMsgCommand( QWidget *parent, const KURL & url = KURL() );

private:
  virtual Result execute();

private slots:
  void slotDataArrived( KIO::Job *job, const QByteArray & data );
  void slotResult( KIO::Job *job );

private:
  static const int MAX_CHUNK_SIZE = 64*1024;
  KURL mUrl;
  DwString mMsgString;
  KIO::TransferJob *mJob;
};

class KMSaveAttachmentsCommand : public KMCommand
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
  KMSaveAttachmentsCommand( QWidget *parent, const QPtrList<KMMsgBase>& msgs );
  /** Use this to save the specified attachments of the given message.
      @param parent       The parent widget of the command used for message
                          boxes.
      @param attachments  The attachments that should be saved.
      @param msg          The message that the attachments belong to.
      @param encoded      True if the transport encoding should not be removed
                          when the attachment is saved.
   */
  KMSaveAttachmentsCommand( QWidget *parent, QPtrList<partNode> &attachments,
                            KMMessage *msg, bool encoded = false  );

private slots:
  void slotSaveAll();

private:
  virtual Result execute();
  Result saveItem( partNode *node, const KURL& url );

private:
  PartNodeMessageMap mAttachmentMap;
  bool mImplicitAttachments;
  bool mEncoded;
};

class KMReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToCommand( QWidget *parent, KMMessage *msg,
                    const QString &selection = QString::null );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMNoQuoteReplyToCommand : public KMCommand
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
		      const QString &selection = QString::null );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMReplyToAllCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToAllCommand( QWidget *parent, KMMessage *msg,
		       const QString &selection = QString::null );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMReplyAuthorCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyAuthorCommand( QWidget *parent, KMMessage *msg,
                        const QString &selection = QString::null );

private:
  virtual Result execute();

private:
  QString mSelection;
};

class KMForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList,
                    uint identity = 0 );
  KMForwardCommand( QWidget *parent, KMMessage * msg,
                    uint identity = 0 );

private:
  virtual Result execute();

private:
  uint mIdentity;
};

class KMForwardAttachedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardAttachedCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList,
			    uint identity = 0, KMComposeWin *win = 0 );
  KMForwardAttachedCommand( QWidget *parent, KMMessage * msg,
			    uint identity = 0, KMComposeWin *win = 0 );

private:
  virtual Result execute();

  uint mIdentity;
  QGuardedPtr<KMComposeWin> mWin;
};

class KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMBounceCommand : public KMCommand
{
  Q_OBJECT

public:
  KMBounceCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};

class KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( QWidget *parent, KMMessage *msg, 
                  bool htmlOverride=false, const QTextCodec *codec = 0 );

private:
  virtual Result execute();

  bool mHtmlOverride;
  const QTextCodec *mCodec;
};

class KMSetStatusCommand : public KMCommand
{
  Q_OBJECT

public:
  // Serial numbers
  KMSetStatusCommand( KMMsgStatus status, const QValueList<Q_UINT32> &,
                      bool toggle=false );

private:
  virtual Result execute();

  KMMsgStatus mStatus;
  QValueList<Q_UINT32> mSerNums;
  QValueList<int> mIds;
  bool mToggle;
};

class KMFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMFilterCommand( const QCString &field, const QString &value );

private:
  virtual Result execute();

  QCString mField;
  QString mValue;
};


class KMFilterActionCommand : public KMCommand
{
  Q_OBJECT

public:
  KMFilterActionCommand( QWidget *parent,
			 const QPtrList<KMMsgBase> &msgList,
			 KMFilter *filter );

private:
  virtual Result execute();

  KMFilter *mFilter;
};


class KMMetaFilterActionCommand : public QObject
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


class KMMailingListFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailingListFilterCommand( QWidget *parent, KMMessage *msg );

private:
  virtual Result execute();
};


  /** Returns a popupmenu containing a hierarchy of folder names.
      Each item in the popupmenu is connected to a slot, if
      move is TRUE this slot will cause all selected messages to
      be moved into the given folder, otherwise messages will be
      copied.
      Am empty @ref KMMenuToFolder must be passed in. */

class KMMenuCommand : public KMCommand
{
  Q_OBJECT

public:
  static void folderToPopupMenu(bool move, QObject *receiver,
    KMMenuToFolder *aMenuToFolder, QPopupMenu *menu );

  static void makeFolderMenu(KMFolderNode* item, bool move,
    QObject *receiver, KMMenuToFolder *aMenuToFolder, QPopupMenu *menu );
};

class KMCopyCommand : public KMMenuCommand
{
  Q_OBJECT

public:
  KMCopyCommand( KMFolder* destFolder,
		 const QPtrList<KMMsgBase> &msgList );
  KMCopyCommand( KMFolder* destFolder, KMMessage *msg );

private:
  virtual Result execute();

  KMFolder *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
};

namespace KPIM {
  class ProgressItem;
}
class KMMoveCommand : public KMMenuCommand
{
  Q_OBJECT

public:
  KMMoveCommand( KMFolder* destFolder, const QPtrList<KMMsgBase> &msgList );
  KMMoveCommand( KMFolder* destFolder, KMMessage * msg );
  KMMoveCommand( KMFolder* destFolder, KMMsgBase * msgBase );
  KMFolder* destFolder() const { return mDestFolder; }

public slots:
  void slotImapFolderCompleted(KMFolderImap *folder, bool success);
  void slotMsgAddedToDestFolder(KMFolder *folder, Q_UINT32 serNum);
  void slotMoveCanceled();

protected:
  // Needed for KMDeleteCommand for "move to trash"
  KMMoveCommand( Q_UINT32 sernum );
  void setDestFolder( KMFolder* folder ) { mDestFolder = folder; }
  void addMsg( KMMsgBase *msg ) { mMsgList.append( msg ); }

private:
  virtual Result execute();
  void completeMove( Result result );

  KMFolder *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
  // List of serial numbers that have to be transferred to a host.
  // Ticked off as they come in via msgAdded signals.
  QValueList<Q_UINT32> mLostBoys;
  KPIM::ProgressItem *mProgressItem;
};

class KMDeleteMsgCommand : public KMMoveCommand
{
  Q_OBJECT

public:
  KMDeleteMsgCommand( KMFolder* srcFolder, const QPtrList<KMMsgBase> &msgList );
  KMDeleteMsgCommand( KMFolder* srcFolder, KMMessage * msg );
  KMDeleteMsgCommand( Q_UINT32 sernum );

private:
  static KMFolder * findTrashFolder( KMFolder * srcFolder );

};

class KMUrlClickedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlClickedCommand( const KURL &url, uint identity,
    KMReaderWin *readerWin, bool mHtmlPref, KMMainWidget *mainWidget = 0 );

private:
  virtual Result execute();

  KURL mUrl;
  uint mIdentity;
  KMReaderWin *mReaderWin;
  bool mHtmlPref;
  KMMainWidget *mMainWidget;
};

class KMLoadPartsCommand : public KMCommand
{
  Q_OBJECT

public:
  KMLoadPartsCommand( QPtrList<partNode>& parts, KMMessage* msg );
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

class KMResendMessageCommand : public KMCommand
{
  Q_OBJECT

public:
  KMResendMessageCommand( QWidget *parent, KMMessage *msg=0 );

private:
  virtual Result execute();
};

class KMMailingListCommand : public KMCommand
{
  Q_OBJECT
public:
  KMMailingListCommand( QWidget *parent, KMFolder *parent );
private:
  virtual Result execute();
private slots:
  void commandCompleted( KMCommand *command );
protected:
  virtual KURL::List urls() const =0;
protected:
  KMFolder *mFolder;
};

class KMMailingListPostCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListPostCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KURL::List urls() const;
};

class KMMailingListSubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListSubscribeCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KURL::List urls() const;
};

class KMMailingListUnsubscribeCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListUnsubscribeCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KURL::List urls() const;
};

class KMMailingListArchivesCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListArchivesCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KURL::List urls() const;
};

class KMMailingListHelpCommand : public KMMailingListCommand
{
  Q_OBJECT
public:
  KMMailingListHelpCommand( QWidget *parent, KMFolder *parent );
protected:
  virtual KURL::List urls() const;
};

class KMIMChatCommand : public KMCommand
{
  Q_OBJECT

public:
  KMIMChatCommand( const KURL &url, KMMessage *msg=0 );

private:
  /**
   * Try to start a chat with the addressee associated the mail address in <i>url</i>.
   * If there is no addressee for the email address, or more than one, a KMessageBox is shown to inform the user.
   * If the addressee does not have instant messaging address(es), this is currently handled by the instant messaging client
   * which handles the request, since we don't have a convenient API for extracting them using KABC.
   */
  virtual Result execute();

  KURL mUrl;
  KMMessage *mMessage;
};


#endif /*KMCommands_h*/
