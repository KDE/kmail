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
class KMFolderNode;
class KMHeaders;
class KMMainWidget;
class KMMessage;
class KMMsgBase;
class KMReaderWin;
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
  virtual ~KMCommand();

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
  QValueList<QGuardedPtr<KMFolder> > mFolders;
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
  KMMailtoReplyCommand( QWidget *parent, const KURL &url,
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
  KMMailtoForwardCommand( QWidget *parent, const KURL &url,
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
  KMEditMsgCommand( QWidget *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMShowMsgSrcCommand : public KMCommand
{
  Q_OBJECT

public:
  KMShowMsgSrcCommand( QWidget *parent, KMMsgBase *msgBase,
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
  KMSaveMsgCommand( QWidget *parent, const QPtrList<KMMsgBase> &msgList );
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
  KMReplyToCommand( QWidget *parent, KMMsgBase *msgBase,
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
  KMNoQuoteReplyToCommand( QWidget *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMReplyListCommand : public KMCommand
{
  Q_OBJECT

public:
  KMReplyListCommand( QWidget *parent, KMMsgBase *msgBase,
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
  KMReplyToAllCommand( QWidget *parent, KMMsgBase *msgBase,
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

private:
  virtual void execute();

  uint mIdentity;
  QGuardedPtr<KMComposeWin> mWin;
};

class KMRedirectCommand : public KMCommand
{
  Q_OBJECT

public:
  KMRedirectCommand( QWidget *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMBounceCommand : public KMCommand
{
  Q_OBJECT

public:
  KMBounceCommand( QWidget *parent, KMMsgBase *msgBase );

private:
  virtual void execute();
};

class KMPrintCommand : public KMCommand
{
  Q_OBJECT

public:
  KMPrintCommand( QWidget *parent, KMMsgBase *msgBase, bool htmlOverride=false );

private:
  virtual void execute();

  bool mHtmlOverride;
};

class KMSetStatusCommand : public KMCommand
{
  Q_OBJECT

public:
  // Serial numbers
  KMSetStatusCommand( KMMsgStatus status, const QValueList<Q_UINT32> & );

private:
  virtual void execute();

  KMMsgStatus mStatus;
  QValueList<Q_UINT32> mSerNums;
  QValueList<int> mIds;
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
  KMMailingListFilterCommand( QWidget *parent, KMMsgBase *msgBase );

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

private:
  virtual void execute();

  KMFolder *mDestFolder;
  QPtrList<KMMsgBase> mMsgList;
};

class KMMoveCommand : public KMMenuCommand
{
  Q_OBJECT

public:
  KMMoveCommand( KMFolder* destFolder, const QPtrList<KMMsgBase> &msgList,
		 KMHeaders* headers = 0 );


private:
  virtual void execute();

  KMFolder *mDestFolder;
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

#endif /*KMCommands_h*/
