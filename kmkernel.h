// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef _KMCONTROL
#define _KMCONTROL

#include <qobject.h>
#include <qstring.h>
#include <weaver.h>
#include <weaverlogger.h>

#include <kconfig.h>
#include <kdeversion.h>

#include "kmailIface.h"

#define kmkernel KMKernel::self()
#define kmconfig KMKernel::config()

namespace KIO {
  class Job;
}
namespace KMail {
  class MailServiceImpl;
  class UndoStack;
}
using KMail::MailServiceImpl;
using KMail::UndoStack;
class KMMsgIndex;
class QLabel;
class KMFolder;
class KMFolderMgr;
class KMAcctMgr;
class KMFilterMgr;
class KMFilterActionDict;
class KMSender;
class KMIdentity;
class KMKernel;
class KMMsgDict;
class IdentityManager;
class KProcess;
class KProgressDialog;
class CryptPlugWrapperList;
class ConfigureDialog;
class KInstance;
class QTimer;
class KProgress;
class KPassivePopup;
class KMMainWin;
class KMainWindow;
class KMGroupware;
class KMailICalIfaceImpl;
class KMReaderWin;
class KSystemTray;

class KMKernel : public QObject, virtual public KMailIface
{
  Q_OBJECT

public:
  KMKernel (QObject *parent=0, const char *name=0);
  ~KMKernel ();

  /** true if user has requested to expire all messages on exit */
  void setCanExpire(bool expire);
  bool canExpire();

  /** dcop callable stuff */

