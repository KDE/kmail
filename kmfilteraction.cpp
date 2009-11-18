// kmfilteraction.cpp

#include "kmfilteraction.h"

// other KMail headers:
#include "customtemplates.h"
#include "customtemplates_kfg.h"
#include "kmcommands.h"
#include "kmfiltermgr.h"
#include "messagesender.h"
#include "kmmainwidget.h"
#include "kmfawidgets.h"
#include "folderrequester.h"

using KMail::FolderRequester;
#include "messageproperty.h"
#include "actionscheduler.h"
using KMail::MessageProperty;
using KMail::ActionScheduler;
#include "regexplineedit.h"
using KMail::RegExpLineEdit;
#include "stringutil.h"
#include "messageviewer/stringutil.h"
#include "kmmessagetag.h"

#include "messagehelper.h"
#include "messageinfo.h"

// KDE PIM libs headers
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimutils/kfileio.h>
#include <kpimutils/email.h>

// KDE headers
#include <kcombobox.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <Phonon/MediaObject>
#include <kshell.h>
#include <kprocess.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#ifdef Nepomuk_FOUND
  #include <nepomuk/tag.h>
#endif

#include <kmime/kmime_message.h>

// Qt headers:
#include <QTextCodec>
#include <QTextDocument>

// other headers:
#include <assert.h>
#include <string.h>

//=============================================================================
//
// KMFilterAction
//
//=============================================================================

KMFilterAction::KMFilterAction( const char* aName, const QString &aLabel )
{
  mName = aName;
  mLabel = aLabel;
}

KMFilterAction::~KMFilterAction()
{
}

void KMFilterAction::processAsync( const KMime::Message::Ptr &msg ) const
{
  ActionScheduler *handler = MessageProperty::filterHandler( msg.get() );
  ReturnCode result = process( msg );
  if (handler)
    handler->actionMessage( result );
}

bool KMFilterAction::requiresBody(KMime::Content *) const
{
  return true;
}

KMFilterAction* KMFilterAction::newAction()
{
  return 0;
}

QWidget* KMFilterAction::createParamWidget(QWidget* parent) const
{
  return new QWidget(parent);
}

void KMFilterAction::applyParamWidgetValue(QWidget*)
{
}

void KMFilterAction::setParamWidgetValue( QWidget * ) const
{
}

void KMFilterAction::clearParamWidget( QWidget * ) const
{
}

bool KMFilterAction::folderRemoved(const Akonadi::Collection&, const Akonadi::Collection&)
{
  return false;
}

int KMFilterAction::tempOpenFolder(KMFolder* aFolder)
{
  return kmkernel->filterMgr()->tempOpenFolder(aFolder);
}

void KMFilterAction::sendMDN( const KMime::Message::Ptr &msg, KMime::MDN::DispositionType d,
                              const QList<KMime::MDN::DispositionModifier> & m ) {
  if ( !msg ) return;

  /* createMDN requires Return-Path and Disposition-Notification-To
   * if it is not set in the message we assume that the notification should go to the
   * sender
   */
  const QString returnPath = msg->headerByType( "Return-Path" ) ? msg->headerByType( "Return-Path ")->asUnicodeString() : "";
  const QString dispNoteTo = msg->headerByType( "Disposition-Notification-To" )? msg->headerByType( "Disposition-Notification-To ")->asUnicodeString() : "";
  if ( returnPath.isEmpty() ) {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "Return-Path", msg.get(), msg->from()->asUnicodeString(), "utf-8" );
    msg->setHeader( header );
  }
  if ( dispNoteTo.isEmpty() ) {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "Disposition-Notification-To", msg.get(), msg->from()->asUnicodeString(), "utf-8" );
    msg->setHeader( header );
  }

  kDebug() << "AKONADI PORT: verify Akonadi::Item() here  " << Q_FUNC_INFO;
  KMime::Message::Ptr mdn = KMail::MessageHelper::createMDN( Akonadi::Item(), msg, KMime::MDN::AutomaticAction, d, false, m );
  if ( mdn && !kmkernel->msgSender()->send( mdn, KMail::MessageSender::SendLater ) ) {
    kDebug() << "Sending failed.";
    //delete mdn;
  }

  //restore orignial header
  if ( returnPath.isEmpty() )
    msg->removeHeader( "Return-Path" );
  if ( dispNoteTo.isEmpty() )
    msg->removeHeader( "Disposition-Notification-To" );
}


//=============================================================================
//
// KMFilterActionWithNone
//
//=============================================================================

KMFilterActionWithNone::KMFilterActionWithNone( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel )
{
}

const QString KMFilterActionWithNone::displayString() const
{
  return label();
}


//=============================================================================
//
// KMFilterActionWithUOID
//
//=============================================================================

KMFilterActionWithUOID::KMFilterActionWithUOID( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel ), mParameter( 0 )
{
}

void KMFilterActionWithUOID::argsFromString( const QString &argsStr )
{
  mParameter = argsStr.trimmed().toUInt();
}

const QString KMFilterActionWithUOID::argsAsString() const
{
  return QString::number( mParameter );
}

const QString KMFilterActionWithUOID::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}


//=============================================================================
//
// KMFilterActionWithString
//
//=============================================================================

KMFilterActionWithString::KMFilterActionWithString( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel )
{
}

QWidget* KMFilterActionWithString::createParamWidget( QWidget* parent ) const
{
  KLineEdit *le = new KLineEdit(parent);
  le->setClearButtonShown( true );
  le->setText( mParameter );
  return le;
}

void KMFilterActionWithString::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((KLineEdit*)paramWidget)->text();
}

void KMFilterActionWithString::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((KLineEdit*)paramWidget)->setText( mParameter );
}

void KMFilterActionWithString::clearParamWidget( QWidget* paramWidget ) const
{
  ((KLineEdit*)paramWidget)->clear();
}

void KMFilterActionWithString::argsFromString( const QString &argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithString::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithString::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

//=============================================================================
//
// class KMFilterActionWithStringList
//
//=============================================================================

KMFilterActionWithStringList::KMFilterActionWithStringList( const char* aName, const QString &aLabel )
  : KMFilterActionWithString( aName, aLabel )
{
}

QWidget* KMFilterActionWithStringList::createParamWidget( QWidget* parent ) const
{
  KComboBox *cb = new KComboBox( parent );
  cb->setEditable( false );
  cb->addItems( mParameterList );
  setParamWidgetValue( cb );
  return cb;
}

void KMFilterActionWithStringList::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((KComboBox*)paramWidget)->currentText();
}

void KMFilterActionWithStringList::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.indexOf( mParameter );
  ((KComboBox*)paramWidget)->setCurrentIndex( idx >= 0 ? idx : 0 );
}

void KMFilterActionWithStringList::clearParamWidget( QWidget* paramWidget ) const
{
  ((KComboBox*)paramWidget)->setCurrentIndex(0);
}

void KMFilterActionWithStringList::argsFromString( const QString &argsStr )
{
  int idx = mParameterList.indexOf( argsStr );
  if ( idx < 0 ) {
    mParameterList.append( argsStr );
    idx = mParameterList.count() - 1;
  }
  mParameter = mParameterList.at( idx );
}


//=============================================================================
//
// class KMFilterActionWithFolder
//
//=============================================================================

KMFilterActionWithFolder::KMFilterActionWithFolder( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel )
{
}

QWidget* KMFilterActionWithFolder::createParamWidget( QWidget* parent ) const
{
  FolderRequester *req = new FolderRequester( parent );
  setParamWidgetValue( req );
  return req;
}

void KMFilterActionWithFolder::applyParamWidgetValue( QWidget* paramWidget )
{
  mFolder = ((FolderRequester *)paramWidget)->folderCollection();
  mFolderName = ((FolderRequester *)paramWidget)->folderId();
}

void KMFilterActionWithFolder::setParamWidgetValue( QWidget* paramWidget ) const
{
  if ( mFolder.isValid() )
    ((FolderRequester *)paramWidget)->setFolder( mFolder );
  else
    ((FolderRequester *)paramWidget)->setFolder( mFolderName );
}

void KMFilterActionWithFolder::clearParamWidget( QWidget* paramWidget ) const
{
  ((FolderRequester *)paramWidget)->setFolder( kmkernel->draftsCollectionFolder() );
}

void KMFilterActionWithFolder::argsFromString( const QString &argsStr )
{
  mFolder = kmkernel->findFolderCollectionById( argsStr );
  if ( mFolder.isValid() )
    mFolderName= QString::number(mFolder.id());
  else
    mFolderName = argsStr;
}

const QString KMFilterActionWithFolder::argsAsString() const
{
  QString result;
  if ( mFolder.isValid() )
    result = QString::number(mFolder.id());
  else
    result = mFolderName;
  return result;
}

