// kmfilteraction.cpp
// The process methods really should use an enum instead of an int
// -1 -> status unchanged, 0 -> success, 1 -> failure, 2-> critical failure
// (GoOn),                 (Ok),         (ErrorButGoOn), (CriticalError)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilteraction.h"

#include "kmcommands.h"
#include "kmmsgpart.h"
#include "kmfiltermgr.h"
#include "kmfolderindex.h"
#include "kmfoldermgr.h"
#include "kmsender.h"
#include "kmmainwidget.h"
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identitycombo.h>
#include <libkdepim/kfileio.h>
#include <libkdepim/collectingprocess.h>
using KPIM::CollectingProcess;
#include "kmfawidgets.h"
#include "folderrequester.h"
using KMail::FolderRequester;
#include "kmmsgbase.h"
#include "messageproperty.h"
#include "actionscheduler.h"
using KMail::MessageProperty;
using KMail::ActionScheduler;
#include <kregexp3.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <kaudioplayer.h>
#include <kurlrequester.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qtextcodec.h>
#include <qtimer.h>
#include <qobject.h>
#include <qstylesheet.h>
#include <assert.h>


//=============================================================================
//
// KMFilterAction
//
//=============================================================================

KMFilterAction::KMFilterAction( const char* aName, const QString aLabel )
{
  mName = aName;
  mLabel = aLabel;
}

KMFilterAction::~KMFilterAction()
{
}

void KMFilterAction::processAsync(KMMessage* msg) const
{
  ActionScheduler *handler = MessageProperty::filterHandler( msg );
  ReturnCode result = process( msg );
  if (handler)
    handler->actionMessage( result );
}

bool KMFilterAction::requiresBody(KMMsgBase*) const
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

bool KMFilterAction::folderRemoved(KMFolder*, KMFolder*)
{
  return FALSE;
}

int KMFilterAction::tempOpenFolder(KMFolder* aFolder)
{
  return kmkernel->filterMgr()->tempOpenFolder(aFolder);
}

void KMFilterAction::sendMDN( KMMessage * msg, KMime::MDN::DispositionType d,
                              const QValueList<KMime::MDN::DispositionModifier> & m ) {
  if ( !msg ) return;
  KMMessage * mdn = msg->createMDN( KMime::MDN::AutomaticAction, d, false, m );
  if ( mdn && !kmkernel->msgSender()->send( mdn, FALSE ) ) {
    kdDebug(5006) << "KMFilterAction::sendMDN(): sending failed." << endl;
    //delete mdn;
  }
}


//=============================================================================
//
// KMFilterActionWithNone
//
//=============================================================================

KMFilterActionWithNone::KMFilterActionWithNone( const char* aName, const QString aLabel )
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

KMFilterActionWithUOID::KMFilterActionWithUOID( const char* aName, const QString aLabel )
  : KMFilterAction( aName, aLabel ), mParameter( 0 )
{
}

void KMFilterActionWithUOID::argsFromString( const QString argsStr )
{
  mParameter = argsStr.stripWhiteSpace().toUInt();
}

const QString KMFilterActionWithUOID::argsAsString() const
{
  return QString::number( mParameter );
}

const QString KMFilterActionWithUOID::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}


//=============================================================================
//
// KMFilterActionWithString
//
//=============================================================================

KMFilterActionWithString::KMFilterActionWithString( const char* aName, const QString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

QWidget* KMFilterActionWithString::createParamWidget( QWidget* parent ) const
{
  QLineEdit *le = new KLineEdit(parent);
  le->setText( mParameter );
  return le;
}

void KMFilterActionWithString::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((QLineEdit*)paramWidget)->text();
}

void KMFilterActionWithString::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((QLineEdit*)paramWidget)->setText( mParameter );
}

void KMFilterActionWithString::clearParamWidget( QWidget* paramWidget ) const
{
  ((QLineEdit*)paramWidget)->clear();
}

void KMFilterActionWithString::argsFromString( const QString argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithString::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithString::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
//
// class KMFilterActionWithStringList
//
//=============================================================================

KMFilterActionWithStringList::KMFilterActionWithStringList( const char* aName, const QString aLabel )
  : KMFilterActionWithString( aName, aLabel )
{
}

QWidget* KMFilterActionWithStringList::createParamWidget( QWidget* parent ) const
{
  QComboBox *cb = new QComboBox( FALSE, parent );
  cb->insertStringList( mParameterList );
  setParamWidgetValue( cb );
  return cb;
}

void KMFilterActionWithStringList::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((QComboBox*)paramWidget)->currentText();
}

void KMFilterActionWithStringList::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  ((QComboBox*)paramWidget)->setCurrentItem( idx >= 0 ? idx : 0 );
}

