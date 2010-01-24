// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef _KMKERNEL_H
#define _KMKERNEL_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QDBusObjectPath>
#include <threadweaver/ThreadWeaver.h>

#include <kconfig.h>
#include <kurl.h>

#include "kmail_export.h"
#include "globalsettings.h"
#include <kcomponentdata.h>
#include <akonadi/kmime/specialmailcollections.h>

#define kmkernel KMKernel::self()
#define kmconfig KMKernel::config()

Q_DECLARE_METATYPE(QVector<QStringList>)

namespace Akonadi {
  class Collection;
  class ChangeRecorder;
}

namespace KIO {
  class Job;
}

class KJob;
/** The KMail namespace contains classes used for KMail.
* This is to keep them out of the way from all the other
* un-namespaced classes in libs and the rest of PIM.
*/
namespace KMail {
  class MailServiceImpl;
  class UndoStack;
  class JobScheduler;
  class MessageSender;
}
namespace KPIM { class ProgressDialog; }
using KMail::MailServiceImpl;
using KMail::UndoStack;
using KMail::JobScheduler;
using KPIM::ProgressDialog;
class KMFilterMgr;
class KMFilterActionDict;
class AkonadiSender;
class FolderCollection;
namespace KPIMIdentities {
  class Identity;
  class IdentityManager;
}
class KMKernel;
class KComponentData;
class QTimer;
class KMMainWin;
class KMainWindow;
class KSystemTrayIcon;
class KMMainWidget;
class ConfigureDialog;
class FolderCollectionMonitor;
class KMAgentManager;

/**
 * @short Central point of coordination in KMail
 *
 * The KMKernel class represents the core of KMail, where the different parts
 * come together and are coordinated. It is currently also the class which exports
 * KMail's main D-BUS interfaces. The kernel is responsible for creating various
 * (singleton) objects such as the UndoStack, the folder managers and filter
 * manager, etc.
 */
class KMAIL_EXPORT KMKernel : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.kmail")

public:
  explicit KMKernel (QObject *parent=0, const char *name=0);
  ~KMKernel ();

/**
 * Start of D-Bus callable stuff. The D-Bus methods need to be public slots,
 * otherwise they can't be accessed.
 */
public Q_SLOTS:

  Q_SCRIPTABLE void checkMail();
  Q_SCRIPTABLE void openReader() { openReader( false ); }

  /**
   * Compact all folders, used for the GUI action (and from D-Bus)
   */
  Q_SCRIPTABLE void compactAllFolders();

  /**
   * Pauses all background jobs and does not
   * allow new background jobs to be started.
   */
  Q_SCRIPTABLE void pauseBackgroundJobs();

  /**
   * Resumes all background jobs and allows
   * new jobs to be started.
   */
  Q_SCRIPTABLE void resumeBackgroundJobs();

  /**
   * Stops all network related jobs and enter offline mode
   * New network jobs cannot be started.
   */
  Q_SCRIPTABLE void stopNetworkJobs();

  /**
   * Resumes all network related jobs and enter online mode
   * New network jobs can be started.
   */
  Q_SCRIPTABLE void resumeNetworkJobs();

  Q_SCRIPTABLE QStringList folderList() const;

  Q_SCRIPTABLE QStringList accounts();

  /**
   * Checks the account with the specified name for new mail.
   * If the account name is empty, all accounts not excluded from manual
   * mail check will be checked.
   */
  Q_SCRIPTABLE void checkAccount( const QString & account );

  Q_SCRIPTABLE void selectFolder( const QString & folder );

  Q_SCRIPTABLE QString getFolder( const QString & vpath );

  Q_SCRIPTABLE bool canQueryClose();

  Q_SCRIPTABLE int timeOfLastMessageCountChange() const;

  Q_SCRIPTABLE bool handleCommandLine( bool noArgsOpensReader );

  Q_SCRIPTABLE QString debugScheduler();

  /**
   * returns id of composer if more are opened
   */
  Q_SCRIPTABLE int openComposer( const QString & to,
                                 const QString & cc,
                                 const QString & bcc,
                                 const QString & subject,
                                 const QString & body,
                                 bool hidden,
                                 const QString & messageFile,
                                 const QStringList & attachmentPaths,
                                 const QStringList & customHeaders );

  Q_SCRIPTABLE int openComposer( const QString & to,
                                 const QString & cc,
                                 const QString & bcc,
                                 const QString & subject,
                                 const QString & body,
                                 bool hidden,
                                 const QString & attachName,
                                 const QByteArray & attachCte,
                                 const QByteArray  &attachData,
                                 const QByteArray & attachType,
                                 const QByteArray & attachSubType,
                                 const QByteArray & attachParamAttr,
                                 const QString & attachParamValue,
                                 const QByteArray & attachContDisp,
                                 const QByteArray & attachCharset,
                                 unsigned int identity );

  Q_SCRIPTABLE QDBusObjectPath openComposer( const QString & to,
                                             const QString & cc,
                                             const QString & bcc,
                                             const QString & subject,
                                             const QString & body,
                                             bool hidden );

  /**
   * D-Bus call used by the Kontact plugin to create a new message.
   */
  Q_SCRIPTABLE QDBusObjectPath newMessage( const QString & to,
                                           const QString & cc,
                                           const QString & bcc,
                                           bool hidden,
                                           bool useFolderId,
                                           const QString & messageFile,
                                           const QString & attachURL );

  Q_SCRIPTABLE bool showMail( quint32 serialNumber, const QString & messageId );

  Q_SCRIPTABLE int viewMessage( const KUrl & messageFile );

