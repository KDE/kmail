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
class KMAddrBook;
class KabAPI;
class KMSender;
class KMIdentity;
class KMKernel;

class KMKernel : public QObject, virtual public KMailIface
{
  Q_OBJECT

public:
  KMKernel (QObject *parent=0, const char *name=0);
  ~KMKernel ();

// dcop calable stuff

  void checkMail ();
  //returns id of composer if more are opened
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile, const KURL &attachURL);
  // For backward compatibility
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile)
  {
    return openComposer(to, cc, bcc, subject, body, hidden, messageFile,
    KURL());
  }
  void openReader();
  int ready();
  int send(int composerId, int how); //0=now, 1=later
  int addAttachment(int composerId, KURL url, QString comment);
  int setBody (int composerId, QString body);
  void compactAllFolders();
  int dcopAddMessage(const QString & foldername, const QString & messageFile);
  int dcopAddMessage(const QString & foldername, const KURL & messageFile);
  // normal control stuff

  static KMKernel *self() { return mySelf; }

  void init ();
  void testDir(const char *_name);
  void recoverDeadLetters(void);
  void initFolders(KConfig* cfg);
  void cleanup(void);
  void quit();
  void transferMail(void);
  void ungrabPtrKb(void);
  void kmailMsgHandler(QtMsgType aType, const char* aMsg);
  void dumpDeadLetters();
  bool doSessionManagement ();
  void action (bool mailto, bool check, const QString &to, const QString &cc,
               const QString &bcc, const QString &subj, const QString &body, const KURL &attach,
               const KURL &messageFile);
  void byteArrayToRemoteFile(const QByteArray&, const KURL&,
    bool overwrite = FALSE);

  inline KMFolder *inboxFolder() { return the_inboxFolder; }
  inline KMFolder *outboxFolder() { return the_outboxFolder; }
  inline KMFolder *sentFolder() { return the_sentFolder; }
  inline KMFolder *trashFolder() { return the_trashFolder; }
  inline KMFolder *draftsFolder() { return the_draftsFolder; }

  inline KBusyPtr *kbp() { return the_kbp; }
  inline KMFolderMgr *folderMgr() { return the_folderMgr; }
  inline KMUndoStack *undoStack() { return the_undoStack; }
  inline KMAcctMgr *acctMgr() { return the_acctMgr; }
  inline KMFilterMgr *filterMgr() { return the_filterMgr; }
  inline KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  inline KMAddrBook *addrBook() { return the_addrBook; }
  KabAPI *KABaddrBook();
  inline KMSender *msgSender() { return the_msgSender; }

  inline bool firstStart() { return the_firstStart; }
  inline QString previousVersion() { return the_previousVersion; }
  inline bool shuttingDown() { return the_shuttingDown; }
  inline bool checkingMail() { return the_checkingMail; }
  inline void setCheckingMail(bool flag) { the_checkingMail = flag; }
  inline void serverReady (bool flag) { the_server_is_ready = flag; }
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
  KMFolderMgr *the_folderMgr;
  KMUndoStack *the_undoStack;
  KMAcctMgr *the_acctMgr;
  KMFilterMgr *the_filterMgr;
  KMFilterActionDict *the_filterActionDict;
  KMAddrBook *the_addrBook;
  KabAPI *the_KAB_addrBook;
  KMSender *the_msgSender;
  struct putData
  {
    KURL url;
    QByteArray data;
  };
  QMap<KIO::Job *, putData> mPutJobs;

  QString the_previousVersion;  // previous KMail version. If different from current, 
                                // the user has just updated. read from config
  bool the_firstStart;          // is this the first start?  read from config
  bool the_shuttingDown;        // are we going down? set from here
  bool the_checkingMail;        // are we checking mail? set from... where the mail is checked
  bool the_server_is_ready;     // are we in the middle of network operations (needed?)

  static KMKernel *mySelf;
};

#endif
