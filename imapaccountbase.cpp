/** -*- c++ -*-
 * imapaccountbase.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 * Copyright (c) 2002 Marc Mutz <mutz@kde.org>
 *
 * This file is based on work on pop3 and imap account implementations
 * by Don Sanders <sanders@kde.org> and Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "imapaccountbase.h"
using KMail::SieveConfig;

#include "kmacctmgr.h"
#include "kmfolder.h"

#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/global.h>
using KIO::MetaData;
#include <kio/passdlg.h>
using KIO::PasswordDialog;
#include <kio/scheduler.h>
//using KIO::Scheduler; // use FQN below

#include <qregexp.h>

namespace KMail {

  static const unsigned short int imapDefaultPort = 143;

  //
  //
  // Ctor and Dtor
  //
  //

  ImapAccountBase::ImapAccountBase( KMAcctMgr * parent, const QString & name )
    : base( parent, name ),
      mPrefix( "/" ),
      mTotal( 0 ),
      mCountUnread( 0 ),
      mCountLastUnread( 0 ),
      mCountRemainChecks( 0 ),
      mAutoExpunge( true ),
      mHiddenFolders( false ),
      mOnlySubscribedFolders( false ),
      mProgressEnabled( false ),
      mIdle( true )
  {
    mPort = imapDefaultPort;
  }

  ImapAccountBase::~ImapAccountBase() {
    kdWarning( mSlave, 5006 )
      << "slave should have been destroyed by subclass!" << endl;
  }

  void ImapAccountBase::init() {
    mPrefix = '/';
    mAutoExpunge = true;
    mHiddenFolders = false;
    mOnlySubscribedFolders = false;
    mProgressEnabled = false;
  }

  void ImapAccountBase::pseudoAssign( const KMAccount * a ) {
    base::pseudoAssign( a );

    const ImapAccountBase * i = dynamic_cast<const ImapAccountBase*>( a );
    if ( !i ) return;

    setPrefix( i->prefix() );
    setAutoExpunge( i->autoExpunge() );
    setHiddenFolders( i->hiddenFolders() );
    setOnlySubscribedFolders( i->onlySubscribedFolders() );
  }

  unsigned short int ImapAccountBase::defaultPort() const {
    return imapDefaultPort;
  }

  QString ImapAccountBase::protocol() const {
    return useSSL() ? "imaps" : "imap";
  }

  //
  //
  // Getters and Setters
  //
  //

  void ImapAccountBase::setPrefix( const QString & prefix ) {
    mPrefix = prefix;
    mPrefix.remove( QRegExp( "[%*\"]" ) );
    if ( mPrefix.isEmpty() || mPrefix[0] != '/' )
      mPrefix.prepend( '/' );
    if ( mPrefix[ mPrefix.length() - 1 ] != '/' )
      mPrefix += '/';
#if 1
    setPrefixHook(); // ### needed while KMFolderCachedImap exists
#else
    if ( mFolder ) mFolder->setImapPath( mPrefix );
#endif
  }

  void ImapAccountBase::setAutoExpunge( bool expunge ) {
    mAutoExpunge = expunge;
  }

  void ImapAccountBase::setHiddenFolders( bool show ) {
    mHiddenFolders = show;
  }

  void ImapAccountBase::setOnlySubscribedFolders( bool show ) {
    mOnlySubscribedFolders = show;
  }

  //
  //
  // read/write config
  //
  //

  void ImapAccountBase::readConfig( /*const*/ KConfig/*Base*/ & config ) {
    base::readConfig( config );

    setPrefix( config.readEntry( "prefix", "/" ) );
    setAutoExpunge( config.readBoolEntry( "auto-expunge", false ) );
    setHiddenFolders( config.readBoolEntry( "hidden-folders", false ) );
    setOnlySubscribedFolders( config.readBoolEntry( "subscribed-folders", false ) );
  }
  
  void ImapAccountBase::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
    base::writeConfig( config );
    
    config.writeEntry( "prefix", prefix() );
    config.writeEntry( "auto-expunge", autoExpunge() );
    config.writeEntry( "hidden-folders", hiddenFolders() );
    config.writeEntry( "subscribed-folders", onlySubscribedFolders() );
  }

  //
  //
  // Network processing
  //
  //

  MetaData ImapAccountBase::slaveConfig() const {
    MetaData m = base::slaveConfig();

    m.insert( "auth", auth() );
    if ( autoExpunge() )
      m.insert( "expunge", "auto" );

    return m;
  }

  bool ImapAccountBase::makeConnection() {
    if ( mSlave ) return true;

    if( mAskAgain || passwd().isEmpty() || login().isEmpty() ) {
      QString log = login();
      QString pass = passwd();
      // We init "store" to true to indicate that we want to have the
      // "keep password" checkbox. Then, we set [Passwords]Keep to
      // storePasswd(), so that the checkbox in the dialog will be
      // init'ed correctly:
      bool store = true;
      KConfigGroup passwords( KGlobal::config(), "Passwords" );
      passwords.writeEntry( "Keep", storePasswd() );
      QString msg = i18n("You need to supply a username and a password to "
			 "access this mailbox.");
      if ( PasswordDialog::getNameAndPassword( log, pass, &store, msg, false,
					       QString::null, name(),
					       i18n("Account:") )
	   != QDialog::Accepted ) {
	emit finishedCheck( false );
	emit newMailsProcessed( 0 ); // taken from kmacctexppop
	return false;
      }
      // The user has been given the chance to change login and
      // password, so copy both from the dialog:
      setPasswd( pass, store );
      setLogin( log );
      mAskAgain = false; // ### taken from kmacctexppop
    }

    mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
    if ( !mSlave ) {
      KMessageBox::error(0, i18n("Could not start process for %1.")
			 .arg( getUrl().protocol() ) );
      return false;
    }

    return true;
  }

  void ImapAccountBase::postProcessNewMail( KMFolder * folder ) {

    disconnect( folder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		this, SLOT(postProcessNewMail(KMFolder*)) );

    mCountRemainChecks--;

    // count the unread messages
    mCountUnread += folder->countUnread();
    if (mCountRemainChecks == 0) {
      // all checks are done
      if (mCountUnread > 0 && mCountUnread > mCountLastUnread) {
	emit finishedCheck(true);
	mCountLastUnread = mCountUnread;
      } else {
	emit finishedCheck(false);
      }
      mCountUnread = 0;
    }
  }


}; // namespace KMail

#include "imapaccountbase.moc"
