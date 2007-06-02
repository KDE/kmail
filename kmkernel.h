// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef _KMCONTROL
#define _KMCONTROL

#include <qobject.h>
#include <qstring.h>
#include <qguardedptr.h>
#include <weaver.h>
#include <weaverlogger.h>

#include <kconfig.h>
#include <kdeversion.h>
#include <kimproxy.h>
#include <kdepimmacros.h>

#include "kmailIface.h"
#include "kmmsgbase.h"
#include "globalsettings.h"

#define kmkernel KMKernel::self()
#define kmconfig KMKernel::config()

namespace KIO {
  class Job;
}
namespace KWallet {
  class Wallet;
}

/** The KMail namespace contains classes used for KMail.
* This is to keep them out of the way from all the other
* un-namespaced classes in libs and the rest of PIM.
*/
namespace KMail {
  class MailServiceImpl;
  class UndoStack;
  class JobScheduler;
  class MessageSender;
  class AccountManager;
}
namespace KPIM { class ProgressDialog; }
using KMail::MailServiceImpl;
using KMail::AccountManager;
using KMail::UndoStack;
using KMail::JobScheduler;
using KPIM::ProgressDialog;
class KMMsgIndex;
class QLabel;
class KMFolder;
class KMFolderMgr;
class KMFilterMgr;
class KMFilterActionDict;
class KMSender;
namespace KPIM {
  class Identity;
  class IdentityManager;
}
class KMKernel;
class KProcess;
class KProgressDialog;
class KInstance;
class QTimer;
class KProgress;
class KPassivePopup;
class KMMainWin;
class KMainWindow;
class KMailICalIfaceImpl;
class KMReaderWin;
class KSystemTray;
class KMMainWidget;
class ConfigureDialog;

/**
 * @short Central point of coordination in KMail
 *
 * The KMKernel class represents the core of KMail, where the different parts
 * come together and are coordinated. It is currently also the class which exports
 * KMail's main DCOP interfaces. The kernel is responsible for creating various
 * (singleton) objects such as the UndoStack, the folder managers and filter
 * manager, etc.
 */
class KDE_EXPORT KMKernel : public QObject, virtual public KMailIface
{
  Q_OBJECT

public:
  KMKernel (QObject *parent=0, const char *name=0);
  ~KMKernel ();

  /** dcop callable stuff */

