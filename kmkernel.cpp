/*  -*- mode: C++; c-file-style: "gnu" -*- */

#include "kmkernel.h"

#include "globalsettings.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmstartup.h"
#include "kmmainwin.h"
#include "composer.h"
#include "kmreadermainwin.h"
#include "kmfiltermgr.h"
#include "kmfilteraction.h"
#include "undostack.h"
#include <kpimutils/kfileio.h>
#include "kmversion.h"
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "recentaddresses.h"
using KPIM::RecentAddresses;
#include "configuredialog.h"
#include "kmcommands.h"
#include "kmsystemtray.h"
#include "stringutil.h"
#include "foldertreeview.h"

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
#include "messageviewer/globalsettings.h"
#include "messagecomposer/akonadisender.h"
#include "messagecomposersettings.h"
#include "messagecomposer/messagehelper.h"
#include "messagecomposer/messagecomposersettings.h"


#include "templateparser/templateparser.h"
#include "templateparser/globalsettings_base.h"

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
#include <kio/netaccess.h>
#include <kprocess.h>
#include <KCrash>

#include <kmime/kmime_message.h>
#include <kmime/kmime_util.h>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/KMime/SpecialMailCollectionsRequestJob>
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

#include <QByteArray>
#include <QDir>
#include <QList>
#include <QObject>
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

#include "searchdescriptionattribute.h"

static KMKernel * mySelf = 0;
static bool s_askingToGoOnline = false;

/********************************************************************/
/*                     Constructor and destructor                   */
/********************************************************************/
KMKernel::KMKernel (QObject *parent, const char *name) :
  QObject(parent),
  mIdentityManager(0), mConfigureDialog(0), mMailService(0)
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::SearchDescriptionAttribute>();

  // Akonadi migration
  // check if there is something to migrate at all
  bool needMigration = true;
  KConfig oldKMailConfig( "kmailrc", KConfig::NoGlobals );
  if ( oldKMailConfig.groupList().isEmpty() ||
       ( oldKMailConfig.groupList().count() == 1 &&
         oldKMailConfig.groupList().first() == "$Version" ) ) {
    const QFileInfo oldDataDirFileInfo( KStandardDirs::locateLocal( "data", "kmail" ) );
    if ( !oldDataDirFileInfo.exists() || !oldDataDirFileInfo.isDir() ) {
      // neither config or data, the migrator cannot do anything useful anyways
      needMigration = false;
    }
  }

  if ( needMigration ) {
    KConfig config( "kmail-migratorrc" );
    KConfigGroup migrationCfg( &config, "Migration" );
    const bool enabled = migrationCfg.readEntry( "Enabled", false );
    const int currentVersion = migrationCfg.readEntry( "Version", 0 );
    const int targetVersion = migrationCfg.readEntry( "TargetVersion", 1 );
    if ( enabled && currentVersion < targetVersion ) {
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
        migrationCfg.writeEntry( "Version", targetVersion );
        migrationCfg.sync();
      } else {
        // exit code 1 means it is already running, so we are probably called by a migrator instance
        kError() << "Akonadi migration failed!";
        kError() << "command was: " << proc.program();
        kError() << "exit code: " << proc.exitCode();
        kError() << "stdout: " << proc.readAllStandardOutput();
        kError() << "stderr: " << proc.readAllStandardError();
        exit( 42 );
      }
    }
  }