Q_SIGNALS:

  Q_SCRIPTABLE void unreadCountChanged();

/**
 * End of D-Bus callable stuff
 */


public:

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

  static KMKernel *self();
  static KSharedConfig::Ptr config();

  void init();
  void setupDBus();
  void readConfig();


  KMAgentManager *agentManager();
  Akonadi::ChangeRecorder *monitor();

//TODO port to akonadi   void cleanupImapFolders();
  void recoverDeadLetters();
  void initFolders();
  void closeAllKMailWindows();
  void cleanup(void);
  void quit();
  bool doSessionManagement();
  bool firstInstance() { return the_firstInstance; }
  void setFirstInstance(bool value) { the_firstInstance = value; }
  void action( bool mailto, bool check, const QString &to, const QString &cc,
               const QString &bcc, const QString &subj, const QString &body,
               const KUrl &messageFile, const KUrl::List &attach,
               const QStringList &customHeaders );
  void byteArrayToRemoteFile( const QByteArray&, const KUrl&,
                              bool overwrite = false );
  bool folderIsDraftOrOutbox(const Akonadi::Collection &);
  bool folderIsDrafts(const Akonadi::Collection&);

  bool folderIsTemplates(const Akonadi::Collection &);
  bool folderIsTrash( const Akonadi::Collection & );

  Akonadi::Collection trashCollectionFromResource( const Akonadi::Collection & col );

  /**
   * Returns true if the folder is one of the sent-mail folders.
   */
  bool folderIsSentMailFolder( const Akonadi::Collection& );

  Akonadi::Collection findFolderCollectionById( const QString& id );

  bool isImapFolder( const Akonadi::Collection& );

  const KComponentData &xmlGuiInstance() { return mXmlGuiInstance; }
  void setXmlGuiInstance( const KComponentData &instance ) { mXmlGuiInstance = instance; }


  Akonadi::Collection inboxCollectionFolder();
  Akonadi::Collection outboxCollectionFolder();
  Akonadi::Collection sentCollectionFolder();
  Akonadi::Collection trashCollectionFolder();
  Akonadi::Collection draftsCollectionFolder();
  Akonadi::Collection templatesCollectionFolder();

  bool isSystemFolderCollection( const Akonadi::Collection &col);


  UndoStack *undoStack() { return the_undoStack; }
  KMFilterMgr *filterMgr() { return the_filterMgr; }
  KMFilterMgr *popFilterMgr() { return the_popFilterMgr; }
  KMFilterActionDict *filterActionDict() { return the_filterActionDict; }
  KMail::MessageSender *msgSender();

  ThreadWeaver::Weaver *weaver() { return the_weaver; }
  /** return the pointer to the identity manager */
  KPIMIdentities::IdentityManager *identityManager();

  JobScheduler* jobScheduler() { return mJobScheduler; }

  /** Expire all folders, used for the gui action */
  void expireAllFoldersNow();

  bool smartQuote() const { return mSmartQuote; }
  bool wordWrap() const { return mWordWrap;}
  int wrapCol() const { return mWrapCol;}

