#ifndef _KMCONTROL
#define _KMCONTROL

#include <qobject.h>
#include <qstring.h>

#include <kconfig.h>
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
  int openComposer (QString to, QString cc, QString bcc, QString subject,
                    int hidden);
  void openReader(KURL messageFile);
  int ready();
  int send(int composerId, int how); //0=now, 1=later
  int addAttachment(int composerId, KURL url, QString comment);
  int setBody (int composerId, QString body);
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
  void action (bool mailto, bool check, QString to, QString cc,
               QString bcc, QString subj, KURL messageFile);
  
  inline KMFolder *inboxFolder() { return the_inboxFolder; }
  inline KMFolder *outboxFolder() { return the_outboxFolder; }
  inline KMFolder *sentFolder() { return the_sentFolder; }
  inline KMFolder *trashFolder() { return the_trashFolder; }

  inline KBusyPtr *kbp() { return the_kbp; }
  inline KMFolderMgr *folderMgr() { return the_folderMgr; }
  inline KMUndoStack *undoStack() { return the_undoStack; }
  inline KMAcctMgr *acctMgr() { return the_acctMgr; }
  inline KMFilterMgr *filterMgr() { return the_filterMgr; }
  inline KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  inline KMAddrBook *addrBook() { return the_addrBook; }
  inline KMSender *msgSender() { return the_msgSender; }
  inline KMIdentity *identity() { return the_identity; }

  inline bool firstStart() { return the_firstStart; }
  inline bool shuttingDown() { return the_shuttingDown; }
  inline bool checkingMail() { return the_checkingMail; }
  inline void setCheckingMail(bool flag) { the_checkingMail = flag; }
  inline void serverReady (bool flag) { the_server_is_ready = flag; }
private:
  KMFolder *the_inboxFolder;
  KMFolder *the_outboxFolder;
  KMFolder *the_sentFolder;
  KMFolder *the_trashFolder;

  KBusyPtr *the_kbp;
  KMFolderMgr *the_folderMgr;
  KMUndoStack *the_undoStack;
  KMAcctMgr *the_acctMgr;
  KMFilterMgr *the_filterMgr;
  KMFilterActionDict *the_filterActionDict;
  KMAddrBook *the_addrBook;
  KMSender *the_msgSender;
  KMIdentity *the_identity;

  bool the_firstStart;          // is this the first start?  read from config
  bool the_shuttingDown;        // are we going down? set from here
  bool the_checkingMail;        // are we checking mail? set from... where the mail is checked
  bool the_server_is_ready;     // are we in the middle of network operations (needed?)

  static KMKernel *mySelf;
};

#endif