const QString KMFilterActionWithFolder::displayString() const
{
  QString result;
  if ( mFolder.isValid() )
    result = mFolder.url().path();
  else
    result = mFolderName;
  return label() + " \"" + Qt::escape( result ) + "\"";
}

bool KMFilterActionWithFolder::folderRemoved( const Akonadi::Collection& aFolder, const Akonadi::Collection& aNewFolder )
{
  if ( aFolder == mFolder ) {
    mFolder = aNewFolder;
    if ( aNewFolder.isValid() )
      mFolderName = mFolder.id();
    return true;
  } else
    return false;
}

//=============================================================================
//
// class KMFilterActionWithAddress
//
//=============================================================================

KMFilterActionWithAddress::KMFilterActionWithAddress( const char* aName, const QString &aLabel )
  : KMFilterActionWithString( aName, aLabel )
{
}

QWidget* KMFilterActionWithAddress::createParamWidget( QWidget* parent ) const
{
  KMFilterActionWithAddressWidget *w = new KMFilterActionWithAddressWidget(parent);
  w->setText( mParameter );
  return w;
}

void KMFilterActionWithAddress::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((KMFilterActionWithAddressWidget*)paramWidget)->text();
}

void KMFilterActionWithAddress::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((KMFilterActionWithAddressWidget*)paramWidget)->setText( mParameter );
}

void KMFilterActionWithAddress::clearParamWidget( QWidget* paramWidget ) const
{
  ((KMFilterActionWithAddressWidget*)paramWidget)->clear();
}

//=============================================================================
//
// class KMFilterActionWithCommand
//
//=============================================================================

KMFilterActionWithCommand::KMFilterActionWithCommand( const char* aName, const QString &aLabel )
  : KMFilterActionWithUrl( aName, aLabel )
{
}

QWidget* KMFilterActionWithCommand::createParamWidget( QWidget* parent ) const
{
  return KMFilterActionWithUrl::createParamWidget( parent );
}

