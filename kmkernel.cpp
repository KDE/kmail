/*  -*- mode: C++; c-file-style: "gnu" -*- */

#include "kmkernel.h"

#include "globalsettings.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmstartup.h"
#include "kmmainwin.h"
#include "composer.h"
#include "kmreadermainwin.h"
#include "undostack.h"
#include <kpimutils/kfileio.h>
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "recentaddresses.h"
using KPIM::RecentAddresses;
#include "configuredialog.h"
#include "kmcommands.h"
#include "kmsystemtray.h"
#include "stringutil.h"
#include "mailutil.h"
#include "mailcommon/pop3settings.h"
#include "mailcommon/foldertreeview.h"
#include "mailcommon/filter/kmfilterdialog.h"


// kdepim includes
#include "kdepim-version.h"

// kdepimlibs includes
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/dispatcherinterface.h>
#include <akonadi/servermanager.h>

#include <kde_file.h>
#include <kwindowsystem.h>
#include "mailserviceimpl.h"
using KMail::MailServiceImpl;
#include "jobscheduler.h"

#include "messagecore/globalsettings.h"
#include "messagelist/core/settings.h"
#include "messagelist/messagelistutil.h"
#include "messageviewer/globalsettings.h"
#include "messagecomposer/akonadisender.h"
#include "messagecomposersettings.h"
#include "messagecomposer/messagehelper.h"
#include "messagecomposer/messagecomposersettings.h"

#include "templateparser/templateparser.h"
#include "templateparser/globalsettings_base.h"
#include "templateparser/templatesutil.h"

#include "foldercollection.h"

#include <kmessagebox.h>
#include <knotification.h>
#include <progressmanager.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kpassivepopup.h>
#include <kapplication.h>
#include <ksystemtrayicon.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kio/jobuidelegate.h>
#include <kprocess.h>
#include <KCrash>

#include <kmime/kmime_message.h>
#include <kmime/kmime_util.h>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/AgentManager>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/AttributeFactory>
#include <Akonadi/Session>
#include <Akonadi/EntityTreeModel>
#include <akonadi/entitymimetypefiltermodel.h>
#include <Akonadi/CollectionStatisticsJob>

#include <QDir>
#include <QWidget>
#include <QFileInfo>
#include <QtDBus/QtDBus>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kcmdlineargs.h>
#include <kstartupinfo.h>
#include <kmailadaptor.h>
#include "kmailinterface.h"
#include "foldercollectionmonitor.h"
#include "imapsettings.h"
#include "util.h"
#include "mailcommon/mailkernel.h"

#include "searchdescriptionattribute.h"

using namespace MailCommon;

static KMKernel * mySelf = 0;
static bool s_askingToGoOnline = false;

/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel (QObject *parent, const char *name) :
  QObject(parent),
  mIdentityManager(0), mConfigureDialog(0), mMailService(0),
  mSystemNetworkStatus ( Solid::Networking::status() )
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::SearchDescriptionAttribute>();
  QDBusConnection::sessionBus().registerService("org.kde.kmail");
  migrateFromKMail1();
  kDebug() << "Starting up...";

  setObjectName( name );
  mySelf = this;
  the_startingUp = true;
  closed_by_user = true;
  the_firstInstance = true;

  the_undoStack = 0;
  the_msgSender = 0;
  mFilterEditDialog = 0;
  mWin = 0;
  mWrapCol = 80;
  // make sure that we check for config updates before doing anything else
  KMKernel::config();
  // this shares the kmailrc parsing too (via KSharedConfig), and reads values from it
  // so better do it here, than in some code where changing the group of config()
  // would be unexpected
  GlobalSettings::self();

  mJobScheduler = new JobScheduler( this );
  mXmlGuiInstance = KComponentData();

  KMime::setFallbackCharEncoding( MessageCore::GlobalSettings::self()->fallbackCharacterEncoding() );
  KMime::setUseOutlookAttachmentEncoding( MessageComposer::MessageComposerSettings::self()->outlookCompatibleAttachments() );

  // cberzan: this crap moved to CodecManager ======================
  netCodec = QTextCodec::codecForName( KGlobal::locale()->encoding() );

  // In the case of Japan. Japanese locale name is "eucjp" but
  // The Japanese mail systems normally used "iso-2022-jp" of locale name.
  // We want to change locale name from eucjp to iso-2022-jp at KMail only.

  // (Introduction to i18n, 6.6 Limit of Locale technology):
  // EUC-JP is the de-facto standard for UNIX systems, ISO 2022-JP
  // is the standard for Internet, and Shift-JIS is the encoding
  // for Windows and Macintosh.
  if ( netCodec->name().toLower() == "eucjp"
#if defined Q_WS_WIN || defined Q_WS_MACX
    || netCodec->name().toLower() == "shift-jis" // OK?
#endif
  )
  {
    netCodec = QTextCodec::codecForName("jis7");
    // QTextCodec *cdc = QTextCodec::codecForName("jis7");
    // QTextCodec::setCodecForLocale(cdc);
    // KGlobal::locale()->setEncoding(cdc->mibEnum());
  }
  // until here ================================================

  mFolderCollectionMonitor = new FolderCollectionMonitor( this );

  connect( mFolderCollectionMonitor->monitor(), SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)), SLOT(slotCollectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );
  connect( mFolderCollectionMonitor->monitor(), SIGNAL(collectionRemoved(Akonadi::Collection)), SLOT(slotCollectionRemoved(Akonadi::Collection)));

  Akonadi::Session *session = new Akonadi::Session( "KMail Kernel ETM", this );
  folderCollectionMonitor()->setSession( session );
  mEntityTreeModel = new Akonadi::EntityTreeModel( folderCollectionMonitor(), this );
  mEntityTreeModel->setIncludeUnsubscribed( false );
  mEntityTreeModel->setItemPopulationStrategy( Akonadi::EntityTreeModel::LazyPopulation );

  mCollectionModel = new Akonadi::EntityMimeTypeFilterModel( this );
  mCollectionModel->setSourceModel( mEntityTreeModel );
  mCollectionModel->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
  mCollectionModel->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );
  mCollectionModel->setDynamicSortFilter( true );
  mCollectionModel->setSortCaseSensitivity( Qt::CaseInsensitive );

  connect( MailTransport::TransportManager::self(),
           SIGNAL(transportRemoved(int,QString)),
           SLOT(transportRemoved(int,QString)) );
  connect( MailTransport::TransportManager::self(),
           SIGNAL(transportRenamed(int,QString,QString)),
           SLOT(transportRenamed(int,QString,QString)) );

  QDBusConnection::sessionBus().connect(QString(), QLatin1String( "/MailDispatcherAgent" ), "org.freedesktop.Akonadi.MailDispatcherAgent", "itemDispatchStarted",this, SLOT(itemDispatchStarted()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceStatusChanged(Akonadi::AgentInstance)),
           this, SLOT(instanceStatusChanged(Akonadi::AgentInstance)) );

  connect( Akonadi::AgentManager::self(), SIGNAL(instanceError(Akonadi::AgentInstance,QString)),
           this, SLOT(slotInstanceError(Akonadi::AgentInstance,QString)) );

  connect( Akonadi::AgentManager::self(), SIGNAL(instanceWarning(Akonadi::AgentInstance,QString)),
           SLOT(slotInstanceWarning(Akonadi::AgentInstance,QString)) );

  connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
           this, SLOT(slotInstanceRemoved(Akonadi::AgentInstance)) );

  connect ( Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
            this, SLOT(slotSystemNetworkStatusChanged(Solid::Networking::Status)) );
  
  connect( KPIM::ProgressManager::instance(), SIGNAL(progressItemCompleted(KPIM::ProgressItem*)),
           this, SLOT(slotProgressItemCompletedOrCanceled(KPIM::ProgressItem*)) );
  connect( KPIM::ProgressManager::instance(), SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
           this, SLOT(slotProgressItemCompletedOrCanceled(KPIM::ProgressItem*)) );
  connect( identityManager(), SIGNAL(deleted(uint)), this, SLOT(slotDeleteIdentity(uint)) );
  CommonKernel->registerKernelIf( this );
  CommonKernel->registerSettingsIf( this );
  CommonKernel->registerFilterIf( this );
}