#ifdef __GNUC__
#warning Remove before the 4.5 release
#endif
  if ( KMessageBox::questionYesNo(
        0,
        i18n( "You are attempting to start an <b>unstable development version</b> of KMail.<br />"
              "KMail 2 is currently being ported to Akonadi and is under heavy development. "
              "Do not use this version for real mails, you will <b>lose data</b>. If you use this "
              "version now, your mails will not be correctly migrated, and you will not be able to "
              "migrate them afterwards.<br />"
              "Because of the current development, there are many bugs and regressions as well as "
              "missing features. <b>Essentially KMail 2 does not work at the moment, do not try to use it.</b><br />"
              "Please do not report any bugs for this version yet.<br /><br />"
              "If you want to use KMail for real mails, please use the version from the KDE SC 4.4 branch instead." ),
        i18n( "Unstable Development version of KMail" ),
        KGuiItem( i18n( "Lose Data" ) ),
        KStandardGuiItem::cancel(),
        "UseAtYourOwnRiskWarning",
        KMessageBox::Dangerous )
      == KMessageBox::No ) {
    kWarning() << "Exiting after development version warning.";
    exit(42);
  }

  kDebug();

  setObjectName( name );
  mySelf = this;
  the_startingUp = true;
  closed_by_user = true;
  the_firstInstance = true;

  the_undoStack = 0;
  the_filterMgr = 0;
  the_popFilterMgr = 0;
  the_filterActionDict = 0;
  the_msgSender = 0;
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
  // till here ================================================

  the_draftsCollectionFolder = -1;
  the_inboxCollectionFolder = -1;
  the_outboxCollectionFolder = -1;
  the_sentCollectionFolder = -1;
  the_templatesCollectionFolder = -1;
  the_trashCollectionFolder = -1;

  mFolderCollectionMonitor = new FolderCollectionMonitor( this );
  Akonadi::Session *session = new Akonadi::Session( "KMail Kernel ETM", this );
  monitor()->setSession( session );
  mEntityTreeModel = new Akonadi::EntityTreeModel( monitor(), this );
  mEntityTreeModel->setItemPopulationStrategy( Akonadi::EntityTreeModel::LazyPopulation );
  mCollectionModel = new Akonadi::EntityMimeTypeFilterModel( this );
  mCollectionModel->setSourceModel( entityTreeModel() );
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

Akonadi::ChangeRecorder * KMKernel::monitor() const
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
    for ( QStringList::ConstIterator it = attachList.constBegin();
          it != attachList.constEnd(); ++it ) {
      if ( !(*it).isEmpty() ) {
        KUrl url( *it );
        if ( url.protocol().isEmpty() ) {
          const QString newUrl = QDir::currentPath () + QDir::separator () + url.fileName();
          attachURLs.append( KUrl( newUrl ) );
        }
        else {
          attachURLs.append( url );
        }
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
    for(int i= 0; i < args->count(); i++)
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
    viewMessage( messageFile );
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

  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( type.identifier() ) );
    if ( group.readEntry( "IncludeInManualChecks", true ) ) {
      if ( !type.isOnline() )
        type.setIsOnline( true );
      type.synchronize();
    }
  }
}

