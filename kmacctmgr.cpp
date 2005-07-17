// KMail Account Manager

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmacctmgr.h"

#include "kmaccount.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "networkaccount.h"
using KMail::NetworkAccount;
#include "kmacctcachedimap.h"
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "kmfiltermgr.h"
#include "globalsettings.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qregexp.h>
#include <qvaluelist.h>

//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr()
    :QObject(), mNewMailArrived( false ), mInteractive( false ),
     mTotalNewMailsArrived( 0 ), mDisplaySummary( false )
{
  mAcctChecking.clear();
  mAcctTodo.clear();
}

//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  writeConfig( false );
}


//-----------------------------------------------------------------------------
void KMAcctMgr::writeConfig( bool withSync )
{
  KConfig* config = KMKernel::config();
  QString groupName;

  KConfigGroupSaver saver(config, "General");
  config->writeEntry("accounts", mAcctList.count());

  // first delete all account groups in the config file:
  QStringList accountGroups =
    config->groupList().grep( QRegExp( "Account \\d+" ) );
  for ( QStringList::Iterator it = accountGroups.begin() ;
	it != accountGroups.end() ; ++it )
    config->deleteGroup( *it );

  // now write new account groups:
  int i = 1;
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it, ++i ) {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
    (*it)->writeConfig(*config);
  }
  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::readConfig(void)
{
  KConfig* config = KMKernel::config();
  KMAccount* acct;
  QString acctType, acctName;
  QCString groupName;
  int i, num;
  uint id;

  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it )
      delete *it;
  mAcctList.clear();

  KConfigGroup general(config, "General");
  num = general.readNumEntry("accounts", 0);

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    KConfigGroupSaver saver(config, groupName);
    acctType = config->readEntry("Type");
    // Provide backwards compatibility
    if (acctType == "advanced pop" || acctType == "experimental pop")
      acctType = "pop";
    acctName = config->readEntry("Name");
    id = config->readUnsignedNumEntry("Id", 0);
    if (acctName.isEmpty()) acctName = i18n("Account %1").arg(i);
    acct = create(acctType, acctName, id);
    if (!acct) continue;
    add(acct);
    acct->readConfig(*config);
  }
}


//-----------------------------------------------------------------------------
void KMAcctMgr::singleCheckMail(KMAccount *account, bool interactive)
{
  mNewMailArrived = false;
  mInteractive = interactive;

  // queue the account
  mAcctTodo.append(account);

  if (account->checkingMail())
  {
    kdDebug(5006) << "account " << account->name() << " busy, queuing" << endl;
    return;
  }

  processNextCheck(false);
}