KMKernel::~KMKernel ()
{
  delete mMailService;
  mMailService = 0;

  stopAgentInstance();
  slotSyncConfig();

  mySelf = 0;
  kDebug();
}

void KMKernel::migrateFromKMail1()
{
  // Akonadi migration
  // check if there is something to migrate at all
  bool needMigration = true;
  KConfig oldKMailConfig( "kmailrc", KConfig::NoGlobals );
  if ( oldKMailConfig.hasGroup("General") ||
       ( oldKMailConfig.groupList().count() == 1 &&
         oldKMailConfig.groupList().first() == "$Version" ) ) {
    const QFileInfo oldDataDirFileInfo( KStandardDirs::locateLocal( "data", "kmail" ) );
    if ( !oldDataDirFileInfo.exists() || !oldDataDirFileInfo.isDir() ) {
      // neither config or data, the migrator cannot do anything useful anyways
      needMigration = false;
    }
  } else {
    needMigration = false;
  }

  KConfig config( "kmail-migratorrc" );
  KConfigGroup migrationCfg( &config, "Migration" );
  if ( needMigration ) {
    const bool enabled = migrationCfg.readEntry( "Enabled", false );
    const int currentVersion = migrationCfg.readEntry( "Version", 0 );
    const int targetVersion = migrationCfg.readEntry( "TargetVersion", 1 );
    if ( enabled && currentVersion < targetVersion ) {
      const int choice = KMessageBox::questionYesNoCancel( 0, i18n(
          "<b>Thanks for using KMail2!</b>"
          "<p>KMail2 uses a new storage technology that requires migration of your current KMail data and configuration.</p>\n"
          "<p>The conversion process can take a lot of time (depending on the amount of email you have) and it <em>must not be interrupted</em>.</p>\n"
          "<p>You can:</p><ul>"
          "<li>Migrate now (be prepared to wait)</li>"
          "<li>Skip the migration and start with fresh data and configuration</li>"
          "<li>Cancel and exit KMail2.</li>"
          "</ul>"
          "<p><a href=\"http://userbase.kde.org/Akonadi\">More Information...</a></p>"
        ), i18n( "KMail Migration" ), KGuiItem(i18n( "Migrate Now" )), KGuiItem(i18n( "Skip Migration" )), KStandardGuiItem::cancel(),
        QString(), KMessageBox::Notify | KMessageBox::Dangerous | KMessageBox::AllowLink );
      if ( choice == KMessageBox::Cancel )
        exit( 1 );

      if ( choice != KMessageBox::Yes ) {  // user skipped migration
        // we only will make one attempt at this
        migrationCfg.writeEntry( "Version", targetVersion );
        migrationCfg.sync();

        return;
      }

      kDebug() << "Performing Akonadi migration. Good luck!";
      KProcess proc;
      QStringList args = QStringList() << "--interactive-on-change";
      const QString path = KStandardDirs::findExe( QLatin1String("kmail-migrator" ) );
      proc.setProgram( path, args );
      proc.start();
      bool result = proc.waitForStarted();
      if ( result ) {
        result = proc.waitForFinished( -1 );
      }
      if ( result && proc.exitCode() == 0 ) {
        kDebug() << "Akonadi migration has been successful";
      } else {
        // exit code 1 means it is already running, so we are probably called by a migrator instance
        kError() << "Akonadi migration failed!";
        kError() << "command was: " << proc.program();
        kError() << "exit code: " << proc.exitCode();
        kError() << "stdout: " << proc.readAllStandardOutput();
        kError() << "stderr: " << proc.readAllStandardError();

        KMessageBox::error( 0, i18n("Migration to KMail 2 failed. In case you want to try again, run 'kmail-migrator --interactive' manually."),
          i18n( "Migration Failed" ) );
        return;
      }
    }
  } else {
    migrationCfg.writeEntry( "Enabled", false );
    migrationCfg.sync();
  }
}

Akonadi::ChangeRecorder * KMKernel::folderCollectionMonitor() const
{
  return mFolderCollectionMonitor->monitor();
}

Akonadi::EntityTreeModel * KMKernel::entityTreeModel() const
{
  return mEntityTreeModel;
}

Akonadi::EntityMimeTypeFilterModel * KMKernel::collectionModel() const
{
  return mCollectionModel;
}

void KMKernel::setupDBus()
{
  (void) new KmailAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/KMail", this );
  mMailService = new MailServiceImpl();
}

static KUrl makeAbsoluteUrl( const QString& str )
{
    KUrl url( str );
    if ( url.protocol().isEmpty() ) {
      const QString newUrl = KCmdLineArgs::cwd() + '/' + url.fileName();
      return KUrl( newUrl );
    }
    else {
      return url;
    }
}

bool KMKernel::handleCommandLine( bool noArgsOpensReader )
{
  QString to, cc, bcc, subj, body;
  QStringList customHeaders;
  KUrl messageFile;
  KUrl::List attachURLs;
  bool mailto = false;
  bool checkMail = false;
  bool viewOnly = false;
  bool calledWithSession = false; // for ignoring '-session foo'

  // process args:
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->isSet("subject"))
  {
     subj = args->getOption("subject");
     // if kmail is called with 'kmail -session abc' then this doesn't mean
     // that the user wants to send a message with subject "ession" but
     // (most likely) that the user clicked on KMail's system tray applet
     // which results in KMKernel::raise() calling "kmail kmail newInstance"
     // via D-Bus which apparently executes the application with the original
     // command line arguments and those include "-session ..." if
     // kmail/kontact was restored by session management
     if ( subj == "ession" ) {
       subj.clear();
       calledWithSession = true;
     }
     else
       mailto = true;
  }

  if (args->isSet("cc"))
  {
     mailto = true;
     cc = args->getOption("cc");
  }

  if (args->isSet("bcc"))
  {
     mailto = true;
     bcc = args->getOption("bcc");
  }

  if (args->isSet("msg"))
  {
     mailto = true;
     messageFile.setPath( args->getOption("msg") );
  }

  if (args->isSet("body"))
  {
     mailto = true;
     body = args->getOption("body");
  }

  const QStringList attachList = args->getOptionList("attach");
  if ( !attachList.isEmpty() ) {
    mailto = true;
    QStringList::ConstIterator end = attachList.constEnd();
    for ( QStringList::ConstIterator it = attachList.constBegin();
          it != end; ++it ) {
      if ( !(*it).isEmpty() ) {
        attachURLs.append( makeAbsoluteUrl( *it ) );
      }
    }
  }

  customHeaders = args->getOptionList("header");

  if (args->isSet("composer"))
    mailto = true;

  if (args->isSet("check"))
    checkMail = true;

  if ( args->isSet( "view" ) ) {
    viewOnly = true;
    const QString filename =
      args->getOption( "view" );
    messageFile = KUrl( filename );
    if ( !messageFile.isValid() ) {
      messageFile = KUrl();
      messageFile.setPath( filename );
    }
  }

  if ( !calledWithSession ) {
    // only read additional command line arguments if kmail/kontact is
    // not called with "-session foo"
    const int nbArgs = args->count();
    for(int i= 0; i < nbArgs; i++)
    {
      if ( args->arg(i).startsWith( QLatin1String( "mailto:" ), Qt::CaseInsensitive ) ) {
        QMap<QString, QString> values = MessageCore::StringUtil::parseMailtoUrl( args->url( i ) );
        if ( !values.value( "to" ).isEmpty() )
          to += values.value( "to" ) + ", ";
        if ( !values.value( "cc" ).isEmpty() )
          cc += values.value( "cc" ) + ", ";
        if ( !values.value( "subject" ).isEmpty() )
          subj = values.value( "subject" );
        if ( !values.value( "body" ).isEmpty() )
          body = values.value( "body" );
        if ( !values.value( "in-reply-to" ).isEmpty() )
          customHeaders << "In-Reply-To:" + values.value( "in-reply-to" );
        const QString attach = values.value( "attachment" );
        if ( !attach.isEmpty() ) {
            attachURLs << makeAbsoluteUrl( attach );
        }
      }
      else {
        QString tmpArg = args->arg(i);
        KUrl url( tmpArg );
        if (url.isValid() && !url.protocol().isEmpty())
          attachURLs += url;
        else
          to += tmpArg + ", ";
      }
      mailto = true;
    }
    if ( !to.isEmpty() ) {
      // cut off the superfluous trailing ", "
      to.truncate( to.length() - 2 );
    }
  }

  if ( !calledWithSession )
    args->clear();

  if ( !noArgsOpensReader && !mailto && !checkMail && !viewOnly )
    return false;

  if ( viewOnly )
    viewMessage( messageFile.url() );
  else
    action( mailto, checkMail, to, cc, bcc, subj, body, messageFile,
            attachURLs, customHeaders );
  return true;
}

