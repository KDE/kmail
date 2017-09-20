//

#ifndef _KMKERNEL_H
#define _KMKERNEL_H

#include "mailcommon/mailinterfaces.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QDBusObjectPath>

#include <qurl.h>

#include "kmail_export.h"
#include "settings/kmailsettings.h"
#include <AkonadiCore/servermanager.h>
#include "messageviewer/viewer.h"
#include "libkdepim/progressmanager.h"

#define kmkernel KMKernel::self()
#define kmconfig KMKernel::config()

class QAbstractItemModel;
namespace Akonadi {
class Collection;
class ChangeRecorder;
class EntityTreeModel;
class EntityMimeTypeFilterModel;
}

namespace Akonadi {
namespace Search {
namespace PIM {
class IndexedItems;
}
}
}

namespace KIO {
class Job;
}

namespace MessageComposer {
class MessageSender;
}
namespace PimCommon {
class AutoCorrection;
}

/** The KMail namespace contains classes used for KMail.
* This is to keep them out of the way from all the other
* un-namespaced classes in libs and the rest of PIM.
*/
namespace KMail {
class MailServiceImpl;
class UndoStack;
class KMSystemTray;
}
using KMail::MailServiceImpl;
using KMail::UndoStack;
namespace MessageComposer {
class AkonadiSender;
}

namespace KIdentityManagement {
class Identity;
class IdentityManager;
}

namespace MailCommon {
class Kernel;
class FolderSettings;
class FolderCollectionMonitor;
class JobScheduler;
class KMFilterDialog;
class MailCommonSettings;
}

class QTimer;
class KMainWindow;
class KMMainWidget;
class ConfigureDialog;
class FolderArchiveManager;
class CheckIndexingManager;

/**
 * @short Central point of coordination in KMail
 *
 * The KMKernel class represents the core of KMail, where the different parts
 * come together and are coordinated. It is currently also the class which exports
 * KMail's main D-BUS interfaces.
 *
 * The kernel is responsible for creating various
 * (singleton) objects such as the identity manager and the message sender.
 *
 * The kernel also creates an Akonadi Session, Monitor and EntityTreeModel. These
 * are shared so that other objects in KMail have access to it. Having only one EntityTreeModel
 * instead of many reduces the overall communication with the Akonadi server.
 *
 * The kernel also manages some stuff that should be factored out:
 * - default collection handling, like inboxCollectionFolder()
 * - job handling, like jobScheduler()
 * - handling of some config settings, like wrapCol()
 * - various other stuff
 */
class KMAIL_EXPORT KMKernel : public QObject, public MailCommon::IKernel, public MailCommon::ISettings, public MailCommon::IFilter
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.kmail")

public:
    explicit KMKernel(QObject *parent = nullptr);
    ~KMKernel();

    /**
    * Start of D-Bus callable stuff. The D-Bus methods need to be public slots,
    * otherwise they can't be accessed.
    */