  void checkMail ();
  QStringList accounts();
  void checkAccount (const QString &account);
  /** returns id of composer if more are opened */
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile, const KURL::List &attachURLs,
                    const QCStringList &customHeaders);
  /** For backward compatibility */
  int openComposer (const QString &to, const QString &cc, const QString &bcc,
                    const QString &subject, const QString &body, int hidden,
                    const KURL &messageFile, const KURL::List &attachURLs)
  {
    QCStringList noCustomHeaders;
    return openComposer(to, cc, bcc, subject, body, hidden, messageFile, attachURLs, noCustomHeaders);
  }
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
  /** For backward compatibility
   * @deprecated
   */
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
                    const QCString &attachContDisp,
                    const QCString &attachCharset);

  DCOPRef openComposer(const QString &to, const QString &cc,
                       const QString &bcc, const QString &subject,
                       const QString &body,bool hidden);

  /** DCOP call used to set the default transport. */

  void setDefaultTransport( const QString & transport );

  /** DCOP call used by the Kontact plugin to create a new message. */
  DCOPRef newMessage(const QString &to,
                     const QString &cc,
                     const QString &bcc,
                     bool hidden,
                     bool useFolderId,
                     const KURL &messageFile,
                     const KURL &attachURL);

  int sendCertificate( const QString& to, const QByteArray& certData );

  void openReader() { openReader( false ); }

  int dcopAddMessage(const QString & foldername, const QString & messagefile,
                     const QString & MsgStatusFlags = QString());
  int dcopAddMessage(const QString & foldername, const KURL & messagefile,
                     const QString & MsgStatusFlags = QString());
  void dcopResetAddMessage();
  /** add messages without rejecting duplicates */
  int dcopAddMessage_fastImport(const QString & foldername, const QString & messagefile,
                                const QString & MsgStatusFlags = QString());
  int dcopAddMessage_fastImport(const QString & foldername, const KURL & messagefile,
                                const QString & MsgStatusFlags = QString());

  QStringList folderList() const;
  DCOPRef getFolder( const QString& vpath );
  void selectFolder( QString folder );
  int timeOfLastMessageCountChange() const;
  virtual bool showMail( Q_UINT32 serialNumber, QString messageId );
  virtual QString getFrom( Q_UINT32 serialNumber );
  virtual QString debugScheduler();
  virtual QString debugSernum( Q_UINT32 serialNumber );
  int viewMessage( const KURL & messageFile );

  /**
   * Pauses all background jobs and does not
   * allow new background jobs to be started.
  */
  virtual void pauseBackgroundJobs();

  /**
   * Resumes all background jobs and allows
   * new jobs to be started.
  */
  virtual void resumeBackgroundJobs();

  /**
   * Stops all network related jobs and enter offline mode
   * New network jobs cannot be started.
  */
  void stopNetworkJobs();

  /**
   * Resumes all network related jobs and enter online mode
   * New network jobs can be started.
  */
  void resumeNetworkJobs();

  /** A static helper function that asks the user
   * if they want to go online.
   * @return true if the user wants to go online
   * @return false if the user wants to stay offline
   */
  static bool askToGoOnline();

  /** Checks if the current network state is online or offline
   * @return true if the network state is offline
   * @return false if the network state is online
   */
  static bool isOffline();

  /** normal control stuff */

  static KMKernel *self() { return mySelf; }
  static KConfig *config();

  void init();
  void readConfig();
  void cleanupImapFolders();
  void testDir(const char *_name);
  void recoverDeadLetters();
  void initFolders(KConfig* cfg);
  void closeAllKMailWindows();
  void cleanup(void);
  void quit();
  /**
   * Returns true if the transfer was successful, otherwise false. In any case
   * destinationDir contains the path to the current mail storage when the
   * method returns.
   */
  bool transferMail( QString & destinationDir );
  void ungrabPtrKb(void);
  void kmailMsgHandler(QtMsgType aType, const char* aMsg);
  bool doSessionManagement();
  bool firstInstance() { return the_firstInstance; }
  void setFirstInstance(bool value) { the_firstInstance = value; }
  void action (bool mailto, bool check, const QString &to, const QString &cc,
               const QString &bcc, const QString &subj, const QString &body,
               const KURL &messageFile, const KURL::List &attach,
               const QCStringList &customHeaders);
  void byteArrayToRemoteFile(const QByteArray&, const KURL&,
			     bool overwrite = FALSE);
  bool folderIsDraftOrOutbox( const KMFolder * );
  bool folderIsDrafts( const KMFolder * );
  bool folderIsTemplates( const KMFolder * );
  bool folderIsTrash( KMFolder * );
  /**
   * Returns true if the folder is one of the sent-mail folders.
   */
  bool folderIsSentMailFolder( const KMFolder * );
  /**
   * Find a folder by ID string in all folder managers
   */
  KMFolder* findFolderById( const QString& idString );

  KInstance *xmlGuiInstance() { return mXmlGuiInstance; }
  void setXmlGuiInstance( KInstance *instance ) { mXmlGuiInstance = instance; }

  KMFolder *inboxFolder() { return the_inboxFolder; }
  KMFolder *outboxFolder() { return the_outboxFolder; }
  KMFolder *sentFolder() { return the_sentFolder; }
  KMFolder *trashFolder() { return the_trashFolder; }
  KMFolder *draftsFolder() { return the_draftsFolder; }
  KMFolder *templatesFolder() { return the_templatesFolder; }

  KMFolderMgr *folderMgr() { return the_folderMgr; }
  KMFolderMgr *imapFolderMgr() { return the_imapFolderMgr; }
  KMFolderMgr *dimapFolderMgr() { return the_dimapFolderMgr; }
  KMFolderMgr *searchFolderMgr() { return the_searchFolderMgr; }
  UndoStack *undoStack() { return the_undoStack; }
  AccountManager *acctMgr() { return the_acctMgr; }
  KMFilterMgr *filterMgr() { return the_filterMgr; }
  KMFilterMgr *popFilterMgr() { return the_popFilterMgr; }
  KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  KMail::MessageSender *msgSender();
  KMMsgIndex *msgIndex();

  KPIM::ThreadWeaver::Weaver *weaver() { return the_weaver; }
  /** return the pointer to the identity manager */
  KPIM::IdentityManager *identityManager();

  JobScheduler* jobScheduler() { return mJobScheduler; }

  /** Compact all folders, used for the gui action (and from DCOP) */
  void compactAllFolders();
  /** Expire all folders, used for the gui action */
  void expireAllFoldersNow();

  KMailICalIfaceImpl& iCalIface();

  bool firstStart() { return the_firstStart; }
  QString previousVersion() { return the_previousVersion; }
  bool startingUp() { return the_startingUp; }
  void setStartingUp (bool flag) { the_startingUp = flag; }
  bool shuttingDown() { return the_shuttingDown; }
  void setShuttingDown(bool flag) { the_shuttingDown = flag; }
  void serverReady (bool flag) { the_server_is_ready = flag; }

  /** Returns the full path of the user's local data directory for KMail.
      The path ends with '/'.
  */
  static QString localDataPath();

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

  /** returns a reference to the first Mainwin or a temporary Mainwin */
  KMainWindow* mainWin();

  // ### The mContextMenuShown flag is necessary to work around bug# 56693
  // ### (kmail freeze with the complete desktop while pinentry-qt appears)
  // ### FIXME: Once the encryption support is asynchron this can be removed
  // ### again.
  void setContextMenuShown( bool flag ) { mContextMenuShown = flag; }
  bool contextMenuShown() const { return mContextMenuShown; }

  /**
   * Get a reference to KMail's KIMProxy instance
   * @return a pointer to a valid KIMProxy
   */
  ::KIMProxy* imProxy();

  /**
   * Returns true IFF the user has requested that the current mail checks
   * should be aborted. Needs to be periodically polled.
   */
  bool mailCheckAborted() const;
  /**  Set the state of the abort requested variable to false,
   * i.e. enable mail checking again
   */
  void enableMailCheck();
  /**
   * Set the state of the abort requested variable to true,
   * (to let the current jobs run, but stop when possible).
   * This is used to cancel mail checks when closing the last mainwindow
   */
  void abortMailCheck();

  bool canQueryClose();

  /**
   * Called by the folder tree if the count of unread/total messages changed.
   */
  void messageCountChanged();

  /** Open KDE wallet and set it to kmail folder */
  KWallet::Wallet *wallet();

  /** Get first mainwidget */
  KMMainWidget *getKMMainWidget();

  /** @return a list of all folders from all folder managers. */
  QValueList< QGuardedPtr<KMFolder> > allFolders();

  void raise();

  void loadProfile( const QString& path );

  void saveToProfile( const QString& path ) const;