void KMFilterActionWithStringList::clearParamWidget( QWidget* paramWidget ) const
{
  ((QComboBox*)paramWidget)->setCurrentItem(0);
}

void KMFilterActionWithStringList::argsFromString( const QString argsStr )
{
  int idx = mParameterList.findIndex( argsStr );
  if ( idx < 0 ) {
    mParameterList.append( argsStr );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
//
// class KMFilterActionWithFolder
//
//=============================================================================

KMFilterActionWithFolder::KMFilterActionWithFolder( const char* aName, const QString aLabel )
  : KMFilterAction( aName, aLabel )
{
  mFolder = 0;
}

QWidget* KMFilterActionWithFolder::createParamWidget( QWidget* parent ) const
{
  FolderRequester *req = new FolderRequester( parent,
      kmkernel->getKMMainWidget()->folderTree() );
  req->setShowImapFolders( false );
  setParamWidgetValue( req );
  return req;
}

void KMFilterActionWithFolder::applyParamWidgetValue( QWidget* paramWidget )
{
  mFolder = ((FolderRequester *)paramWidget)->folder();
  if (mFolder)
  {
     mFolderName = QString::null;
  }
  else
  {
     mFolderName = ((FolderRequester *)paramWidget)->text();
  }
}

void KMFilterActionWithFolder::setParamWidgetValue( QWidget* paramWidget ) const
{
  if ( mFolder )
    ((FolderRequester *)paramWidget)->setFolder( mFolder );
  else
    ((FolderRequester *)paramWidget)->setFolder( mFolderName );
}

void KMFilterActionWithFolder::clearParamWidget( QWidget* paramWidget ) const
{
  ((FolderRequester *)paramWidget)->setFolder( kmkernel->draftsFolder() );
}

void KMFilterActionWithFolder::argsFromString( const QString argsStr )
{
  mFolder = kmkernel->folderMgr()->findIdString( argsStr );
  if (!mFolder)
     mFolder = kmkernel->dimapFolderMgr()->findIdString( argsStr );
  if (mFolder)
     mFolderName = QString::null;
  else
     mFolderName = argsStr;
}

const QString KMFilterActionWithFolder::argsAsString() const
{
  QString result;
  if ( mFolder )
    result = mFolder->idString();
  else
    result = mFolderName;
  return result;
}

const QString KMFilterActionWithFolder::displayString() const
{
  QString result;
  if ( mFolder )
    result = mFolder->prettyURL();
  else
    result = mFolderName;
  return label() + " \"" + QStyleSheet::escape( result ) + "\"";
}

bool KMFilterActionWithFolder::folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder )
{
  if ( aFolder == mFolder ) {
    mFolder = aNewFolder;
    if ( aNewFolder )
      mFolderName = QString::null;
    else
      mFolderName = i18n( "<select a folder>" );
    return TRUE;
  } else
    return FALSE;
}

//=============================================================================
//
// class KMFilterActionWithAddress
//
//=============================================================================

KMFilterActionWithAddress::KMFilterActionWithAddress( const char* aName, const QString aLabel )
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

KMFilterActionWithCommand::KMFilterActionWithCommand( const char* aName, const QString aLabel )
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

QString KMFilterActionWithCommand::substituteCommandLineArgsFor( KMMessage *aMsg, QPtrList<KTempFile> & aTempFileList ) const
{
  QString result = mParameter;
  QValueList<int> argList;
  QRegExp r( "%[0-9-]+" );

  // search for '%n'
  int start = -1;
  while ( ( start = r.search( result, start + 1 ) ) > 0 ) {
    int len = r.matchedLength();
    // and save the encountered 'n' in a list.
    bool OK = false;
    int n = result.mid( start + 1, len - 1 ).toInt( &OK );
    if ( OK )
      argList.append( n );
  }

  // sort the list of n's
  qHeapSort( argList );

  // and use QString::arg to substitute filenames for the %n's.
  int lastSeen = -2;
  QString tempFileName;
  for ( QValueList<int>::Iterator it = argList.begin() ; it != argList.end() ; ++it ) {
    // setup temp files with check for duplicate %n's
    if ( (*it) != lastSeen ) {
      KTempFile *tf = new KTempFile();
      if ( tf->status() != 0 ) {
        tf->close();
        delete tf;
        kdDebug(5006) << "KMFilterActionWithCommand: Could not create temp file!" << endl;
        return QString::null;
      }
      tf->setAutoDelete(TRUE);
      aTempFileList.append( tf );
      tempFileName = tf->name();
      if ((*it) == -1)
        KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
                          false, false, false );
      else if (aMsg->numBodyParts() == 0)
        KPIM::kByteArrayToFile( aMsg->bodyDecodedBinary(), tempFileName,
                          false, false, false );
      else {
        KMMessagePart msgPart;
        aMsg->bodyPart( (*it), &msgPart );
        KPIM::kByteArrayToFile( msgPart.bodyDecodedBinary(), tempFileName,
                          false, false, false );
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
  QRegExp header_rx( "%\\{([a-z0-9-]+)\\}", false );
  int idx = 0;
  while ( ( idx = header_rx.search( result, idx ) ) != -1 ) {
    QString replacement = KProcess::quote( aMsg->headerField( header_rx.cap(1).latin1() ) );
    result.replace( idx, header_rx.matchedLength(), replacement );
    idx += replacement.length();
  }

  return result;
}


KMFilterAction::ReturnCode KMFilterActionWithCommand::genericProcess(KMMessage* aMsg, bool withOutput) const
{
  Q_ASSERT( aMsg );

  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // KProcess doesn't support a QProcess::launch() equivalent, so
  // we must use a temp file :-(
  KTempFile * inFile = new KTempFile;
  inFile->setAutoDelete(TRUE);

  QPtrList<KTempFile> atmList;
  atmList.setAutoDelete(TRUE);
  atmList.append( inFile );

  QString commandLine = substituteCommandLineArgsFor( aMsg , atmList );
  if ( commandLine.isEmpty() )
    return ErrorButGoOn;

  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  "(" + commandLine + ") <" + inFile->name();

  // write message to file
  QString tempFileName = inFile->name();
  KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
                  false, false, false );
  inFile->close();

  CollectingProcess shProc;
  shProc.setUseShell(true);
  shProc << commandLine;

  // run process:
  if ( !shProc.start( KProcess::Block,
                      withOutput ? KProcess::Stdout
                                 : KProcess::NoCommunication ) )
    return ErrorButGoOn;

  if ( !shProc.normalExit() || shProc.exitStatus() != 0 ) {
    return ErrorButGoOn;
  }

  if ( withOutput ) {
    // read altered message:
    QByteArray msgText = shProc.collectedStdout();

    if ( !msgText.isEmpty() ) {
    /* If the pipe through alters the message, it could very well
       happen that it no longer has a X-UID header afterwards. That is
       unfortunate, as we need to removed the original from the folder
       using that, and look it up in the message. When the (new) message
       is uploaded, the header is stripped anyhow. */
      QString uid = aMsg->headerField("X-UID");
      aMsg->fromByteArray( msgText );
      aMsg->setHeaderField("X-UID",uid);
    }
    else
      return ErrorButGoOn;
  }
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
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionSendReceipt::process(KMMessage* msg) const
{
  KMMessage *receipt = msg->createDeliveryReceipt();
  if ( !receipt ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the receipt before sending and b) for speed reasons.
  kmkernel->msgSender()->send( receipt, FALSE );

  return GoOn;
}



//=============================================================================
// KMFilterActionSetTransport - set transport to...
// Specify mail transport (smtp server) to be used when replying to a message
//=============================================================================
class KMFilterActionTransport: public KMFilterActionWithString
{
public:
  KMFilterActionTransport();
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionTransport::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  msg->setHeaderField( "X-KMail-Transport", mParameter );
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
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionReplyTo::process(KMMessage* msg) const
{
  msg->setHeaderField( "Reply-To", mParameter );
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
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionIdentity::process(KMMessage* msg) const
{
  msg->setHeaderField( "X-KMail-Identity", QString::number( mParameter ) );
  return GoOn;
}

QWidget * KMFilterActionIdentity::createParamWidget( QWidget * parent ) const
{
  KPIM::IdentityCombo * ic = new KPIM::IdentityCombo( kmkernel->identityManager(), parent );
  ic->setCurrentIdentity( mParameter );
  return ic;
}

void KMFilterActionIdentity::applyParamWidgetValue( QWidget * paramWidget )
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
  assert( ic );
  mParameter = ic->currentIdentity();
}

void KMFilterActionIdentity::clearParamWidget( QWidget * paramWidget ) const
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentItem( 0 );
  //ic->setCurrentIdentity( kmkernel->identityManager()->defaultIdentity() );
}

void KMFilterActionIdentity::setParamWidgetValue( QWidget * paramWidget ) const
{
  KPIM::IdentityCombo * ic = dynamic_cast<KPIM::IdentityCombo*>( paramWidget );
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
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;

  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString argsStr );
  virtual const QString argsAsString() const;
  virtual const QString displayString() const;
};


static const KMMsgStatus stati[] =
{
  KMMsgStatusFlag,
  KMMsgStatusRead,
  KMMsgStatusUnread,
  KMMsgStatusReplied,
  KMMsgStatusForwarded,
  KMMsgStatusOld,
  KMMsgStatusNew,
  KMMsgStatusWatched,
  KMMsgStatusIgnored,
  KMMsgStatusSpam,
  KMMsgStatusHam
};
static const int StatiCount = sizeof( stati ) / sizeof( KMMsgStatus );

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
  mParameterList.append( i18n("msg status","Important") );
  mParameterList.append( i18n("msg status","Read") );
  mParameterList.append( i18n("msg status","Unread") );
  mParameterList.append( i18n("msg status","Replied") );
  mParameterList.append( i18n("msg status","Forwarded") );
  mParameterList.append( i18n("msg status","Old") );
  mParameterList.append( i18n("msg status","New") );
  mParameterList.append( i18n("msg status","Watched") );
  mParameterList.append( i18n("msg status","Ignored") );
  mParameterList.append( i18n("msg status","Spam") );
  mParameterList.append( i18n("msg status","Ham") );

  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionSetStatus::process(KMMessage* msg) const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  KMMsgStatus status = stati[idx-1] ;
  msg->setStatus( status );
  return GoOn;
}

bool KMFilterActionSetStatus::requiresBody(KMMsgBase*) const
{
  return false;
}

void KMFilterActionSetStatus::argsFromString( const QString argsStr )
{
  if ( argsStr.length() == 1 ) {
    for ( int i = 0 ; i < StatiCount ; i++ )
      if ( KMMsgBase::statusToStr(stati[i])[0] == argsStr[0] ) {
        mParameter = *mParameterList.at(i+1);
        return;
      }
  }
  mParameter = *mParameterList.at(0);
}

const QString KMFilterActionSetStatus::argsAsString() const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return QString::null;

  KMMsgStatus status = stati[idx-1];
  return KMMsgBase::statusToStr(status);
}