public Q_SLOTS:

    Q_SCRIPTABLE void checkMail();
    Q_SCRIPTABLE void openReader();

    /**
    * Enables/disables systray icon changing when mail arrives.
    * With this disabled the systray icon will always be the same.
    */
    Q_SCRIPTABLE void setSystrayIconNotificationsEnabled(bool enabled);

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

    Q_SCRIPTABLE QStringList accounts();

    Q_SCRIPTABLE void makeResourceOnline(MessageViewer::Viewer::ResourceOnlineMode mode);

    /**
    * Checks the account with the specified name for new mail.
    * If the account name is empty, all accounts not excluded from manual
    * mail check will be checked.
    */
    Q_SCRIPTABLE void checkAccount(const QString &account);

    Q_SCRIPTABLE bool selectFolder(const QString &folder);

    Q_SCRIPTABLE bool canQueryClose();

    Q_SCRIPTABLE bool handleCommandLine(bool noArgsOpensReader, const QStringList &args, const QString &workingDir);

    /**
    * Opens a composer window and prefills it with different
    * message parts.
    *
    *
    * @param to A comma separated list of To addresses.
    * @param cc A comma separated list of CC addresses.
    * @param bcc A comma separated list of BCC addresses.
    * @param subject The message subject.
    * @param body The message body.
    * @param hidden Whether the composer window shall initially be hidden.
    * @param messageFile A message file that will be used as message body.
    * @param attachmentPaths A list of files that will be attached to the message.
    * @param customHeaders A list of custom headers.
    */
    Q_SCRIPTABLE void openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden, const QString &messageFile,
                                   const QStringList &attachmentPaths, const QStringList &customHeaders, const QString &replyTo = QString(),
                                   const QString &inReplyTo = QString(), const QString &identity = QString());

    /**
    * Opens a composer window and prefills it with different
    * message parts.
    *
    * @param to A comma separated list of To addresses.
    * @param cc A comma separated list of CC addresses.
    * @param bcc A comma separated list of BCC addresses.
    * @param subject The message subject.
    * @param body The message body.
    * @param hidden Whether the composer window shall initially be hidden.
    * @param attachName The name of the attachment.
    * @param attachCte The content transfer encoding of the attachment.
    * @param attachData The raw data of the attachment.
    * @param attachType The mime type of the attachment.
    * @param attachSubType The sub mime type of the attachment.
    * @param attachParamAttr The parameter attribute of the attachment.
    * @param attachParamValue The parameter value of the attachment.
    * @param attachContDisp The content display type of the attachment.
    * @param attachCharset The charset of the attachment.
    * @param identity The identity identifier which will be used as sender identity.
    */
    Q_SCRIPTABLE void openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden, const QString &attachName,
                                   const QByteArray &attachCte, const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr,
                                   const QString &attachParamValue, const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity);

    /**
    * Opens a composer window and prefills it with different
    * message parts.
    * @since 5.0
    *
    * @param to A comma separated list of To addresses.
    * @param cc A comma separated list of CC addresses.
    * @param bcc A comma separated list of BCC addresses.
    * @param subject The message subject.
    * @param body The message body.
    * @param attachName The name of the attachment.
    * @param attachCte The content transfer encoding of the attachment.
    * @param attachData The raw data of the attachment.
    * @param attachType The mime type of the attachment.
    * @param attachSubType The sub mime type of the attachment.
    * @param attachParamAttr The parameter attribute of the attachment.
    * @param attachParamValue The parameter value of the attachment.
    * @param attachContDisp The content display type of the attachment.
    * @param attachCharset The charset of the attachment.
    * @param identity The identity identifier which will be used as sender identity.
    */
    Q_SCRIPTABLE void openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, const QString &attachName, const QByteArray &attachCte,
                                   const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue,
                                   const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity);

    /**
    * Opens a composer window and prefills it with different
    * message parts.
    *
    *
    * @param to A comma separated list of To addresses.
    * @param cc A comma separated list of CC addresses.
    * @param bcc A comma separated list of BCC addresses.
    * @param subject The message subject.
    * @param body The message body.
    * @param hidden Whether the composer window shall initially be hidden.
    */
    Q_SCRIPTABLE void openComposer(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden);

    /**
    * Opens a composer window and prefills it with different
    * message parts.
    *
    * @returns The DBus object path for the composer.
    *
    * @param to A comma separated list of To addresses.
    * @param cc A comma separated list of CC addresses.
    * @param bcc A comma separated list of BCC addresses.
    * @param subject The message subject.
    * @param body The message body.
    * @param hidden Whether the composer window shall initially be hidden.
    * @param useFolderId The id of the folder whose associated identity will be used.
    * @param messageFile A message file that will be used as message body.
    * @param attachURL The URL to the file that will be attached to the message.
    */
    Q_SCRIPTABLE void newMessage(const QString &to, const QString &cc, const QString &bcc, bool hidden, bool useFolderId, const QString &messageFile, const QString &attachURL);

    Q_SCRIPTABLE bool showMail(qint64 serialNumber);

    Q_SCRIPTABLE int viewMessage(const QString &messageFile);

    Q_SCRIPTABLE void updateConfig();

    Q_SCRIPTABLE void showFolder(const QString &collectionId);

    Q_SCRIPTABLE void reloadFolderArchiveConfig();

    /**
    * End of D-Bus callable stuff
    */

