// kmfilteraction.cpp
// The process methods really should use an enum instead of an int
// -1 -> status unchanged, 0 -> success, 1 -> failure, 2-> critical failure
// (GoOn),                 (Ok),         (ErrorButGoOn), (CriticalError)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilteraction.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "kfileio.h"
#include "kmfawidgets.h"

#include <kstddirs.h>
#include <kconfig.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>

#include <qcombobox.h>
#include <qvaluelist.h>
#include <qtl.h>  // QT Template Library, needed for qHeapSort


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
// KMFilterActionWithString
//
//=============================================================================

KMFilterActionWithNone::KMFilterActionWithNone( const char* aName, const QString aLabel )
  : KMFilterAction( aName, aLabel )
{
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
  mParameter = *mParameterList.at( idx >= 0 ? idx : 0 );
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
  kernel->folderMgr()->createI18nFolderList( &mFolderNames, &mFolderList );
}

QWidget* KMFilterActionWithFolder::createParamWidget( QWidget* parent ) const
{
  QComboBox *cb = new QComboBox( FALSE, parent );
  cb->insertStringList( mFolderNames );
  setParamWidgetValue( cb );
  return cb;
}

void KMFilterActionWithFolder::applyParamWidgetValue( QWidget* paramWidget )
{
  // let's hope that QValueListIterator::operator*(QValueList::end()) == NULL.
  mFolder = *mFolderList.at( ((QComboBox*)paramWidget)->currentItem() );
}

void KMFilterActionWithFolder::setParamWidgetValue( QWidget* paramWidget ) const
{
  int idx = mFolderList.findIndex( mFolder );
  ((QComboBox*)paramWidget)->setCurrentItem( idx >= 0 ? idx : 0 );
}

void KMFilterActionWithFolder::clearParamWidget( QWidget* paramWidget ) const
{
  ((QComboBox*)paramWidget)->setCurrentItem( 0 );
}

void KMFilterActionWithFolder::argsFromString( const QString argsStr )
{
  mFolder = kernel->folderMgr()->findIdString( argsStr );
}

const QString KMFilterActionWithFolder::argsAsString() const
{
  QString result;
  if ( mFolder )
    result = mFolder->idString();
  else
    result = "";
  return result;
}

bool KMFilterActionWithFolder::folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder )
{
  if ( aFolder == mFolder ) {
    mFolder = aNewFolder;
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

QString KMFilterActionWithCommand::substituteCommandLineArgsFor( KMMessage *aMsg, QList<KTempFile> & aTempFileList ) const
{
  QString result = mParameter;
  QValueList<int> argList;
  QRegExp r( "%[\\-0-9]+" );
  int start=-1, len=0;
  bool OK = FALSE;
  
  // search for '%n'
  while ( ( start = r.match( result, start + 1, &len ) ) > 0 ) {
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
	kdDebug() << "KMFilterActionWithCommand: Could not create temp file!" << endl;
	return QString::null;
      }
      tf->setAutoDelete(TRUE);
      aTempFileList.append( tf );
      tempFileName = tf->name();
      if ((*it) == -1)
        kCStringToFile( aMsg->asString().data(), tempFileName,
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
class KMFilterActionIdentity: public KMFilterActionWithStringList
{
public:
  KMFilterActionIdentity();
  virtual ReturnCode process(KMMessage* msg) const;
  static KMFilterAction* newAction();
};

KMFilterAction* KMFilterActionIdentity::newAction()
{
  return (new KMFilterActionIdentity);
}

KMFilterActionIdentity::KMFilterActionIdentity()
  : KMFilterActionWithStringList( "set identity", i18n("set identity to") )
{
  mParameterList = KMIdentity::identities();
  mParameter = *mParameterList.at(0);
}

KMFilterAction::ReturnCode KMFilterActionIdentity::process(KMMessage* msg) const
{
  msg->setHeaderField( "X-KMail-Identity", mParameter );
  return GoOn;
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

  virtual void KMFilterActionSetStatus::argsFromString( const QString argsStr );
  virtual const QString KMFilterActionSetStatus::argsAsString() const;
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
    return Finished; // the message already has a parent??? so what?
  if ( mFolder->moveMsg(msg) == 0 )
    return Finished; // ok, added
  else {
    kdDebug() << "KMfilterAction - couldn't move msg" << endl;
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
  KMMessage* msg;
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  msg = aMsg->createForward();
  msg->setTo( mParameter );

  if ( !kernel->msgSender()->send(msg) ) {
    kdDebug() << "KMFilterAction: could not forward message (sending failed)" << endl;
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
    kdDebug() << "KMFilterAction: could not redirect message (sending failed)" << endl;
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
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  QList<KTempFile> atmList;
  atmList.setAutoDelete(TRUE);

  assert( aMsg );

  QString commandLine = substituteCommandLineArgsFor( aMsg, atmList );

  if ( commandLine.isEmpty() )
    return ErrorButGoOn;

  KShellProcess shProc;
  shProc << commandLine;

  if ( !shProc.start( KProcess::Block, KProcess::NoCommunication ) )
    return ErrorButGoOn;

  if ( shProc.normalExit() && shProc.exitStatus() == 0 )
    return GoOn;
  else
    return ErrorButGoOn;
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
  if ( mParameter.isEmpty() )
    return ErrorButGoOn;

  ///////////////
  KTempFile * inFile = new KTempFile;
  KTempFile * outFile = new KTempFile;
  inFile->setAutoDelete(TRUE);
  outFile->setAutoDelete(TRUE);
  outFile->close();

  QList<KTempFile> atmList;
  atmList.setAutoDelete(TRUE);
  atmList.append( inFile );
  atmList.append( outFile );

  assert( aMsg );

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
  commandLine = QString( "(%1) <%2 >%3" ).arg( commandLine ).arg( inFile->name() ).arg( outFile->name() );

  // write message to file
  QString msgText, tempFileName;

  tempFileName = inFile->name();
  kCStringToFile( aMsg->asString().data(), tempFileName,
		  false, false, false );
  inFile->close();

  // run process
  KShellProcess shProc;
  shProc << commandLine;

  if ( !shProc.start( KProcess::Block, KProcess::NoCommunication ) )
    return ErrorButGoOn;

  if ( !shProc.normalExit() || shProc.exitStatus() != 0 )
    return ErrorButGoOn;

  // read altered message
  msgText = kFileToString( outFile->name(), false, false );

  if ( !msgText.isEmpty() )
    aMsg->fromString( msgText );
  else
    return ErrorButGoOn;
  outFile->close();
  
  return GoOn;
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

