#ifndef _KMCONTROL
#define _KMCONTROL

#include <qobject.h>
#include <qstring.h>

#include <kconfig.h>
#include <kio/job.h>
#include <kcmdlineargs.h>
#include <kmailIface.h>

#define kernel KMKernel::self()

class KMFolder;
class KBusyPtr;
class KBusyPtr;
class KMFolderMgr;
class KMUndoStack;
class KMAcctMgr;
class KMFilterMgr;
class KMFilterActionDict;
class KabAPI;
class KMSender;
class KMIdentity;
class KMKernel;
class KMMsgDict;

class KMKernel : public QObject, virtual public KMailIface
{
  Q_OBJECT

public:
  KMKernel (QObject *parent=0, const char *name=0);
  ~KMKernel ();

  /** true if user has requested to expire all messages on exit */
  void setCanExpire(bool expire);
  bool canExpire();

  /** dcop calable stuff */

  void checkMail ();
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
  void openReader();
  void compactAllFolders();
  int dcopAddMessage(const QString & foldername, const QString & messageFile);
  int dcopAddMessage(const QString & foldername, const KURL & messageFile);
  /** normal control stuff */

  static KMKernel *self() { return mySelf; }

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
  void dumpDeadLetters();
  bool doSessionManagement();
  bool firstInstance() { return the_firstInstance; }
  void setFirstInstance(bool value) { the_firstInstance = value; }
  void action (bool mailto, bool check, const QString &to, const QString &cc,
               const QString &bcc, const QString &subj, const QString &body, const KURL &messageFile,
               const KURL::List &attach);
  void byteArrayToRemoteFile(const QByteArray&, const KURL&,
    bool overwrite = FALSE);
	bool folderIsDraftOrOutbox(KMFolder *);	

  inline KMFolder *inboxFolder() { return the_inboxFolder; }
  inline KMFolder *outboxFolder() { return the_outboxFolder; }
  inline KMFolder *sentFolder() { return the_sentFolder; }
  inline KMFolder *trashFolder() { return the_trashFolder; }
  inline KMFolder *draftsFolder() { return the_draftsFolder; }

  inline KBusyPtr *kbp() { return the_kbp; }
  inline KMFolderMgr *folderMgr() { return the_folderMgr; }
  inline KMFolderMgr *imapFolderMgr() { return the_imapFolderMgr; }
  inline KMUndoStack *undoStack() { return the_undoStack; }
  inline KMAcctMgr *acctMgr() { return the_acctMgr; }
  inline KMFilterMgr *filterMgr() { return the_filterMgr; }
  inline KMFilterMgr *popFilterMgr() { return the_popFilterMgr; }
  inline KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  KabAPI *KABaddrBook();
  inline KMSender *msgSender() { return the_msgSender; }
  inline KMMsgDict *msgDict() { return the_msgDict; }

  inline bool firstStart() { return the_firstStart; }
  inline QString previousVersion() { return the_previousVersion; }
  inline bool shuttingDown() { return the_shuttingDown; }
  inline bool checkingMail() { return the_checkingMail; }
  inline void setCheckingMail(bool flag) { the_checkingMail = flag; }
  inline void serverReady (bool flag) { the_server_is_ready = flag; }
  void notClosedByUser();

  void emergencyExit( const QString& reason );

  /** Returns a message serial number that hasn't been used yet. */
  unsigned long getNextMsgSerNum();
  QTextCodec *networkCodec() { return netCodec; }

protected slots:
  void slotDataReq(KIO::Job*,QByteArray&);
  void slotResult(KIO::Job*);
private:
  KMFolder *the_inboxFolder;
  KMFolder *the_outboxFolder;
  KMFolder *the_sentFolder;
  KMFolder *the_trashFolder;
  KMFolder *the_draftsFolder;

  KBusyPtr *the_kbp;
  KMFolderMgr *the_folderMgr, *the_imapFolderMgr;
  KMUndoStack *the_undoStack;
  KMAcctMgr *the_acctMgr;
  KMFilterMgr *the_filterMgr;
  KMFilterMgr *the_popFilterMgr;
  KMFilterActionDict *the_filterActionDict;
  KabAPI *the_KAB_addrBook;
  KMSender *the_msgSender;
  KMMsgDict *the_msgDict;
  struct putData
  {
    KURL url;
    QByteArray data;
  };
  QMap<KIO::Job *, putData> mPutJobs;
  /** previous KMail version. If different from current,
      the user has just updated. read from config */
  QString the_previousVersion;
  /** is this the first start?  read from config */
  bool the_firstStart;
  /** are we going down? set from here */
  bool the_shuttingDown;
  /** are we checking mail? set from... where the mail is checked */
  bool the_checkingMail;
  /** are we in the middle of network operations (needed?) */
  bool the_server_is_ready;
  /** true unles kmail is closed by session management */
  bool closed_by_user;
  bool allowedToExpire;
  bool the_firstInstance;
  static KMKernel *mySelf;
  QTextCodec *netCodec;
};

#endif