void KMFilterActionWithCommand::applyParamWidgetValue( QWidget* paramWidget )
{
  KMFilterActionWithUrl::applyParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::setParamWidgetValue( QWidget* paramWidget ) const
{
  KMFilterActionWithUrl::setParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::clearParamWidget( QWidget* paramWidget ) const
{
  KMFilterActionWithUrl::clearParamWidget( paramWidget );
}

QString KMFilterActionWithCommand::substituteCommandLineArgsFor( const KMime::Message::Ptr &aMsg, QList<KTemporaryFile*> & aTempFileList ) const
{
  QString result = mParameter;
  QList<int> argList;
  QRegExp r( "%[0-9-]+" );

  // search for '%n'
  int start = -1;
  while ( ( start = r.indexIn( result, start + 1 ) ) > 0 ) {
    int len = r.matchedLength();
    // and save the encountered 'n' in a list.
    bool OK = false;
    int n = result.mid( start + 1, len - 1 ).toInt( &OK );
    if ( OK )
      argList.append( n );
  }

  // sort the list of n's
  qSort( argList );

  // and use QString::arg to substitute filenames for the %n's.
  int lastSeen = -2;
  QString tempFileName;
  for ( QList<int>::Iterator it = argList.begin() ; it != argList.end() ; ++it ) {
    // setup temp files with check for duplicate %n's
    if ( (*it) != lastSeen ) {
      KTemporaryFile *tf = new KTemporaryFile();
      if ( !tf->open() ) {
        delete tf;
        kDebug() << "KMFilterActionWithCommand: Could not create temp file!";
        return QString();
      }
      aTempFileList.append( tf );
      tempFileName = tf->fileName();
      if ((*it) == -1)
        KPIMUtils::kByteArrayToFile( aMsg->encodedContent(), tempFileName, //###
                          false, false, false );
      else if (aMsg->contents().size() == 0)
        KPIMUtils::kByteArrayToFile( aMsg->decodedContent(), tempFileName,
                          false, false, false );
      else {
        KMime::Content content;
#if 0 //TODO port to akonadi
        aMsg->bodyPart( (*it), &msgPart );
        KPIMUtils::kByteArrayToFile( msgPart.bodyDecodedBinary(), tempFileName,
                          false, false, false );
#else
        kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
      }
      tf->close();
    }
    // QString( "%0 and %1 and %1" ).arg( 0 ).arg( 1 )
    // returns "0 and 1 and %1", so we must call .arg as
    // many times as there are %n's, regardless of their multiplicity.
    if ((*it) == -1) result.replace( "%-1", tempFileName );
    else result = result.arg( tempFileName );
  }

  // And finally, replace the %{foo} with the content of the foo
  // header field:
  QRegExp header_rx( "%\\{([a-z0-9-]+)\\}", Qt::CaseInsensitive );
  int idx = 0;
  while ( ( idx = header_rx.indexIn( result, idx ) ) != -1 ) {
    QString replacement = KShell::quoteArg( aMsg->headerByType( header_rx.cap(1).toLatin1() ) ? aMsg->headerByType( header_rx.cap(1).toLatin1() )->as7BitString(): "");
    result.replace( idx, header_rx.matchedLength(), replacement );
    idx += replacement.length();
  }

  return result;
}


KMFilterAction::ReturnCode KMFilterActionWithCommand::genericProcess( const KMime::Message::Ptr &aMsg, bool withOutput ) const
{
  Q_ASSERT( aMsg );

  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // KProcess doesn't support a QProcess::launch() equivalent, so
  // we must use a temp file :-(
  KTemporaryFile * inFile = new KTemporaryFile;
  if ( !inFile->open() )
    return ErrorButGoOn;

  QList<KTemporaryFile*> atmList;
  atmList.append( inFile );

  QString commandLine = substituteCommandLineArgsFor( aMsg, atmList );
  if ( commandLine.isEmpty() )
  {
    qDeleteAll( atmList );
    atmList.clear();
    return ErrorButGoOn;
  }
  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  '(' + commandLine + ") <" + inFile->fileName();

  // write message to file
  QString tempFileName = inFile->fileName();
  if ( !KPIMUtils::kByteArrayToFile( aMsg->encodedContent(), tempFileName, //###
                                     false, false, false ) ) {
    qDeleteAll( atmList );
    atmList.clear();
    return CriticalError;
  }
  inFile->close();

  KProcess shProc;
  shProc.setOutputChannelMode( KProcess::SeparateChannels );
  shProc.setShellCommand( commandLine );
  int result = shProc.execute();

  if ( result != 0 ) {
    qDeleteAll( atmList );
    atmList.clear();
    return ErrorButGoOn;
  }

  if ( withOutput ) {
    // read altered message:
    QByteArray msgText = shProc.readAllStandardOutput();

    if ( !msgText.isEmpty() ) {
    /* If the pipe through alters the message, it could very well
       happen that it no longer has a X-UID header afterwards. That is
       unfortunate, as we need to removed the original from the folder
       using that, and look it up in the message. When the (new) message
       is uploaded, the header is stripped anyhow. */
      QString uid = aMsg->headerByType( "X-UID" ) ?aMsg->headerByType( "X-UID")->asUnicodeString() : "" ;
      aMsg->setContent( msgText );

      KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-UID", aMsg.get(), uid, "utf-8" );
      aMsg->setHeader( header );
    }
    else {
      qDeleteAll( atmList );
      atmList.clear();
      return ErrorButGoOn;
    }
  }
  qDeleteAll( atmList );
  atmList.clear();
  return GoOn;
}


//=============================================================================
//
//   Specific  Filter  Actions
//
//=============================================================================

//=============================================================================
// KMFilterActionSendReceipt - send receipt
// Return delivery receipt.
//=============================================================================
class KMFilterActionSendReceipt : public KMFilterActionWithNone
{
public:
  KMFilterActionSendReceipt();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionSendReceipt::newAction(void)
{
  return (new KMFilterActionSendReceipt);
}

KMFilterActionSendReceipt::KMFilterActionSendReceipt()
  : KMFilterActionWithNone( "confirm delivery", i18n("Confirm Delivery") )
{
}

KMFilterAction::ReturnCode KMFilterActionSendReceipt::process( const KMime::Message::Ptr &msg ) const
{
  kDebug() << "AKONADI PORT: verify Akonadi::Item() here  " << Q_FUNC_INFO;

  KMime::Message::Ptr receipt = KMail::MessageHelper::createDeliveryReceipt( Akonadi::Item(), msg );
  if ( !receipt ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the receipt before sending and b) for speed reasons.
  kmkernel->msgSender()->send( receipt, KMail::MessageSender::SendLater );

  return GoOn;
}



//=============================================================================
// KMFilterActionSetTransport - set transport to...
// Specify mail transport (smtp server) to be used when replying to a message
// TODO: use TransportComboBox so the user does not enter an invalid transport
//=============================================================================
class KMFilterActionTransport: public KMFilterActionWithString
{
public:
  KMFilterActionTransport();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionTransport::newAction(void)
{
  return (new KMFilterActionTransport);
}

KMFilterActionTransport::KMFilterActionTransport()
  : KMFilterActionWithString( "set transport", i18n("Set Transport To") )
{
}

KMFilterAction::ReturnCode KMFilterActionTransport::process( const KMime::Message::Ptr &msg ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", msg.get(), mParameter, "utf-8");
  msg->setHeader( header );
  return GoOn;
}


//=============================================================================
// KMFilterActionReplyTo - set Reply-To to
// Set the Reply-to header in a message
//=============================================================================
class KMFilterActionReplyTo: public KMFilterActionWithString
{
public:
  KMFilterActionReplyTo();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionReplyTo::newAction(void)
{
  return (new KMFilterActionReplyTo);
}

KMFilterActionReplyTo::KMFilterActionReplyTo()
  : KMFilterActionWithString( "set Reply-To", i18n("Set Reply-To To") )
{
  mParameter = "";
}

KMFilterAction::ReturnCode KMFilterActionReplyTo::process( const KMime::Message::Ptr &msg ) const
{
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "Reply-To", msg.get(), mParameter, "utf-8");
  msg->setHeader( header );
  return GoOn;
}



//=============================================================================
// KMFilterActionIdentity - set identity to
// Specify Identity to be used when replying to a message
//=============================================================================
class KMFilterActionIdentity: public KMFilterActionWithUOID
{
public:
  KMFilterActionIdentity();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction();

  QWidget * createParamWidget( QWidget * parent ) const;
  void applyParamWidgetValue( QWidget * parent );
  void setParamWidgetValue( QWidget * parent ) const;
  void clearParamWidget( QWidget * param ) const;
};

KMFilterAction* KMFilterActionIdentity::newAction()
{
  return (new KMFilterActionIdentity);
}

KMFilterActionIdentity::KMFilterActionIdentity()
  : KMFilterActionWithUOID( "set identity", i18n("Set Identity To") )
{
  mParameter = kmkernel->identityManager()->defaultIdentity().uoid();
}

KMFilterAction::ReturnCode KMFilterActionIdentity::process( const KMime::Message::Ptr &msg ) const
{
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg.get(), QString::number(mParameter), "utf-8");
  msg->setHeader( header );
  return GoOn;
}

QWidget * KMFilterActionIdentity::createParamWidget( QWidget * parent ) const
{
  KPIMIdentities::IdentityCombo * ic = new KPIMIdentities::IdentityCombo( kmkernel->identityManager(), parent );
  ic->setCurrentIdentity( mParameter );
  return ic;
}

void KMFilterActionIdentity::applyParamWidgetValue( QWidget * paramWidget )
{
  KPIMIdentities::IdentityCombo * ic = dynamic_cast<KPIMIdentities::IdentityCombo*>( paramWidget );
  assert( ic );
  mParameter = ic->currentIdentity();
}

void KMFilterActionIdentity::clearParamWidget( QWidget * paramWidget ) const
{
  KPIMIdentities::IdentityCombo * ic = dynamic_cast<KPIMIdentities::IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentIndex( 0 );
  //ic->setCurrentIdentity( kmkernel->identityManager()->defaultIdentity() );
}

void KMFilterActionIdentity::setParamWidgetValue( QWidget * paramWidget ) const
{
  KPIMIdentities::IdentityCombo * ic = dynamic_cast<KPIMIdentities::IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentIdentity( mParameter );
}

//=============================================================================
// KMFilterActionSetStatus - set status to
// Set the status of messages
//=============================================================================
class KMFilterActionSetStatus: public KMFilterActionWithStringList
{
public:
  KMFilterActionSetStatus();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual bool requiresBody(KMime::Content*) const;

  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString &argsStr );
  virtual const QString argsAsString() const;
  virtual const QString displayString() const;
};


static const MessageStatus stati[] =
{
  MessageStatus::statusImportant(),
  MessageStatus::statusRead(),
  MessageStatus::statusUnread(),
  MessageStatus::statusReplied(),
  MessageStatus::statusForwarded(),
  MessageStatus::statusOld(),
  MessageStatus::statusNew(),
  MessageStatus::statusWatched(),
  MessageStatus::statusIgnored(),
  MessageStatus::statusSpam(),
  MessageStatus::statusHam(),
  MessageStatus::statusToAct()
};
static const int StatiCount = sizeof( stati ) / sizeof( MessageStatus );

KMFilterAction* KMFilterActionSetStatus::newAction()
{
  return (new KMFilterActionSetStatus);
}

KMFilterActionSetStatus::KMFilterActionSetStatus()
  : KMFilterActionWithStringList( "set status", i18n("Mark As") )
{
  // if you change this list, also update
  // KMFilterActionSetStatus::stati above
  mParameterList.append( "" );
  mParameterList.append( i18nc("msg status","Important") );
  mParameterList.append( i18nc("msg status","Read") );
  mParameterList.append( i18nc("msg status","Unread") );
  mParameterList.append( i18nc("msg status","Replied") );
  mParameterList.append( i18nc("msg status","Forwarded") );
  mParameterList.append( i18nc("msg status","Old") );
  mParameterList.append( i18nc("msg status","New") );
  mParameterList.append( i18nc("msg status","Watched") );
  mParameterList.append( i18nc("msg status","Ignored") );
  mParameterList.append( i18nc("msg status","Spam") );
  mParameterList.append( i18nc("msg status","Ham") );
  mParameterList.append( i18nc("msg status","Action Item") );

  mParameter = mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionSetStatus::process( const KMime::Message::Ptr &msg ) const
{
#if 0 //TODO port to akonadi
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  msg->setStatus( stati[idx-1] );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return GoOn;
}

bool KMFilterActionSetStatus::requiresBody(KMime::Content*) const
{
  return false;
}

void KMFilterActionSetStatus::argsFromString( const QString &argsStr )
{
  if ( argsStr.length() == 1 ) {
    MessageStatus status;
    int i;
    for ( i = 0 ; i < StatiCount ; i++ )
    {
      status = stati[i];
      if ( status.getStatusStr()[0] == argsStr[0].toLatin1() ) {
        mParameter = mParameterList.at(i+1);
        return;
      }
    }
  }
  mParameter = mParameterList.at(0);
}

const QString KMFilterActionSetStatus::argsAsString() const
{
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return QString();

  return stati[idx-1].getStatusStr();
}

const QString KMFilterActionSetStatus::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionAddTag - append tag to message
// Appends a tag to messages
//=============================================================================
class KMFilterActionAddTag: public KMFilterActionWithStringList
{
public:
  KMFilterActionAddTag();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual bool requiresBody( KMime::Content* ) const;

  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString &argsStr );
  virtual const QString argsAsString() const;
  virtual const QString displayString() const;

private:
  QStringList mLabelList;
};

KMFilterAction* KMFilterActionAddTag::newAction()
{
  return (new KMFilterActionAddTag);
}

KMFilterActionAddTag::KMFilterActionAddTag()
  : KMFilterActionWithStringList( "add tag", i18n("Add Tag") )
{
  const QHash<QString, KMMessageTagDescription *> * tagDict = kmkernel->msgTagMgr()->msgTagDict();
  if ( tagDict ) {
    foreach ( const KMMessageTagDescription * tagDesc, *tagDict ) {
      mParameterList.append( tagDesc->name() );
      mLabelList.append( tagDesc->label() );
    }
  }
}

KMFilterAction::ReturnCode KMFilterActionAddTag::process( const KMime::Message::Ptr &msg ) const
{
#if 0 //TODO port to akonadi
  const int idx = mParameterList.indexOf( mParameter );
  if ( idx == -1 ) return ErrorButGoOn;

  KMMessageTagList tagList;
  if ( msg->tagList() )
    tagList = * msg->tagList();

  const QString& tagLabel = mLabelList.at( idx );
  const int tagPosition = tagList.indexOf( tagLabel );
  if ( tagPosition == -1 ) {
    tagList.append( tagLabel );
#ifdef Nepomuk_FOUND
    n_resource.addTag( mParameter );
#endif
    msg->setTagList( tagList );
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return GoOn;
}

bool KMFilterActionAddTag::requiresBody( KMime::Content* ) const
{
  return false;
}

void KMFilterActionAddTag::argsFromString( const QString &argsStr )
{
  foreach ( const QString& tag, mParameterList ) {
    if ( tag == argsStr ) {
      mParameter = tag;
      return;
    }
  }
  mParameter = mParameterList.at( 0 );
}

const QString KMFilterActionAddTag::argsAsString() const
{
  const int idx = mParameterList.indexOf( mParameter );
  if ( idx == -1 ) return QString();

  return mParameterList.at( idx );
}

const QString KMFilterActionAddTag::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionFakeDisposition - send fake MDN
// Sends a fake MDN or forces an ignore.
//=============================================================================
class KMFilterActionFakeDisposition: public KMFilterActionWithStringList
{
public:
  KMFilterActionFakeDisposition();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction() {
    return (new KMFilterActionFakeDisposition);
  }

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString &argsStr );
  virtual const QString argsAsString() const;
  virtual const QString displayString() const;
};


// if you change this list, also update
// the count in argsFromString
static const KMime::MDN::DispositionType mdns[] =
{
  KMime::MDN::Displayed,
  KMime::MDN::Deleted,
  KMime::MDN::Dispatched,
  KMime::MDN::Processed,
  KMime::MDN::Denied,
  KMime::MDN::Failed,
};
static const int numMDNs = sizeof mdns / sizeof *mdns;


KMFilterActionFakeDisposition::KMFilterActionFakeDisposition()
  : KMFilterActionWithStringList( "fake mdn", i18n("Send Fake MDN") )
{
  // if you change this list, also update
  // mdns above
  mParameterList.append( "" );
  mParameterList.append( i18nc("MDN type","Ignore") );
  mParameterList.append( i18nc("MDN type","Displayed") );
  mParameterList.append( i18nc("MDN type","Deleted") );
  mParameterList.append( i18nc("MDN type","Dispatched") );
  mParameterList.append( i18nc("MDN type","Processed") );
  mParameterList.append( i18nc("MDN type","Denied") );
  mParameterList.append( i18nc("MDN type","Failed") );

  mParameter = mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionFakeDisposition::process( const KMime::Message::Ptr &msg ) const
{
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  if ( idx == 1 ) // ignore
    KMail::MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
  else // send
    sendMDN( msg, mdns[idx-2] ); // skip first two entries: "" and "ignore"
  return GoOn;
}

void KMFilterActionFakeDisposition::argsFromString( const QString &argsStr )
{
  if ( argsStr.length() == 1 ) {
    if ( argsStr[0] == 'I' ) { // ignore
      mParameter = mParameterList.at(1);
      return;
    }
    for ( int i = 0 ; i < numMDNs ; i++ )
      if ( char(mdns[i]) == argsStr[0] ) { // send
        mParameter = mParameterList.at(i+2);
        return;
      }
  }
  mParameter = mParameterList.at(0);
}

const QString KMFilterActionFakeDisposition::argsAsString() const
{
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return QString();

  return QString( QChar( idx < 2 ? 'I' : char(mdns[idx-2]) ) );
}

const QString KMFilterActionFakeDisposition::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionRemoveHeader - remove header
// Remove all instances of the given header field.
//=============================================================================
class KMFilterActionRemoveHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRemoveHeader();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;

  static KMFilterAction* newAction();
};

KMFilterAction* KMFilterActionRemoveHeader::newAction()
{
  return (new KMFilterActionRemoveHeader);
}

KMFilterActionRemoveHeader::KMFilterActionRemoveHeader()
  : KMFilterActionWithStringList( "remove header", i18n("Remove Header") )
{
  mParameterList << ""
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = mParameterList.at(0);
}

QWidget* KMFilterActionRemoveHeader::createParamWidget( QWidget* parent ) const
{
  KComboBox *cb = new KComboBox( parent );
  cb->setEditable( true );
  cb->setInsertPolicy( QComboBox::InsertAtBottom );
  setParamWidgetValue( cb );
  return cb;
}

KMFilterAction::ReturnCode KMFilterActionRemoveHeader::process( const KMime::Message::Ptr &msg ) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  while ( msg->headerByType( mParameter.toLatin1() ) )
    msg->removeHeader( mParameter.toLatin1() );
  return GoOn;
}

void KMFilterActionRemoveHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  KComboBox * cb = dynamic_cast<KComboBox*>(paramWidget);
  Q_ASSERT( cb );

  int idx = mParameterList.indexOf( mParameter );
  cb->clear();
  cb->addItems( mParameterList );
  if ( idx < 0 ) {
    cb->addItem( mParameter );
    cb->setCurrentIndex( cb->count() - 1 );
  } else {
    cb->setCurrentIndex( idx );
  }
}