/********************************************************************/
/*             D-Bus-callable, and command line actions              */
/********************************************************************/
void KMKernel::checkMail () //might create a new reader but won't show!!
{
   if ( !kmkernel->askToGoOnline() )
    return;

  const QString resourceGroupPattern( "Resource %1" );

  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    const QString id = type.identifier();
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( id ) );
    if ( group.readEntry( "IncludeInManualChecks", true ) ) {
      if ( !type.isOnline() )
        type.setIsOnline( true );
      if ( mResourcesBeingChecked.isEmpty() ) {
        kDebug() << "Starting manual mail check";
        emit startCheckMail();
      }

      if ( !mResourcesBeingChecked.contains( id ) ) {
        mResourcesBeingChecked.append( id );
      }
      type.synchronize();
    }
  }
}

QStringList KMKernel::accounts()
{
  QStringList accountLst;
  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach ( const Akonadi::AgentInstance& type, lst )
  {
    // Explicitly make a copy, as we're not changing values of the list but only
    // the local copy which is passed to action.
    accountLst<<type.identifier();
  }
  return accountLst;
}

void KMKernel::checkAccount( const QString &account ) //might create a new reader but won't show!!
{
  kDebug();
  if ( account.isEmpty() )
    checkMail();
  else {
    Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance( account );
    if ( agent.isValid() )
      agent.synchronize();
    else
      kDebug() << "- account with name '" << account <<"' not found";
  }
}

void KMKernel::openReader( bool onlyCheck )
{
  mWin = 0;
  KMainWindow *ktmw = 0;
  kDebug();

  foreach ( KMainWindow *window, KMainWindow::memberList() )
  {
    if ( ::qobject_cast<KMMainWin *>( window ) )
    {
      ktmw = window;
      break;
    }
  }

  bool activate;
  if (ktmw) {
    mWin = (KMMainWin *) ktmw;
    activate = !onlyCheck; // existing window: only activate if not --check
    if ( activate )
       mWin->show();
  }
  else {
    mWin = new KMMainWin;
    mWin->show();
    activate = false; // new window: no explicit activation (#73591)
  }

  if ( activate ) {
    // Activate window - doing this instead of KWindowSystem::activateWindow(mWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( mWin, kapp->startupId() );
#endif
  }
}

