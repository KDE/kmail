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

#include <kstddirs.h>
#include <kconfig.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <klocale.h>

#include <qcombobox.h>
#include <qlineedit.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h> //for alarm (sven)
#include <sys/types.h>
#include <sys/stat.h>



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
  // later on this will be replaced with a line edit alongside an
  // "..." button that calls the address book.
  return KMFilterActionWithString::createParamWidget( parent );
}

void KMFilterActionWithAddress::applyParamWidgetValue( QWidget* paramWidget )
{
  KMFilterActionWithString::applyParamWidgetValue( paramWidget );
}

void KMFilterActionWithAddress::setParamWidgetValue( QWidget* paramWidget ) const
{
  KMFilterActionWithString::setParamWidgetValue( paramWidget );
}

void KMFilterActionWithAddress::clearParamWidget( QWidget* paramWidget ) const
{
  KMFilterActionWithString::clearParamWidget( paramWidget );
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




//=============================================================================
//
//   Specific  Filter  Actions
//
//=============================================================================

#ifndef KMFILTERACTION_NO_BOUNCE
//=============================================================================
// KMFilterActionBounce - bounce
// Return mail as undelivered.
//=============================================================================
class KMFilterActionBounce : public KMFilterActionWithNone
{
public:
  KMFilterActionBounce();
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionBounce::process(KMMessage* msg, bool& ) const
{
  return ErrorButGoOn;
}
#endif

//=============================================================================
// KMFilterActionSetTransport - set transport to...
// Specify mail transport (smtp server) to be used when replying to a message
//=============================================================================
class KMFilterActionTransport: public KMFilterActionWithString
{
public:
  KMFilterActionTransport();
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionTransport::process(KMMessage* msg, bool& ) const
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
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionReplyTo::process(KMMessage* msg, bool& ) const
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
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionIdentity::process(KMMessage* msg, bool& ) const
{
  msg->setHeaderField( "X-KMail-Identity", mParameter );
  return GoOn;
}

//=============================================================================
// KMFilterActionMove - move to folder
// Move message to another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterActionWithFolder
{
public:
  KMFilterActionMove();
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionMove::process(KMMessage* msg, bool&stopIt) const
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
    stopIt = TRUE;
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
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionForward::process(KMMessage* aMsg, bool& /*stop*/) const
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
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
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

KMFilterAction::ReturnCode KMFilterActionRedirect::process(KMMessage* aMsg, bool& /*stop*/) const
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
  KMFilterActionExec(const char* aName, const QString aLabel );
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
  static KMFilterAction* newAction(void);
  static void dummySigHandler(int);
};

KMFilterAction* KMFilterActionExec::newAction(void)
{
  return (new KMFilterActionExec(0,0)); // ###
}

KMFilterActionExec::KMFilterActionExec(const char* aName, const QString aLabel )
  : KMFilterActionWithCommand( aName ? aName : "execute" , aLabel ? aLabel : i18n("execute command") )
{
}

void KMFilterActionExec::dummySigHandler(int)
{
}

KMFilterAction::ReturnCode KMFilterActionExec::process(KMMessage *aMsg, bool& /*stop*/) const
{
  // should use K{,Shell}Process....
  void (*oldSigHandler)(int);
  int rc;
  if (mParameter.isEmpty()) return ErrorButGoOn;
  QString fullCmd = mParameter + " ";
  QList<KTempFile> atmList;
  KMMessagePart msgPart;
  int i, nr;
  while (fullCmd.contains("%"))
  {
    // this code seems broken for commands that
    // specify e.g. %2 before %1. (mmutz)
    i = fullCmd.find("%") + 1;
    nr = fullCmd.mid(i, fullCmd.find(" ", i) - i).toInt();
    aMsg->bodyPart(nr, &msgPart);
    KTempFile *atmTempFile = new KTempFile();
    atmList.append( atmTempFile );
    atmTempFile->setAutoDelete( true );
    kByteArrayToFile(msgPart.bodyDecodedBinary(), atmTempFile->name(),
		     false, false, false);
    fullCmd = fullCmd.arg( atmTempFile->name() );
  }
  oldSigHandler = signal(SIGALRM, &KMFilterActionExec::dummySigHandler);
  alarm(30);
  rc = system(fullCmd.left(fullCmd.length() - 1 ));
  alarm(0);
  signal(SIGALRM, oldSigHandler);
  if (rc & 255)
    return ErrorButGoOn;
  else
    return GoOn;
}

// support suspended until proted to KProcess.
#define KMFILTERACTIONEXTFILTER_IS_BROKEN

#ifndef KMFILTERACTIONEXTFILTER_IS_BROKEN

//=============================================================================
// KMFilterActionExtFilter - use external filter app
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================
class KMFilterActionExtFilter: public KMFilterActionExec
{
public:
  KMFilterActionExtFilter();
  virtual ReturnCode process(KMMessage* msg, bool& stopIt) const;
  static KMFilterAction* newAction(void);
};

KMFilterAction* KMFilterActionExtFilter::newAction(void)
{
  return (new KMFilterActionExtFilter);
}

KMFilterActionExtFilter::KMFilterActionExtFilter()
  : KMFilterActionExec( "filter app", i18n("use external filter app") )
{
}

KMFilterAction::ReturnCode KMFilterActionExtFilter::process(KMMessage* aMsg, bool& stop) const
{
  int len;
  ReturnCode rc=Ok;
  QString msgText, origCmd;
  char buf[8192];
  FILE *fh;
  bool ok = TRUE;
  KTempFile inFile(locateLocal("tmp", "kmail-filter"), "in");
  KTempFile outFile(locateLocal("tmp", "kmail-filter"), "out");

  if (mParameter.isEmpty()) return ErrorButGoOn;

  // write message to file
  fh = inFile.fstream();
  if (fh)
  {
    msgText = aMsg->asString();
    if (!fwrite(msgText, msgText.length(), 1, fh)) ok = FALSE;
    inFile.close();
  }
  else ok = FALSE;

  outFile.close();
  if (ok)
  {
    // execute filter
    origCmd = mParameter;
    mParameter += " <" + inFile.name() + " >" + outFile.name();
    rc = KMFilterActionExec::process(aMsg, stop);
    mParameter = origCmd;

    // read altered message
    fh = fopen(outFile.name(), "r");
    if (fh)
    {
      msgText = "";
      while (1)
      {
	len = fread(buf, 1, 1023, fh);
	if (len <= 0) break;
	buf[len] = 0;
	msgText += buf;
      }
      outFile.close();
      if (!msgText.isEmpty()) aMsg->fromString(msgText);
    }
    else ok = FALSE;
  }

  inFile.unlink();
  outFile.unlink();

  return rc;
}

#endif


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert( KMFilterActionMove::newAction );
  insert( KMFilterActionIdentity::newAction );
  insert( KMFilterActionTransport::newAction );
  insert( KMFilterActionReplyTo::newAction );
  insert( KMFilterActionForward::newAction );
  insert( KMFilterActionRedirect::newAction );
#ifndef KMFILTERACTION_NO_BOUNCE
  insert( KMFilterActionBounce::newAction );
#endif
  insert( KMFilterActionExec::newAction );
#ifndef KMFILTERACTIONEXTFILTER_IS_BROKEN
  insert( KMFilterActionExtFilter::newAction );
#endif
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