//=============================================================================
// KMFilterActionAddHeader - add header
// Add a header with the given value.
//=============================================================================
class KMFilterActionAddHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionAddHeader();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString &argsStr );

  virtual const QString displayString() const;

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionAddHeader);
  }
private:
  QString mValue;
};

KMFilterActionAddHeader::KMFilterActionAddHeader()
  : KMFilterActionWithStringList( "add header", i18n("Add Header") )
{
  mParameterList << ""
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionAddHeader::process( const KMime::Message::Ptr &msg ) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  KMime::Headers::Generic *header = new KMime::Headers::Generic( mParameter.toLatin1(), msg.get(), mValue, "utf-8" );
  msg->setHeader( header );
  return GoOn;
}

QWidget* KMFilterActionAddHeader::createParamWidget( QWidget* parent ) const
{
  QWidget *w = new QWidget( parent );
  QHBoxLayout *hbl = new QHBoxLayout( w );
  hbl->setSpacing( 4 );
  hbl->setMargin( 0 );
  KComboBox *cb = new KComboBox( w );
  cb->setObjectName( "combo" );
  cb->setEditable( true );
  cb->setInsertPolicy( QComboBox::InsertAtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );
  QLabel *l = new QLabel( i18n("With value:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );
  KLineEdit *le = new KLineEdit( w );
  le->setObjectName( "ledit" );
  le->setClearButtonShown( true );
  hbl->addWidget( le, 1 );
  setParamWidgetValue( w );
  return w;
}

void KMFilterActionAddHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.indexOf( mParameter );
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );
  cb->clear();
  cb->addItems( mParameterList );
  if ( idx < 0 ) {
    cb->addItem( mParameter );
    cb->setCurrentIndex( cb->count() - 1 );
  } else {
    cb->setCurrentIndex( idx );
  }
  KLineEdit *le = paramWidget->findChild<KLineEdit*>("ledit");
  Q_ASSERT( le );
  le->setText( mValue );
}

void KMFilterActionAddHeader::applyParamWidgetValue( QWidget* paramWidget )
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  KLineEdit *le = paramWidget->findChild<KLineEdit*>("ledit");
  Q_ASSERT( le );
  mValue = le->text();
}

void KMFilterActionAddHeader::clearParamWidget( QWidget* paramWidget ) const
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );
  cb->setCurrentIndex(0);
  KLineEdit *le = paramWidget->findChild<KLineEdit*>("ledit");
  Q_ASSERT( le );
  le->clear();
}

const QString KMFilterActionAddHeader::argsAsString() const
{
  QString result = mParameter;
  result += '\t';
  result += mValue;

  return result;
}

const QString KMFilterActionAddHeader::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

void KMFilterActionAddHeader::argsFromString( const QString &argsStr )
{
  QStringList l = argsStr.split( '\t' );
  QString s;
  if ( l.count() < 2 ) {
    s = l[0];
    mValue = "";
  } else {
    s = l[0];
    mValue = l[1];
  }

  int idx = mParameterList.indexOf( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionRewriteHeader - rewrite header
// Rewrite a header using a regexp.
//=============================================================================
class KMFilterActionRewriteHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRewriteHeader();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString &argsStr );

  virtual const QString displayString() const;

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionRewriteHeader);
  }
private:
  QRegExp mRegExp;
  QString mReplacementString;
};

