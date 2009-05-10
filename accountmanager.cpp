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
  qDeleteAll(mAcctList);
}

//-----------------------------------------------------------------------------
QStringList AccountManager::accountGroups() const
{
  return KMKernel::config()->groupList().filter( QRegExp( "Account \\d+" ) );
}

//-----------------------------------------------------------------------------
void AccountManager::writeConfig( bool withSync )
{
  KConfig* config = KMKernel::config();

  QStringList accountGroupsInConfig = accountGroups();
  QStringList accountGroupsToKeep;

  // Write all account config groups to the config file and remember
  // the config group names
  foreach( KMAccount *account, mAcctList ) {
    uint accountId = account->id();
    QString groupName = QString( "Account %1" ).arg( accountId );
    accountGroupsToKeep += groupName;
    KConfigGroup group( config, groupName );
    account->writeConfig( group );
  }

  // Now, delete all config groups with "Account" in them which don't
  // belong to the accounts we just saved (these are deleted accounts, then
  // NOTE: This has to be done _after_ writing out the accounts, otherwise
  //       there is the risk of data loss, see bug 169166
  foreach( const QString &groupName, accountGroupsInConfig ) {
    if ( !accountGroupsToKeep.contains( groupName ) )
      config->deleteGroup( groupName );
  }

  if ( withSync )
    config->sync();
}


//-----------------------------------------------------------------------------
void AccountManager::readConfig(void)
{
  // Delete all in-memory accounts
  for ( AccountList::Iterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it )
      delete *it;
  mAcctList.clear();

  // Now loop over all account groups and load the accounts in them
  KConfig* config = KMKernel::config();
  QStringList accountGroupNames = accountGroups();
  int accountNum = 1;
  foreach( const QString &accountGroupName, accountGroupNames ) {
    KConfigGroup group( config, accountGroupName );
    uint id = group.readEntry( "Id", 0 );
    KAccount::Type acctType = KAccount::typeForName( group.readEntry( "Type" ) );
    QString accountName = group.readEntry( "Name" );

    // Fixes for config compatibilty: We do have an update script, but that is not reliable, because
    // sometimes the scripts don't run (or already run in KDE3, but don't actually change anything)
    // This is a big problem, since incorrect account type values make you loose your account settings,
    // which in case of disconnected IMAP means you'll loose your cache.
    // So for extra saftey, convert the account type values here as well.
    // See also kmail-4.0-misc.pl
    if ( acctType == -1 ) {
      kWarning() << "Config upgrade failed! Danger! Trying to convert old account type for account" << accountName;
      const QString accountTypeName = group.readEntry( "Type" );
      if ( accountTypeName == "cachedimap" )
        acctType = KAccount::DImap;
      if ( accountTypeName == "pop" )
        acctType = KAccount::Pop;
      if ( accountTypeName == "local" )
        acctType = KAccount::Local;
      if ( accountTypeName == "maildir" )
        acctType = KAccount::Maildir;
      if ( accountTypeName == "imap" )
        acctType = KAccount::Imap;
      if ( acctType == -1 )
        kWarning() << "Unrecognized account type. Your account settings are now lost, sorry.";
      else
        kDebug() << "Account type upgraded.";
    }

    if ( accountName.isEmpty() )
      accountName = i18n( "Account %1", accountNum++ );
    KMAccount *account = create( acctType, accountName, id );
    if ( !account )
      continue;
    add( account );
    account->readConfig( group );
  }
}


//-----------------------------------------------------------------------------
void AccountManager::singleCheckMail(KMAccount *account, bool interactive)
{
  mNewMailArrived = false;
  mInteractive = interactive;

 // if sync has been requested by the user then check if check-interval was disabled by user, if yes, then 
 // de-install the timer
 // Safe guard against an infinite sync loop (kolab/issue2607)
  if ( mInteractive ) 
      account->readTimerConfig();

  // queue the account
  mAcctTodo.append(account);

  if (account->checkingMail())
  {
    kDebug() <<"account" << account->name() <<" busy, queuing";
    return;
  }

  processNextCheck( false );
}

//-----------------------------------------------------------------------------
void AccountManager::processNextCheck( bool _newMail )
{
  kDebug() <<"processNextCheck, remaining" << mAcctTodo.count();
  if ( _newMail )
    mNewMailArrived = true;

  for ( AccountList::Iterator it( mAcctChecking.begin() ), end( mAcctChecking.end() ); it != end;  ) {
    KMAccount* acct = *it;
    ++it;
    if ( acct->checkingMail() )
      continue;
    // check done
    kDebug() <<"account" << acct->name() <<" finished check";
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

  kDebug() <<"processing next mail check for" << curAccount->name();

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
      kWarning() <<"Attempt to instantiate a non-existing account type!";
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

    // KMAccount::setFolder has a side
    // effect, it calls KMAcctFolder::addAccount which initialises the
    // folder's account list (KMFolder::mAcctList). This list is then checked
    // in FolderTreeBase::hideLocalInbox (via KMFolder::hasAccounts) to
    // decide whether the inbox is to be shown.
    //
    // Unless this is done, the folder's account list is never initialised
    // with the newly created account.
    // This fixes bug 168544.
    KMFolder *folder = account->folder();
    if ( folder && !folder->hasAccounts() )
      account->setFolder( folder, true );

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
QList<KMAccount*>::iterator AccountManager::begin()
{
  return mAcctList.begin();
}

//-----------------------------------------------------------------------------
QList<KMAccount*>::iterator AccountManager::end()
{
  return mAcctList.end();
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

//-----------------------------------------------------------------------------
bool AccountManager::isUnique( const QString &aName ) const
{
  for ( AccountList::ConstIterator it( mAcctList.begin() ), end( mAcctList.end() ); it != end; ++it ) {
    if ( (*it)->name() == aName ) return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
QString AccountManager::makeUnique( const QString &name ) const
{
  int suffix = 1;
  QString result = name;
  while ( getAccounts().contains( result ) ) {
    result = i18nc( "%1: name; %2: number appended to it to make it unique"
                    "among a list of names", "%1 #%2",
                    name, suffix );
    suffix++;
  }
  return result;
}

#include "accountmanager.moc"