const QString KMFilterActionSetStatus::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionFakeDisposition - send fake MDN
// Sends a fake MDN or forces an ignore.
//=============================================================================
class KMFilterActionFakeDisposition: public KMFilterActionWithStringList
{
public:
  KMFilterActionFakeDisposition();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction() {
    return (new KMFilterActionFakeDisposition);
  }

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString argsStr );
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
  mParameterList.append( i18n("MDN type","Ignore") );
  mParameterList.append( i18n("MDN type","Displayed") );
  mParameterList.append( i18n("MDN type","Deleted") );
  mParameterList.append( i18n("MDN type","Dispatched") );
  mParameterList.append( i18n("MDN type","Processed") );
  mParameterList.append( i18n("MDN type","Denied") );
  mParameterList.append( i18n("MDN type","Failed") );

  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionFakeDisposition::process(KMMessage* msg) const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return ErrorButGoOn;

  if ( idx == 1 ) // ignore
    msg->setMDNSentState( KMMsgMDNIgnore );
  else // send
    sendMDN( msg, mdns[idx-2] ); // skip first two entries: "" and "ignore"
  return GoOn;
}

void KMFilterActionFakeDisposition::argsFromString( const QString argsStr )
{
  if ( argsStr.length() == 1 ) {
    if ( argsStr[0] == 'I' ) { // ignore
      mParameter = *mParameterList.at(1);
      return;
    }
    for ( int i = 0 ; i < numMDNs ; i++ )
      if ( char(mdns[i]) == argsStr[0] ) { // send
        mParameter = *mParameterList.at(i+2);
        return;
      }
  }
  mParameter = *mParameterList.at(0);
}