#if 0 //TODO port to akonadi
  KMailICalIfaceImpl& iCalIface();
#endif

  bool firstStart() const { return the_firstStart; }
  /** Mark first start as done */
  void firstStartDone() { the_firstStart = false; }
  QString previousVersion() const { return the_previousVersion; }
  bool startingUp() const { return the_startingUp; }
  void setStartingUp (bool flag) { the_startingUp = flag; }
  bool shuttingDown() const { return the_shuttingDown; }
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

  bool registerSystemTrayApplet( const KSystemTrayIcon* );
  bool unregisterSystemTrayApplet( const KSystemTrayIcon* );

  /// Reimplemented from KMailIface
  void emergencyExit( const QString& reason );

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
   * Called by the folder tree if the count of unread/total messages changed.
   */
  void messageCountChanged();


  /** Get first mainwidget */
  KMMainWidget *getKMMainWidget();

  /** @return a list of all folders from all folder managers. */
  QList<Akonadi::Collection> allFoldersCollection();
  void raise();

  /** Custom templates have changed, so all windows using them need
      to regenerate their menus */
  void updatedTemplates();


  void findCreateDefaultCollection( Akonadi::SpecialMailCollections::Type );
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
  void slotResult(KJob*);

signals:
  void configChanged();
  void onlineStatusChanged( GlobalSettings::EnumNetworkState::type );
  void customTemplatesChanged();

private slots:
  /** Updates identities when a transport has been deleted. */
  void transportRemoved( int id, const QString &name );
  /** Updates identities when a transport has been renamed. */
  void transportRenamed( int id, const QString &oldName, const QString &newName );
  void createDefaultCollectionDone( KJob * job);

private:
  void openReader( bool onlyCheck );
  QSharedPointer<FolderCollection> currentFolderCollection();



  Akonadi::Collection the_inboxCollectionFolder;
  Akonadi::Collection the_outboxCollectionFolder;
  Akonadi::Collection the_sentCollectionFolder;
  Akonadi::Collection the_trashCollectionFolder;
  Akonadi::Collection the_draftsCollectionFolder;
  Akonadi::Collection the_templatesCollectionFolder;

  UndoStack *the_undoStack;
  KMFilterMgr *the_filterMgr;
  KMFilterMgr *the_popFilterMgr;
  KMFilterActionDict *the_filterActionDict;
  mutable KPIMIdentities::IdentityManager *mIdentityManager;
  AkonadiSender *the_msgSender;
  struct putData
  {
    KUrl url;
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

  KSharedConfig::Ptr mConfig;
  QTextCodec *netCodec;
  KComponentData mXmlGuiInstance;
  ConfigureDialog *mConfigureDialog;

  QTimer *mBackgroundTasksTimer;
  JobScheduler* mJobScheduler;
  // temporary mainwin
  KMMainWin *mWin;
  MailServiceImpl *mMailService;

  // the time of the last change of the unread or total count of a folder;
  // this can be queried via D-Bus in order to determine whether the counts
  // need to be updated (e.g. in the Summary in Kontact)
  int mTimeOfLastMessageCountChange;

  // true if the context menu of KMFolderTree or KMHeaders is shown
  // this is necessary to know in order to prevent a dead lock between the
  // context menus and the pinentry program
  bool mContextMenuShown;

  QList<const KSystemTrayIcon*> systemTrayApplets;

  /* Weaver */
  ThreadWeaver::Weaver *the_weaver;


  // variables used by dbusAddMessage()
  QStringList           mAddMessageMsgIds;
  QString               mAddMessageLastFolder;

  FolderCollectionMonitor *mFolderCollectionMonitor;
  KMAgentManager *mAgentManager;

  // special debug area
  int mStorageDebug;
  bool mSmartQuote : 1, mWordWrap : 1;
  int mWrapCol;
};

#endif // _KMKERNEL_H