KMFilterActionRewriteHeader::KMFilterActionRewriteHeader()
  : KMFilterActionWithStringList( "rewrite header", i18n("Rewrite Header") )
{
  mParameterList << ""
                 << "Subject"
                 << "Reply-To"
                 << "Delivered-To"
                 << "X-KDE-PR-Message"
                 << "X-KDE-PR-Package"
                 << "X-KDE-PR-Keywords";
  mParameter = mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionRewriteHeader::process( const KMime::Message::Ptr &msg ) const
{
  if ( mParameter.isEmpty() || !mRegExp.isValid() )
    return ErrorButGoOn;
  // QString::replace is not const.
  QString value = msg->headerByType( mParameter.toLatin1() ) ? msg->headerByType( mParameter.toLatin1() )->asUnicodeString(): "";

  KMime::Headers::Generic *header = new KMime::Headers::Generic( mParameter.toLatin1(), msg.get(), value.replace( mRegExp, mReplacementString ), "utf-8" );
  msg->setHeader( header );

  return GoOn;
}

QWidget* KMFilterActionRewriteHeader::createParamWidget( QWidget* parent ) const
{
  QWidget *w = new QWidget( parent );
  QHBoxLayout *hbl = new QHBoxLayout( w );
  hbl->setSpacing( 4 );
  hbl->setMargin( 0 );

  KComboBox *cb = new KComboBox( w );
  cb->setEditable( true );
  cb->setObjectName( "combo" );
  cb->setInsertPolicy( QComboBox::InsertAtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );

  QLabel *l = new QLabel( i18n("Replace:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  RegExpLineEdit *rele = new RegExpLineEdit( w );
  rele->setObjectName( "search" );
  hbl->addWidget( rele, 1 );

  l = new QLabel( i18n("With:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  KLineEdit *le = new KLineEdit( w );
  le->setObjectName( "replace" );
  le->setClearButtonShown( true );
  hbl->addWidget( le, 1 );

  setParamWidgetValue( w );
  return w;
}

void KMFilterActionRewriteHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.indexOf( mParameter );
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );

  cb->clear();
  cb->addItems( mParameterList );
  if ( idx < 0 ) {
    cb->addItem( mParameter );
    cb->setCurrentIndex( cb->count() - 1 );
  } else {
    cb->setCurrentIndex( idx );
  }

  RegExpLineEdit *rele = paramWidget->findChild<RegExpLineEdit*>("search");
  Q_ASSERT( rele );
  rele->setText( mRegExp.pattern() );

  KLineEdit *le = paramWidget->findChild<KLineEdit*>("replace");
  Q_ASSERT( le );
  le->setText( mReplacementString );
}

void KMFilterActionRewriteHeader::applyParamWidgetValue( QWidget* paramWidget )
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  RegExpLineEdit *rele = paramWidget->findChild<RegExpLineEdit*>("search");
  Q_ASSERT( rele );
  mRegExp.setPattern( rele->text() );

  KLineEdit *le = paramWidget->findChild<KLineEdit*>("replace");
  Q_ASSERT( le );
  mReplacementString = le->text();
}

void KMFilterActionRewriteHeader::clearParamWidget( QWidget* paramWidget ) const
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>("combo");
  Q_ASSERT( cb );
  cb->setCurrentIndex(0);

  RegExpLineEdit *rele = paramWidget->findChild<RegExpLineEdit*>("search");
  Q_ASSERT( rele );
  rele->clear();

  KLineEdit *le = paramWidget->findChild<KLineEdit*>("replace");
  Q_ASSERT( le );
  le->clear();
}

const QString KMFilterActionRewriteHeader::argsAsString() const
{
  QString result = mParameter;
  result += '\t';
  result += mRegExp.pattern();
  result += '\t';
  result += mReplacementString;

  return result;
}

const QString KMFilterActionRewriteHeader::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

void KMFilterActionRewriteHeader::argsFromString( const QString &argsStr )
{
  QStringList l = argsStr.split( '\t' );
  QString s;

  s = l[0];
  mRegExp.setPattern( l[1] );
  mReplacementString = l[2];

  int idx = mParameterList.indexOf( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionMove - move into folder
// File message into another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterActionWithFolder
{
public:
  KMFilterActionMove();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual bool requiresBody(KMime::Content*) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionMove::newAction(void)
{
  return (new KMFilterActionMove);
}

KMFilterActionMove::KMFilterActionMove()
  : KMFilterActionWithFolder( "transfer", i18n("Move Into Folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionMove::process( const KMime::Message::Ptr &msg ) const
{
  if ( !mFolder.isValid() )
    return ErrorButGoOn;
#if 0
  ActionScheduler *handler = MessageProperty::filterHandler( msg );
  if (handler) {
    MessageProperty::setFilterFolder( msg, mFolder );
  } else {
    // The old filtering system does not support online imap targets.
    // Skip online imap targets when using the old system.
    KMFolder *check;
    check = kmkernel->imapFolderMgr()->findIdString( argsAsString() );
    if (mFolder && (check != mFolder)) {
      MessageProperty::setFilterFolder( msg, mFolder );
    }
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return GoOn;
}

bool KMFilterActionMove::requiresBody(KMime::Content*) const
{
    return false; //iff mFolder->folderMgr == msgBase->parent()->folderMgr;
}


//=============================================================================
// KMFilterActionCopy - copy into folder
// Copy message into another mail folder
//=============================================================================
class KMFilterActionCopy: public KMFilterActionWithFolder
{
public:
  KMFilterActionCopy();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual void processAsync( const KMime::Message::Ptr &msg ) const;
  virtual bool requiresBody( KMime::Content *) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionCopy::newAction( void )
{
  return ( new KMFilterActionCopy );
}

KMFilterActionCopy::KMFilterActionCopy()
  : KMFilterActionWithFolder( "copy", i18n("Copy Into Folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionCopy::process( const KMime::Message::Ptr &msg ) const
{
#if 0 //TODO port to akonadi

  // TODO opening and closing the folder is a trade off.
  // Perhaps Copy is a seldomly used action for now,
  // but I gonna look at improvements ASAP.
  if ( !mFolder.isValid() || mFolder->open( "filtercopy" ) != 0 ) {
    return ErrorButGoOn;
  }

  // copy the message 1:1
  KMime::Message* msgCopy = new KMime::Message;
  msgCopy->setContent( msg->encodedContent() );
  int index;
  int rc = mFolder->addMsg( msgCopy, &index );
  if ( rc == 0 && index != -1 ) {
    mFolder->unGetMsg( index );
  }
  mFolder->close( "filtercopy" );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return GoOn;
}

void KMFilterActionCopy::processAsync( const KMime::Message::Ptr &msg ) const
{
  ActionScheduler *handler = MessageProperty::filterHandler( msg.get() );
#ifdef OLD_COMMAND
  KMCommand *cmd = new KMCopyCommand( mFolder, msg );
  QObject::connect( cmd, SIGNAL( completed( KMCommand * ) ),
                    handler, SLOT( copyMessageFinished( KMCommand * ) ) );
  cmd->start();
#endif
}

bool KMFilterActionCopy::requiresBody( KMime::Content *msg ) const
{
  Q_UNUSED( msg );
  return true;
}

//=============================================================================
// KMFilterActionForward - forward to
// Forward message to another user, with a defined template
//=============================================================================
class KMFilterActionForward: public KMFilterActionWithAddress
{
  public:
    KMFilterActionForward();
    static KMFilterAction* newAction( void );
    virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
    virtual QWidget* createParamWidget( QWidget* parent ) const;
    virtual void applyParamWidgetValue( QWidget* paramWidget );
    virtual void setParamWidgetValue( QWidget* paramWidget ) const;
    virtual void clearParamWidget( QWidget* paramWidget ) const;
    virtual void argsFromString( const QString &argsStr );
    virtual const QString argsAsString() const;
    virtual const QString displayString() const;

private:

  mutable QString mTemplate;
};

KMFilterAction *KMFilterActionForward::newAction( void )
{
  return ( new KMFilterActionForward );
}

KMFilterActionForward::KMFilterActionForward()
  : KMFilterActionWithAddress( "forward", i18n("Forward To") )
{
}

KMFilterAction::ReturnCode KMFilterActionForward::process( const KMime::Message::Ptr &msg ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // avoid endless loops when this action is used in a filter
  // which applies to sent messages
  if ( MessageViewer::StringUtil::addressIsInAddressList( mParameter, QStringList( msg->to()->asUnicodeString() ) ) ) {
    kWarning() << "Attempt to forward to receipient of original message, ignoring.";
    return ErrorButGoOn;
  }
#if 0
  KMime::Message *fwdMsg = KMail::MessageHelper::createForward( msg, mTemplate );
  fwdMsg->to()->fromUnicodeString( fwdMsg->to()->asUnicodeString() + ',' + mParameter, "utf-8" );

  if ( !kmkernel->msgSender()->send( fwdMsg, KMail::MessageSender::SendDefault ) ) {
    kWarning() << "KMFilterAction: could not forward message (sending failed)";
    return ErrorButGoOn; // error: couldn't send
  }
  else
    sendMDN( msg, KMime::MDN::Dispatched );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;

#endif
  // (the msgSender takes ownership of the message, so don't delete it here)
  return GoOn;
}

QWidget* KMFilterActionForward::createParamWidget( QWidget* parent ) const
{
  QWidget *addressAndTemplate = new QWidget( parent );
  QHBoxLayout *hBox = new QHBoxLayout( addressAndTemplate );
  hBox->setMargin( 0 );
  QWidget *addressEdit = KMFilterActionWithAddress::createParamWidget( addressAndTemplate );
  addressEdit->setObjectName( "addressEdit" );
  hBox->addWidget( addressEdit );

  KLineEdit *lineEdit = addressEdit->findChild<KLineEdit*>( "addressEdit" );
  Q_ASSERT( lineEdit );
  lineEdit->setToolTip( i18n( "The addressee to whom the message will be forwarded." ) );
  lineEdit->setWhatsThis( i18n( "The filter will forward the message to the addressee entered here." ) );

  KComboBox *templateCombo = new KComboBox( addressAndTemplate );
  templateCombo->setObjectName( "templateCombo" );
  hBox->addWidget( templateCombo );

  templateCombo->addItem( i18n( "Default Template" ) );
  QStringList templateNames = GlobalSettingsBase::self()->customTemplates();
  foreach( const QString &templateName, templateNames ) {
    CTemplates templat( templateName );
    if ( templat.type() == CustomTemplates::TForward ||
         templat.type() == CustomTemplates::TUniversal )
      templateCombo->addItem( templateName );
  }
  templateCombo->setEnabled( templateCombo->count() > 1 );
  templateCombo->setToolTip( i18n( "The template used when forwarding" ) );
  templateCombo->setWhatsThis( i18n( "Set the forwarding template that will be used with this filter." ) );

  return addressAndTemplate;
}

void KMFilterActionForward::applyParamWidgetValue( QWidget* paramWidget )
{
  QWidget *addressEdit = paramWidget->findChild<QWidget*>( "addressEdit" );
  Q_ASSERT( addressEdit );
  KMFilterActionWithAddress::applyParamWidgetValue( addressEdit );

  KComboBox *templateCombo = paramWidget->findChild<KComboBox*>( "templateCombo" );
  Q_ASSERT( templateCombo );

  if ( templateCombo->currentIndex() == 0 ) {
    // Default template, so don't use a custom one
    mTemplate.clear();
  }
  else {
    mTemplate = templateCombo->currentText();
  }
}

void KMFilterActionForward::setParamWidgetValue( QWidget* paramWidget ) const
{
  QWidget *addressEdit = paramWidget->findChild<QWidget*>( "addressEdit" );
  Q_ASSERT( addressEdit );
  KMFilterActionWithAddress::setParamWidgetValue( addressEdit );

  KComboBox *templateCombo = paramWidget->findChild<KComboBox*>( "templateCombo" );
  Q_ASSERT( templateCombo );

  if ( mTemplate.isEmpty() ) {
    templateCombo->setCurrentIndex( 0 );
  }
  else {
    int templateIndex = templateCombo->findText( mTemplate );
    if ( templateIndex != -1 ) {
      templateCombo->setCurrentIndex( templateIndex );
    }
    else {
      mTemplate.clear();
    }
  }
}

void KMFilterActionForward::clearParamWidget( QWidget* paramWidget ) const
{
  QWidget *addressEdit = paramWidget->findChild<QWidget*>( "addressEdit" );
  Q_ASSERT( addressEdit );
  KMFilterActionWithAddress::clearParamWidget( addressEdit );

  KComboBox *templateCombo = paramWidget->findChild<KComboBox*>( "templateCombo" );
  Q_ASSERT( templateCombo );

  templateCombo->setCurrentIndex( 0 );
}

// We simply place a "@$$@" between the two parameters. The template is the last
// parameter in the string, for compatibility reasons.
static const QString forwardFilterArgsSeperator = "@$$@";

void KMFilterActionForward::argsFromString( const QString &argsStr )
{
  int seperatorPos = argsStr.indexOf( forwardFilterArgsSeperator );

  if ( seperatorPos == - 1 ) {
    // Old config, assume that the whole string is the addressee
    KMFilterActionWithAddress::argsFromString( argsStr );
  }
  else {
    QString addressee = argsStr.left( seperatorPos );
    mTemplate = argsStr.mid( seperatorPos + forwardFilterArgsSeperator.length() );
    KMFilterActionWithAddress::argsFromString( addressee );
  }
}

const QString KMFilterActionForward::argsAsString() const
{
  return KMFilterActionWithAddress::argsAsString() + forwardFilterArgsSeperator + mTemplate;
}

const QString KMFilterActionForward::displayString() const
{
  if ( mTemplate.isEmpty() )
    return i18n( "Forward to %1 with default template", mParameter );
  else
    return i18n( "Forward to %1 with template %2", mParameter, mTemplate );
}

//=============================================================================
// KMFilterActionRedirect - redirect to
// Redirect message to another user
//=============================================================================
class KMFilterActionRedirect: public KMFilterActionWithAddress
{
public:
  KMFilterActionRedirect();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionRedirect::newAction(void)
{
  return (new KMFilterActionRedirect);
}

KMFilterActionRedirect::KMFilterActionRedirect()
  : KMFilterActionWithAddress( "redirect", i18n("Redirect To") )
{
}

KMFilterAction::ReturnCode KMFilterActionRedirect::process( const KMime::Message::Ptr &aMsg ) const
{
  KMime::Message::Ptr msg;
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
#if 0
  msg = KMail::MessageHelper::createRedirect( aMsg, mParameter );

  sendMDN( aMsg, KMime::MDN::Dispatched );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  if ( !kmkernel->msgSender()->send( msg, KMail::MessageSender::SendLater ) ) {
    kDebug() << "KMFilterAction: could not redirect message (sending failed)";
    return ErrorButGoOn; // error: couldn't send
  }
  return GoOn;
}


//=============================================================================
// KMFilterActionExec - execute command
// Execute a shell command
//=============================================================================
class KMFilterActionExec : public KMFilterActionWithCommand
{
public:
  KMFilterActionExec();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExec::newAction(void)
{
  return (new KMFilterActionExec());
}

KMFilterActionExec::KMFilterActionExec()
  : KMFilterActionWithCommand( "execute", i18n("Execute Command") )
{
}

KMFilterAction::ReturnCode KMFilterActionExec::process( const KMime::Message::Ptr &aMsg ) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, false ); // ignore output
}

//=============================================================================
// KMFilterActionExtFilter - use external filter app
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Job.h>
#include <threadweaver/DebuggingAids.h>
class PipeJob : public ThreadWeaver::Job
{
  Q_OBJECT

  public:
    PipeJob(QObject* parent = 0, const KMime::Message::Ptr &aMsg = KMime::Message::Ptr(),
            QString cmd = 0, QString tempFileName = 0 )
      : ThreadWeaver::Job ( parent ),
        mTempFileName( tempFileName ),
        mCmd( cmd ),
        mMsg( aMsg )
    {
      connect( this, SIGNAL( done(  ThreadWeaver::Job* ) ),
               this, SLOT( slotDone( ThreadWeaver::Job* ) ) );
    }

    ~PipeJob() {}

  private slots:

    void slotDone( ThreadWeaver::Job * )
    {
      deleteLater( );
    }

  protected:

    virtual void run()
    {
      ThreadWeaver::debug (1, "PipeJob::run: doing it .\n");
      FILE *p;
      QByteArray ba;

      // backup the serial number in case the header gets lost
      QString origSerNum = mMsg->headerByType( "X-KMail-Filtered" ) ?  mMsg->headerByType( "X-KMail-Filtered" )->asUnicodeString() : "";

      p = popen(QFile::encodeName(mCmd), "r");
      int len =100;
      char buffer[100];
      // append data to ba:
      while (true)  {
        if (! fgets( buffer, len, p ) ) break;
        int oldsize = ba.size();
        ba.resize( oldsize + strlen(buffer) );
        memmove( ba.begin() + oldsize, buffer, strlen(buffer) );
      }
      pclose(p);
      if ( !ba.isEmpty() ) {
#if 0 //TODO port to akonadi
        ThreadWeaver::debug (1, "PipeJob::run: %s", ba.constData() );
        KMFolder *filterFolder =  mMsg->parent();
        ActionScheduler *handler = MessageProperty::filterHandler( mMsg->getMsgSerNum() );

        // If the pipe through alters the message, it could very well
        // happen that it no longer has a X-UID header afterwards. That is
        // unfortunate, as we need to removed the original from the folder
        // using that, and look it up in the message. When the (new) message
        // is uploaded, the header is stripped anyhow. */
        const QString uid = mMsg->headerField( "X-UID", KMime::Message::NoEncoding );
        mMsg->fromString( ba );
        if ( !uid.isEmpty() )
          mMsg->setHeaderField( "X-UID", uid, KMime::Message::Unstructured, false, KMime::Message::NoEncoding );

        if ( !origSerNum.isEmpty() )
          mMsg->setHeaderField( "X-KMail-Filtered", origSerNum );
        if ( filterFolder && handler ) {
          handler->addMsgToIgnored( mMsg->getMsgSerNum() );
          filterFolder->take( filterFolder->find( mMsg ) );
          filterFolder->addMsg( mMsg );
        } else {
          kDebug() << "Warning: Cannot refresh the message from the external filter.";
        }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
      }

      ThreadWeaver::debug (1, "PipeJob::run: done.\n" );
      // unlink the tempFile
      QFile::remove(mTempFileName);
    }

    QString mTempFileName;
    QString mCmd;
    KMime::Message::Ptr mMsg;
};

class KMFilterActionExtFilter: public KMFilterActionWithCommand
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual void processAsync( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExtFilter::newAction(void)
{
  return (new KMFilterActionExtFilter);
}

KMFilterActionExtFilter::KMFilterActionExtFilter()
  : KMFilterActionWithCommand( "filter app", i18n("Pipe Through") )
{
}
KMFilterAction::ReturnCode KMFilterActionExtFilter::process( const KMime::Message::Ptr &aMsg ) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, true ); // use output
}

void KMFilterActionExtFilter::processAsync( const KMime::Message::Ptr &aMsg ) const
{
#if 0 //TODO port to akonadi
  ActionScheduler *handler = MessageProperty::filterHandler( aMsg->getMsgSerNum() );
  KTemporaryFile *inFile = new KTemporaryFile;
  inFile->setAutoRemove(false);
  if ( !inFile->open() ) {
    delete inFile;
    handler->actionMessage( ErrorButGoOn );
    return;
  }

  QList<KTemporaryFile*> atmList;
  atmList.append( inFile );

  QString commandLine = substituteCommandLineArgsFor( aMsg, atmList );
  if ( commandLine.isEmpty() ) {
    handler->actionMessage( ErrorButGoOn );
    qDeleteAll( atmList );
    atmList.clear();
    return;
  }

  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  '(' + commandLine + ") <" + inFile->fileName();

  // write message to file
  QString tempFileName = inFile->fileName();
  if ( !KPIMUtils::kByteArrayToFile( aMsg->asString(), tempFileName, //###
      false, false, false ) ) {
    handler->actionMessage( CriticalError );
    qDeleteAll( atmList );
    atmList.clear();
    return;
  }

  inFile->close();
  qDeleteAll( atmList );
  atmList.clear();

  PipeJob *job = new PipeJob(0, aMsg, commandLine, tempFileName);
  QObject::connect ( job, SIGNAL( done( ThreadWeaver::Job* ) ),
                     handler, SLOT( actionMessage() ) );
  kmkernel->weaver()->enqueue(job);
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//=============================================================================
// KMFilterActionExecSound - execute command
// Execute a sound
//=============================================================================
class KMFilterActionExecSound : public KMFilterActionWithTest
{
public:
  KMFilterActionExecSound();
  ~KMFilterActionExecSound();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  virtual bool requiresBody(KMime::Content*) const;
  static KMFilterAction* newAction(void);
private:
  mutable Phonon::MediaObject* mPlayer;
};

KMFilterActionWithTest::KMFilterActionWithTest( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel )
{
}

KMFilterActionWithTest::~KMFilterActionWithTest()
{
}

QWidget* KMFilterActionWithTest::createParamWidget( QWidget* parent ) const
{
  KMSoundTestWidget *le = new KMSoundTestWidget(parent);
  le->setUrl( mParameter );
  return le;
}


void KMFilterActionWithTest::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((KMSoundTestWidget*)paramWidget)->url();
}

void KMFilterActionWithTest::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((KMSoundTestWidget*)paramWidget)->setUrl( mParameter );
}

void KMFilterActionWithTest::clearParamWidget( QWidget* paramWidget ) const
{
  ((KMSoundTestWidget*)paramWidget)->clear();
}

void KMFilterActionWithTest::argsFromString( const QString &argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithTest::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithTest::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}


KMFilterActionExecSound::KMFilterActionExecSound()
  : KMFilterActionWithTest( "play sound", i18n("Play Sound") ),
    mPlayer(0)
{
}

KMFilterActionExecSound::~KMFilterActionExecSound()
{
  delete mPlayer;
}

KMFilterAction* KMFilterActionExecSound::newAction(void)
{
  return (new KMFilterActionExecSound());
}

KMFilterAction::ReturnCode KMFilterActionExecSound::process( const KMime::Message::Ptr & ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  if ( !mPlayer )
    mPlayer = Phonon::createPlayer(Phonon::NotificationCategory);

  mPlayer->setCurrentSource( mParameter );
  mPlayer->play();
  return GoOn;
}

bool KMFilterActionExecSound::requiresBody(KMime::Content*) const
{
  return false;
}

KMFilterActionWithUrl::KMFilterActionWithUrl( const char* aName, const QString &aLabel )
  : KMFilterAction( aName, aLabel )
{
}

KMFilterActionWithUrl::~KMFilterActionWithUrl()
{
}

QWidget* KMFilterActionWithUrl::createParamWidget( QWidget* parent ) const
{
  KUrlRequester *le = new KUrlRequester(parent);
  le->setUrl( KUrl( mParameter ) );
  return le;
}


void KMFilterActionWithUrl::applyParamWidgetValue( QWidget* paramWidget )
{
    const KUrl url = ((KUrlRequester*)paramWidget)->url();
  mParameter = url.isLocalFile() ? url.toLocalFile() : url.path();
}

void KMFilterActionWithUrl::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((KUrlRequester*)paramWidget)->setUrl( KUrl( mParameter ) );
}

void KMFilterActionWithUrl::clearParamWidget( QWidget* paramWidget ) const
{
  ((KUrlRequester*)paramWidget)->clear();
}

void KMFilterActionWithUrl::argsFromString( const QString &argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithUrl::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithUrl::displayString() const
{
  return label() + " \"" + Qt::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionAddToAddressBook
// - add email address from header to address book
//=============================================================================
class KMFilterActionAddToAddressBook: public KMFilterActionWithStringList
{
public:
  KMFilterActionAddToAddressBook();
  virtual ReturnCode process( const KMime::Message::Ptr &msg ) const;
  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString &argsStr );

private:
  void updateResourceMaps( bool force = true ) const;
  QString mCategory, mResourceName;
  mutable QMap<QString, KABC::Resource*> mResourceByName, mResourceByID;
  const QString mStdResourceStr, mFromStr, mToStr, mCCStr, mBCCStr;
  mutable bool mResourceMapsInitalized;
};

KMFilterAction* KMFilterActionAddToAddressBook::newAction()
{
  return ( new KMFilterActionAddToAddressBook );
}

KMFilterActionAddToAddressBook::KMFilterActionAddToAddressBook()
  : KMFilterActionWithStringList( "add to address book", i18n( "Add to Address Book" ) ),
    mStdResourceStr( i18n( "<placeholder>Default</placeholder>" ) ), mFromStr( i18nc( "Email sender", "From" ) ),
    mToStr( i18nc( "Email recipient", "To" ) ), mCCStr( i18n( "CC" ) ), mBCCStr( i18n( "BCC" ) ),
    mResourceMapsInitalized( false )
{
  mParameterList.append( mFromStr );
  mParameterList.append( mToStr );
  mParameterList.append( mCCStr );
  mParameterList.append( mBCCStr );

  mParameter = mParameterList.at( 0 );

  mResourceName = mStdResourceStr;
  mCategory = i18n( "KMail Filter" );
}

void KMFilterActionAddToAddressBook::updateResourceMaps( bool force ) const
{
  if ( force )
    mResourceMapsInitalized = false;

  if ( mResourceMapsInitalized )
    return;

  //find resources in standard addressbook
  //and prepare lookups
  mResourceByID.clear();
  mResourceByName.clear();

  const QList<KABC::Resource*> list = KABC::StdAddressBook::self()->resources();
  foreach( KABC::Resource* const res, list ) {
    if ( !res->readOnly() && res->isOpen() ) {
      mResourceByID.insert( res->identifier(), res );
      mResourceByName.insert( res->resourceName(), res );
    }
  }

  mResourceMapsInitalized = true;
}

KMFilterAction::ReturnCode KMFilterActionAddToAddressBook::process( const KMime::Message::Ptr &msg ) const
{
  QString headerLine;
  if ( mParameter == mFromStr )
    headerLine = msg->from()->asUnicodeString();
  else if ( mParameter == mToStr )
    headerLine = msg->to()->asUnicodeString();
  else if ( mParameter == mCCStr )
    headerLine = msg->cc()->asUnicodeString();
  else if ( mParameter == mBCCStr )
    headerLine = msg->bcc()->asUnicodeString();

  const QStringList emails = KPIMUtils::splitAddressList( headerLine );

  KABC::AddressBook *ab = KABC::StdAddressBook::self();
  // force a reload of the address book file so that changes that were made
  // by other programs are loaded
  ab->load();

  // We update the resource maps here, and not in the constructor, since that could create D-Bus
  // lockups when using IMAP addressbooks
  updateResourceMaps( false /* don't force reload */ );

  QString email;
  QString name;

  QMap<QString, KABC::Resource*>::const_iterator it = mResourceByName.constFind( mResourceName );
  KABC::Resource* res = 0;
  //if it==end(), then the resouce has been removed from addressbook
  //or default has been selected, in either case store in default resource
  if ( it != mResourceByName.constEnd() )
    res = it.value();

  KABC::Ticket *ticket = ab->requestSaveTicket( res );

  if ( ticket ) {
    foreach ( const QString singleEmail, emails ) {
      KABC::Addressee::parseEmailAddress( singleEmail, name, email );
      KABC::Addressee::List addressees = ab->findByEmail( email );

      if ( addressees.isEmpty() ) {
        KABC::Addressee a;
        a.setNameFromString( name );
        a.insertEmail( email, true );
        if ( !mCategory.isEmpty() )
          a.insertCategory( mCategory );
        if ( res )
          a.setResource( res );

        ab->insertAddressee( a );
      }
    }

    if ( !ab->save( ticket ) ) {
      ab->error( i18n( "Cannot save new addresses to address book." ) );
      return ErrorButGoOn;
    }
  } else {
    ab->error( i18n( "Cannot save to address book: address book is locked." ) );
    return ErrorButGoOn;
  }

  return GoOn;
}

QWidget* KMFilterActionAddToAddressBook::createParamWidget( QWidget* parent ) const
{
  QWidget *w = new QWidget( parent );
  QGridLayout *gridlayout = new QGridLayout ( w );

  KComboBox *cb = new KComboBox( w );
  cb->setObjectName( "FilterTargetCombo" );
  cb->setInsertPolicy( QComboBox::InsertAtBottom );
  gridlayout->addWidget( cb, 0, 0, 2, 1, Qt::AlignVCenter );

  QLabel *l = new QLabel( i18n( "with category" ), w );
  gridlayout->addWidget( l, 0, 1 );

  KLineEdit *le = new KLineEdit( w );
  le->setObjectName( "ledit" );
  gridlayout->addWidget( le, 0, 2 );

  l = new QLabel( i18n( "in address book" ), w );
  gridlayout->addWidget( l, 1, 1 );

  KComboBox *cbAdressBook = new KComboBox( w );
  cbAdressBook->setObjectName( "AddressBookCombo" );
  cbAdressBook->setToolTip( i18n( "<p>This defines the preferred address book.<br />"
        "If it is not accessible, the filter will fallback to the default address book.</p>" ) );
  cbAdressBook->setInsertPolicy( QComboBox::InsertAtBottom );
  gridlayout->addWidget( cbAdressBook, 1, 2 );

  setParamWidgetValue( w );
  return w;
}

void KMFilterActionAddToAddressBook::setParamWidgetValue( QWidget* paramWidget ) const
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>( "FilterTargetCombo" );
  Q_ASSERT( cb );
  cb->clear();
  cb->addItems( mParameterList );
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 0 ) {
    kDebug() << "unknown mParameter. strange???";
    cb->addItem( mParameter );
    cb->setCurrentIndex( cb->count() - 1 );
  } else {
    cb->setCurrentIndex( idx );
  }

  KLineEdit *le = paramWidget->findChild<KLineEdit*>( "ledit" );
  Q_ASSERT( le );
  le->setText( mCategory );

  QStringList list = mResourceByName.keys();
  idx = list.indexOf( mResourceName );
  KComboBox *cbAdressBook = paramWidget->findChild<KComboBox*>( "AddressBookCombo" );
  Q_ASSERT( cbAdressBook );
  cbAdressBook->clear();
  cbAdressBook->addItem( mStdResourceStr );
  cbAdressBook->addItems( mResourceByName.keys() );
  if ( idx < 0 ) {
    cbAdressBook->setCurrentIndex( 0 );
  } else {
    cbAdressBook->setCurrentIndex( idx + 1 /*mStdResourceStr is not in keys() */ );
  }
}

void KMFilterActionAddToAddressBook::applyParamWidgetValue( QWidget* paramWidget )
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>( "FilterTargetCombo" );
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  KLineEdit *le = paramWidget->findChild<KLineEdit*>( "ledit" );
  Q_ASSERT( le );
  mCategory = le->text();

  KComboBox *cbAdressBook = paramWidget->findChild<KComboBox*>( "AddressBookCombo" );
  Q_ASSERT( cbAdressBook );
  mResourceName = cbAdressBook->currentText();
}

void KMFilterActionAddToAddressBook::clearParamWidget( QWidget* paramWidget ) const
{
  KComboBox *cb = paramWidget->findChild<KComboBox*>( "FilterTargetCombo" );
  Q_ASSERT( cb );
  cb->setCurrentItem( 0 );
  KLineEdit *le = paramWidget->findChild<KLineEdit*>( "ledit" );
  Q_ASSERT( le );
  le->setText( mCategory );
  KComboBox *cbAdressBook = paramWidget->findChild<KComboBox*>( "AddressBookCombo" );
  Q_ASSERT( cbAdressBook );
  cbAdressBook->setCurrentItem( 0 );
}

const QString KMFilterActionAddToAddressBook::argsAsString() const
{
  QString result;

  if ( mParameter == mFromStr )
    result = "From";
  else if ( mParameter == mToStr )
    result = "To";
  else if ( mParameter == mCCStr )
    result = "CC";
  else if ( mParameter == mBCCStr )
    result = "BCC";

  result += '\t';

  QMap<QString, KABC::Resource*>::const_iterator it =  mResourceByName.constFind( mResourceName );
  if ( it != mResourceByName.constEnd() )
    result += it.value()->identifier();
  else
    result += mStdResourceStr;

  result += '\t';

  result += mCategory;

  return result;
}

void KMFilterActionAddToAddressBook::argsFromString( const QString &argsStr )
{
  updateResourceMaps();

  QStringList l = argsStr.split( '\t', QString::KeepEmptyParts );
  mParameter = l[0];

  if ( l.count() >= 2 ) {
    QMap<QString, KABC::Resource*>::iterator it  = mResourceByID.find( l[1] );
    if ( it != mResourceByID.end() )
      mResourceName = it.value()->resourceName();
    else
      mResourceName = mStdResourceStr;
  } else {
    mResourceName = mStdResourceStr;
  }

  if ( l.count() < 3 )
    mCategory.clear();
  else
    mCategory = l[2];
}

//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
KMFilterActionDict::~KMFilterActionDict()
{
  qDeleteAll( mList );
}

void KMFilterActionDict::init(void)
{
  insert( KMFilterActionMove::newAction );
  insert( KMFilterActionCopy::newAction );
  insert( KMFilterActionIdentity::newAction );
  insert( KMFilterActionSetStatus::newAction );
  insert( KMFilterActionAddTag::newAction );
  insert( KMFilterActionFakeDisposition::newAction );
  insert( KMFilterActionTransport::newAction );
  insert( KMFilterActionReplyTo::newAction );
  insert( KMFilterActionForward::newAction );
  insert( KMFilterActionRedirect::newAction );
  insert( KMFilterActionSendReceipt::newAction );
  insert( KMFilterActionExec::newAction );
  insert( KMFilterActionExtFilter::newAction );
  insert( KMFilterActionRemoveHeader::newAction );
  insert( KMFilterActionAddHeader::newAction );
  insert( KMFilterActionRewriteHeader::newAction );
  insert( KMFilterActionExecSound::newAction );
  insert( KMFilterActionAddToAddressBook::newAction );
  // Register custom filter actions below this line.
}
// The int in the QDict constructor (41) must be a prime
// and should be greater than the double number of KMFilterAction types
KMFilterActionDict::KMFilterActionDict()
  : QMultiHash<QString, KMFilterActionDesc*>()
{
  init();
}

void KMFilterActionDict::insert( KMFilterActionNewFunc aNewFunc )
{
  KMFilterAction *action = aNewFunc();
  KMFilterActionDesc* desc = new KMFilterActionDesc;
  desc->name = action->name();
  desc->label = action->label();
  desc->create = aNewFunc;
  QMultiHash<QString, KMFilterActionDesc*>::insert( desc->name, desc );
  QMultiHash<QString, KMFilterActionDesc*>::insert( desc->label, desc );
  mList.append( desc );
  delete action;
}

#include "kmfilteraction.moc"