const QString KMFilterActionFakeDisposition::argsAsString() const
{
  int idx = mParameterList.findIndex( mParameter );
  if ( idx < 1 ) return QString::null;

  return QString( QChar( idx < 2 ? 'I' : char(mdns[idx-2]) ) );
}

const QString KMFilterActionFakeDisposition::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}

//=============================================================================
// KMFilterActionRemoveHeader - remove header
// Remove all instances of the given header field.
//=============================================================================
class KMFilterActionRemoveHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRemoveHeader();
  virtual ReturnCode process(KMMessage* msg) const;
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
  mParameter = *mParameterList.at(0);
}

QWidget* KMFilterActionRemoveHeader::createParamWidget( QWidget* parent ) const
{
  QComboBox *cb = new QComboBox( TRUE/*editable*/, parent );
  cb->setInsertionPolicy( QComboBox::AtBottom );
  setParamWidgetValue( cb );
  return cb;
}

KMFilterAction::ReturnCode KMFilterActionRemoveHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  while ( !msg->headerField( mParameter.latin1() ).isEmpty() )
    msg->removeHeaderField( mParameter.latin1() );
  return GoOn;
}

void KMFilterActionRemoveHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  QComboBox * cb = dynamic_cast<QComboBox*>(paramWidget);
  Q_ASSERT( cb );

  int idx = mParameterList.findIndex( mParameter );
  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
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
  virtual ReturnCode process(KMMessage* msg) const;
  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString argsStr );

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
  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionAddHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() ) return ErrorButGoOn;

  msg->setHeaderField( mParameter.latin1(), mValue );
  return GoOn;
}

