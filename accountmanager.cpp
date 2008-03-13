// KMail Account Manager

#include "accountmanager.h"

#include "kmaccount.h"
#include "kmacctmaildir.h"
#include "kmacctlocal.h"
#include "popaccount.h"
#include "kmacctimap.h"
#include "networkaccount.h"
#include "kmacctcachedimap.h"
#include "broadcaststatus.h"
#include "kmfiltermgr.h"
#include "globalsettings.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <QRegExp>
#include <krandom.h>
#include <kconfiggroup.h>

using namespace KMail;

//-----------------------------------------------------------------------------
AccountManager::AccountManager()
    :QObject(), mNewMailArrived( false ), mInteractive( false ),
     mTotalNewMailsArrived( 0 ), mDisplaySummary( false )
{
  mAcctChecking.clear();
  mAcctTodo.clear();
}

//-----------------------------------------------------------------------------
AccountManager::~AccountManager()
{
  writeConfig( false );
}


//-----------------------------------------------------------------------------
void AccountManager::writeConfig( bool withSync )
{
  KConfig* config = KMKernel::config();

  // Delete all accounts for groups in the config file not having 
  // Enabled=false flag (accountGroups) 
  // and replace them with account groups existing in memory (mAcctList)
  uint accounts = 0;
  QStringList accountGroups =
    config->groupList().filter( QRegExp( "Account \\d+" ) );
  AccountList::ConstIterator enabledAccountIt = mAcctList.constBegin();
  for ( QStringList::ConstIterator it = accountGroups.constBegin() ;; ) {
    QString groupName;
    bool appendNewGroup = false;
    if ( it == accountGroups.constEnd() ) {
      if ( enabledAccountIt == mAcctList.constEnd() )
        break;
      appendNewGroup = true;
      groupName.sprintf( "Account %d", accounts + 1 );
    }
    else {
      groupName = *it;
      ++it;
    }

    KConfigGroup group(config, groupName);
    uint id = group.readEntry( "Id", 0 );
    if ( mDisabledAccounts.contains( id ) )
      accounts++; // do not modify disabled account - skip
    else {
      if ( appendNewGroup ) {
        (*enabledAccountIt)->writeConfig( group );
        ++enabledAccountIt;
        accounts++;
      }
      else // no such account on the list - disabled / enabled
        config->deleteGroup( groupName );
    }
  }

  KConfigGroup group(config, "General");
  group.writeEntry("accounts", accounts);

  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
void AccountManager::readConfig(void)
{
  KConfig* config = KMKernel::config();
  KMAccount* acct;
  QString acctName;
  QString groupName;
  int i, num;

  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it )
      delete *it;
  mAcctList.clear();

  KConfigGroup general(config, "General");
  num = general.readEntry( "accounts", 0 );

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    KConfigGroup group(config, groupName);
    uint id = group.readEntry( "Id", 0 );
    if ( !group.readEntry("Enabled", true) ) {
      mDisabledAccounts += id;
      continue;
    }

    KAccount::Type acctType = KAccount::typeForName( group.readEntry( "Type" ) );
    acctName = group.readEntry("Name");
    if (acctName.isEmpty()) acctName = i18n("Account %1", i);
    acct = create(acctType, acctName, id);
    if (!acct) continue;
    add(acct);
    acct->readConfig(group);
  }
}


//-----------------------------------------------------------------------------
void AccountManager::singleCheckMail(KMAccount *account, bool interactive)
{
  mNewMailArrived = false;
  mInteractive = interactive;

  // queue the account
  mAcctTodo.append(account);

  if (account->checkingMail())
  {
    kDebug(5006) <<"account" << account->name() <<" busy, queuing";
    return;
  }

  processNextCheck( false );
}

//-----------------------------------------------------------------------------
void AccountManager::processNextCheck( bool _newMail )
{
  kDebug(5006) <<"processNextCheck, remaining" << mAcctTodo.count();
  if ( _newMail )
    mNewMailArrived = true;

  for ( AccountList::Iterator it( mAcctChecking.begin() ), end( mAcctChecking.end() ); it != end;  ) {
    KMAccount* acct = *it;
    ++it;
    if ( acct->checkingMail() )
      continue;
    // check done
    kDebug(5006) <<"account" << acct->name() <<" finished check";
    mAcctChecking.removeAll( acct );
    kmkernel->filterMgr()->deref();
    disconnect( acct, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                      this, SLOT( processNextCheck( bool ) ) );
  }
  if ( mAcctChecking.isEmpty() ) {
    // all checks finished, display summary
    if ( mDisplaySummary )
      KPIM::BroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
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
    if ( !acct->checkingMail() && acct->mailCheckCanProceed() ) {
      curAccount = acct;
      mAcctTodo.removeAll( acct );
      break;
    }
  }
  if ( !curAccount ) return; // no account or all of them are already checking

  if ( curAccount->type() != KAccount::Imap && 
       curAccount->type() != KAccount::DImap &&
       curAccount->folder() == 0 ) {
    QString tmp = i18n("Account %1 has no mailbox defined:\n"
        "mail checking aborted;\n"
        "check your account settings.",
       curAccount->name());
    KMessageBox::information(0,tmp);
    emit checkedMail( false, mInteractive, mTotalNewInFolder );
    mTotalNewMailsArrived = 0;
    mTotalNewInFolder.clear();
    return;
  }

  connect( curAccount, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                this, SLOT( processNextCheck( bool ) ) );

  KPIM::BroadcastStatus::instance()->setStatusMsg(
      i18n("Checking account %1 for new mail", curAccount->name()));

  kDebug(5006) <<"processing next mail check for" << curAccount->name();

  curAccount->setCheckingMail( true );
  mAcctChecking.append( curAccount );
  kmkernel->filterMgr()->ref();
  curAccount->processNewMail( mInteractive );
}

