#ifndef KMCommands_h
#define KMCommands_h

#include <qptrlist.h>
#include <kio/job.h>
#include "kmmsgbase.h" // for KMMsgStatus

class QPopupMenu;
class QTextCodec;
class KIO::Job;
class KMainWindow;
class KProgressDialog;
class KMFolder;
class KMFolderNode;
class KMHeaders;
class KMMessage;
class KMMsgBase;
class KMMainWin;
class KMReaderWin;
typedef QMap<int,KMFolder*> KMMenuToFolder;

class KMCommand : public QObject
{
  Q_OBJECT

public:
  // Trival constructor, don't retrieve any messages
  KMCommand( QWidget *parent = 0 );
  // Retrieve all messages in msgList when start is called.
  KMCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList, 
		  KMFolder *folder );
  // Retrieve the single message msgBase when start is called.
  KMCommand( QWidget *parent, KMMsgBase *msgBase );
  virtual ~KMCommand();
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
  /** the transfer was cancelled */
  void slotTransferCancelled();

signals:
  void messagesTransfered(bool);

private:
  // ProgressDialog for transfering messages
  KProgressDialog* mProgressDialog;
  //Currently only one async command allowed at a time
  static int mCountJobs;
  int mCountMsgs;

  QWidget *mParent;
  QPtrList<KMMessage> mRetrievedMsgs;
  QPtrList<KMMsgBase> mMsgList;
  KMFolder *mFolder;
};

class KMMailtoComposeCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoComposeCommand( const KURL &url, KMMsgBase *msgBase = 0 );

private:
  virtual void execute();

  KURL mUrl;
  KMMsgBase *mMsgBase;
};

class KMMailtoReplyCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoReplyCommand( KMainWindow *parent, const KURL &url,
			KMMsgBase *msgBase, const QString &selection );

private:
  virtual void execute();

  KURL mUrl;
  QString mSelection;
};

class KMMailtoForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailtoForwardCommand( KMainWindow *parent, const KURL &url, 
			  KMMsgBase *msgBase );

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
  KMUrlCopyCommand( const KURL &url, KMMainWin *mainWin = 0 );

private:
  virtual void execute();

  KURL mUrl;
  KMMainWin *mMainWin;
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
  KMEditMsgCommand( KMainWindow *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMShowMsgSrcCommand : public KMCommand
{
  Q_OBJECT

public:
  KMShowMsgSrcCommand( KMainWindow *parent, KMMsgBase *msgBase, 
		       const QTextCodec *codec, bool fixedFont );
  virtual void execute();

private:
  const QTextCodec *mCodec;
  bool mFixedFont;
};

class KMSaveMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSaveMsgCommand( KMainWindow *parent, const QPtrList<KMMsgBase> &msgList, 
		    KMFolder *folder );
  KURL url();

private:
  virtual void execute();

private:
  KURL mUrl;
};

class KMReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToCommand( KMainWindow *parent, KMMsgBase *msgBase, 
		    const QString &selection );

private:
  virtual void execute();

private:
  QString mSelection;
};

class KMNoQuoteReplyToCommand : public KMCommand
{
  Q_OBJECT

public:
  KMNoQuoteReplyToCommand( KMainWindow *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMReplyListCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyListCommand( KMainWindow *parent, KMMsgBase *msgBase, 
		      const QString &selection );

private:
  virtual void execute();

private:
  QString mSelection;
};

class KMReplyToAllCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyToAllCommand( KMainWindow *parent, KMMsgBase *msgBase, 
		       const QString &selection );

private:
  virtual void execute();

private:
  QString mSelection;
};

class KMForwardCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardCommand( KMainWindow *parent, const QPtrList<KMMsgBase> &msgList, 
		    KMFolder *folder );

private:
  virtual void execute();

private:
  QWidget *mParent;
  KMFolder *mFolder;
};

class KMForwardAttachedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMForwardAttachedCommand( KMainWindow *parent, 
			    const QPtrList<KMMsgBase> &msgList, KMFolder *folder );

private:
  virtual void execute();

  KMFolder *mFolder;
};

class KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( KMainWindow *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMBounceCommand : public KMCommand
{
  Q_OBJECT

public:
  KMBounceCommand( KMainWindow *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMToggleFixedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMToggleFixedCommand( KMReaderWin *readerWin );

private:
  virtual void execute();

  KMReaderWin *mReaderWin;
};

class KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( KMainWindow *parent, KMMsgBase *msgBase, bool htmlOverride );

private:
  virtual void execute();

  bool mHtmlOverride;
};

class KMSetStatusCommand : public KMCommand
{
  Q_OBJECT

public:
  KMSetStatusCommand( KMMsgStatus status, const QValueList<int> &ids, 
		      KMFolder *folder );

private:
  virtual void execute();

  KMMsgStatus mStatus;
  QValueList<int> mIds;
  KMFolder *mFolder;
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


class KMMailingListFilterCommand : public KMCommand
{
  Q_OBJECT

public:
  KMMailingListFilterCommand( KMainWindow *parent, KMMsgBase *msgBase );

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
  KMCopyCommand( KMFolder* srcFolder, KMFolder* destFolder, 
		 const QPtrList<KMMsgBase> &msgList );

private:
  virtual void execute();

  KMFolder *mSrcFolder, *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
};

class KMMoveCommand : public KMMenuCommand
{
  Q_OBJECT

public:
  KMMoveCommand( KMFolder* srcFolder, KMFolder* destFolder, 
		 const QPtrList<KMMsgBase> &msgList, KMHeaders* headers = 0 );
		 

private:
  virtual void execute();

  KMFolder *mSrcFolder, *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
  KMHeaders *mHeaders;
};

class KMDeleteMsgCommand : public KMCommand
{
  Q_OBJECT

public:
  KMDeleteMsgCommand( KMFolder* srcFolder, const QPtrList<KMMsgBase> &msgList, 
		      KMHeaders* headers = 0 );
		      
  virtual void execute();
};

class KMUrlClickedCommand : public KMCommand
{
  Q_OBJECT

public:
  KMUrlClickedCommand( const KURL &url, KMFolder* folder, 
    KMReaderWin *readerWin, bool mHtmlPref, KMMainWin *mainWin = 0 );

private:
  virtual void execute();

  KURL mUrl;
  KMFolder *mFolder;
  KMReaderWin *mReaderWin;
  bool mHtmlPref;
  KMMainWin *mMainWin;
};

#endif /*KMCommands_h*/