public:
    void checkMailOnStartup();

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
    KSharedConfig::Ptr config() override;
    void syncConfig() override;

    void init();
    void setupDBus();

    void expunge(Akonadi::Collection::Id col, bool sync) override;
    Akonadi::ChangeRecorder *folderCollectionMonitor() const override;

    /**
    * Returns the main model, which contains all folders and the items of recently opened folders.
    */
    Akonadi::EntityTreeModel *entityTreeModel() const;

    /**
    * Returns a model of all folders in KMail. This is basically the same as entityTreeModel(),
    * but with items filtered out, the model contains only collections.
    */
    Akonadi::EntityMimeTypeFilterModel *collectionModel() const override;

    void recoverDeadLetters();
    void closeAllKMailWindows();
    void cleanup(void);
    void quit();
    bool doSessionManagement();
    bool firstInstance() const;
    void setFirstInstance(bool value);
    void action(bool mailto, bool check, const QString &to, const QString &cc, const QString &bcc, const QString &subj, const QString &body, const QUrl &messageFile, const QList<QUrl> &attach,
                const QStringList &customHeaders, const QString &replyTo, const QString &inReplyTo, const QString &identity);

    //sets online status for akonadi accounts. true for online, false for offline
    void setAccountStatus(bool);

    const QString xmlGuiInstanceName() const;
    void setXmlGuiInstanceName(const QString &instance);

    UndoStack *undoStack() const;
    MessageComposer::MessageSender *msgSender() override;

    void openFilterDialog(bool createDummyFilter = true) override;
    void createFilter(const QByteArray &field, const QString &value) override;

    /** return the pointer to the identity manager */
    KIdentityManagement::IdentityManager *identityManager() override;

    MailCommon::JobScheduler *jobScheduler() const override
    {
        return mJobScheduler;
    }

    /** Expire all folders, used for the gui action */
    void expireAllFoldersNow();

    bool firstStart() const;
    QString previousVersion() const;
    bool shuttingDown() const;
    void setShuttingDown(bool flag);

    /** Returns true if we have a system tray applet. This is needed in order
    *  to know whether the application should be allowed to exit in case the
    *  last visible composer or separate message window is closed.
    */
    bool haveSystemTrayApplet() const;

    QTextCodec *networkCodec() const;

    /** returns a reference to the first Mainwin or a temporary Mainwin */
    KMainWindow *mainWin();

    /** Get first mainwidget */
    KMMainWidget *getKMMainWidget();

    /**
    * Returns a list of all currently loaded folders. Since folders are loaded async, this
    * is empty at startup.
    */
    Akonadi::Collection::List allFolders() const;

    /**
     * Includes all subfolders of @p col, including the @p col itself.
     */
    Akonadi::Collection::List subfolders(const Akonadi::Collection &col) const;

    //
    void selectCollectionFromId(Akonadi::Collection::Id id);

    void raise();

    void stopAgentInstance();

    //ISettings
    bool showPopupAfterDnD() override;

    bool excludeImportantMailFromExpiry() override;

    qreal closeToQuotaThreshold() override;

    Akonadi::Collection::Id lastSelectedFolder() override;
    void setLastSelectedFolder(Akonadi::Collection::Id col) override;

    QStringList customTemplates() override;

    void checkFolderFromResources(const Akonadi::Collection::List &collectionList);

    const QAbstractItemModel *treeviewModelSelection();

    void savePaneSelection();

    void updatePaneTagComboBox();

    PimCommon::AutoCorrection *composerAutoCorrection();

    void toggleSystemTray();
    FolderArchiveManager *folderArchiveManager() const;

    bool allowToDebugAkonadiSeachSupport() const;

    Akonadi::Search::PIM::IndexedItems *indexedItems() const;

    void cleanupTemporaryFiles();
    MailCommon::MailCommonSettings *mailCommonSettings() const;

protected:
    void agentInstanceBroken(const Akonadi::AgentInstance &instance);