QStringList KMKernel::accounts()
{
  QStringList accountLst;
  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
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
  // tentatively decode to, cc and bcc because invokeMailer calls us with
  // RFC 2047 encoded addresses in order to protect non-ASCII email addresses
  if (!to.isEmpty())
    msg->to()->fromUnicodeString( KMime::decodeRFC2047String( to.toLocal8Bit() ), "utf-8" );
  if (!cc.isEmpty())
    msg->cc()->fromUnicodeString( KMime::decodeRFC2047String( cc.toLocal8Bit() ), "utf-8" );
  if (!bcc.isEmpty())
    msg->bcc()->fromUnicodeString( KMime::decodeRFC2047String( bcc.toLocal8Bit() ), "utf-8"  );
  if (!subject.isEmpty()) msg->subject()->fromUnicodeString(subject, "utf-8" );

  KUrl messageUrl = KUrl( messageFile );
  if ( !messageUrl.isEmpty() && messageUrl.isLocalFile() ) {
    QByteArray str = KPIMUtils::kFileToByteArray( messageUrl.toLocalFile(), true, false );
    if( !str.isEmpty() ) {
      context = KMail::Composer::NoTemplate;
      msg->setBody( QString::fromLocal8Bit( str.data(), str.size() ).toUtf8() );
    }
    else {
      TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
      parser.process( KMime::Message::Ptr() );
    }
  }
  else if ( !body.isEmpty() ) {
    context = KMail::Composer::NoTemplate;
    msg->setBody( body.toUtf8() );
  }
  else {
    TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
    parser.process( KMime::Message::Ptr() );
  }

  if ( !customHeaders.isEmpty() )
  {
    for ( QStringList::ConstIterator it = customHeaders.begin() ; it != customHeaders.end() ; ++it )
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
      msgPart->contentType()->setMimeType( attachType + "/" +  attachSubType );
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
    cWin->disableRecipientNumberCheck();
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
    msg->setBody(body.toUtf8());
  } else {
    TemplateParser::TemplateParser parser( msg, TemplateParser::TemplateParser::NewMessage );
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
  parser.process( KMime::Message::Ptr(), folder ? folder->collection() : Akonadi::Collection() );

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

int KMKernel::viewMessage( const KUrl & messageFile )
{
  KMOpenMsgCommand *openCommand = new KMOpenMsgCommand( 0, messageFile );

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

  const Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach ( Akonadi::AgentInstance type, lst ) {
    type.setIsOnline( false );
  }

  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Offline );
  BroadcastStatus::instance()->setStatusMsg( i18n("KMail is set to be offline; all network jobs are suspended"));
  emit onlineStatusChanged( (GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState() );

}

void KMKernel::resumeNetworkJobs()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Online )
    return;

  const Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach ( Akonadi::AgentInstance type, lst ) {
    type.setIsOnline( true );
  }

  GlobalSettings::setNetworkState( GlobalSettings::EnumNetworkState::Online );
  BroadcastStatus::instance()->setStatusMsg( i18n("KMail is set to be online; all network jobs resumed"));
  emit onlineStatusChanged( (GlobalSettings::EnumNetworkState::type)GlobalSettings::networkState() );

  if ( MessageComposer::MessageComposerSettings::self()->sendImmediate() ) {
    kmkernel->msgSender()->sendQueued();
  }
}

bool KMKernel::isOffline()
{
  if ( GlobalSettings::self()->networkState() == GlobalSettings::EnumNetworkState::Offline )
    return true;
  else
    return false;
}

void KMKernel::checkMailOnStartup()
{
  if ( !kmkernel->askToGoOnline() )
    return;

  const QString resourceGroupPattern( "Resource %1" );

  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( type.identifier() ) );
    if ( group.readEntry( "CheckOnStartup", false ) ) {
      if ( !type.isOnline() )
        type.setIsOnline( true );
      type.synchronize();
    }

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

  if ( kmkernel->isOffline() ) {
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
  return true;
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
    if( file.fileName() == "." || file.fileName() == ".." || file.isDir() )
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
      autoSaveWin->setMsg( autoSaveMessage );
      autoSaveWin->setAutoSaveFileName( file.fileName() );
      autoSaveWin->show();
      autoSaveFile.close();
    } else {
      KMessageBox::sorry( 0, i18n( "Failed to open autosave file at %1.\nReason: %2" ,
                                   file.absoluteFilePath(), autoSaveFile.errorString() ),
                          i18n( "Opening Autosave File Failed" ) );
    }
  }
}

void KMKernel::findCreateDefaultCollection( Akonadi::SpecialMailCollections::Type type )
{
  if( Akonadi::SpecialMailCollections::self()->hasDefaultCollection( type ) ) {
    const Akonadi::Collection col = Akonadi::SpecialMailCollections::self()->defaultCollection( type );
    if ( !( col.rights() & Akonadi::Collection::AllRights ) )
      emergencyExit( i18n("You do not have read/write permission to your inbox folder.") );
  }
  else {
    Akonadi::SpecialMailCollectionsRequestJob *job = new Akonadi::SpecialMailCollectionsRequestJob( this );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( createDefaultCollectionDone( KJob* ) ) );
    job->requestDefaultCollection( type );
  }
}

void KMKernel::createDefaultCollectionDone( KJob * job)
{
  if ( job->error() ) {
    emergencyExit( job->errorText() );
    return;
  }

  Akonadi::SpecialMailCollectionsRequestJob *requestJob = qobject_cast<Akonadi::SpecialMailCollectionsRequestJob*>( job );
  const Akonadi::Collection col = requestJob->collection();
  if ( !( col.rights() & Akonadi::Collection::AllRights ) )
    emergencyExit( i18n("You do not have read/write permission to your inbox folder.") );

  connect( Akonadi::SpecialMailCollections::self(), SIGNAL( defaultCollectionsChanged() ),
           this, SLOT( slotDefaultCollectionsChanged () ) );
}

