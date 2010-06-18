// kmfilteraction.cpp

#include "kmfilteraction.h"

// other KMail headers:
#include "kmcommands.h"
#include "kmfiltermgr.h"
#include "kmmainwidget.h"
#include "folderrequester.h"
#include "kmsoundtestwidget.h"
#include "util.h"

using KMail::FolderRequester;
#include "messageproperty.h"
using KMail::MessageProperty;
#include "regexplineedit.h"
using KMail::RegExpLineEdit;
#include "stringutil.h"

// KD PIM headers
#include "messagecore/stringutil.h"
#include "messagecore/messagehelpers.h"

#include "messagecore/kmfawidgets.h"

#include "messagecomposer/messagesender.h"
#include "messagecomposer/messagefactory.h"
using MessageComposer::MessageFactory;
#include "messagecomposer/messageinfo.h"

#include "templateparser/customtemplates.h"
#include "templateparser/customtemplates_kfg.h"

#include "libkdepim/addcontactjob.h"

// KDE PIM libs headers
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimutils/kfileio.h>
#include <kpimutils/email.h>
#include <akonadi/itemcopyjob.h>
#include <akonadi/itemmodifyjob.h>
#include <kmime/kmime_message.h>

// KDE headers
#include <akonadi/collectioncombobox.h>
#include <kabc/addressee.h>
#include <kcombobox.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <phonon/mediaobject.h>
#include <kshell.h>
#include <kprocess.h>
#include <nepomuk/tag.h>

// Qt headers:

// other headers:
#include <assert.h>
#include <string.h>
#include "mdnadvicedialog.h"
#include "mdnadvicedialog.h"

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