//-----------------------------------------------------------------------------
void KMAcctMgr::processNextCheck( bool _newMail )
{
  kdDebug(5006) << "processNextCheck, remaining " << mAcctTodo.count() << endl;
  mNewMailArrived |= _newMail;

  for ( AccountList::Iterator it( mAcctChecking.begin() ), end( mAcctChecking.end() ); it != end;  ) {
    KMAccount* acct = *it;
    ++it;
    if ( acct->checkingMail() )
      continue;
    // check done
    kdDebug(5006) << "account " << acct->name() << " finished check" << endl;
    mAcctChecking.remove( acct );
    kmkernel->filterMgr()->deref();
    disconnect( acct, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                      this, SLOT( processNextCheck( bool ) ) );
    QString hostname = hostForAccount( acct );
    if ( !hostname.isEmpty() ) {
      if ( mServerConnections.find( hostname ) != mServerConnections.end() &&
           mServerConnections[hostname] > 0 ) {
        mServerConnections[hostname] -= 1;
        kdDebug(5006) << "connections to server " << hostname
                      << " now " << mServerConnections[hostname] << endl;
      }
    }
  }
  if ( mAcctChecking.isEmpty() ) {
    // all checks finished, display summary
    if ( mDisplaySummary )
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
          mTotalNewMailsArrived );
    emit checkedMail( mNewMailArrived, mInteractive, mTotalNewInFolder );
    mTotalNewMailsArrived = 0;
    mTotalNewInFolder.clear();
    mDisplaySummary = false;
  }
  if ( mAcctTodo.isEmpty() ) return;

  QString accountHostName;

  KMAccount *curAccount = 0;
  for ( AccountList::Iterator it ( mAcctTodo.begin() ), last ( mAcctTodo.end() ); it != last; ) {
    KMAccount *acct = *it;
    ++it;
    accountHostName = hostForAccount(acct);
    kdDebug(5006) << "for host " << accountHostName
                  << " current connections="
                  << (mServerConnections.find(accountHostName)==mServerConnections.end() ? 0 : mServerConnections[accountHostName])
                  << " and limit is " << GlobalSettings::self()->maxConnectionsPerHost()
                  << endl;
    bool connectionLimitForHostReached =
      !accountHostName.isNull() &&
      GlobalSettings::self()->maxConnectionsPerHost() > 0 &&
      mServerConnections.find( accountHostName ) != mServerConnections.end() &&
      mServerConnections[accountHostName] >= GlobalSettings::self()->maxConnectionsPerHost();
    kdDebug(5006) << "connection limit reached: "
                  << connectionLimitForHostReached << endl;
    if ( !acct->checkingMail() && !connectionLimitForHostReached ) {
      curAccount = acct;
      mAcctTodo.remove( acct );
      break;
    }
  }
  if ( !curAccount ) return; // no account or all of them are already checking

  if ( curAccount->type() != "imap" && curAccount->type() != "cachedimap" &&
       curAccount->folder() == 0 ) {
    QString tmp = i18n("Account %1 has no mailbox defined:\n"
        "mail checking aborted;\n"
        "check your account settings.")
      .arg(curAccount->name());
    KMessageBox::information(0,tmp);
    emit checkedMail( false, mInteractive, mTotalNewInFolder );
    mTotalNewMailsArrived = 0;
    mTotalNewInFolder.clear();
    return;
  }

  connect( curAccount, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                this, SLOT( processNextCheck( bool ) ) );

  BroadcastStatus::instance()->setStatusMsg(
      i18n("Checking account %1 for new mail").arg(curAccount->name()));

  kdDebug(5006) << "processing next mail check for " << curAccount->name() << endl;

  curAccount->setCheckingMail( true );
  mAcctChecking.append( curAccount );
  kmkernel->filterMgr()->ref();
  curAccount->processNewMail( mInteractive );

  if ( !accountHostName.isEmpty() ) {
    if ( mServerConnections.find( accountHostName ) != mServerConnections.end() )
      mServerConnections[accountHostName] += 1;
    else
      mServerConnections[accountHostName] = 1;
    kdDebug(5006) << "check mail started - connections for host "
                  << accountHostName << " now is "
                  << mServerConnections[accountHostName] << endl;
  }
}