public Q_SLOTS:

    void updateSystemTray() override;

    /** Custom templates have changed, so all windows using them need
      to regenerate their menus */
    void updatedTemplates();

    /// Save contents of all open composer widnows to ~/dead.letter
    void dumpDeadLetters();

    /** Call this slot instead of directly KConfig::sync() to
      minimize the overall config writes. Calling this slot will
      schedule a sync of the application config file using a timer, so
      that many consecutive calls can be condensed into a single
      sync, which is more efficient. */
    void slotRequestConfigSync();

    /**
    * Sync the config immediatley
    */
    void slotSyncConfig();

    void slotShowConfigurationDialog();
    void slotRunBackgroundTasks();

    void slotConfigChanged();

Q_SIGNALS:
    void configChanged();
    void onlineStatusChanged(KMailSettings::EnumNetworkState::type);
    void customTemplatesChanged();

    void startCheckMail();
    void endCheckMail();

private Q_SLOTS:
    /** Updates identities when a transport has been deleted. */
    void transportRemoved(int id, const QString &name);
    /** Updates identities when a transport has been renamed. */
    void transportRenamed(int id, const QString &oldName, const QString &newName);
    void itemDispatchStarted();
    void instanceStatusChanged(const Akonadi::AgentInstance &);

    void akonadiStateChanged(Akonadi::ServerManager::State);
    void slotProgressItemCompletedOrCanceled(KPIM::ProgressItem *item);
    void slotInstanceError(const Akonadi::AgentInstance &instance, const QString &message);
    void slotInstanceWarning(const Akonadi::AgentInstance &instance, const QString &message);
    void slotCollectionRemoved(const Akonadi::Collection &col);
    void slotDeleteIdentity(uint identity);
    void slotInstanceRemoved(const Akonadi::AgentInstance &);
    void slotSystemNetworkStatusChanged(bool isOnline);
    void slotCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &set);

    void slotCheckAccount(Akonadi::ServerManager::State state);
private:
    void viewMessage(const QUrl &url);
    Akonadi::Collection currentCollection();

    /*
     * Fills a composer cWin
     *
    */
    void fillComposer(bool hidden, const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, const QString &attachName, const QByteArray &attachCte,
                      const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue,
                      const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity, bool forceShowWindow);

    void verifyAccount();
    void resourceGoOnLine();
    void openReader(bool onlyCheck);
    QSharedPointer<MailCommon::FolderSettings> currentFolderCollection();

    UndoStack *the_undoStack = nullptr;
    MessageComposer::AkonadiSender *the_msgSender = nullptr;
    /** previous KMail version. If different from current,
      the user has just updated. read from config */
    QString the_previousVersion;
    /** is this the first start?  read from config */
    bool the_firstStart;
    /** are we going down? set from here */
    bool the_shuttingDown;
    /** true unles kmail is closed by session management */
    bool the_firstInstance;

    KSharedConfig::Ptr mConfig;
    QTextCodec *mNetCodec = nullptr;
    QString mXmlGuiInstance;
    ConfigureDialog *mConfigureDialog = nullptr;

    QTimer *mBackgroundTasksTimer = nullptr;
    MailCommon::JobScheduler *mJobScheduler = nullptr;
    MailServiceImpl *mMailService = nullptr;

    bool mSystemNetworkStatus = true;

    KMail::KMSystemTray *mSystemTray = nullptr;
    QHash<QString, KPIM::ProgressItem::CryptoStatus> mResourceCryptoSettingCache;
    MailCommon::FolderCollectionMonitor *mFolderCollectionMonitor = nullptr;
    Akonadi::EntityTreeModel *mEntityTreeModel = nullptr;
    Akonadi::EntityMimeTypeFilterModel *mCollectionModel = nullptr;

    /// List of Akonadi resources that are currently being checked.
    QStringList mResourcesBeingChecked;

    QPointer<MailCommon::KMFilterDialog> mFilterEditDialog;
    PimCommon::AutoCorrection *mAutoCorrection = nullptr;
    FolderArchiveManager *mFolderArchiveManager = nullptr;
    CheckIndexingManager *mCheckIndexingManager = nullptr;
    Akonadi::Search::PIM::IndexedItems *mIndexedItems = nullptr;
    MailCommon::MailCommonSettings *mMailCommonSettings = nullptr;
    bool mDebugAkonadiSearch = false;
};

#endif // _KMKERNEL_H