void KMKernel::slotDefaultCollectionsChanged()
{
  initFolders();
}


//-----------------------------------------------------------------------------
void KMKernel::initFolders()
{
  kDebug() << "KMail is initialize and looking for default specialcollection folders.";
  the_draftsCollectionFolder = the_inboxCollectionFolder = the_outboxCollectionFolder = the_sentCollectionFolder
    = the_templatesCollectionFolder = the_trashCollectionFolder = -1;
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::Inbox );
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::Outbox );
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::SentMail );
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::Drafts );
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::Trash );
  findCreateDefaultCollection( Akonadi::SpecialMailCollections::Templates );
}

void  KMKernel::akonadiStateChanged( Akonadi::ServerManager::State state )
{
  kDebug() << "KMKernel has akonadi state changed to:" << state;

  if( state == Akonadi::ServerManager::Running ) {
    initFolders();
  }
}
static void kmCrashHandler( int sigId )
{
  fprintf( stderr, "*** KMail got signal %d (Exiting)\n", sigId );
  // try to cleanup all windows
  if ( kmkernel ) {
    kmkernel->dumpDeadLetters();
    fprintf( stderr, "*** Dead letters dumped.\n" );
  }
}

void KMKernel::init()
{
  the_shuttingDown = false;

  KSharedConfig::Ptr cfg = KMKernel::config();

  KConfigGroup group(cfg, "General");
  the_firstStart = group.readEntry("first-start", true );
  group.writeEntry("first-start", false);
  the_previousVersion = group.readEntry("previous-version");
  group.writeEntry("previous-version", KMAIL_VERSION);

  readConfig();

  the_undoStack     = new UndoStack(20);
  the_filterMgr     = new KMFilterMgr();
  the_popFilterMgr     = new KMFilterMgr(true);
  the_filterActionDict = new KMFilterActionDict;

  the_filterMgr->readConfig();
  the_popFilterMgr->readConfig();
  the_msgSender = new AkonadiSender;
  readConfig();
  // filterMgr->dump();

  mBackgroundTasksTimer = new QTimer( this );
  mBackgroundTasksTimer->setSingleShot( true );
  connect( mBackgroundTasksTimer, SIGNAL( timeout() ), this, SLOT( slotRunBackgroundTasks() ) );
#ifdef DEBUG_SCHEDULER // for debugging, see jobscheduler.h
  mBackgroundTasksTimer->start( 10000 ); // 10s, singleshot
#else
  mBackgroundTasksTimer->start( 5 * 60000 ); // 5 minutes, singleshot
#endif

  KCrash::setEmergencySaveFunction( kmCrashHandler );

  kDebug() << "KMail init with akonadi server state:" << Akonadi::ServerManager::state();
  if( Akonadi::ServerManager::state() == Akonadi::ServerManager::Running ) {
    initFolders();
  }

  connect( Akonadi::ServerManager::self(), SIGNAL( stateChanged( Akonadi::ServerManager::State ) ), this, SLOT( akonadiStateChanged( Akonadi::ServerManager::State ) ) );

}