int KMKernel::openComposer( const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, bool hidden,
                            const QString &messageFile,
                            const QStringList &attachmentPaths,
                            const QStringList &customHeaders )
{
  kDebug();
  KMail::Composer::TemplateContext context = KMail::Composer::New;
  KMime::Message::Ptr msg( new KMime::Message );
  MessageHelper::initHeader( msg, identityManager() );
  msg->contentType()->setCharset("utf-8");

  if ( !to.isEmpty() )
    msg->to()->fromUnicodeString( to, "utf-8" );

  if ( !cc.isEmpty() )
    msg->cc()->fromUnicodeString( cc, "utf-8" );

  if ( !bcc.isEmpty() )
    msg->bcc()->fromUnicodeString( bcc, "utf-8" );

  if ( !subject.isEmpty() )
    msg->subject()->fromUnicodeString( subject, "utf-8" );

  KUrl messageUrl = KUrl( messageFile );
  if ( !messageUrl.isEmpty() && messageUrl.isLocalFile() ) {
    const QByteArray str = KPIMUtils::kFileToByteArray( messageUrl.toLocalFile(), true, false );
    if( !str.isEmpty() ) {
      context = KMail::Composer::NoTemplate;
      msg->setBody( QString::fromLocal8Bit( str.data(), str.size() ).toUtf8() );
    }
    else {
      TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
      parser.setIdentityManager( KMKernel::self()->identityManager() );
      parser.process( KMime::Message::Ptr() );
    }
  }
  else if ( !body.isEmpty() ) {
    context = KMail::Composer::NoTemplate;
    msg->setBody( body.toLatin1() );
  }
  else {
    TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
    parser.setIdentityManager( KMKernel::self()->identityManager() );
    parser.process( KMime::Message::Ptr() );
  }

  if ( !customHeaders.isEmpty() )
  {
    QStringList::ConstIterator end = customHeaders.constEnd();
    for ( QStringList::ConstIterator it = customHeaders.constBegin() ; it != end ; ++it )
      if ( !(*it).isEmpty() )
      {
        const int pos = (*it).indexOf( ':' );
        if ( pos > 0 )
        {
          QString header = (*it).left( pos ).trimmed();
          QString value = (*it).mid( pos+1 ).trimmed();
          if ( !header.isEmpty() && !value.isEmpty() ) {
            KMime::Headers::Generic *h = new KMime::Headers::Generic( header.toUtf8(), msg.get(), value.toUtf8() );
            msg->setHeader( h );
          }
        }
      }
  }

  KMail::Composer * cWin = KMail::makeComposer( msg, context );
  if (!to.isEmpty())
    cWin->setFocusToSubject();
  KUrl::List attachURLs = KUrl::List( attachmentPaths );
  for ( KUrl::List::ConstIterator it = attachURLs.constBegin() ; it != attachURLs.constEnd() ; ++it )
    cWin->addAttachment( (*it), "" );
  if ( !hidden ) {
    cWin->show();
    // Activate window - doing this instead of KWindowSystem::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  }
  return 1;
}

int KMKernel::openComposer (const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, bool hidden,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset,
                            unsigned int identity )
{
  kDebug();
  KMail::Composer::TemplateContext context = KMail::Composer::New;
  KMime::Message::Ptr msg( new KMime::Message );
  KMime::Content *msgPart = 0;
  MessageHelper::initHeader( msg, identityManager() );
  msg->contentType()->setCharset( "utf-8" );
  if ( !cc.isEmpty() )      msg->cc()->fromUnicodeString( cc, "utf-8" );
  if ( !bcc.isEmpty() )     msg->bcc()->fromUnicodeString( bcc, "utf-8" );
  if ( !subject.isEmpty() ) msg->subject()->fromUnicodeString( subject, "utf-8" );
  if ( !to.isEmpty() )      msg->to()->fromUnicodeString( to, "utf-8" );
  if ( identity > 0 ) {
    KMime::Headers::Generic *h = new KMime::Headers::Generic("X-KMail-Identity", msg.get(), QByteArray::number( identity ) );
    msg->setHeader( h );
  }
  if ( !body.isEmpty() ) {
    msg->setBody(body.toUtf8());
    context = KMail::Composer::NoTemplate;
  } else {
    TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
    parser.setIdentityManager( KMKernel::self()->identityManager() );
    parser.process( KMime::Message::Ptr() );
  }

  bool iCalAutoSend = false;
  bool noWordWrap = false;
  bool isICalInvitation = false;
  KConfigGroup options( config(), "Groupware" );
  if ( !attachData.isEmpty() ) {
    isICalInvitation = attachName == "cal.ics" &&
      attachType == "text" &&
      attachSubType == "calendar" &&
      attachParamAttr == "method";
    // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
    if ( isICalInvitation && bcc.isEmpty() )
      msg->bcc()->clear();
    if ( isICalInvitation &&
        MessageViewer::GlobalSettings::self()->legacyBodyInvites() ) {
      // KOrganizer invitation caught and to be sent as body instead
      msg->setBody( attachData );
      context = KMail::Composer::NoTemplate;
      msg->contentType()->from7BitString(
                           QString( "text/calendar; method=%1; "
                                    "charset=\"utf-8\"" ).
                           arg( attachParamValue ).toLatin1() );

      iCalAutoSend = true; // no point in editing raw ICAL
      noWordWrap = true; // we shant word wrap inline invitations
    } else {
      // Just do what we're told to do
      msgPart = new KMime::Content;
      msgPart->contentType()->setName( attachName, "utf-8" );
      msgPart->contentTransferEncoding()->fromUnicodeString(attachCte, "utf-8" );
      msgPart->setBody( attachData ); //TODO: check if was setBodyEncoded
      msgPart->contentType()->setMimeType( attachType + '/' +  attachSubType );
      msgPart->contentDisposition()->setParameter( attachParamAttr, attachParamValue ); //TODO: Check if the content disposition parameter needs to be set!
       if( ! MessageViewer::GlobalSettings::self()->exchangeCompatibleInvitations() ) {
        msgPart->contentDisposition()->fromUnicodeString(attachContDisp, "utf-8" );
      }
      if( !attachCharset.isEmpty() ) {
        // kDebug() << "Set attachCharset to" << attachCharset;
        msgPart->contentType()->setCharset( attachCharset );
      }
      // Don't show the composer window if the automatic sending is checked
      iCalAutoSend = MessageViewer::GlobalSettings::self()->automaticSending();
    }
  }

  KMail::Composer * cWin = KMail::makeComposer( KMime::Message::Ptr(), context );
  cWin->setMsg( msg, !isICalInvitation /* mayAutoSign */ );
  cWin->setSigningAndEncryptionDisabled( isICalInvitation
      && MessageViewer::GlobalSettings::self()->legacyBodyInvites() );
  if ( noWordWrap )
    cWin->disableWordWrap();
  if ( msgPart )
    cWin->addAttach( msgPart );
  if ( isICalInvitation ) {
    cWin->forceDisableHtml();
    //cWin->disableRecipientNumberCheck();
    cWin->disableForgottenAttachmentsCheck();
  }

  if ( !hidden && !iCalAutoSend ) {
    cWin->show();
    // Activate window - doing this instead of KWin::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  } else {

    // Always disable word wrap when we don't show the composer, since otherwise QTextEdit
    // gets the widget size wrong and wraps much too early.
    cWin->disableWordWrap();
    cWin->slotSendNow();
  }

  return 1;
}

QDBusObjectPath KMKernel::openComposer( const QString &to, const QString &cc,
                                        const QString &bcc,
                                        const QString &subject,
                                        const QString &body, bool hidden )
{
  KMime::Message::Ptr msg( new KMime::Message );
  MessageHelper::initHeader( msg, identityManager() );
  msg->contentType()->setCharset("utf-8");
  if ( !cc.isEmpty() )      msg->cc()->fromUnicodeString( cc, "utf-8" );
  if ( !bcc.isEmpty() )     msg->bcc()->fromUnicodeString( bcc, "utf-8" );
  if ( !subject.isEmpty() ) msg->subject()->fromUnicodeString( subject, "utf-8" );
  if ( !to.isEmpty() )      msg->to()->fromUnicodeString( to, "utf-8" );
  if ( !body.isEmpty() ) {
    msg->setBody(body.toLatin1());
  } else {
    TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
    parser.setIdentityManager( KMKernel::self()->identityManager() );
    parser.process( KMime::Message::Ptr() );
  }

  const KMail::Composer::TemplateContext context = body.isEmpty() ? KMail::Composer::New :
                                                   KMail::Composer::NoTemplate;
  KMail::Composer * cWin = KMail::makeComposer( msg, context );
  if ( !hidden ) {
    cWin->show();
    // Activate window - doing this instead of KWindowSystem::activateWindow(cWin->winId());
    // so that it also works when called from KMailApplication::newInstance()
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    KStartupInfo::setNewStartupId( cWin, kapp->startupId() );
#endif
  } else {
    // Always disable word wrap when we don't show the composer; otherwise,
    // QTextEdit gets the widget size wrong and wraps much too early.
    cWin->disableWordWrap();
    cWin->slotSendNow();
  }

  return QDBusObjectPath(cWin->dbusObjectPath());
}

QDBusObjectPath KMKernel::newMessage( const QString &to,
                                      const QString &cc,
                                      const QString &bcc,
                                      bool hidden,
                                      bool useFolderId,
                                      const QString & /*messageFile*/,
                                      const QString &_attachURL)
{
  KUrl attachURL( _attachURL );
  KMime::Message::Ptr msg( new KMime::Message );
  QSharedPointer<FolderCollection> folder;
  uint id = 0;

  if ( useFolderId ) {
    //create message with required folder identity
    folder = currentFolderCollection();
    id = folder ? folder->identity() : 0;
  }
  MessageHelper::initHeader( msg, identityManager(), id );
  msg->contentType()->setCharset( "utf-8" );
  //set basic headers
  if ( !cc.isEmpty() )      msg->cc()->fromUnicodeString( cc, "utf-8" );
  if ( !bcc.isEmpty() )     msg->bcc()->fromUnicodeString( bcc, "utf-8" );
  if ( !to.isEmpty() )      msg->to()->fromUnicodeString( to, "utf-8" );

  TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
  parser.setIdentityManager( identityManager() );
  parser.process( msg, folder ? folder->collection() : Akonadi::Collection() );

  KMail::Composer *win = makeComposer( msg, KMail::Composer::New, id );

  //Add the attachment if we have one
  if ( !attachURL.isEmpty() && attachURL.isValid() ) {
    win->addAttachment( attachURL, "" );
  }

  //only show window when required
  if ( !hidden ) {
    win->show();
  }
  return QDBusObjectPath( win->dbusObjectPath() );
}

int KMKernel::viewMessage( const QString & messageFile )
{
  KMOpenMsgCommand *openCommand = new KMOpenMsgCommand( 0, KUrl( messageFile ) );

  openCommand->start();
  return 1;
}

void KMKernel::raise()
{
  QDBusInterface iface( "org.kde.kmail", "/MainApplication",
                        "org.kde.KUniqueApplication",
                        QDBusConnection::sessionBus());
  QDBusReply<int> reply;
  if ( !iface.isValid() || !( reply = iface.call( "newInstance" ) ).isValid() )
  {
    QDBusError err = iface.lastError();
    kError() << "Communication problem with KMail. "
             << "Error message was:" << err.name() << ": \"" << err.message() << "\"";
  }

}

bool KMKernel::showMail( quint32 serialNumber, const QString& /* messageId */ )
{
  KMMainWidget *mainWidget = 0;

  // First look for a KMainWindow.
  foreach ( KMainWindow* window, KMainWindow::memberList() ) {
    // Then look for a KMMainWidget.
    QList<KMMainWidget*> l = window->findChildren<KMMainWidget*>();
    if ( !l.isEmpty() && l.first() ) {
      mainWidget = l.first();
      if ( window->isActiveWindow() )
        break;
    }
  }
  if ( mainWidget ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Akonadi::Item(serialNumber ),this );
    job->fetchScope().fetchFullPayload();
    if ( job->exec() ) {
      if ( job->items().count() >= 1 ) {
        KMReaderMainWin *win = new KMReaderMainWin( false, false );
        win->showMessage( MessageCore::GlobalSettings::self()->overrideCharacterEncoding(),
                          job->items().at( 0 ) );
        win->show();
        return true;
      }
    }
  }
  return false;
}

void KMKernel::pauseBackgroundJobs()
{
  mBackgroundTasksTimer->stop();
  mJobScheduler->pause();
}

void KMKernel::resumeBackgroundJobs()
{
  mJobScheduler->resume();
  mBackgroundTasksTimer->start( 4 * 60 * 60 * 1000 );
}

void KMKernel::stopNetworkJobs()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return;

  setAccountStatus(false);
  
  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Offline );
  BroadcastStatus::instance()->setStatusMsg( i18n("KMail is set to be offline; all network jobs are suspended"));
  emit onlineStatusChanged( (GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState() );

}

void KMKernel::setAccountStatus(bool goOnline)
{
  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances(false);
  foreach ( Akonadi::AgentInstance type, lst ) {
    const QString identifier( type.identifier() ); 
    if ( identifier.contains( IMAP_RESOURCE_IDENTIFIER ) ||
         identifier.contains( POP3_RESOURCE_IDENTIFIER ) ||
         identifier.contains( MAILDISPATCHER_RESOURCE_IDENTIFIER ) ) {
      type.setIsOnline( goOnline );
    }
  }
  if ( goOnline &&  MessageComposer::MessageComposerSettings::self()->sendImmediate() ) {
    const qint64 nbMsgOutboxCollection = MailCommon::Util::updatedCollection( CommonKernel->outboxCollectionFolder() ).statistics().count();
    if(nbMsgOutboxCollection > 0) {
      kmkernel->msgSender()->sendQueued();
    }
  }
}