public slots:

  /// Save contents of all open composer widnows to ~/dead.letter
  void dumpDeadLetters();

  /** Call this slot instead of directly KConfig::sync() to
      minimize the overall config writes. Calling this slot will
      schedule a sync of the application config file using a timer, so
      that many consecutive calls can be condensed into a single
      sync, which is more efficient. */
  void slotRequestConfigSync();

  /** empty all the trash bins */
  void slotEmptyTrash();

  void slotShowConfigurationDialog();
  void slotRunBackgroundTasks();

  void slotConfigChanged();

protected slots:
  void slotDataReq(KIO::Job*,QByteArray&);
  void slotResult(KIO::Job*);

signals:
  void configChanged();
  void folderRemoved( KMFolder* aFolder );
  void onlineStatusChanged( GlobalSettings::EnumNetworkState::type );

private:
  void openReader( bool onlyCheck );
  KMMsgStatus strToStatus(const QString &flags);
  KMFolder *currentFolder();

  KMFolder *the_inboxFolder;
  KMFolder *the_outboxFolder;
  KMFolder *the_sentFolder;
  KMFolder *the_trashFolder;
  KMFolder *the_draftsFolder;
  KMFolder *the_templatesFolder;

  KMFolderMgr *the_folderMgr;
  KMFolderMgr *the_imapFolderMgr;
  KMFolderMgr *the_dimapFolderMgr;
  KMFolderMgr *the_searchFolderMgr;
  UndoStack *the_undoStack;
  AccountManager *the_acctMgr;
  KMFilterMgr *the_filterMgr;
  KMFilterMgr *the_popFilterMgr;
  KMFilterActionDict *the_filterActionDict;
  mutable KPIM::IdentityManager *mIdentityManager;
  KMSender *the_msgSender;
  KMMsgIndex *the_msgIndex;
  struct putData
  {
    KURL url;
    QByteArray data;
    int offset;
  };
  QMap<KIO::Job *, putData> mPutJobs;
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
  bool the_firstInstance;
  bool mMailCheckAborted;
  static KMKernel *mySelf;
  KSharedConfig::Ptr mConfig;
  QTextCodec *netCodec;
  KInstance* mXmlGuiInstance;
  ConfigureDialog *mConfigureDialog;

  QTimer *mBackgroundTasksTimer;
  KMailICalIfaceImpl* mICalIface;
  JobScheduler* mJobScheduler;
  // temporary mainwin
  KMMainWin *mWin;
  MailServiceImpl *mMailService;

  // the time of the last change of the unread or total count of a folder;
  // this can be queried via DCOP in order to determine whether the counts
  // need to be updated (e.g. in the Summary in Kontact)
  int mTimeOfLastMessageCountChange;

  // true if the context menu of KMFolderTree or KMHeaders is shown
  // this is necessary to know in order to prevent a dead lock between the
  // context menus and the pinentry program
  bool mContextMenuShown;

  QValueList<const KSystemTray*> systemTrayApplets;

  /* Weaver */
  KPIM::ThreadWeaver::Weaver *the_weaver;
  KPIM::ThreadWeaver::WeaverThreadLogger *the_weaverLogger;

  KWallet::Wallet *mWallet;

  // variables used by dcopAddMessage()
  QStringList mAddMessageMsgIds;
  QString     mAddMessageLastFolder;
  KMFolder    *mAddMsgCurrentFolder;
};

#endif