QWidget* KMFilterActionAddHeader::createParamWidget( QWidget* parent ) const
{
  QWidget *w = new QWidget( parent );
  QHBoxLayout *hbl = new QHBoxLayout( w );
  hbl->setSpacing( 4 );
  QComboBox *cb = new QComboBox( TRUE, w, "combo" );
  cb->setInsertionPolicy( QComboBox::AtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );
  QLabel *l = new QLabel( i18n("With value:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );
  QLineEdit *le = new KLineEdit( w, "ledit" );
  hbl->addWidget( le, 1 );
  setParamWidgetValue( w );
  return w;
}

void KMFilterActionAddHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
  }
  QLineEdit *le = (QLineEdit*)paramWidget->child("ledit");
  Q_ASSERT( le );
  le->setText( mValue );
}

void KMFilterActionAddHeader::applyParamWidgetValue( QWidget* paramWidget )
{
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  QLineEdit *le = (QLineEdit*)paramWidget->child("ledit");
  Q_ASSERT( le );
  mValue = le->text();
}

void KMFilterActionAddHeader::clearParamWidget( QWidget* paramWidget ) const
{
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->setCurrentItem(0);
  QLineEdit *le = (QLineEdit*)paramWidget->child("ledit");
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
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}

void KMFilterActionAddHeader::argsFromString( const QString argsStr )
{
  QStringList l = QStringList::split( '\t', argsStr, TRUE /*allow empty entries*/ );
  QString s;
  if ( l.count() < 2 ) {
    s = l[0];
    mValue = "";
  } else {
    s = l[0];
    mValue = l[1];
  }

  int idx = mParameterList.findIndex( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionRewriteHeader - rewrite header
// Rewrite a header using a regexp.
//=============================================================================
class KMFilterActionRewriteHeader: public KMFilterActionWithStringList
{
public:
  KMFilterActionRewriteHeader();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual QWidget* createParamWidget( QWidget* parent ) const;
  virtual void setParamWidgetValue( QWidget* paramWidget ) const;
  virtual void applyParamWidgetValue( QWidget* paramWidget );
  virtual void clearParamWidget( QWidget* paramWidget ) const;

  virtual const QString argsAsString() const;
  virtual void argsFromString( const QString argsStr );

  virtual const QString displayString() const;

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionRewriteHeader);
  }
private:
  KRegExp3 mRegExp;
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
  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionRewriteHeader::process(KMMessage* msg) const
{
  if ( mParameter.isEmpty() || !mRegExp.isValid() )
    return ErrorButGoOn;

  KRegExp3 rx = mRegExp; // KRegExp3::replace is not const.

  QString newValue = rx.replace( msg->headerField( mParameter.latin1() ),
                                     mReplacementString );

  msg->setHeaderField( mParameter.latin1(), newValue );
  return GoOn;
}

QWidget* KMFilterActionRewriteHeader::createParamWidget( QWidget* parent ) const
{
  QWidget *w = new QWidget( parent );
  QHBoxLayout *hbl = new QHBoxLayout( w );
  hbl->setSpacing( 4 );

  QComboBox *cb = new QComboBox( TRUE, w, "combo" );
  cb->setInsertionPolicy( QComboBox::AtBottom );
  hbl->addWidget( cb, 0 /* stretch */ );

  QLabel *l = new QLabel( i18n("Replace:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  QLineEdit *le = new KLineEdit( w, "search" );
  hbl->addWidget( le, 1 );

  l = new QLabel( i18n("With:"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  le = new KLineEdit( w, "replace" );
  hbl->addWidget( le, 1 );

  setParamWidgetValue( w );
  return w;
}

void KMFilterActionRewriteHeader::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mParameterList.findIndex( mParameter );
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );

  cb->clear();
  cb->insertStringList( mParameterList );
  if ( idx < 0 ) {
    cb->insertItem( mParameter );
    cb->setCurrentItem( cb->count() - 1 );
  } else {
    cb->setCurrentItem( idx );
  }

  QLineEdit *le = (QLineEdit*)paramWidget->child("search");
  Q_ASSERT( le );
  le->setText( mRegExp.pattern() );

  le = (QLineEdit*)paramWidget->child("replace");
  Q_ASSERT( le );
  le->setText( mReplacementString );
}

void KMFilterActionRewriteHeader::applyParamWidgetValue( QWidget* paramWidget )
{
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  mParameter = cb->currentText();

  QLineEdit *le = (QLineEdit*)paramWidget->child("search");
  Q_ASSERT( le );
  mRegExp.setPattern( le->text() );

  le = (QLineEdit*)paramWidget->child("replace");
  Q_ASSERT( le );
  mReplacementString = le->text();
}

void KMFilterActionRewriteHeader::clearParamWidget( QWidget* paramWidget ) const
{
  QComboBox *cb = (QComboBox*)paramWidget->child("combo");
  Q_ASSERT( cb );
  cb->setCurrentItem(0);

  QLineEdit *le = (QLineEdit*)paramWidget->child("search");
  Q_ASSERT( le );
  le->clear();

  le = (QLineEdit*)paramWidget->child("replace");
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
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}

void KMFilterActionRewriteHeader::argsFromString( const QString argsStr )
{
  QStringList l = QStringList::split( '\t', argsStr, TRUE /*allow empty entries*/ );
  QString s;

  s = l[0];
  mRegExp.setPattern( l[1] );
  mReplacementString = l[2];

  int idx = mParameterList.findIndex( s );
  if ( idx < 0 ) {
    mParameterList.append( s );
    idx = mParameterList.count() - 1;
  }
  mParameter = *mParameterList.at( idx );
}


//=============================================================================
// KMFilterActionMove - move into folder
// File message into another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterActionWithFolder
{
public:
  KMFilterActionMove();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
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

KMFilterAction::ReturnCode KMFilterActionMove::process(KMMessage* msg) const
{
  if ( !mFolder )
    return ErrorButGoOn;

  MessageProperty::setFilterFolder( msg, mFolder );
  return GoOn;
}

bool KMFilterActionMove::requiresBody(KMMsgBase*) const
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
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionCopy::newAction(void)
{
  return (new KMFilterActionCopy);
}

KMFilterActionCopy::KMFilterActionCopy()
  : KMFilterActionWithFolder( "copy", i18n("Copy Into Folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionCopy::process(KMMessage* msg) const
{
  if ( !mFolder )
    return ErrorButGoOn;

  // copy the message 1:1
  KMMessage* msgCopy = new KMMessage;
  msgCopy->fromDwString(msg->asDwString());

  // TODO opening and closing the folder is a trade off.
  // Perhaps Copy is a seldomly used action for now,
  // but I gonna look at improvements ASAP.
  mFolder->open();
  int index;
  int rc = mFolder->addMsg(msgCopy, &index);
  if (rc == 0 && index != -1)
    mFolder->unGetMsg( mFolder->count() - 1 );
  mFolder->close();

  return GoOn;
}

bool KMFilterActionCopy::requiresBody(KMMsgBase*) const
{
    return true;
}


//=============================================================================
// KMFilterActionForward - forward to
// Forward message to another user
//=============================================================================
class KMFilterActionForward: public KMFilterActionWithAddress
{
public:
  KMFilterActionForward();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionForward::newAction(void)
{
  return (new KMFilterActionForward);
}

KMFilterActionForward::KMFilterActionForward()
  : KMFilterActionWithAddress( "forward", i18n("Forward To") )
{
}

KMFilterAction::ReturnCode KMFilterActionForward::process(KMMessage* aMsg) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  // avoid endless loops when this action is used in a filter
  // which applies to sent messages
  if ( KMMessage::addressIsInAddressList( mParameter, aMsg->to() ) )
    return ErrorButGoOn;

  // Create the forwarded message by hand to make forwarding of messages with
  // attachments work.
  // Note: This duplicates a lot of code from KMMessage::createForward() and
  //       KMComposeWin::applyChanges().
  // ### FIXME: Remove the code duplication again.

  KMMessage* msg = new KMMessage;

  msg->initFromMessage( aMsg );

  QString st = QString::fromUtf8( aMsg->createForwardBody() );
  QCString
    encoding = KMMsgBase::autoDetectCharset( aMsg->charset(),
                                             KMMessage::preferredCharsets(),
                                             st );
  if( encoding.isEmpty() )
    encoding = "utf-8";
  QCString str = KMMsgBase::codecForName( encoding )->fromUnicode( st );

  msg->setCharset( encoding );
  msg->setTo( mParameter );
  msg->setSubject( "Fwd: " + aMsg->subject() );

  bool isQP = kmkernel->msgSender()->sendQuotedPrintable();

  if( aMsg->numBodyParts() == 0 )
  {
    msg->setAutomaticFields( true );
    msg->setHeaderField( "Content-Type", "text/plain" );
    // msg->setCteStr( isQP ? "quoted-printable": "8bit" );
    QValueList<int> dummy;
    msg->setBodyAndGuessCte(str, dummy, !isQP);
    msg->setCharset( encoding );
    if( isQP )
      msg->setBodyEncoded( str );
    else
      msg->setBody( str );
  }
  else
  {
    KMMessagePart bodyPart, msgPart;

    msg->removeHeaderField( "Content-Type" );
    msg->removeHeaderField( "Content-Transfer-Encoding" );
    msg->setAutomaticFields( true );
    msg->setBody( "This message is in MIME format.\n\n" );

    bodyPart.setTypeStr( "text" );
    bodyPart.setSubtypeStr( "plain" );
    // bodyPart.setCteStr( isQP ? "quoted-printable": "8bit" );
    QValueList<int> dummy;
    bodyPart.setBodyAndGuessCte(str, dummy, !isQP);
    bodyPart.setCharset( encoding );
    bodyPart.setBodyEncoded( str );
    msg->addBodyPart( &bodyPart );

    for( int i = 0; i < aMsg->numBodyParts(); i++ )
    {
      aMsg->bodyPart( i, &msgPart );
      if( i > 0 || qstricmp( msgPart.typeStr(), "text" ) != 0 )
        msg->addBodyPart( &msgPart );
    }
  }
  msg->cleanupHeader();
  msg->link( aMsg, KMMsgStatusForwarded );

  sendMDN( aMsg, KMime::MDN::Dispatched );

  if ( !kmkernel->msgSender()->send( msg, FALSE ) ) {
    kdDebug(5006) << "KMFilterAction: could not forward message (sending failed)" << endl;
    return ErrorButGoOn; // error: couldn't send
  }
  return GoOn;
}


//=============================================================================
// KMFilterActionRedirect - redirect to
// Redirect message to another user
//=============================================================================
class KMFilterActionRedirect: public KMFilterActionWithAddress
{
public:
  KMFilterActionRedirect();
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionRedirect::process(KMMessage* aMsg) const
{
  KMMessage* msg;
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  msg = aMsg->createRedirect( mParameter );

  sendMDN( aMsg, KMime::MDN::Dispatched );

  if ( !kmkernel->msgSender()->send( msg, FALSE ) ) {
    kdDebug(5006) << "KMFilterAction: could not redirect message (sending failed)" << endl;
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
  virtual ReturnCode process(KMMessage* msg) const;
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

KMFilterAction::ReturnCode KMFilterActionExec::process(KMMessage *aMsg) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, false ); // ignore output
}

//=============================================================================
// KMFilterActionExtFilter - use external filter app
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================

#include <weaver.h>
class PipeJob : public KPIM::ThreadWeaver::Job
{
  public:
    PipeJob(QObject* parent = 0 , const char* name = 0, KMMessage* aMsg = 0, QString cmd = 0, QString tempFileName = 0 )
      : Job (parent, name),
        mTempFileName(tempFileName),
        mCmd(cmd),
        mMsg( aMsg )
    {
    }

    ~PipeJob() {}
    virtual void processEvent( KPIM::ThreadWeaver::Event *ev )
    {
      KPIM::ThreadWeaver::Job::processEvent( ev );
      if ( ev->action() == KPIM::ThreadWeaver::Event::JobFinished )
        deleteLater( );
    }
  protected:
    void run()
    {
      KPIM::ThreadWeaver::debug (1, "PipeJob::run: doing it .\n");
      FILE *p;
      QByteArray ba;

      p = popen(QFile::encodeName(mCmd), "r");
      int len =100;
      char buffer[100];
      // append data to ba:
      while (true)  {
        if (! fgets( buffer, len, p ) ) break;
        int oldsize = ba.size();
        ba.resize( oldsize + strlen(buffer) );
        qmemmove( ba.begin() + oldsize, buffer, strlen(buffer) );
      }
      pclose(p);
      if ( !ba.isEmpty() ) {
        KPIM::ThreadWeaver::debug (1, "PipeJob::run: %s", QString(ba).latin1() );
        mMsg->fromByteArray( ba );
      }

      KPIM::ThreadWeaver::debug (1, "PipeJob::run: done.\n" );
      // unlink the tempFile
      QFile::remove(mTempFileName);
    }
    QString mTempFileName;
    QString mCmd;
    KMMessage *mMsg;
};

class KMFilterActionExtFilter: public KMFilterActionWithCommand
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual void processAsync(KMMessage* msg) const;
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
KMFilterAction::ReturnCode KMFilterActionExtFilter::process(KMMessage* aMsg) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, true ); // use output
}

void KMFilterActionExtFilter::processAsync(KMMessage* aMsg) const
{

  ActionScheduler *handler = MessageProperty::filterHandler( aMsg->getMsgSerNum() );
  KTempFile * inFile = new KTempFile;
  inFile->setAutoDelete(FALSE);

  QPtrList<KTempFile> atmList;
  atmList.setAutoDelete(TRUE);
  atmList.append( inFile );

  QString commandLine = substituteCommandLineArgsFor( aMsg , atmList );
  if ( commandLine.isEmpty() )
    handler->actionMessage( ErrorButGoOn );

  // The parentheses force the creation of a subshell
  // in which the user-specified command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine =  "(" + commandLine + ") <" + inFile->name();

  // write message to file
  QString tempFileName = inFile->name();
  KPIM::kCStringToFile( aMsg->asString(), tempFileName, //###
      false, false, false );
  inFile->close();

  PipeJob *job = new PipeJob(0, 0, aMsg, commandLine, tempFileName);
  QObject::connect ( job, SIGNAL( done() ), handler, SLOT( actionMessage() ) );
  kmkernel->weaver()->enqueue(job);
}

//=============================================================================
// KMFilterActionExecSound - execute command
// Execute a sound
//=============================================================================
class KMFilterActionExecSound : public KMFilterActionWithTest
{
public:
  KMFilterActionExecSound();
  virtual ReturnCode process(KMMessage* msg) const;
  virtual bool requiresBody(KMMsgBase*) const;
  static KMFilterAction* newAction(void);
};

KMFilterActionWithTest::KMFilterActionWithTest( const char* aName, const QString aLabel )
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

void KMFilterActionWithTest::argsFromString( const QString argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithTest::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithTest::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}


KMFilterActionExecSound::KMFilterActionExecSound()
  : KMFilterActionWithTest( "play sound", i18n("Play Sound") )
{
}

KMFilterAction* KMFilterActionExecSound::newAction(void)
{
  return (new KMFilterActionExecSound());
}

KMFilterAction::ReturnCode KMFilterActionExecSound::process(KMMessage*) const
{
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;
  QString play = mParameter;
  QString file = QString::fromLatin1("file:");
  if (mParameter.startsWith(file))
    play = mParameter.mid(file.length());
  KAudioPlayer::play(QFile::encodeName(play));
  return GoOn;
}

bool KMFilterActionExecSound::requiresBody(KMMsgBase*) const
{
  return false;
}

KMFilterActionWithUrl::KMFilterActionWithUrl( const char* aName, const QString aLabel )
  : KMFilterAction( aName, aLabel )
{
}

KMFilterActionWithUrl::~KMFilterActionWithUrl()
{
}

QWidget* KMFilterActionWithUrl::createParamWidget( QWidget* parent ) const
{
  KURLRequester *le = new KURLRequester(parent);
  le->setURL( mParameter );
  return le;
}


void KMFilterActionWithUrl::applyParamWidgetValue( QWidget* paramWidget )
{
  mParameter = ((KURLRequester*)paramWidget)->url();
}

void KMFilterActionWithUrl::setParamWidgetValue( QWidget* paramWidget ) const
{
  ((KURLRequester*)paramWidget)->setURL( mParameter );
}

void KMFilterActionWithUrl::clearParamWidget( QWidget* paramWidget ) const
{
  ((KURLRequester*)paramWidget)->clear();
}

void KMFilterActionWithUrl::argsFromString( const QString argsStr )
{
  mParameter = argsStr;
}

const QString KMFilterActionWithUrl::argsAsString() const
{
  return mParameter;
}

const QString KMFilterActionWithUrl::displayString() const
{
  // FIXME after string freeze:
  // return i18n("").arg( );
  return label() + " \"" + QStyleSheet::escape( argsAsString() ) + "\"";
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert( KMFilterActionMove::newAction );
  insert( KMFilterActionCopy::newAction );
  insert( KMFilterActionIdentity::newAction );
  insert( KMFilterActionSetStatus::newAction );
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
  // Register custom filter actions below this line.
}
// The int in the QDict constructor (41) must be a prime
// and should be greater than the double number of KMFilterAction types
KMFilterActionDict::KMFilterActionDict()
  : QDict<KMFilterActionDesc>(41)
{
  mList.setAutoDelete(TRUE);
  init();
}

void KMFilterActionDict::insert( KMFilterActionNewFunc aNewFunc )
{
  KMFilterAction *action = aNewFunc();
  KMFilterActionDesc* desc = new KMFilterActionDesc;
  desc->name = action->name();
  desc->label = action->label();
  desc->create = aNewFunc;
  QDict<KMFilterActionDesc>::insert( desc->name, desc );
  QDict<KMFilterActionDesc>::insert( desc->label, desc );
  mList.append( desc );
  delete action;
}