void KMKernel::resumeNetworkJobs()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online )
    return;

  if ( ( mSystemNetworkStatus == Solid::Networking::Connected ) ||
    ( mSystemNetworkStatus == Solid::Networking::Unknown ) ) {
    setAccountStatus(true);
    BroadcastStatus::instance()->setStatusMsg( i18n("KMail is set to be online; all network jobs resumed"));
  }
  else {
    BroadcastStatus::instance()->setStatusMsg( i18n ( "KMail is set to be online; all network jobs will resume when a network connection is detected" ) );
  }
  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Online );
  emit onlineStatusChanged( (GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState() );
}

bool KMKernel::isOffline()
{
  Solid::Networking::Status networkStatus = Solid::Networking::status();
  if ( ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline ) ||
       ( networkStatus == Solid::Networking::Unconnected ) ||
       ( networkStatus == Solid::Networking::Disconnecting ) ||
       ( networkStatus == Solid::Networking::Connecting ))
    return true;
  else
    return false;
}

void KMKernel::checkMailOnStartup()
{
  if ( !kmkernel->askToGoOnline() )
    return;

  const QString resourceGroupPattern( "Resource %1" );

  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( type.identifier() ) );
    if ( group.readEntry( "CheckOnStartup", false ) ) {
      if ( !type.isOnline() )
        type.setIsOnline( true );
      type.synchronize();
    }

    // "false" is also hardcoded in ConfigureDialog, don't forget to change there.
    if ( group.readEntry( "OfflineOnShutdown", false ) ) {
      if ( !type.isOnline() )
        type.setIsOnline( true );
    }
  }
}

bool KMKernel::askToGoOnline()
{
  // already asking means we are offline and need to wait anyhow
  if ( s_askingToGoOnline ) {
    return false;
  }

  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline ) {
    s_askingToGoOnline = true;
    int rc =
    KMessageBox::questionYesNo( KMKernel::self()->mainWin(),
                                i18n("KMail is currently in offline mode. "
                                     "How do you want to proceed?"),
                                i18n("Online/Offline"),
                                KGuiItem(i18n("Work Online")),
                                KGuiItem(i18n("Work Offline")));

    s_askingToGoOnline = false;
    if( rc == KMessageBox::No ) {
      return false;
    } else {
      kmkernel->resumeNetworkJobs();
    }
  }
  if( kmkernel->isOffline() )
    return false;

  return true;
}

void KMKernel::slotSystemNetworkStatusChanged( Solid::Networking::Status status )
{
  mSystemNetworkStatus = status;
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return;

  if ( status == Solid::Networking::Connected || status == Solid::Networking::Unknown) {
   BroadcastStatus::instance()->setStatusMsg( i18n(
      "Network connection detected, all network jobs resumed") );
    kmkernel->setAccountStatus( true );
 }
  else {
    BroadcastStatus::instance()->setStatusMsg( i18n(
      "No network connection detected, all network jobs are suspended"));
    kmkernel->setAccountStatus( false );
  }
}

/********************************************************************/
/*                        Kernel methods                            */
/********************************************************************/

void KMKernel::quit()
{
  // Called when all windows are closed. Will take care of compacting,
  // sending... should handle session management too!!
}
  /* TODO later:
   Asuming that:
     - msgsender is nonblocking
       (our own, QSocketNotifier based. Pops up errors and sends signal
        senderFinished when done)

   o If we are getting mail, stop it (but don't lose something!)
         [Done already, see mailCheckAborted]
   o If we are sending mail, go on UNLESS this was called by SM,
       in which case stop ASAP that too (can we warn? should we continue
       on next start?)
   o If we are compacting, or expunging, go on UNLESS this was SM call.
       In that case stop compacting ASAP and continue on next start, before
       touching any folders. [Not needed anymore with CompactionJob]

   KMKernel::quit ()
   {
     SM call?
       if compacting, stop;
       if sending, stop;
       if receiving, stop;
       Windows will take care of themselves (composer should dump
        its messages, if any but not in deadMail)
       declare us ready for the End of the Session

     No, normal quit call
       All windows are off. Anything to do, should compact or sender sends?
         Yes, maybe put an icon in panel as a sign of life
         if sender sending, connect us to his finished slot, declare us ready
                            for quit and wait for senderFinished
         if not, Folder manager, go compact sent-mail and outbox
}                (= call slotFinished())

void KMKernel::slotSenderFinished()
{
  good, Folder manager go compact sent-mail and outbox
  clean up stage1 (release folders and config, unregister from dcop)
    -- another kmail may start now ---
  qApp->quit();
}
*/


/********************************************************************/
/*            Init, Exit, and handler  methods                      */
/********************************************************************/

//-----------------------------------------------------------------------------
// Open a composer for each message found in the dead.letter folder
void KMKernel::recoverDeadLetters()
{
  const QString pathName = localDataPath();
  QDir dir( pathName );
  if ( !dir.exists( "autosave" ) )
    return;

  dir.cd( localDataPath() + "autosave" );
  const QFileInfoList autoSaveFiles = dir.entryInfoList();
  foreach( const QFileInfo &file, autoSaveFiles ) {
    // Disregard the '.' and '..' folders
    const QString filename = file.fileName();
    if( filename == QLatin1String( "." ) ||
        filename == QLatin1String( ".." )
        || file.isDir() )
      continue;
    kDebug() << "Opening autosave file:" << file.absoluteFilePath();
    QFile autoSaveFile( file.absoluteFilePath() );
    if( autoSaveFile.open( QIODevice::ReadOnly ) ) {
      const KMime::Message::Ptr autoSaveMessage( new KMime::Message() );
      const QByteArray msgData = autoSaveFile.readAll();
      autoSaveMessage->setContent( msgData );
      autoSaveMessage->parse();

      // Show the a new composer dialog for the message
      KMail::Composer * autoSaveWin = KMail::makeComposer();
      autoSaveWin->setMsg( autoSaveMessage, false );
      autoSaveWin->setAutoSaveFileName( filename );
      autoSaveWin->show();
      autoSaveFile.close();
    } else {
      KMessageBox::sorry( 0, i18n( "Failed to open autosave file at %1.\nReason: %2" ,
                                   file.absoluteFilePath(), autoSaveFile.errorString() ),
                          i18n( "Opening Autosave File Failed" ) );
    }
  }
}


void  KMKernel::akonadiStateChanged( Akonadi::ServerManager::State state )
{
  kDebug() << "KMKernel has akonadi state changed to:" << int( state );

  if( state == Akonadi::ServerManager::Running ) {
    CommonKernel->initFolders();
  }
}
static void kmCrashHandler( int sigId )
{
  fprintf( stderr, "*** KMail got signal %d (Exiting)\n", sigId );
  // try to cleanup all windows
  if ( kmkernel ) {
    kmkernel->dumpDeadLetters();
    fprintf( stderr, "*** Dead letters dumped.\n" );
    kmkernel->stopAgentInstance();
  }
}

void KMKernel::init()
{
  the_shuttingDown = false;

  the_firstStart = GlobalSettings::self()->firstStart();
  GlobalSettings::self()->setFirstStart( false );
  the_previousVersion = GlobalSettings::self()->previousVersion();
  GlobalSettings::self()->setPreviousVersion( KDEPIM_VERSION );

  readConfig();

  the_undoStack     = new UndoStack(20);

  the_msgSender = new AkonadiSender;
  readConfig();
  // filterMgr->dump();

  mBackgroundTasksTimer = new QTimer( this );
  mBackgroundTasksTimer->setSingleShot( true );
  connect( mBackgroundTasksTimer, SIGNAL(timeout()), this, SLOT(slotRunBackgroundTasks()) );
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
  mBackgroundTasksTimer->start( 10000 ); // 10s, singleshot
#else
  mBackgroundTasksTimer->start( 5 * 60000 ); // 5 minutes, singleshot
#endif

  KCrash::setEmergencySaveFunction( kmCrashHandler );

  kDebug() << "KMail init with akonadi server state:" << int( Akonadi::ServerManager::state() );
  if( Akonadi::ServerManager::state() == Akonadi::ServerManager::Running ) {
    CommonKernel->initFolders();
  }

  connect( Akonadi::ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)), this, SLOT(akonadiStateChanged(Akonadi::ServerManager::State)) );
  connect( folderCollectionMonitor(), SIGNAL(itemRemoved(Akonadi::Item)), the_undoStack, SLOT(msgDestroyed(Akonadi::Item)) );


}

