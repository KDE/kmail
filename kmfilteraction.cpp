// kmfilteraction.cpp
// The process methods really should use an enum instead of an int
// -1 -> status unchanged, 0 -> success, 1 -> failure, 2-> critical failure
// (GoOn),                 (Ok),         (ErrorButGoOn), (CriticalError)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilteraction.h"
#include "kmmsgpart.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "identitymanager.h"
#include "identitycombo.h"
#include "kfileio.h"
#include "kmfawidgets.h"
#include "kmfoldercombobox.h"
#include "kmmessage.h"

#include <kregexp3.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <kdeversion.h>

#include <qtl.h>  // QT Template Library, needed for qHeapSort
#include <qlabel.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qtextcodec.h>

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
  return kernel->filterMgr()->tempOpenFolder(aFolder);
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
  return QString().setNum( mParameter );
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
  QLineEdit *le = new QLineEdit(parent);
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
  KMFolderComboBox *cb = new KMFolderComboBox( parent );
  cb->showImapFolders( false );
  setParamWidgetValue( cb );
  return cb;
}

void KMFilterActionWithFolder::applyParamWidgetValue( QWidget* paramWidget )
{
  mFolder = ((KMFolderComboBox *)paramWidget)->getFolder();
  if (mFolder)
  {
     mFolderName = QString::null;
  }
  else
  {
     mFolderName = ((KMFolderComboBox *)paramWidget)->currentText();
  }
}

void KMFilterActionWithFolder::setParamWidgetValue( QWidget* paramWidget ) const
{
  if ( mFolder )
    ((KMFolderComboBox *)paramWidget)->setFolder( mFolder );
  else
    ((KMFolderComboBox *)paramWidget)->setFolder( mFolderName );
}

void KMFilterActionWithFolder::clearParamWidget( QWidget* paramWidget ) const
{
  ((KMFolderComboBox *)paramWidget)->setFolder( kernel->draftsFolder() );
}