//-----------------------------------------------------------------------------
KMAccount* AccountManager::create( const KAccount::Type aType,
                                   const QString &aName, uint id )
{
  KMAccount* act = 0;
  if ( id == 0 )
    id = createId();

  if ( aType == KAccount::Local) {
    act = new KMAcctLocal(this, aName.isEmpty() ? i18n("Local Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == KAccount::Maildir) {
    act = new KMAcctMaildir(this, aName.isEmpty() ? i18n("Local Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == KAccount::Pop) {
    act = new KMail::PopAccount(this, aName.isEmpty() ? i18n("POP Account") : aName, id);
    act->setFolder( kmkernel->inboxFolder() );
  } else if ( aType == KAccount::Imap) {
    act = new KMAcctImap(this, aName.isEmpty() ? i18n("IMAP Account") : aName, id);
  } else if (aType == KAccount::DImap) {
    act = new KMAcctCachedImap(this, aName.isEmpty() ? i18n("IMAP Account") : aName, id);
  }
  if ( !act ) {
      kWarning(5006) <<"Attempt to instantiate a non-existing account type!";
      return 0;
  }
  act->setType( aType );
  connect( act, SIGNAL( newMailsProcessed( const QMap<QString, int> & ) ),
                this, SLOT( addToTotalNewMailCount( const QMap<QString, int> & ) ) );
  return act;
}


//-----------------------------------------------------------------------------
void AccountManager::add( KMAccount *account )
{
  if ( account ) {
    mAcctList.append( account );
    emit accountAdded( account );
    account->installTimer();
  }
}


//-----------------------------------------------------------------------------
KMAccount* AccountManager::findByName(const QString &aName) const
{
  if ( aName.isEmpty() ) return 0;

  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( (*it)->name() == aName ) return (*it);
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* AccountManager::find( const uint id ) const
{
  if (id == 0) return 0;
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( (*it)->id() == id ) return (*it);
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMAccount* AccountManager::first()
{
  if ( !mAcctList.empty() ) {
    mPtrListInterfaceProxyIterator = mAcctList.begin();
    return *mPtrListInterfaceProxyIterator;
  } else {
    return 0;
  }
}

//-----------------------------------------------------------------------------
KMAccount* AccountManager::next()
{
    ++mPtrListInterfaceProxyIterator;
    if ( mPtrListInterfaceProxyIterator == mAcctList.end() )
        return 0;
    else
        return *mPtrListInterfaceProxyIterator;
}

//-----------------------------------------------------------------------------
bool AccountManager::remove( KMAccount* acct )
{
  if( !acct )
    return false;
  mAcctList.removeAll( acct );
  emit accountRemoved( acct );
  return true;
}

//-----------------------------------------------------------------------------
void AccountManager::checkMail( bool _interactive )
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
void AccountManager::singleInvalidateIMAPFolders(KMAccount *account) {
  account->invalidateIMAPFolders();
}


void AccountManager::invalidateIMAPFolders()
{
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it )
    singleInvalidateIMAPFolders( *it );
}


//-----------------------------------------------------------------------------
QStringList  AccountManager::getAccounts() const
{
  QStringList strList;
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    strList.append( (*it)->name() );
  }
  return strList;
}

//-----------------------------------------------------------------------------
void AccountManager::intCheckMail(int item, bool _interactive)
{
  mNewMailArrived = false;
  mTotalNewMailsArrived = 0;
  mTotalNewInFolder.clear();
  if ( KMAccount *acct = mAcctList[ item ] )
    singleCheckMail( acct, _interactive );
  mDisplaySummary = false;
}


//-----------------------------------------------------------------------------
void AccountManager::addToTotalNewMailCount( const QMap<QString, int> & newInFolder )
{
  for ( QMap<QString, int>::const_iterator it = newInFolder.begin();
        it != newInFolder.end(); ++it ) {
    mTotalNewMailsArrived += it.value();
    if ( !mTotalNewInFolder.contains( it.key() )  )
      mTotalNewInFolder[it.key()] = it.value();
    else
      mTotalNewInFolder[it.key()] += it.value();
  }
}

//-----------------------------------------------------------------------------
uint AccountManager::createId()
{
  QList<uint> usedIds;
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    usedIds << (*it)->id();
  }

  usedIds << 0; // 0 is default for unknown
  int newId;
  do
  {
    newId = KRandom::random();
  } while ( usedIds.contains(newId)  );

  return newId;
}

//-----------------------------------------------------------------------------
void AccountManager::cancelMailCheck()
{
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    (*it)->cancelMailCheck();
  }
}


//-----------------------------------------------------------------------------
void AccountManager::readPasswords()
{
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    NetworkAccount *acct = dynamic_cast<NetworkAccount*>( (*it) );
    if ( acct )
      acct->readPassword();
  }
}

#include "accountmanager.moc"