void KMKernel::readConfig()
{
  mWrapCol = MessageComposer::MessageComposerSettings::self()->lineWrapWidth();
  if ((mWrapCol == 0) || (mWrapCol > 78))
    mWrapCol = 78;
  else if (mWrapCol < 30)
    mWrapCol = 30;
}


bool KMKernel::doSessionManagement()
{

  // Do session management
  if (kapp->isSessionRestored()){
    int n = 1;
    while (KMMainWin::canBeRestored(n)){
      //only restore main windows! (Matthias);
      if (KMMainWin::classNameOfToplevel(n) == "KMMainWin")
        (new KMMainWin)->restoreDockedState(n);
      n++;
    }
    return true; // we were restored by SM
  }
  return false;  // no, we were not restored
}

void KMKernel::closeAllKMailWindows()
{
  QList<KMainWindow*> windowsToDelete;

  foreach ( KMainWindow* window, KMainWindow::memberList() ) {
    if ( ::qobject_cast<KMMainWin *>(window) ||
         ::qobject_cast<KMail::SecondaryWindow *>(window) )
    {
      // close and delete the window
      window->setAttribute(Qt::WA_DeleteOnClose);
      window->close();
      windowsToDelete.append( window );
    }
  }

  // We delete all main windows here. Above we called close(), but that calls
  // deleteLater() internally, therefore does not delete it immediately.
  // This would lead to problems when closing Kontact when a composer window
  // is open, because the destruction order is:
  //
  // 1. destructor of the Kontact mainwinow
  // 2.   delete all parts
  // 3.     the KMail part destructor calls KMKernel::cleanup(), which calls
  //        this function
  // 4. delete all other mainwindows
  //
  // Deleting the composer windows here will make sure that step 4 will not delete
  // any composer window, which would fail because the kernel is already deleted.
  qDeleteAll( windowsToDelete );
  windowsToDelete.clear();
}

void KMKernel::cleanup(void)
{
  dumpDeadLetters();
  the_shuttingDown = true;
  closeAllKMailWindows();

  // Flush the cache of foldercollection objects. This results
  // in configuration writes, so we need to do it early enough.
  MailCommon::FolderCollection::clearCache();

  // Write the config while all other managers are alive
  delete the_msgSender;
  the_msgSender = 0;
  delete the_undoStack;
  the_undoStack = 0;

  KSharedConfig::Ptr config =  KMKernel::config();
  Akonadi::Collection trashCollection = CommonKernel->trashCollectionFolder();
  if ( trashCollection.isValid() ) {
    if ( GlobalSettings::self()->emptyTrashOnExit() ) {
      Akonadi::CollectionStatisticsJob *jobStatistics = new Akonadi::CollectionStatisticsJob( trashCollection );
      if ( jobStatistics->exec() ) {
        if ( jobStatistics->statistics().count() > 0 ) {
          mFolderCollectionMonitor->expunge( trashCollection, true /*sync*/ );
        }
      }
    }
  }
  delete mConfigureDialog;
  mConfigureDialog = 0;
  // do not delete, because mWin may point to an existing window
  // delete mWin;
  mWin = 0;

  if ( RecentAddresses::exists() )
    RecentAddresses::self( config.data() )->save( config.data() );
}

void KMKernel::dumpDeadLetters()
{
  if ( shuttingDown() )
    return; //All documents should be saved before shutting down is set!

  // make all composer windows autosave their contents
  foreach ( KMainWindow* window, KMainWindow::memberList() ) {
    if ( KMail::Composer * win = ::qobject_cast<KMail::Composer*>( window ) ) {
      win->autoSaveMessage(true);

      while ( win->isComposing() ) {
        kWarning() << "Danger, using an event loop, this should no longer be happening!";
        qApp->processEvents();
      }
    }
  }
}



void KMKernel::action( bool mailto, bool check, const QString &to,
                       const QString &cc, const QString &bcc,
                       const QString &subj, const QString &body,
                       const KUrl &messageFile,
                       const KUrl::List &attachURLs,
                       const QStringList &customHeaders )
{
  if ( mailto )
    openComposer( to, cc, bcc, subj, body, 0,
                  messageFile.pathOrUrl(), attachURLs.toStringList(),
                  customHeaders );
  else
    openReader( check );

  if ( check )
    checkMail();
  //Anything else?
}

void KMKernel::slotRequestConfigSync()
{
  // ### FIXME: delay as promised in the kdoc of this function ;-)
  slotSyncConfig();
}

void KMKernel::slotSyncConfig()
{
  MessageCore::GlobalSettings::self()->writeConfig();
  MessageViewer::GlobalSettings::self()->writeConfig();
  MessageComposer::MessageComposerSettings::self()->writeConfig();
  TemplateParser::GlobalSettings::self()->writeConfig();
  MessageList::Core::Settings::self()->writeConfig();
  GlobalSettings::self()->writeConfig();
  KMKernel::config()->sync();
}

void KMKernel::updateConfig()
{
  slotConfigChanged();
}

void KMKernel::slotShowConfigurationDialog()
{
  if( KMKernel::getKMMainWidget() == 0 ) {
    // ensure that there is a main widget available
    // as parts of the configure dialog (identity) rely on this
    // and this slot can be called when there is only a KMComposeWin showing
    KMMainWin *win = new KMMainWin;
    win->show();

  }

  if( !mConfigureDialog ) {
    mConfigureDialog = new ConfigureDialog( 0, false );
    mConfigureDialog->setObjectName( "configure" );
    connect( mConfigureDialog, SIGNAL(configChanged()),
             this, SLOT(slotConfigChanged()) );
  }

  // Save all current settings.
  if( getKMMainWidget() )
    getKMMainWidget()->writeReaderConfig();

  if( mConfigureDialog->isHidden() ) {
    mConfigureDialog->show();
  } else {
    mConfigureDialog->raise();
  }
}

void KMKernel::slotConfigChanged()
{
  readConfig();
  emit configChanged();
}

//-------------------------------------------------------------------------------
//static
QString KMKernel::localDataPath()
{
  return KStandardDirs::locateLocal( "data", "kmail2/" );
}

//-------------------------------------------------------------------------------

bool KMKernel::haveSystemTrayApplet() const
{
  return !systemTrayApplets.isEmpty();
}

void KMKernel::updateSystemTray()
{
  if ( haveSystemTrayApplet() ) {
    const int nbSystemTray = systemTrayApplets.count();
    for (int i = 0; i < nbSystemTray; ++i) {
      systemTrayApplets.at( i )->updateSystemTray();
    }
  }
}

bool KMKernel::registerSystemTrayApplet( KMail::KMSystemTray* applet )
{
  if ( !systemTrayApplets.contains( applet ) ) {
    systemTrayApplets.append( applet );
    return true;
  }
  else
    return false;
}

bool KMKernel::unregisterSystemTrayApplet( KMail::KMSystemTray* applet )
{
  return systemTrayApplets.removeAll( applet ) > 0;
}


KPIMIdentities::IdentityManager * KMKernel::identityManager() {
  if ( !mIdentityManager ) {
    kDebug();
    mIdentityManager = new KPIMIdentities::IdentityManager( false, this, "mIdentityManager" );
  }
  return mIdentityManager;
}

KMainWindow* KMKernel::mainWin()
{
  // First look for a KMMainWin.
  foreach ( KMainWindow* window, KMainWindow::memberList() )
    if ( ::qobject_cast<KMMainWin *>(window) )
      return window;

  // There is no KMMainWin. Use any other KMainWindow instead (e.g. in
  // case we are running inside Kontact) because we anyway only need
  // it for modal message boxes and for KNotify events.
  if ( !KMainWindow::memberList().isEmpty() ) {
    KMainWindow *kmWin = KMainWindow::memberList().first();
    if ( kmWin )
      return kmWin;
  }

  // There's not a single KMainWindow. Create a KMMainWin.
  // This could happen if we want to pop up an error message
  // while we are still doing the startup wizard and no other
  // KMainWindow is running.
  mWin = new KMMainWin;
  return mWin;
}