//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::create( const QString &aType, const QString &aName, uint id )
{
  KMAccount* act = 0;
  if ( id == 0 )
    id = createId();

  if ( aType == "local" ) {
    act = new KMAcctLocal(this, aName.isEmpty() ? i18n("Local Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == "maildir" ) {
    act = new KMAcctMaildir(this, aName.isEmpty() ? i18n("Local Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == "pop" ) {
    act = new KMail::PopAccount(this, aName.isEmpty() ? i18n("POP Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == "imap" ) {
    act = new KMAcctImap(this, aName.isEmpty() ? i18n("IMAP Account") : aName, id);
  } else if (aType == "cachedimap") {
    act = new KMAcctCachedImap(this, aName.isEmpty() ? i18n("IMAP Account") : aName, id);
  }
  if ( !act ) {
      kdWarning(5006) << "Attempt to instantiate a non-existing account type!" << endl;
      return 0;
  }
  connect( act, SIGNAL( newMailsProcessed( const QMap<QString, int> & ) ),
                this, SLOT( addToTotalNewMailCount( const QMap<QString, int> & ) ) );
  return act;
}


//-----------------------------------------------------------------------------
void KMAcctMgr::add( KMAccount *account )
{
  if ( account ) {
    mAcctList.append( account );
    emit accountAdded( account );
    account->installTimer();
  }
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::findByName(const QString &aName)
{
  if ( aName.isEmpty() ) return 0;

  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( (*it)->name() == aName ) return (*it);
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::find( const uint id )
{
  if (id == 0) return 0;
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( (*it)->id() == id ) return (*it);
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::first()
{
    mPtrListInterfaceProxyIterator = mAcctList.begin();
    return *mPtrListInterfaceProxyIterator;
}

//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::next()
{
    ++mPtrListInterfaceProxyIterator;
    if ( mPtrListInterfaceProxyIterator == mAcctList.end() )
        return 0;
    else
        return *mPtrListInterfaceProxyIterator;
}

//-----------------------------------------------------------------------------
bool KMAcctMgr::remove( KMAccount* acct )
{
  if( !acct )
    return false;
  mAcctList.remove( acct );
  emit accountRemoved( acct );
  return true;
}

//-----------------------------------------------------------------------------
void KMAcctMgr::checkMail( bool _interactive )
{
  mNewMailArrived = false;

  if ( mAcctList.isEmpty() ) {
    KMessageBox::information( 0,i18n("You need to add an account in the network "
                    "section of the settings in order to receive mail.") );
    return;
  }
  mDisplaySummary = true;

  mTotalNewMailsArrived=0;
  mTotalNewInFolder.clear();

  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( !(*it)->checkExclude() )
      singleCheckMail( (*it), _interactive);
  }
}


//-----------------------------------------------------------------------------
void KMAcctMgr::singleInvalidateIMAPFolders(KMAccount *account) {
  account->invalidateIMAPFolders();
}


void KMAcctMgr::invalidateIMAPFolders()
{
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it )
    singleInvalidateIMAPFolders( *it );
}


//-----------------------------------------------------------------------------
QStringList  KMAcctMgr::getAccounts()
{
  QStringList strList;
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    strList.append( (*it)->name() );
  }
  return strList;
}

//-----------------------------------------------------------------------------
void KMAcctMgr::intCheckMail(int item, bool _interactive)
{
  mNewMailArrived = false;
  mTotalNewMailsArrived = 0;
  mTotalNewInFolder.clear();
  if ( KMAccount *acct = mAcctList[ item ] )
    singleCheckMail( acct, _interactive );
  mDisplaySummary = false;
}


//-----------------------------------------------------------------------------
void KMAcctMgr::addToTotalNewMailCount( const QMap<QString, int> & newInFolder )
{
  for ( QMap<QString, int>::const_iterator it = newInFolder.begin();
        it != newInFolder.end(); ++it ) {
    mTotalNewMailsArrived += it.data();
    if ( mTotalNewInFolder.find( it.key() ) == mTotalNewInFolder.end() )
      mTotalNewInFolder[it.key()] = it.data();
    else
      mTotalNewInFolder[it.key()] += it.data();
  }
}

//-----------------------------------------------------------------------------
uint KMAcctMgr::createId()
{
  QValueList<uint> usedIds;
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    usedIds << (*it)->id();
  }

  usedIds << 0; // 0 is default for unknown
  int newId;
  do
  {
    newId = kapp->random();
  } while ( usedIds.find(newId) != usedIds.end() );

  return newId;
}

//-----------------------------------------------------------------------------
void KMAcctMgr::cancelMailCheck()
{
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    (*it)->cancelMailCheck();
  }
}

//-----------------------------------------------------------------------------
QString KMAcctMgr::hostForAccount( const KMAccount *acct ) const
{
  const NetworkAccount *net_acct = dynamic_cast<const NetworkAccount*>( acct );
  return net_acct ? net_acct->host() : QString::null;
}

//-----------------------------------------------------------------------------
void KMAcctMgr::readPasswords()
{
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    NetworkAccount *acct = dynamic_cast<NetworkAccount*>( (*it) );
    if ( acct )
      acct->readPassword();
  }
}

#include "kmacctmgr.moc"