void KMKernel::readConfig()
{
  mWrapCol = GlobalSettings::self()->lineWrapWidth();
  if ((mWrapCol == 0) || (mWrapCol > 78))
    mWrapCol = 78;
  if (mWrapCol < 30)
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

  // Write the config while all other managers are alive
  delete the_filterMgr;
  the_filterMgr = 0;
  delete the_msgSender;
  the_msgSender = 0;
  delete the_filterActionDict;
  the_filterActionDict = 0;
  delete the_undoStack;
  the_undoStack = 0;
  delete the_popFilterMgr;
  the_popFilterMgr = 0;

  KSharedConfig::Ptr config =  KMKernel::config();
  KConfigGroup group(config, "General");
  if ( the_trashCollectionFolder > 0 ) {
    if ( group.readEntry( "empty-trash-on-exit", false ) ) {
      if ( collectionFromId( the_trashCollectionFolder ).statistics().count() > 0 ) {
        mFolderCollectionMonitor->expunge( collectionFromId( the_trashCollectionFolder ) );
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
  slotSyncConfig();
}

void KMKernel::dumpDeadLetters()
{
  if ( shuttingDown() )
    return; //All documents should be saved before shutting down is set!

  // make all composer windows autosave their contents
  foreach ( KMainWindow* window, KMainWindow::memberList() ) {
    if ( KMail::Composer * win = ::qobject_cast<KMail::Composer*>( window ) ) {
      win->autoSaveMessage();
      // saving the message has to be finished right here, we are called from a dtor,
      // therefore we have no chance to finish this later
      // yes, this is ugly and potentially dangerous, but the alternative is losing
      // currently composed messages...
      while ( win->isComposing() )
        qApp->processEvents();
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

void KMKernel::slotShowConfigurationDialog()
{
  if( !mConfigureDialog ) {
    mConfigureDialog = new ConfigureDialog( 0, false );
    mConfigureDialog->setObjectName( "configure" );
    connect( mConfigureDialog, SIGNAL( configChanged() ),
             this, SLOT( slotConfigChanged() ) );
  }

  if( KMKernel::getKMMainWidget() == 0 ) {
    // ensure that there is a main widget available
    // as parts of the configure dialog (identity) rely on this
    // and this slot can be called when there is only a KMComposeWin showing
    KMMainWin *win = new KMMainWin;
    win->show();

  }

  // Save all current settings.
  if( getKMMainWidget() )
    getKMMainWidget()->writeConfig();

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

bool KMKernel::haveSystemTrayApplet()
{
  return !systemTrayApplets.isEmpty();
}

bool KMKernel::registerSystemTrayApplet( const KStatusNotifierItem* applet )
{
  if ( !systemTrayApplets.contains( applet ) ) {
    systemTrayApplets.append( applet );
    return true;
  }
  else
    return false;
}

bool KMKernel::unregisterSystemTrayApplet( const KStatusNotifierItem* applet )
{
  return systemTrayApplets.removeAll( applet ) > 0;
}

void KMKernel::emergencyExit( const QString& reason )
{
  QString mesg;
  if ( reason.length() == 0 ) {
    mesg = i18n("KMail encountered a fatal error and will terminate now");
  }
  else {
    mesg = i18n("KMail encountered a fatal error and will "
                      "terminate now.\nThe error was:\n%1", reason );
  }

  kWarning() << mesg;
  KMessageBox::error( 0, mesg );

  ::exit(1);
}

/**
 * Returns true if the folder is either the outbox or one of the drafts-folders
 */
bool KMKernel::folderIsDraftOrOutbox(const Akonadi::Collection & col)
{
  if ( col == Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Outbox ) )
    return true;
  return folderIsDrafts( col );
}

bool KMKernel::folderIsDrafts(const Akonadi::Collection & col)
{
  if ( col ==  Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Drafts ) )
    return true;

  QString idString = QString::number( col.id() );
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the drafts-folder
  const KPIMIdentities::IdentityManager * im = identityManager();
  for( KPIMIdentities::IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).drafts() == idString ) return true;
  return false;
}

bool KMKernel::folderIsTemplates(const Akonadi::Collection &col)
{
  if ( col ==  Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Templates ) )
    return true;

  QString idString = QString::number( col.id() );
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the templates-folder
  const KPIMIdentities::IdentityManager * im = identityManager();
  for( KPIMIdentities::IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).templates() == idString ) return true;
  return false;
}

Akonadi::Collection KMKernel::trashCollectionFromResource( const Akonadi::Collection & col )
{
  Akonadi::Collection trashCol;
  if ( col.isValid() ) {
    if ( col.resource().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiImapSettingsInterface *iface = KMail::Util::createImapSettingsInterface( col.resource() );
      if ( iface->isValid() ) {

        trashCol =  Akonadi::Collection( iface->trashCollection() );
        delete iface;
        return trashCol;
      }
      delete iface;
    }
  }
  return trashCol;
}

bool KMKernel::folderIsTrash( const Akonadi::Collection & col )
{
  if ( col == Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Trash ) )
    return true;
  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach ( const Akonadi::AgentInstance& type, lst ) {
    //TODO verify it.
    if ( type.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiImapSettingsInterface *iface = KMail::Util::createImapSettingsInterface( type.identifier() );
      if ( iface->isValid() ) {
        if ( iface->trashCollection() == col.id() ) {
          delete iface;
          return true;
        }
      }
      delete iface;
    }
  }
  return false;
}

bool KMKernel::folderIsSentMailFolder( const Akonadi::Collection &col )
{
  if ( col == Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::SentMail ) )
    return true;

  QString idString = QString::number( col.id() );
  if ( idString.isEmpty() ) return false;

  // search the identities if the folder matches the sent-folder
  const KPIMIdentities::IdentityManager * im = identityManager();
  for( KPIMIdentities::IdentityManager::ConstIterator it = im->begin(); it != im->end(); ++it )
    if ( (*it).fcc() == idString ) return true;
  return false;
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


/**
 * Empties all trash folders
 */
void KMKernel::slotEmptyTrash()
{
  QString title = i18n("Empty Trash");
  QString text = i18n("Are you sure you want to empty the trash folders of all accounts?");
  if (KMessageBox::warningContinueCancel(0, text, title,
                                         KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                                         "confirm_empty_trash")
      != KMessageBox::Continue)
  {
    return;
  }
  Akonadi::Collection trash = trashCollectionFolder();
  mFolderCollectionMonitor->expunge( trash );

  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach ( const Akonadi::AgentInstance& type, lst ) {
    //TODO verify it.
    if ( type.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      OrgKdeAkonadiImapSettingsInterface *iface = KMail::Util::createImapSettingsInterface( type.identifier() );
      if ( iface->isValid() ) {
        int trashImap = iface->trashCollection();
        if ( trashImap != trash.id() ) {
          mFolderCollectionMonitor->expunge( Akonadi::Collection( trashImap ) );
        }
      }
      delete iface;
    }
  }
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
    MessageViewer::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageViewer::GlobalSettings::self()->readConfig();
    TemplateParser::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    TemplateParser::GlobalSettings::self()->readConfig();
    MessageComposer::MessageComposerSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageComposer::MessageComposerSettings::self()->readConfig();
    MessageCore::GlobalSettings::self()->setSharedConfig( mySelf->mConfig );
    MessageCore::GlobalSettings::self()->readConfig();
  }
  return mySelf->mConfig;
}


