#ifndef KMCommands_h
#define KMCommands_h

#include <qguardedptr.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <kio/job.h>
#include "kmmsgbase.h" // for KMMsgStatus

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

class KMCommand : public QObject
{
  Q_OBJECT

public:
  // Trival constructor, don't retrieve any messages
  KMCommand( QWidget *parent = 0 );
  // Retrieve all messages in msgList when start is called.
  KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
  // Retrieve the single message msgBase when start is called.
  KMCommand( QWidget *parent, KMMsgBase *msgBase );
  // Retrieve the single message msgBase when start is called.
  KMCommand( QWidget *parent, KMMessage *message );
  virtual ~KMCommand();

  bool deletesItself () { return mDeletesItself; }
  void setDeletesItself( bool deletesItself )
  { mDeletesItself = deletesItself; }


public slots:
  // Retrieve messages then calls execute
  void start();

protected:
  // Returns list of messages retrieved
  const QPtrList<KMMessage> retrievedMsgs() const;
  // Returns the single message retrieved
  KMMessage *retrievedMessage() const;

private:
  // execute should be implemented by derived classes
  virtual void execute() = 0;

  void preTransfer();

  /** transfers the list of (imap)-messages
   *  this is a necessary preparation for e.g. forwarding */
  void transferSelectedMsgs();

private slots:
  void slotPostTransfer(bool);
  /** the msg has been transferred */
  void slotMsgTransfered(KMMessage* msg);
  /** the KMImapJob is finished */
  void slotJobFinished();
  /** the transfer was canceled */
  void slotTransferCancelled();
signals:
  void messagesTransfered(bool);
  /** Emitted when the command has completed.
   * @success Success or error. */
  void completed( bool success);

private:
  // ProgressDialog for transferring messages
  KProgressDialog* mProgressDialog;
  //Currently only one async command allowed at a time
  static int mCountJobs;
  int mCountMsgs;
  bool mDeletesItself;

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
  virtual void execute();

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
  virtual void execute();

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
  virtual void execute();

  KURL mUrl;
};

class KMMailtoAddAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoAddAddrBookCommand( const KURL &url, QWidget *parent );

private:
  virtual void execute();

  KURL mUrl;
  QWidget *mParent;
};

class KMAddBookmarksCommand : public KMCommand
{
  Q_OBJECT

public:
  KMAddBookmarksCommand( const KURL &url, QWidget *parent );

private:
  virtual void execute();

  KURL mUrl;
  QWidget *mParent;
};


class KMMailtoOpenAddrBookCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoOpenAddrBookCommand( const KURL &url, QWidget *parent );

private:
  virtual void execute();

  KURL mUrl;
  QWidget *mParent;
};

class KMUrlCopyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlCopyCommand( const KURL &url, KMMainWidget *mainWidget = 0 );

private:
  virtual void execute();

  KURL mUrl;
  KMMainWidget *mMainWidget;
};

class KMUrlOpenCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlOpenCommand( const KURL &url, KMReaderWin *readerWin );

private:
  virtual void execute();

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
  virtual void execute();

  KURL mUrl;
  QWidget *mParent;
};

class KMEditMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMEditMsgCommand( QWidget *parent, KMMessage *msg );

private:
  virtual void execute();
};

class KMShowMsgSrcCommand : public KMCommand
{
  Q_OBJECT

public:
  KMShowMsgSrcCommand( QWidget *parent, KMMessage *msg,
		       bool fixedFont );
  virtual void execute();

private:
  bool mFixedFont;
};

class KMSaveMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSaveMsgCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
  KMSaveMsgCommand( QWidget *parent, KMMessage * msg );
  KURL url();

private:
  virtual void execute();

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

