// -*- c++ -*-

#ifndef _KMCONTROL
#define _KMCONTROL

#include <qobject.h>
#include <qstring.h>

#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kmailIface.h>

#include <cryptplugwrapperlist.h>

#define kernel KMKernel::self()
#define kmconfig KMKernel::config()

namespace KIO {
  class Job;
}
class KMMsgIndex;
class QLabel;
class KMFolder;
class KMFolderMgr;
class KMUndoStack;
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
class KMGroupware;

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

  void openReader();
  void compactAllFolders();
  int dcopAddMessage(const QString & foldername, const QString & messageFile);
  int dcopAddMessage(const QString & foldername, const KURL & messageFile);
  void requestAddresses( QString filename );
  bool lockContactsFolder();
  bool unlockContactsFolder();
  bool storeAddresses( QString addresses, QStringList delUIDs );
  QStringList folderList() const;
  DCOPRef getFolder( const QString& vpath );
  /** normal control stuff */

  static KMKernel *self() { return mySelf; }
  static KConfig *config();

  void init ();
  void cleanupImapFolders();
  void testDir(const char *_name);
  void recoverDeadLetters(void);
  void initFolders(KConfig* cfg);
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

  KInstance *xmlGuiInstance() { return mXmlGuiInstance; }
  void setXmlGuiInstance( KInstance *instance ) { mXmlGuiInstance = instance; }

  KMFolder *inboxFolder() { return the_inboxFolder; }
  KMFolder *outboxFolder() { return the_outboxFolder; }
  KMFolder *sentFolder() { return the_sentFolder; }
  KMFolder *trashFolder() { return the_trashFolder; }
  KMFolder *draftsFolder() { return the_draftsFolder; }

  KMFolderMgr *folderMgr() { return the_folderMgr; }
  KMFolderMgr *imapFolderMgr() { return the_imapFolderMgr; }
  KMFolderMgr *searchFolderMgr() { return the_searchFolderMgr; }
  KMUndoStack *undoStack() { return the_undoStack; }
  KMAcctMgr *acctMgr() { return the_acctMgr; }
  KMFilterMgr *filterMgr() { return the_filterMgr; }
  KMFilterMgr *popFilterMgr() { return the_popFilterMgr; }
  KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  KMSender *msgSender() { return the_msgSender; }
  KMMsgDict *msgDict();
  KMMsgIndex *msgIndex();

  /** return the pointer to the identity manager */
  IdentityManager *identityManager();
  CryptPlugWrapperList * cryptPlugList() { return &mCryptPlugList; }

  KMGroupware& groupware();

  bool firstStart() { return the_firstStart; }
  QString previousVersion() { return the_previousVersion; }
  bool startingUp() { return the_startingUp; }
  void setStartingUp (bool flag) { the_startingUp = flag; }
  bool shuttingDown() { return the_shuttingDown; }
  void serverReady (bool flag) { the_server_is_ready = flag; }
  void notClosedByUser();

  void emergencyExit( const QString& reason );

  /** Returns a message serial number that hasn't been used yet. */
  unsigned long getNextMsgSerNum();
  QTextCodec *networkCodec() { return netCodec; }

  /** See @ref slotCollectStdOut */
  QByteArray getCollectedStdOut(KProcess*);
  /** See @ref slotCollectStdErr */
  QByteArray getCollectedStdErr(KProcess*);

  /** returns a reference to the first Mainwin or a temporary Mainwin */
  KMMainWin* mainWin();

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

private:
  KMFolder *the_inboxFolder;
  KMFolder *the_outboxFolder;
  KMFolder *the_sentFolder;
  KMFolder *the_trashFolder;
  KMFolder *the_draftsFolder;

  KMFolderMgr *the_folderMgr, *the_imapFolderMgr, *the_searchFolderMgr;
  KMUndoStack *the_undoStack;
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
  static KConfig *myConfig;
  QTextCodec *netCodec;
  KProgress *mProgress;
  KPassivePopup *mCleanupPopup;
  QLabel *mCleanupLabel;
  CryptPlugWrapperList mCryptPlugList;
  KInstance* mXmlGuiInstance;
  ConfigureDialog *mConfigureDialog;
  QTimer *mDeadLetterTimer;
  int mDeadLetterInterval;
  KMGroupware * mGroupware;
  // temporary mainwin
  KMMainWin *mWin;
};

#endif