KMKernel* KMKernel::self()
{
  return mySelf;
}

KSharedConfig::Ptr KMKernel::config()
{
  assert( mySelf );
  if ( !mySelf->mConfig )
  {
    mySelf->mConfig = KSharedConfig::openConfig( "kmail2rc" );
    // Check that all updates have been run on the config file:
    KMail::checkConfigUpdates();
    MessageList::Core::Settings::self()->setSharedConfig( mySelf->mConfig );
    MessageList::Core::Settings::self()->readConfig();
    TemplateParser::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    TemplateParser::GlobalSettings::self()->readConfig();
    MessageComposer::MessageComposerSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageComposer::MessageComposerSettings::self()->readConfig();
    MessageCore::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageCore::GlobalSettings::self()->readConfig();
    MessageViewer::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageViewer::GlobalSettings::self()->readConfig();

  }
  return mySelf->mConfig;
}

void KMKernel::syncConfig()
{
  slotRequestConfigSync();
}


void KMKernel::selectCollectionFromId( const Akonadi::Collection::Id id)
{
  KMMainWidget *widget = getKMMainWidget();
  Q_ASSERT( widget );
  if ( !widget )
    return;

  Akonadi::Collection colFolder = CommonKernel->collectionFromId( id );

  if( colFolder.isValid() )
    widget->slotSelectCollectionFolder( colFolder );
}

bool KMKernel::selectFolder( const QString &folder )
{
  KMMainWidget *widget = getKMMainWidget();
  Q_ASSERT( widget );
  if ( !widget )
    return false;

  const Akonadi::Collection colFolder = CommonKernel->collectionFromId( folder.toLongLong() );

  if( colFolder.isValid() ) {
    widget->slotSelectCollectionFolder( colFolder );
    return true;
  }
  return false;
}

KMMainWidget *KMKernel::getKMMainWidget()
{
  //This could definitely use a speadup
  QWidgetList l = QApplication::topLevelWidgets();
  QWidget *wid;

  Q_FOREACH( wid, l ) {
    QList<KMMainWidget*> l2 = wid->window()->findChildren<KMMainWidget*>();
    if ( !l2.isEmpty() && l2.first() )
      return l2.first();
  }
  return 0;
}

void KMKernel::slotRunBackgroundTasks() // called regularly by timer
{
  // Hidden KConfig keys. Not meant to be used, but a nice fallback in case
  // a stable kmail release goes out with a nasty bug in CompactionJob...
  if ( GlobalSettings::self()->autoExpiring() ) {
      mFolderCollectionMonitor->expireAllFolders( false /*scheduled, not immediate*/, entityTreeModel() );
  }

#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
  mBackgroundTasksTimer->start( 60 * 1000 ); // check again in 1 minute
#else
  mBackgroundTasksTimer->start( 4 * 60 * 60 * 1000 ); // check again in 4 hours
#endif

}

static Akonadi::Collection::List collect_collections( const QAbstractItemModel *model,
                                                      const QModelIndex &parent )
{
  Akonadi::Collection::List collections;
  const int numberOfCollection( model->rowCount( parent ) );
  for ( int i = 0; i < numberOfCollection; ++i ) {
    const QModelIndex child = model->index( i, 0, parent );
    Akonadi::Collection collection =
        model->data( child, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( collection.isValid() ) {
      collections << collection;
    }
    collections += collect_collections( model, child );
  }
  return collections;
}

Akonadi::Collection::List KMKernel::allFolders() const
{
  return collect_collections( collectionModel(), QModelIndex() );
}

void KMKernel::expireAllFoldersNow() // called by the GUI
{
  mFolderCollectionMonitor->expireAllFolders( true /*immediate*/, entityTreeModel() );
}

bool KMKernel::canQueryClose()
{
  if ( KMMainWidget::mainWidgetList() &&
       KMMainWidget::mainWidgetList()->count() > 1 )
    return true;
  KMMainWidget *widget = getKMMainWidget();
  if ( !widget )
    return true;
  KMail::KMSystemTray* systray = widget->systray();
  if ( !systray || GlobalSettings::closeDespiteSystemTray() )
      return true;
  if ( systray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowAlways ) {
    systray->hideKMail();
    return false;
  } else if ( ( systray->mode() == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) && ( systray->hasUnreadMail() )) {
    systray->setStatus( KStatusNotifierItem::Active );
    systray->hideKMail();
    return false;
  }
  return true;
}

QSharedPointer<FolderCollection> KMKernel::currentFolderCollection()
{
  KMMainWidget *widget = getKMMainWidget();
  QSharedPointer<FolderCollection> folder;
  if ( widget  ) {
    folder = widget->currentFolder();
  }
  return folder;
}

// can't be inline, since KMSender isn't known to implement
// KMail::MessageSender outside this .cpp file
MessageSender * KMKernel::msgSender()
{
  return the_msgSender;
}

void KMKernel::transportRemoved(int id, const QString & name)
{
  Q_UNUSED( id );

  // reset all identities using the deleted transport
  QStringList changedIdents;
  KPIMIdentities::IdentityManager * im = identityManager();
  KPIMIdentities::IdentityManager::Iterator end = im->modifyEnd();
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it ) {
    if ( name == (*it).transport() ) {
      (*it).setTransport( QString() );
      changedIdents += (*it).identityName();
    }
  }

  // if the deleted transport is the currently used transport reset it to default
  const QString& currentTransport = GlobalSettings::self()->currentTransport();
  if ( name == currentTransport )
    GlobalSettings::self()->setCurrentTransport( QString() );

  if ( !changedIdents.isEmpty() ) {
    QString information = i18np( "This identity has been changed to use the default transport:",
                          "These %1 identities have been changed to use the default transport:",
                          changedIdents.count() );
    KMessageBox::informationList( mWin, information, changedIdents );
    im->commit();
  }
}

void KMKernel::transportRenamed(int id, const QString & oldName, const QString & newName)
{
  Q_UNUSED( id );

  QStringList changedIdents;
  KPIMIdentities::IdentityManager * im = identityManager();
  KPIMIdentities::IdentityManager::Iterator end = im->modifyEnd();
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != end; ++it ) {
    if ( oldName == (*it).transport() ) {
      (*it).setTransport( newName );
      changedIdents << (*it).identityName();
    }
  }

  if ( !changedIdents.isEmpty() ) {
    const QString information =
      i18np( "This identity has been changed to use the modified transport:",
             "These %1 identities have been changed to use the modified transport:",
             changedIdents.count() );
    KMessageBox::informationList( mWin, information, changedIdents );
    im->commit();
  }
}

void KMKernel::itemDispatchStarted()
{
  // Watch progress of the MDA.
  KPIM::ProgressManager::createProgressItem( 0,
      MailTransport::DispatcherInterface().dispatcherInstance(),
      QString::fromAscii( "Sender" ),
      i18n( "Sending messages" ),
      i18n( "Initiating sending process..." ),
      true );
}

void KMKernel::instanceStatusChanged( Akonadi::AgentInstance instance )
{
  if ( MailCommon::Util::agentInstances(true).contains( instance ) ) {
    if ( instance.status() == Akonadi::AgentInstance::Running ) {

      if ( mResourcesBeingChecked.isEmpty() ) {
        kDebug() << "A Resource started to synchronize, starting a mail check.";
        emit startCheckMail();
      }

      const QString identifier(instance.identifier());
      if ( !mResourcesBeingChecked.contains( identifier ) ) {
        mResourcesBeingChecked.append( identifier );
      }

      bool useCrypto = false;
      if(mResourceCryptoSettingCache.contains(identifier)) {
          useCrypto = mResourceCryptoSettingCache.value(identifier);
      } else {
        if ( identifier.contains( IMAP_RESOURCE_IDENTIFIER ) ) {
            OrgKdeAkonadiImapSettingsInterface *iface = MailCommon::Util::createImapSettingsInterface( identifier );
            if ( iface->isValid() ) {
                const QString imapSafety = iface->safety();
                useCrypto = ( imapSafety == QLatin1String( "SSL" ) || imapSafety == QLatin1String( "STARTTLS" ) );
                mResourceCryptoSettingCache.insert(identifier,useCrypto);
            }
            delete iface;
        } else if ( identifier.contains( POP3_RESOURCE_IDENTIFIER ) ) {
            OrgKdeAkonadiPOP3SettingsInterface *iface = MailCommon::Util::createPop3SettingsInterface( identifier );
            if ( iface->isValid() ) {
                useCrypto = ( iface->useSSL() || iface->useTLS() );
                mResourceCryptoSettingCache.insert(identifier,useCrypto);
            }
            delete iface;
        }
     }

      
      // Creating a progress item twice is ok, it will simply return the already existing
      // item
      KPIM::ProgressItem *progress =  KPIM::ProgressManager::createProgressItem( 0, instance,
                                        instance.identifier(), instance.name(), instance.statusMessage(),
                                        true, useCrypto );
      progress->setProperty( "AgentIdentifier", instance.identifier() );
    } else if ( instance.status() == Akonadi::AgentInstance::Broken ) {
      agentInstanceBroken( instance );
    }
  }
}