void KMFilterActionWithFolder::argsFromString( const QString argsStr )
{
  mFolder = kernel->folderMgr()->findIdString( argsStr );
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

bool KMFilterActionWithFolder::folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder )
{
  if ( aFolder == mFolder ) {
    mFolder = aNewFolder;
    mFolderName = QString::null;
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
  : KMFilterActionWithString( aName, aLabel )
{
}

QWidget* KMFilterActionWithCommand::createParamWidget( QWidget* parent ) const
{
  return KMFilterActionWithString::createParamWidget( parent );
}

void KMFilterActionWithCommand::applyParamWidgetValue( QWidget* paramWidget )
{
  KMFilterActionWithString::applyParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::setParamWidgetValue( QWidget* paramWidget ) const
{
  KMFilterActionWithString::setParamWidgetValue( paramWidget );
}

void KMFilterActionWithCommand::clearParamWidget( QWidget* paramWidget ) const
{
  KMFilterActionWithString::clearParamWidget( paramWidget );
}

QString KMFilterActionWithCommand::substituteCommandLineArgsFor( KMMessage *aMsg, QPtrList<KTempFile> & aTempFileList ) const
{
  QString result = mParameter;
  QValueList<int> argList;
  QRegExp r( "%[\\-0-9]+" );
  int start=-1, len=0;
  bool OK = FALSE;

  // search for '%n'
  while ( ( start = r.search( result, start + 1 ) ) > 0 ) {
    len = r.matchedLength();
    // and save the encountered 'n' in a list.
    int n = result.mid( start + 1, len - 1 ).toInt( &OK );
    if ( OK )
      argList.append( n );
  }

  // sort the list of n's
  qHeapSort( argList );

  // and use QString::arg to substitute filenames for the %n's.
  int lastSeen = -2;
  QString tempFileName;
  KMMessagePart msgPart;
  QValueList<int>::Iterator it;
  for ( it = argList.begin() ; it != argList.end() ; ++it ) {
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
        kCStringToFile( aMsg->asString(), tempFileName, //###
                          false, false, false );
      else if (aMsg->numBodyParts() == 0)
        kByteArrayToFile( aMsg->bodyDecodedBinary(), tempFileName,
                          false, false, false );
      else {
        aMsg->bodyPart( (*it), &msgPart );
        kByteArrayToFile( msgPart.bodyDecodedBinary(), tempFileName,
                          false, false, false );
      }
      tf->close();
    }
    // QString( "%0 and %1 and %1" ).arg( 0 ).arg( 1 )
    // returns "0 and 1 and %1", so we must call .arg as
    // many times as there are %n's, regardless of their multiplicity.
    if ((*it) == -1) result.replace( QRegExp("%-1"), tempFileName );
    else result = result.arg( tempFileName );
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
  inFile->close();

  QPtrList<KTempFile> atmList;
  atmList.setAutoDelete(TRUE);
  atmList.append( inFile );

  QString commandLine = substituteCommandLineArgsFor( aMsg , atmList );
  if ( commandLine.isEmpty() )
    return ErrorButGoOn;

  // The parentheses force the creation of a subshell
  // in which the user-specifed command is executed.
  // This is to really catch all output of the command as well
  // as to avoid clashes of our redirection with the ones
  // the user may have specified. In the long run, we
  // shouldn't be using tempfiles at all for this class, due
  // to security aspects. (mmutz)
  commandLine = QString( "(%1) <%2" ).arg( commandLine ).arg( inFile->name() );

  // write message to file
  QString tempFileName = inFile->name();
  kCStringToFile( aMsg->asString(), tempFileName, //###
		  false, false, false );

#if KDE_VERSION >= 305
  KProcess shProc;
  shProc.setUseShell(true);
#else
  KShellProcess shProc;
#endif
  shProc << commandLine;

  // let the kernel collect the output for us:
  if ( withOutput )
    QObject::connect( &shProc, SIGNAL(receivedStdout(KProcess*,char*,int)),
		      kernel, SLOT(slotCollectStdOut(KProcess*,char*,int)) );

  // run process:
  if ( !shProc.start( KProcess::Block,
		      withOutput ? KProcess::Stdout : KProcess::NoCommunication ) )
    return ErrorButGoOn;

  if ( !shProc.normalExit() || shProc.exitStatus() != 0 )
    return ErrorButGoOn;

  if ( withOutput ) {
    // read altered message:
    QByteArray msgText = kernel->getCollectedStdOut( &shProc );

    if ( !msgText.isEmpty() )
      aMsg->fromString( QCString( msgText.data(), msgText.size() ) );
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
// KMFilterActionBounce - bounce
// Return mail as undelivered.
//=============================================================================
class KMFilterActionBounce : public KMFilterActionWithNone
{
public:
  KMFilterActionBounce();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionBounce::newAction(void)
{
  return (new KMFilterActionBounce);
}

KMFilterActionBounce::KMFilterActionBounce()
  : KMFilterActionWithNone( "bounce", i18n("bounce") )
{
}

KMFilterAction::ReturnCode KMFilterActionBounce::process(KMMessage* msg) const
{
  KMMessage *bounceMsg = msg->createBounce( FALSE );
  if ( !bounceMsg ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the bounces before sending and b) for speed reasons.
  kernel->msgSender()->send( bounceMsg, FALSE );

  return GoOn;
}


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
  : KMFilterActionWithNone( "confirm delivery", i18n("confirm delivery") )
{
}

KMFilterAction::ReturnCode KMFilterActionSendReceipt::process(KMMessage* msg) const
{
  KMMessage *receipt = msg->createDeliveryReceipt();
  if ( !receipt ) return ErrorButGoOn;

  // Queue message. This is a) so that the user can check
  // the receipt before sending and b) for speed reasons.
  kernel->msgSender()->send( receipt, FALSE );

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
  : KMFilterActionWithString( "set transport", i18n("set transport to") )
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
  : KMFilterActionWithString( "set Reply-To", i18n("set Reply-To to") )
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
  : KMFilterActionWithUOID( "set identity", i18n("set identity to") )
{
  mParameter = kernel->identityManager()->defaultIdentity().uoid();
}

KMFilterAction::ReturnCode KMFilterActionIdentity::process(KMMessage* msg) const
{
  msg->setHeaderField( "X-KMail-Identity", QString().setNum( mParameter ) );
  return GoOn;
}

QWidget * KMFilterActionIdentity::createParamWidget( QWidget * parent ) const
{
  IdentityCombo * ic = new IdentityCombo( parent );
  ic->setCurrentIdentity( mParameter );
  return ic;
}

void KMFilterActionIdentity::applyParamWidgetValue( QWidget * paramWidget )
{
  IdentityCombo * ic = dynamic_cast<IdentityCombo*>( paramWidget );
  assert( ic );
  mParameter = ic->currentIdentity();
}

void KMFilterActionIdentity::clearParamWidget( QWidget * paramWidget ) const
{
  IdentityCombo * ic = dynamic_cast<IdentityCombo*>( paramWidget );
  assert( ic );
  ic->setCurrentItem( 0 );
  //ic->setCurrentIdentity( kernel->identityManager()->defaultIdentity() );
}

void KMFilterActionIdentity::setParamWidgetValue( QWidget * paramWidget ) const
{
  IdentityCombo * ic = dynamic_cast<IdentityCombo*>( paramWidget );
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
  static KMFilterAction* newAction();

  virtual bool isEmpty() const { return false; }

  virtual void argsFromString( const QString argsStr );
  virtual const QString argsAsString() const;
};


// if you change this list, also update
// the count in argsFromString
static const KMMsgStatus stati[] =
{
  KMMsgStatusFlag,
  KMMsgStatusRead,
  KMMsgStatusUnread,
  KMMsgStatusReplied,
  KMMsgStatusForwarded,
  KMMsgStatusOld,
  KMMsgStatusNew
};

KMFilterAction* KMFilterActionSetStatus::newAction()
{
  return (new KMFilterActionSetStatus);
}

KMFilterActionSetStatus::KMFilterActionSetStatus()
  : KMFilterActionWithStringList( "set status", i18n("mark as") )
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

void KMFilterActionSetStatus::argsFromString( const QString argsStr )
{
  if ( argsStr.length() == 1 ) {
    for ( int i = 0 ; i < 8 ; i++ )
      if ( char(stati[i]) == argsStr[0] ) {
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
  return QString( QChar( char(status) ) );
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
  : KMFilterActionWithStringList( "remove header", i18n("remove header") )
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

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionAddHeader);
  }
private:
  QString mValue;
};

KMFilterActionAddHeader::KMFilterActionAddHeader()
  : KMFilterActionWithStringList( "add header", i18n("add header") )
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
  QLabel *l = new QLabel( i18n("with value"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );
  QLineEdit *le = new QLineEdit( w, "ledit" );
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

  static KMFilterAction* newAction()
  {
    return (new KMFilterActionRewriteHeader);
  }
private:
  KRegExp3 mRegExp;
  QString mReplacementString;
};

KMFilterActionRewriteHeader::KMFilterActionRewriteHeader()
  : KMFilterActionWithStringList( "rewrite header", i18n("rewrite header") )
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

  QLabel *l = new QLabel( i18n("replace"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  QLineEdit *le = new QLineEdit( w, "search" );
  hbl->addWidget( le, 1 );

  l = new QLabel( i18n("with"), w );
  l->setFixedWidth( l->sizeHint().width() );
  hbl->addWidget( l, 0 );

  le = new QLineEdit( w, "replace" );
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
// KMFilterActionMove - move to folder
// Move message to another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterActionWithFolder
{
public:
  KMFilterActionMove();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionMove::newAction(void)
{
  return (new KMFilterActionMove);
}

KMFilterActionMove::KMFilterActionMove()
  : KMFilterActionWithFolder( "transfer", i18n("move to folder") )
{
}

KMFilterAction::ReturnCode KMFilterActionMove::process(KMMessage* msg) const
{
  if ( !mFolder )
    return ErrorButGoOn;

  KMFilterAction::tempOpenFolder( mFolder );

  if ( msg->parent() )
    return Moved; // the message already has a parent??? so what?
  if ( mFolder->moveMsg(msg) == 0 )
    return Moved; // ok, added
  else {
    kdDebug(5006) << "KMfilterAction - couldn't move msg" << endl;
    return CriticalError; // critical error: couldn't add
  }
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
  : KMFilterActionWithAddress( "forward", i18n("forward to") )
{
}

KMFilterAction::ReturnCode KMFilterActionForward::process(KMMessage* aMsg) const
{
  if ( mParameter.isEmpty() )
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

  bool isQP = kernel->msgSender()->sendQuotedPrintable();

  if( aMsg->numBodyParts() == 0 )
  {
    msg->setAutomaticFields( true );
    msg->setHeaderField( "Content-Type", "text/plain" );
    msg->setCteStr( isQP ? "quoted-printable": "8bit" );
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
    bodyPart.setCteStr( isQP ? "quoted-printable": "8bit" );
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

  if ( !kernel->msgSender()->send(msg) ) {
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
  : KMFilterActionWithAddress( "redirect", i18n("redirect to") )
{
}

KMFilterAction::ReturnCode KMFilterActionRedirect::process(KMMessage* aMsg) const
{
  KMMessage* msg;
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  msg = aMsg->createRedirect();
  msg->setTo( mParameter );

  if ( !kernel->msgSender()->send(msg) ) {
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
  : KMFilterActionWithCommand( "execute", i18n("execute command") )
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
class KMFilterActionExtFilter: public KMFilterActionWithCommand
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExtFilter::newAction(void)
{
  return (new KMFilterActionExtFilter);
}

KMFilterActionExtFilter::KMFilterActionExtFilter()
  : KMFilterActionWithCommand( "filter app", i18n("pipe through") )
{
}

KMFilterAction::ReturnCode KMFilterActionExtFilter::process(KMMessage* aMsg) const
{
  return KMFilterActionWithCommand::genericProcess( aMsg, true ); // use output
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert( KMFilterActionMove::newAction );
  insert( KMFilterActionIdentity::newAction );
  insert( KMFilterActionSetStatus::newAction );
  insert( KMFilterActionTransport::newAction );
  insert( KMFilterActionReplyTo::newAction );
  insert( KMFilterActionForward::newAction );
  insert( KMFilterActionRedirect::newAction );
  insert( KMFilterActionBounce::newAction );
  insert( KMFilterActionSendReceipt::newAction );
  insert( KMFilterActionExec::newAction );
  insert( KMFilterActionExtFilter::newAction );
  insert( KMFilterActionRemoveHeader::newAction );
  insert( KMFilterActionAddHeader::newAction );
  insert( KMFilterActionRewriteHeader::newAction );
  // Register custom filter actions below this line.
}
// The int in the QDict constructor (23) must be a prime
// and should be greater than the double number of KMFilterAction types
KMFilterActionDict::KMFilterActionDict()
  : QDict<KMFilterActionDesc>(23)
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

