// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdialogquotatab.cpp
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 *
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "folderdialogquotatab.h"
#include "kmfolder.h"
#include "kmfoldertype.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "imapaccountbase.h"

#include <qstackedwidget.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>

#include "folderdialogquotatab_p.h"

#include <assert.h>

using namespace KMail;

KMail::FolderDialogQuotaTab::FolderDialogQuotaTab( KMFolderDialog* dlg, QWidget* parent, const char* name )
  : FolderDialogTab( parent, name ),
    mImapAccount( 0 ),
    mDlg( dlg )
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  // We need a widget stack to show either a label ("no qutoa support", "please wait"...)
  // or quota info
  mStack = new QStackedWidget( this );
  topLayout->addWidget( mStack );

  mLabel = new QLabel( mStack );
  mLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  mLabel->setWordWrap( true );
  mStack->addWidget( mLabel );

  mQuotaWidget = new KMail::QuotaWidget( mStack );
}


void KMail::FolderDialogQuotaTab::initializeWithValuesFromFolder( KMFolder* folder )
{
  // This can be simplified once KMFolderImap and KMFolderCachedImap have a common base class
  mFolderType = folder->folderType();
  if ( mFolderType == KMFolderTypeImap ) {
    KMFolderImap* folderImap = static_cast<KMFolderImap*>( folder->storage() );
    mImapAccount = folderImap->account();
    mImapPath = folderImap->imapPath();
  }
  else if ( mFolderType == KMFolderTypeCachedImap ) {
    KMFolderCachedImap* folderImap = static_cast<KMFolderCachedImap*>( folder->storage() );
    mImapAccount = folderImap->account();
    mQuotaInfo = folderImap->quotaInfo();
  }
  else
    assert( 0 ); // see KMFolderDialog constructor
}

void KMail::FolderDialogQuotaTab::load()
{
  if ( mDlg->folder() ) {
    // existing folder
    initializeWithValuesFromFolder( mDlg->folder() );
  } else if ( mDlg->parentFolder() ) {
    // new folder
    initializeWithValuesFromFolder( mDlg->parentFolder() );
  }

  if ( mFolderType == KMFolderTypeCachedImap ) {
    showQuotaWidget();
    return;
  }

  assert( mFolderType == KMFolderTypeImap );

  // Loading, for online IMAP, consists of two steps:
  // 1) connect
  // 2) get quota info

  // First ensure we are connected
  mStack->setCurrentWidget( mLabel );
  if ( !mImapAccount ) { // hmmm?
    mLabel->setText( i18n( "Error: no IMAP account defined for this folder" ) );
    return;
  }
  KMFolder* folder = mDlg->folder() ? mDlg->folder() : mDlg->parentFolder();
  if ( folder && folder->storage() == mImapAccount->rootFolder() )
    return; // nothing to be done for the (virtual) account folder
  mLabel->setText( i18n( "Connecting to server %1, please wait...", mImapAccount->host() ) );
  ImapAccountBase::ConnectionState state = mImapAccount->makeConnection();
  if ( state == ImapAccountBase::Error ) { // Cancelled by user, or slave can't start
    slotConnectionResult( -1, QString() );
  } else if ( state == ImapAccountBase::Connecting ) {
    connect( mImapAccount, SIGNAL( connectionResult(int, const QString&) ),
             this, SLOT( slotConnectionResult(int, const QString&) ) );
  } else { // Connected
    slotConnectionResult( 0, QString() );
  }

}

void KMail::FolderDialogQuotaTab::slotConnectionResult( int errorCode, const QString& errorMsg )
{
  disconnect( mImapAccount, SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( slotConnectionResult(int, const QString&) ) );
  if ( errorCode ) {
    if ( errorCode == -1 )  // unspecified error
      mLabel->setText( i18n( "Error connecting to server %1", mImapAccount->host() ) );
    else
      // Connection error (error message box already shown by the account)
      mLabel->setText( KIO::buildErrorString( errorCode, errorMsg ) );
    return;
  }
  connect( mImapAccount, SIGNAL( receivedStorageQuotaInfo( KMFolder*, KIO::Job*, const KMail::QuotaInfo& ) ),
          this, SLOT( slotReceivedQuotaInfo( KMFolder*, KIO::Job*, const KMail::QuotaInfo& ) ) );
  KMFolder* folder = mDlg->folder() ? mDlg->folder() : mDlg->parentFolder();
  mImapAccount->getStorageQuotaInfo( folder, mImapPath );
}

void KMail::FolderDialogQuotaTab::slotReceivedQuotaInfo( KMFolder* folder,
                                                      KIO::Job* job,
                                                      const KMail::QuotaInfo& info )
{
  if ( folder == mDlg->folder() ? mDlg->folder() : mDlg->parentFolder() ) {
    //KMFolderImap* folderImap = static_cast<KMFolderImap*>( folder->storage() );

    disconnect( mImapAccount, SIGNAL(receivedStorageQuotaInfo( KMFolder*, KIO::Job*, const KMail::QuotaInfo& )),
                this, SLOT(slotReceivedQuotaInfo( KMFolder*, KIO::Job*, const KMail::QuotaInfo& )) );

    if ( job && job->error() ) {
      if ( job->error() == KIO::ERR_UNSUPPORTED_ACTION )
        mLabel->setText( i18n( "This account does not have support for quota information." ) );
      else
        mLabel->setText( i18n( "Error retrieving quota information from server\n%1", job->errorString() ) );
    } else {
      mQuotaInfo = info;
    }
    showQuotaWidget();
  }
}

void KMail::FolderDialogQuotaTab::showQuotaWidget()
{
  if ( !mQuotaInfo.isValid() ) {
    if ( !mImapAccount->hasQuotaSupport() ) {
      mLabel->setText( i18n( "This account does not have support for quota information." ) );
    }
  } else {
    if ( !mQuotaInfo.isEmpty() ) {
      mStack->setCurrentWidget( mQuotaWidget );
      mQuotaWidget->setQuotaInfo( mQuotaInfo );
    } else {
      mLabel->setText( i18n( "No quota is set for this folder." ) );
    }
  }
}


KMail::FolderDialogTab::AcceptStatus KMail::FolderDialogQuotaTab::accept()
{
  if ( mFolderType == KMFolderTypeCachedImap || mFolderType == KMFolderTypeImap )
    return Accepted;
  else
    assert(0);
  return Canceled;
}

bool KMail::FolderDialogQuotaTab::save()
{
  // nothing to do, we are read-only
  return true;
}

bool KMail::FolderDialogQuotaTab::supports( KMFolder* refFolder )
{
  ImapAccountBase* imapAccount = 0;
  if ( refFolder->folderType() == KMFolderTypeImap )
    imapAccount = static_cast<KMFolderImap*>( refFolder->storage() )->account();
  else if ( refFolder->folderType() == KMFolderTypeCachedImap )
    imapAccount = static_cast<KMFolderCachedImap*>( refFolder->storage() )->account();
  return imapAccount && imapAccount->hasQuotaSupport(); // support for Quotas (or not tried connecting yet)
}

#include "folderdialogquotatab.moc"