void KMKernel::agentInstanceBroken( const Akonadi::AgentInstance& instance )
{
  const QString summary = i18n( "Resource %1 is broken. This resource is now %2",  instance.name(), instance.isOnline() ? i18n( "online" ) : i18n( "offline" ) );
  if( xmlGuiInstance().isValid() ) {
    KNotification::event( "akonadi-resource-broken",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout,
                          xmlGuiInstance() );
  } else {
    KNotification::event( "akonadi-resource-broken",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout );
  }

}

void KMKernel::slotProgressItemCompletedOrCanceled( KPIM::ProgressItem * item )
{
  const QString identifier = item->property( "AgentIdentifier" ).toString();
  const Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance( identifier );
  if ( agent.isValid() ) {
    mResourcesBeingChecked.removeAll( identifier );
    if ( mResourcesBeingChecked.isEmpty() ) {
      kDebug() << "Last resource finished syncing, mail check done";
      emit endCheckMail();
    }
  }
}

void KMKernel::updatedTemplates()
{
  emit customTemplatesChanged();
}


bool KMKernel::isImapFolder( const Akonadi::Collection &col, bool &isOnline ) const
{
  const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance( col.resource() );
  isOnline = agentInstance.isOnline();

  return (agentInstance.type().identifier() == IMAP_RESOURCE_IDENTIFIER);
}


void KMKernel::stopAgentInstance()
{
  const QString resourceGroupPattern( "Resource %1" );

  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( type.identifier() ) );

    // "false" is also hardcoded in ConfigureDialog, don't forget to change there.
    if ( group.readEntry( "OfflineOnShutdown", false ) )
      type.setIsOnline( false );
  }
}

void KMKernel::slotCollectionRemoved(const Akonadi::Collection& col)
{
  KConfigGroup group( KMKernel::config(), MailCommon::FolderCollection::configGroupName( col ) );
  group.deleteGroup();
  group.sync();
  const QString colStr = QString::number( col.id() );
  TemplateParser::Util::deleteTemplate( colStr );
  MessageList::Util::deleteConfig( colStr );
}

void KMKernel::slotDeleteIdentity( uint identity)
{
  TemplateParser::Util::deleteTemplate( QString::fromLatin1( "IDENTITY_%1" ).arg( identity ) );
}

void KMKernel::slotCollectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination )
{
  //TODO add undo/redo move collection
}

bool KMKernel::showPopupAfterDnD()
{
  return GlobalSettings::self()->showPopupAfterDnD();
}

bool KMKernel::excludeImportantMailFromExpiry()
{
  return GlobalSettings::self()->excludeImportantMailFromExpiry();
}

qreal KMKernel::closeToQuotaThreshold()
{
  return GlobalSettings::self()->closeToQuotaThreshold();
}

Akonadi::Entity::Id KMKernel::lastSelectedFolder()
{
  return GlobalSettings::self()->lastSelectedFolder();
}

void KMKernel::setLastSelectedFolder(const Akonadi::Entity::Id& col)
{
  GlobalSettings::self()->setLastSelectedFolder( col );
}

QStringList KMKernel::customTemplates()
{
  return GlobalSettingsBase::self()->customTemplates();
}

void KMKernel::openFilterDialog(bool createDummyFilter)
{
  if ( !mFilterEditDialog ) {
    mFilterEditDialog = new MailCommon::KMFilterDialog( getKMMainWidget()->actionCollections(), 0, createDummyFilter );
    mFilterEditDialog->setObjectName( "filterdialog" );
  }
  mFilterEditDialog->show();
  mFilterEditDialog->raise();
  mFilterEditDialog->activateWindow();
}

void KMKernel::createFilter(const QByteArray& field, const QString& value)
{
  mFilterEditDialog->createFilter( field, value );

}


void KMKernel::checkFolderFromResources( const Akonadi::Collection::List& collectionList )
{
  const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
  foreach( const Akonadi::AgentInstance& type, lst ) {
    if ( type.status() == Akonadi::AgentInstance::Broken )
      continue;
    if ( type.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiImapSettingsInterface *iface = MailCommon::Util::createImapSettingsInterface( type.identifier() );
      if ( iface->isValid() ) {
        foreach( const Akonadi::Collection& collection, collectionList ) {
          const Akonadi::Collection::Id collectionId = collection.id();
          if ( iface->trashCollection() == collectionId ) {
            //Use default trash
            iface->setTrashCollection( CommonKernel->trashCollectionFolder().id() );
            iface->writeConfig();
            break; 
          }
        }
      }
      delete iface;
    }
    else if ( type.identifier().contains( POP3_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiPOP3SettingsInterface *iface = MailCommon::Util::createPop3SettingsInterface( type.identifier() );
      if ( iface->isValid() ) {
        foreach( const Akonadi::Collection& collection, collectionList ) {
          const Akonadi::Collection::Id collectionId = collection.id();
          if ( iface->targetCollection() == collectionId ) {
            //Use default inbox
            iface->setTargetCollection( CommonKernel->inboxCollectionFolder().id() );
            iface->writeConfig();
            break;
          }
        }
      }
      delete iface;
    }
  }
}

const QAbstractItemModel* KMKernel::treeviewModelSelection()
{
  if ( getKMMainWidget() )
    return getKMMainWidget()->folderTreeView()->selectionModel()->model();
  else
    return entityTreeModel();
}

void KMKernel::slotInstanceWarning(const Akonadi::AgentInstance&instance , const QString& message)
{
  const QString summary = i18nc( "<source>: <error message>", "%1: %2", instance.name(), message );
  if( xmlGuiInstance().isValid() ) {
    KNotification::event( "akonadi-instance-warning",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout,
                          xmlGuiInstance() );
  } else {
    KNotification::event( "akonadi-instance-warning",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout );
  }
}

void KMKernel::slotInstanceError(const Akonadi::AgentInstance& instance, const QString & message)
{
  const QString summary = i18nc( "<source>: <error message>", "%1: %2", instance.name(), message );
  if( xmlGuiInstance().isValid() ) {
    KNotification::event( "akonadi-instance-error",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout,
                          xmlGuiInstance() );
  } else {
    KNotification::event( "akonadi-instance-error",
                          summary,
                          QPixmap(),
                          0,
                          KNotification::CloseOnTimeout );
  }
}


void KMKernel::slotInstanceRemoved(const Akonadi::AgentInstance& instance)
{
  const QString identifier(instance.identifier());
  const QString resourceGroup = QString::fromLatin1( "Resource %1" ).arg( identifier );
  if ( KMKernel::config()->hasGroup( resourceGroup ) ) {
    KConfigGroup group( KMKernel::config(), resourceGroup );
    group.deleteGroup();
    group.sync();
  }
  if(mResourceCryptoSettingCache.contains(identifier)) {
    mResourceCryptoSettingCache.remove(identifier);
  }
}

void KMKernel::savePaneSelection()
{
  KMMainWidget *widget = getKMMainWidget();
  if ( widget  ) {
    widget->savePaneSelection();
  }
}

void KMKernel::updatePaneTagComboBox()
{
  KMMainWidget *widget = getKMMainWidget();
  if ( widget  ) {
    widget->updatePaneTagComboBox();
  }
}

#include "kmkernel.moc"