void KMKernel::selectFolder( const QString &folder )
{
  KMMainWidget *widget = getKMMainWidget();
  Q_ASSERT( widget );
  if ( !widget )
    return;

  Akonadi::Collection colFolder = collectionFromId( folder );

  if( colFolder.isValid() )
    widget->selectCollectionFolder( colFolder );
}

KMMainWidget *KMKernel::getKMMainWidget()
{
  //This could definitely use a speadup
  QWidgetList l = QApplication::topLevelWidgets();
  QWidget *wid;

  Q_FOREACH( wid, l ) {
    QList<KMMainWidget*> l2 = wid->topLevelWidget()->findChildren<KMMainWidget*>();
    if ( !l2.isEmpty() && l2.first() )
      return l2.first();
  }
  return 0;
}

void KMKernel::slotRunBackgroundTasks() // called regularly by timer
{
  // Hidden KConfig keys. Not meant to be used, but a nice fallback in case
  // a stable kmail release goes out with a nasty bug in CompactionJob...
  KConfigGroup generalGroup( config(), "General" );
  if ( generalGroup.readEntry( "auto-expiring", true ) ) {
      mFolderCollectionMonitor->expireAllFolders( false /*scheduled, not immediate*/ );
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
  for ( int i = 0; i < model->rowCount( parent ); i++ ) {
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
  mFolderCollectionMonitor->expireAllFolders( true /*immediate*/ );
}

Akonadi::Collection KMKernel::findFolderCollectionById( const QString& idString )
{
  int id = idString.toInt();
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base, this );
  if ( job->exec() ) {
    Akonadi::Collection::List lst = job->collections();
    if ( lst.count() == 1 )
      return lst.at( 0 );
  }
  delete job;
  return Akonadi::Collection();
}

Akonadi::Collection KMKernel::collectionFromId( const QString &idString ) const
{
  bool ok;
  Akonadi::Collection::Id id = idString.toLongLong( &ok );
  if ( !ok )
    return Akonadi::Collection();
  return collectionFromId( id );
}

Akonadi::Collection KMKernel::collectionFromId(const Akonadi::Collection::Id& id) const
{
  const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(
    collectionModel(), Akonadi::Collection(id)
  );
  return idx.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

bool KMKernel::canQueryClose()
{
  if ( KMMainWidget::mainWidgetList() &&
       KMMainWidget::mainWidgetList()->count() > 1 )
    return true;
  KMMainWidget *widget = getKMMainWidget();
  if ( !widget )
    return true;
  KMSystemTray* systray = widget->systray();
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
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != im->modifyEnd(); ++it ) {
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
  for ( KPIMIdentities::IdentityManager::Iterator it = im->modifyBegin(); it != im->modifyEnd(); ++it ) {
    if ( oldName == (*it).transport() ) {
      (*it).setTransport( newName );
      changedIdents << (*it).identityName();
    }
  }

  if ( !changedIdents.isEmpty() ) {
    QString information =
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
  kDebug() << "Created ProgressItem";
}


void KMKernel::updatedTemplates()
{
  emit customTemplatesChanged();
}


Akonadi::Collection KMKernel::inboxCollectionFolder()
{
  if ( the_inboxCollectionFolder < 0 )
    the_inboxCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Inbox ).id();
  return collectionFromId( the_inboxCollectionFolder );
}

Akonadi::Collection KMKernel::outboxCollectionFolder()
{
  if ( the_outboxCollectionFolder < 0 )
    the_outboxCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Outbox ).id();
  return collectionFromId( the_outboxCollectionFolder );
}

Akonadi::Collection KMKernel::sentCollectionFolder()
{
  if ( the_sentCollectionFolder < 0 )
    the_sentCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::SentMail ).id();
  return collectionFromId( the_sentCollectionFolder );
}