  void checkMail ();
  QStringList accounts();
  void checkAccount (const QString &account);
  /** returns id of composer if more are opened */
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile, const KURL::List &attachURLs);
  /** For backward compatibility */
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile, const KURL& attachURL)
  {
    return openComposer(to, cc, bcc, subject, body, hidden, messageFile, KURL::List(attachURL));
  }
  /** For backward compatibility */
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile)
  {
    return openComposer(to, cc, bcc, subject, body, hidden, messageFile, KURL::List());
  }
  int openComposer (const QString &to, const QString &cc,
                    const QString &bcc, const QString &subject,
                    const QString &body, int hidden,
                    const QString &attachName,
                    const QCString &attachCte,
                    const QCString &attachData,
                    const QCString &attachType,
                    const QCString &attachSubType,
                    const QCString &attachParamAttr,
                    const QString &attachParamValue,
                    const QCString &attachContDisp);
  DCOPRef openComposer(const QString &to, const QString &cc,
                       const QString &bcc, const QString &subject,
                       const QString &body,bool hidden);

  int sendCertificate( const QString& to, const QByteArray& certData );

  void openReader() { openReader( false ); }
  void compactAllFolders();
  int dcopAddMessage(const QString & foldername, const QString & messageFile);
  int dcopAddMessage(const QString & foldername, const KURL & messageFile);
  QStringList folderList() const;
  DCOPRef getFolder( const QString& vpath );
  virtual bool showMail( Q_UINT32 serialNumber, QString messageId );
  /** normal control stuff */

  static KMKernel *self() { return mySelf; }
  static KConfig *config();

  void init ();
  void cleanupImapFolders();
  void testDir(const char *_name);
  void recoverDeadLetters(void);
  void initFolders(KConfig* cfg);
  void closeAllKMailWindows();
  void notClosedByUser();
  void cleanup(void);
  void quit();
  void transferMail(void);
  void ungrabPtrKb(void);
  void kmailMsgHandler(QtMsgType aType, const char* aMsg);
  bool doSessionManagement();
  bool firstInstance() { return the_firstInstance; }
  void setFirstInstance(bool value) { the_firstInstance = value; }
  void action (bool mailto, bool check, const QString &to, const QString &cc,
               const QString &bcc, const QString &subj, const QString &body,
	       const KURL &messageFile, const KURL::List &attach);
  void byteArrayToRemoteFile(const QByteArray&, const KURL&,
			     bool overwrite = FALSE);
  bool folderIsDraftOrOutbox(const KMFolder *);
  bool folderIsTrash(KMFolder *);
  /**
   * Returns true if the folder is one of the sent-mail folders.
   */
  bool folderIsSentMailFolder( const KMFolder * );

  KInstance *xmlGuiInstance() { return mXmlGuiInstance; }
  void setXmlGuiInstance( KInstance *instance ) { mXmlGuiInstance = instance; }

  KMFolder *inboxFolder() { return the_inboxFolder; }
  KMFolder *outboxFolder() { return the_outboxFolder; }
  KMFolder *sentFolder() { return the_sentFolder; }
  KMFolder *trashFolder() { return the_trashFolder; }
  KMFolder *draftsFolder() { return the_draftsFolder; }

  KMFolderMgr *folderMgr() { return the_folderMgr; }
  KMFolderMgr *imapFolderMgr() { return the_imapFolderMgr; }
  KMFolderMgr *dimapFolderMgr() { return the_dimapFolderMgr; }
  KMFolderMgr *searchFolderMgr() { return the_searchFolderMgr; }
  UndoStack *undoStack() { return the_undoStack; }
  KMAcctMgr *acctMgr() { return the_acctMgr; }
  KMFilterMgr *filterMgr() { return the_filterMgr; }
  KMFilterMgr *popFilterMgr() { return the_popFilterMgr; }
  KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  KMSender *msgSender() { return the_msgSender; }
  KMMsgDict *msgDict();
  KMMsgIndex *msgIndex();

  KPIM::ThreadWeaver::Weaver *weaver() { return the_weaver; }
  /** return the pointer to the identity manager */
  IdentityManager *identityManager();
  CryptPlugWrapperList * cryptPlugList() const { return mCryptPlugList; }

  KMGroupware& groupware();
  KMailICalIfaceImpl& iCalIface();

  bool firstStart() { return the_firstStart; }
  QString previousVersion() { return the_previousVersion; }
  bool startingUp() { return the_startingUp; }
  void setStartingUp (bool flag) { the_startingUp = flag; }
  bool shuttingDown() { return the_shuttingDown; }
  void setShuttingDown(bool flag) { the_shuttingDown = flag; }
  void serverReady (bool flag) { the_server_is_ready = flag; }

  /** Returns true if we have a system tray applet. This is needed in order
   *  to know whether the application should be allowed to exit in case the
   *  last visible composer or separate message window is closed.
   */
  bool haveSystemTrayApplet();

  bool registerSystemTrayApplet( const KSystemTray* );
  bool unregisterSystemTrayApplet( const KSystemTray* );

  /// Reimplemented from KMailIface
  bool handleCommandLine( bool noArgsOpensReader );
  void emergencyExit( const QString& reason );

  /** Returns a message serial number that hasn't been used yet. */
  unsigned long getNextMsgSerNum();
  QTextCodec *networkCodec() { return netCodec; }

  /** See @ref slotCollectStdOut */
  QByteArray getCollectedStdOut(KProcess*);
  /** See @ref slotCollectStdErr */
  QByteArray getCollectedStdErr(KProcess*);

  /** returns a reference to the first Mainwin or a temporary Mainwin */
  KMainWindow* mainWin();

  // ### The mContextMenuShown flag is necessary to work around bug# 56693
  // ### (kmail freeze with the complete desktop while pinentry-qt appears)
  // ### FIXME: Once the encryption support is asynchron this can be removed
  // ### again.
  void setContextMenuShown( bool flag ) { mContextMenuShown = flag; }
  bool contextMenuShown() const { return mContextMenuShown; }