class KMSaveAttachmentsCommand : public KMCommand
{
  Q_OBJECT
public:
  KMSaveAttachmentsCommand( QWidget *parent, KMMessage *msg  );
  KMSaveAttachmentsCommand( QWidget *parent, const QPtrList<KMMsgBase>& msgs );
  KMSaveAttachmentsCommand( QWidget *parent, QPtrList<partNode> &attachments, 
      KMMessage *msg, bool encoded = false  );

protected slots:
  void slotSaveAll();

private:
  virtual void execute();
private:
  void parse( partNode *rootNode );
  void saveAll( const QPtrList<partNode>& attachments );
  void saveItem( partNode *node, const QString& filename );
private:
  QWidget *mParent;
  QPtrList<partNode> mAttachments;
  bool mEncoded;
};

class KMReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToCommand( QWidget *parent, KMMessage *msg,
		    const QString &selection = QString::null );

private:
  virtual void execute();

private:
  QString mSelection;
};

class KMNoQuoteReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMNoQuoteReplyToCommand( QWidget *parent, KMMessage *msg );

private:
  virtual void execute();
};

class KMReplyListCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyListCommand( QWidget *parent, KMMessage *msg,
		      const QString &selection = QString::null );

private:
  virtual void execute();

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
  virtual void execute();

private:
  QString mSelection;
};

class KMForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
  KMForwardCommand( QWidget *parent, KMMessage * msg );

private:
  virtual void execute();

private:
  QWidget *mParent;
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
  virtual void execute();

  uint mIdentity;
  QGuardedPtr<KMComposeWin> mWin;
};

class KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( QWidget *parent, KMMessage *msg );

private:
  virtual void execute();
};

class KMBounceCommand : public KMCommand
{
  Q_OBJECT

public:
  KMBounceCommand( QWidget *parent, KMMessage *msg );

private:
  virtual void execute();
};

class KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( QWidget *parent, KMMessage *msg, bool htmlOverride=false );

private:
  virtual void execute();

  bool mHtmlOverride;
};

class KMSetStatusCommand : public KMCommand
{
  Q_OBJECT

public:
  // Serial numbers
  KMSetStatusCommand( KMMsgStatus status, const QValueList<Q_UINT32> &,
                      bool toggle=false );

private:
  virtual void execute();

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
  virtual void execute();

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
  virtual void execute();

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
  virtual void execute();
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
  static QPopupMenu* folderToPopupMenu(bool move, QObject *receiver,
    KMMenuToFolder *aMenuToFolder, QPopupMenu *menu );

  static QPopupMenu* makeFolderMenu(KMFolderNode* item, bool move,
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
  virtual void execute();

  KMFolder *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
};

class KMMoveCommand : public KMMenuCommand
{
  Q_OBJECT

public:
  KMMoveCommand( KMFolder* destFolder, const QPtrList<KMMsgBase> &msgList );
  KMMoveCommand( KMFolder* destFolder, KMMessage * msg );
  KMMoveCommand( KMFolder* destFolder, KMMsgBase * msgBase );

public slots:
  void slotImapFolderCompleted(KMFolderImap *folder, bool success);
  void slotMsgAddedToDestFolder(KMFolder *folder, Q_UINT32 serNum);

private:
  virtual void execute();

  KMFolder *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
  // List of serial numbers that have to be transferred to a host.
  // Ticked off as they come in via msgAdded signals. 
  QValueList<Q_UINT32> mLostBoys;
};

class KMDeleteMsgCommand : public KMMoveCommand
{
  Q_OBJECT

public:
  KMDeleteMsgCommand( KMFolder* srcFolder, const QPtrList<KMMsgBase> &msgList );
  KMDeleteMsgCommand( KMFolder* srcFolder, KMMessage * msg );

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
  virtual void execute();

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

public slots:
  // Retrieve parts then calls execute
  void start();
  void slotPartRetrieved( KMMessage* msg, QString partSpecifier );  

signals:
  void partsRetrieved();

private:
  virtual void execute();

  QPtrList<partNode> mParts;
  int mNeedsRetrieval;
  KMMessage *mMsg;
};

#endif /*KMCommands_h*/