bool KMFilterAction::requiresBody() const
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
  KMime::MDN::SendingMode s = MDNAdviceDialog::checkMDNHeaders( msg );
  KConfigGroup mdnConfig( KMKernel::config(), "MDN" );
  int quote = mdnConfig.readEntry<int>( "quote-message", 0 );
  MessageFactory factory( msg, Akonadi::Item().id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  KMime::Message::Ptr mdn = factory.createMDN( KMime::MDN::AutomaticAction, d, s, quote, m );
  if ( mdn && !kmkernel->msgSender()->send( mdn, MessageSender::SendLater ) ) {
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
  mFolder = kmkernel->collectionFromId( argsStr );
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

static KMime::Content* findMimeNodeForIndex( KMime::Content* node, int &index )
{
  if ( index <= 0 )
    return node;
  foreach ( KMime::Content* child, node->contents() ) {
    KMime::Content *result = findMimeNodeForIndex( child, --index );
    if ( result )
      return result;
  }
  return 0;
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
        int index = *it; // we pass by reference below, so this is not const
        KMime::Content *content = findMimeNodeForIndex( aMsg.get(), index );
        if ( content ) {
          KPIMUtils::kByteArrayToFile( content->decodedContent(), tempFileName,
                            false, false, false );
        }
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


KMFilterAction::ReturnCode KMFilterActionWithCommand::genericProcess( const Akonadi::Item &item, bool withOutput ) const
{
  const KMime::Message::Ptr aMsg = item.payload<KMime::Message::Ptr>();
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionSendReceipt::process( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  MessageFactory factory( msg, item.id() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  const KMime::Message::Ptr receipt = factory.createDeliveryReceipt();
  if ( !receipt ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the receipt before sending and b) for speed reasons.
  kmkernel->msgSender()->send( receipt, MessageSender::SendLater );

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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionTransport::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", msg.get(), mParameter, "utf-8");
  msg->setHeader( header );
  msg->assemble();

  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionReplyTo::process( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "Reply-To", msg.get(), mParameter, "utf-8");
  msg->setHeader( header );
  msg->assemble();

  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionIdentity::process( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg.get(), QString::number(mParameter), "utf-8");
  msg->setHeader( header );
  msg->assemble();

  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  virtual bool requiresBody() const;

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

KMFilterAction::ReturnCode KMFilterActionSetStatus::process( const Akonadi::Item &item ) const
{
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  KPIM::MessageStatus status;
  status.set( stati[ idx ] );
  Akonadi::Item i( item );
  i.setFlags( status.getStatusFlags() );
  new Akonadi::ItemModifyJob( i, kmkernel->filterMgr() ); // TODO handle error
  return GoOn;
}

bool KMFilterActionSetStatus::requiresBody() const
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  virtual bool requiresBody() const;

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
  foreach( const Nepomuk::Tag &tag, Nepomuk::Tag::allTags() ) {
    mParameterList.append( tag.label() );
    mLabelList.append( tag.resourceUri().toString() );
  }
}

KMFilterAction::ReturnCode KMFilterActionAddTag::process( const Akonadi::Item &item ) const
{
  const int idx = mParameterList.indexOf( mParameter );
  if ( idx == -1 ) return ErrorButGoOn;

  Nepomuk::Resource n_resource( item.url() );
  n_resource.addTag( mParameter );
  return GoOn;
}

bool KMFilterActionAddTag::requiresBody() const
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
  if ( mParameterList.size() > 0 )
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionFakeDisposition::process( const Akonadi::Item &item ) const
{
  int idx = mParameterList.indexOf( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  if ( idx == 1 ) // ignore
    MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionRemoveHeader::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  while ( msg->headerByType( mParameter.toLatin1() ) )
    msg->removeHeader( mParameter.toLatin1() );
  msg->assemble();
  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionAddHeader::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  KMime::Headers::Generic *header = new KMime::Headers::Generic( mParameter.toLatin1(), msg.get(), mValue, "utf-8" );
  msg->setHeader( header );
  msg->assemble();
  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionRewriteHeader::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() || !mRegExp.isValid() )
    return ErrorButGoOn;

  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  // QString::replace is not const.
  QString value = msg->headerByType( mParameter.toLatin1() ) ? msg->headerByType( mParameter.toLatin1() )->asUnicodeString(): "";

  KMime::Headers::Generic *header = new KMime::Headers::Generic( mParameter.toLatin1(), msg.get(), value.replace( mRegExp, mReplacementString ), "utf-8" );
  msg->setHeader( header );
  msg->assemble();

  new Akonadi::ItemModifyJob( item, kmkernel->filterMgr() );
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  virtual bool requiresBody() const;
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

KMFilterAction::ReturnCode KMFilterActionMove::process( const Akonadi::Item &item ) const
{
  if ( !mFolder.isValid() )
    return ErrorButGoOn;
  MessageProperty::setFilterFolder( item, mFolder );
  return GoOn;
}

bool KMFilterActionMove::requiresBody() const
{
    return false;
}


//=============================================================================
// KMFilterActionCopy - copy into folder
// Copy message into another mail folder
//=============================================================================
class KMFilterActionCopy: public KMFilterActionWithFolder
{
public:
  KMFilterActionCopy();
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  virtual bool requiresBody() const;
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

KMFilterAction::ReturnCode KMFilterActionCopy::process( const Akonadi::Item &item ) const
{
  // copy the message 1:1
  new Akonadi::ItemCopyJob( item, mFolder, kmkernel->filterMgr() ); // TODO handle error
  return GoOn;
}

bool KMFilterActionCopy::requiresBody() const
{
  return false;
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
    virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionForward::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  // avoid endless loops when this action is used in a filter
  // which applies to sent messages
  if ( MessageCore::StringUtil::addressIsInAddressList( mParameter,
                                                   QStringList( msg->to()->asUnicodeString() ) ) ) {
    kWarning() << "Attempt to forward to receipient of original message, ignoring.";
    return ErrorButGoOn;
  }
  MessageFactory factory( msg, item.id() );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setTemplate( mTemplate );
  KMime::Message::Ptr fwdMsg = factory.createForward();
  fwdMsg->to()->fromUnicodeString( fwdMsg->to()->asUnicodeString() + ',' + mParameter, "utf-8" );
  if ( !kmkernel->msgSender()->send( fwdMsg, MessageSender::SendDefault ) ) {
    kWarning() << "KMFilterAction: could not forward message (sending failed)";
    return ErrorButGoOn; // error: couldn't send
  }
  else
    sendMDN( msg, KMime::MDN::Dispatched );

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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionRedirect::process( const Akonadi::Item &item ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  KMime::Message::Ptr msg = MessageCore::Util::message( item );
  MessageFactory factory( msg, item.id() );
  factory.setFolderIdentity( KMail::Util::folderIdentity( item ) );
  factory.setIdentityManager( KMKernel::self()->identityManager() );
  KMime::Message::Ptr rmsg = factory.createRedirect( mParameter );
  if ( !rmsg )
    return ErrorButGoOn;

  sendMDN( msg, KMime::MDN::Dispatched );

  if ( !kmkernel->msgSender()->send( rmsg, MessageSender::SendLater ) ) {
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
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

KMFilterAction::ReturnCode KMFilterActionExec::process( const Akonadi::Item &item ) const
{
  return KMFilterActionWithCommand::genericProcess( item, false ); // ignore output
}

//=============================================================================
// KMFilterActionExtFilter - use external filter app
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================

class KMFilterActionExtFilter: public KMFilterActionWithCommand
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process( const Akonadi::Item &item2 ) const;
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
KMFilterAction::ReturnCode KMFilterActionExtFilter::process( const Akonadi::Item &item ) const
{
  return KMFilterActionWithCommand::genericProcess( item, true ); // use output
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  virtual bool requiresBody() const;
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

KMFilterAction::ReturnCode KMFilterActionExecSound::process( const Akonadi::Item& ) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  if ( !mPlayer )
    mPlayer = Phonon::createPlayer(Phonon::NotificationCategory);

  mPlayer->setCurrentSource( mParameter );
  mPlayer->play();
  return GoOn;
}

bool KMFilterActionExecSound::requiresBody() const
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
  virtual ReturnCode process( const Akonadi::Item &item ) const;
  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString &argsStr );

private:
  enum HeaderType
  {
    FromHeader,
    ToHeader,
    CcHeader,
    BccHeader
  };

  const QString mFromStr, mToStr, mCCStr, mBCCStr;
  HeaderType mHeaderType;
  Akonadi::Collection::Id mCollectionId;
  QString mCategory;
};

KMFilterAction* KMFilterActionAddToAddressBook::newAction()
{
  return ( new KMFilterActionAddToAddressBook );
}

KMFilterActionAddToAddressBook::KMFilterActionAddToAddressBook()
  : KMFilterActionWithStringList( "add to address book", i18n( "Add to Address Book" ) ),
    mFromStr( i18nc( "Email sender", "From" ) ),
    mToStr( i18nc( "Email recipient", "To" ) ),
    mCCStr( i18n( "CC" ) ),
    mBCCStr( i18n( "BCC" ) ),
    mHeaderType( FromHeader ),
    mCollectionId( -1 ),
    mCategory( i18n( "KMail Filter" ) )
{
}

KMFilterAction::ReturnCode KMFilterActionAddToAddressBook::process( const Akonadi::Item &item ) const
{
  const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  QString headerLine;
  switch ( mHeaderType ) {
    case FromHeader: headerLine = msg->from()->asUnicodeString(); break;
    case ToHeader: headerLine = msg->to()->asUnicodeString(); break;
    case CcHeader: headerLine = msg->cc()->asUnicodeString(); break;
    case BccHeader: headerLine = msg->bcc()->asUnicodeString(); break;
  }

  const QStringList emails = KPIMUtils::splitAddressList( headerLine );

  foreach ( const QString singleEmail, emails ) {
    QString name, email;
    KABC::Addressee::parseEmailAddress( singleEmail, name, email );

    KABC::Addressee contact;
    contact.setNameFromString( name );
    contact.insertEmail( email, true );
    if ( !mCategory.isEmpty() )
      contact.insertCategory( mCategory );

    KPIM::AddContactJob *job = new KPIM::AddContactJob( contact, Akonadi::Collection( mCollectionId ) );
    job->start();
  }

  return GoOn;
}

QWidget* KMFilterActionAddToAddressBook::createParamWidget( QWidget* parent ) const
{
  QWidget *widget = new QWidget( parent );
  QGridLayout *layout = new QGridLayout ( widget );

  KComboBox *headerCombo = new KComboBox( widget );
  headerCombo->setObjectName( "HeaderComboBox" );
  layout->addWidget( headerCombo, 0, 0, 2, 1, Qt::AlignVCenter );

  QLabel *label = new QLabel( i18n( "with category" ), widget );
  layout->addWidget( label, 0, 1 );

  KLineEdit *categoryEdit = new KLineEdit( widget );
  categoryEdit->setObjectName( "CategoryEdit" );
  layout->addWidget( categoryEdit, 0, 2 );

  label = new QLabel( i18n( "in address book" ), widget );
  layout->addWidget( label, 1, 1 );

  Akonadi::CollectionComboBox *collectionComboBox = new Akonadi::CollectionComboBox( widget );
  collectionComboBox->setMimeTypeFilter( QStringList() << KABC::Addressee::mimeType() );
  collectionComboBox->setAccessRightsFilter( Akonadi::Collection::CanCreateItem );

  collectionComboBox->setObjectName( "AddressBookComboBox" );
  collectionComboBox->setToolTip( i18n( "<p>This defines the preferred address book.<br />"
        "If it is not accessible, the filter will fallback to the default address book.</p>" ) );
  layout->addWidget( collectionComboBox, 1, 2 );

  setParamWidgetValue( widget );

  return widget;
}

void KMFilterActionAddToAddressBook::setParamWidgetValue( QWidget* paramWidget ) const
{
  KComboBox *headerCombo = paramWidget->findChild<KComboBox*>( "HeaderComboBox" );
  Q_ASSERT( headerCombo );
  headerCombo->clear();
  headerCombo->addItem( mFromStr, FromHeader );
  headerCombo->addItem( mToStr, ToHeader );
  headerCombo->addItem( mCCStr, CcHeader );
  headerCombo->addItem( mBCCStr, BccHeader );

  headerCombo->setCurrentIndex( headerCombo->findData( mHeaderType ) );

  KLineEdit *categoryEdit = paramWidget->findChild<KLineEdit*>( "CategoryEdit" );
  Q_ASSERT( categoryEdit );
  categoryEdit->setText( mCategory );

  Akonadi::CollectionComboBox *collectionComboBox = paramWidget->findChild<Akonadi::CollectionComboBox*>( "AddressBookComboBox" );
  Q_ASSERT( collectionComboBox );
  collectionComboBox->setDefaultCollection( Akonadi::Collection( mCollectionId ) );
  collectionComboBox->setProperty( "collectionId", mCollectionId );
}

void KMFilterActionAddToAddressBook::applyParamWidgetValue( QWidget* paramWidget )
{
  KComboBox *headerCombo = paramWidget->findChild<KComboBox*>( "HeaderComboBox" );
  Q_ASSERT( headerCombo );
  mHeaderType = static_cast<HeaderType>( headerCombo->itemData( headerCombo->currentIndex() ).toInt() );

  KLineEdit *categoryEdit = paramWidget->findChild<KLineEdit*>( "CategoryEdit" );
  Q_ASSERT( categoryEdit );
  mCategory = categoryEdit->text();

  Akonadi::CollectionComboBox *collectionComboBox = paramWidget->findChild<Akonadi::CollectionComboBox*>( "AddressBookComboBox" );
  Q_ASSERT( collectionComboBox );
  const Akonadi::Collection collection = collectionComboBox->currentCollection();

  // it might be that the model of collectionComboBox has not finished loading yet, so
  // we use the previously 'stored' value from the 'collectionId' property
  if ( collection.isValid() )
    mCollectionId = collection.id();
  else {
    const QVariant value = collectionComboBox->property( "collectionId" );
    if ( value.isValid() )
      mCollectionId = value.toLongLong();
  }
}

void KMFilterActionAddToAddressBook::clearParamWidget( QWidget* paramWidget ) const
{
  KComboBox *headerCombo = paramWidget->findChild<KComboBox*>( "HeaderComboBox" );
  Q_ASSERT( headerCombo );
  headerCombo->setCurrentItem( 0 );

  KLineEdit *categoryEdit = paramWidget->findChild<KLineEdit*>( "CategoryEdit" );
  Q_ASSERT( categoryEdit );
  categoryEdit->setText( mCategory );
}

const QString KMFilterActionAddToAddressBook::argsAsString() const
{
  QString result;

  switch ( mHeaderType ) {
    case FromHeader: result = QLatin1String( "From" ); break;
    case ToHeader: result = QLatin1String( "To" ); break;
    case CcHeader: result = QLatin1String( "CC" ); break;
    case BccHeader: result = QLatin1String( "BCC" ); break;
  }

  result += QLatin1Char( '\t' );
  result += QString::number( mCollectionId );
  result += QLatin1Char( '\t' );
  result += mCategory;

  return result;
}

void KMFilterActionAddToAddressBook::argsFromString( const QString &argsStr )
{
  const QStringList parts = argsStr.split( QLatin1Char( '\t' ), QString::KeepEmptyParts );
  if ( parts[ 0 ] == QLatin1String( "From" ) )
    mHeaderType = FromHeader;
  else if ( parts[ 0 ] == QLatin1String( "To" ) )
    mHeaderType = ToHeader;
  else if ( parts[ 0 ] == QLatin1String( "CC" ) )
    mHeaderType = CcHeader;
  else if ( parts[ 0 ] == QLatin1String( "BCC" ) )
    mHeaderType = BccHeader;

  if ( parts.count() >= 2 )
    mCollectionId = parts[ 1 ].toLongLong();

  if ( parts.count() < 3 )
    mCategory.clear();
  else
    mCategory = parts[ 2 ];
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