public slots:
  //Save contents of all open composer widnows to ~/dead.letter
  void dumpDeadLetters();

  /** Connect the received* signals of K(Shell)Process to these slots
      to let the kernel collect the output for you.

      Works by keeping a map of QByteArrays, indexed by the KProcess
      pointers.

      After the command finished, you can then get a QByteArray with
      the data by calling @ref getCollectedStdOut with the same
      KProcess* pointer as when calling these slots.

      See KMFilterActionWithCommand and derived classes for example
      usages.
  */
  void slotCollectStdOut(KProcess*,char*,int);
  /** Same as @ref slotCollectStdOut, but for stderr. */
  void slotCollectStdErr(KProcess*,char*,int);

  /** Call this slot instead of directly @ref KConfig::sync() to
      minimize the overall config writes. Calling this slot will
      schedule a sync of the application config file using a timer, so
      that many consecutive calls can be condensed into a single
      sync, which is more efficient. */
  void slotRequestConfigSync();

  /** empty all the trash bins */
  void slotEmptyTrash();

  void slotShowConfigurationDialog();

protected slots:
  void slotDataReq(KIO::Job*,QByteArray&);
  void slotResult(KIO::Job*);
  void cleanupLoop();
  void cleanupProgress();

signals:
  void configChanged();

private:
  void openReader( bool onlyCheck );

  KMFolder *the_inboxFolder;
  KMFolder *the_outboxFolder;
  KMFolder *the_sentFolder;
  KMFolder *the_trashFolder;
  KMFolder *the_draftsFolder;

  KMFolderMgr *the_folderMgr;
  KMFolderMgr *the_imapFolderMgr;
  KMFolderMgr *the_dimapFolderMgr;
  KMFolderMgr *the_searchFolderMgr;
  UndoStack *the_undoStack;
  KMAcctMgr *the_acctMgr;
  KMFilterMgr *the_filterMgr;
  KMFilterMgr *the_popFilterMgr;
  KMFilterActionDict *the_filterActionDict;
  mutable IdentityManager *mIdentityManager;
  KMSender *the_msgSender;
  KMMsgDict *the_msgDict;
  KMMsgIndex *the_msgIndex;
  struct putData
  {
    KURL url;
    QByteArray data;
    int offset;
  };
  QMap<KIO::Job *, putData> mPutJobs;
  QMap<KProcess*,QByteArray> mStdOutCollection;
  QMap<KProcess*,QByteArray> mStdErrCollection;
  /** previous KMail version. If different from current,
      the user has just updated. read from config */
  QString the_previousVersion;
  /** is this the first start?  read from config */
  bool the_firstStart;
  /** are we starting up? set in main.cpp directly before kapp->exec() */
  bool the_startingUp;
  /** are we going down? set from here */
  bool the_shuttingDown;
  /** are we in the middle of network operations (needed?) */
  bool the_server_is_ready;
  /** true unles kmail is closed by session management */
  bool closed_by_user;
  bool allowedToExpire;
  bool the_firstInstance;
  static KMKernel *mySelf;
#if KDE_IS_VERSION( 3, 1, 92 )
  KSharedConfig::Ptr mConfig;
#else
  static KConfig * myConfig;
#endif
  QTextCodec *netCodec;
  KProgress *mProgress;
  KPassivePopup *mCleanupPopup;
  QLabel *mCleanupLabel;
  CryptPlugWrapperList *mCryptPlugList;
  KInstance* mXmlGuiInstance;
  ConfigureDialog *mConfigureDialog;
  QTimer *mDeadLetterTimer;
  int mDeadLetterInterval;
  KMGroupware * mGroupware;
  KMailICalIfaceImpl* mICalIface;
  // temporary mainwin
  KMMainWin *mWin;
  MailServiceImpl *mMailService;

  // true if the context menu of KMFolderTree or KMHeaders is shown
  // this is necessary to know in order to prevent a dead lock between the
  // context menus and the pinentry program
  bool mContextMenuShown;

  QValueList<const KSystemTray*> systemTrayApplets;

  /* Weaver */
  KPIM::ThreadWeaver::Weaver *the_weaver;
  KPIM::ThreadWeaver::WeaverThreadLogger *the_weaverLogger;
};

#endif