Akonadi::Collection KMKernel::trashCollectionFolder()
{
  if ( the_trashCollectionFolder < 0 )
    the_trashCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Trash ).id();
  return collectionFromId( the_trashCollectionFolder );
}

Akonadi::Collection KMKernel::draftsCollectionFolder()
{
  if ( the_draftsCollectionFolder < 0 )
    the_draftsCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Drafts ).id();
  return collectionFromId( the_draftsCollectionFolder );
}

Akonadi::Collection KMKernel::templatesCollectionFolder()
{
  if ( the_templatesCollectionFolder < 0 )
    the_templatesCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Templates ).id();
  return collectionFromId( the_templatesCollectionFolder );
}

bool KMKernel::isSystemFolderCollection( const Akonadi::Collection &col)
{
  return ( col == inboxCollectionFolder() ||
           col == outboxCollectionFolder() ||
           col == sentCollectionFolder() ||
           col == trashCollectionFolder() ||
           col == draftsCollectionFolder() ||
           col == templatesCollectionFolder() );
}

bool KMKernel::isMainFolderCollection( const Akonadi::Collection &col )
{
  return col == inboxCollectionFolder();
}

bool KMKernel::isImapFolder( const Akonadi::Collection &col )
{
  Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance( col.resource() );
  return agentInstance.type().identifier() == IMAP_RESOURCE_IDENTIFIER;
}


void KMKernel::stopAgentInstance()
{
  const QString resourceGroupPattern( "Resource %1" );

  Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
  foreach( Akonadi::AgentInstance type, lst ) {
    KConfigGroup group( KMKernel::config(), resourceGroupPattern.arg( type.identifier() ) );
    if ( group.readEntry( "OfflineOnShutdown", false ) )
      type.setIsOnline( false );
  }
}


#include "kmkernel.moc"
