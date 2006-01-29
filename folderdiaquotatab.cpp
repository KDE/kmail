// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdiaquotatab.cpp
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

#include "folderdiaquotatab.h"
#include "kmfolder.h"
#include "kmfoldertype.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "imapaccountbase.h"

#include <qwidgetstack.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>

#include <assert.h>

using namespace KMail;

class FolderDiaQuotaTab::QuotaWidget : public QWidget {
public:
    QuotaWidget::QuotaWidget( QWidget* parent, const char* name = 0 )
        :QWidget( parent, name )
    {
      QVBoxLayout *box = new QVBoxLayout(this);
      QWidget *stuff = new QWidget( this );
      QGridLayout* layout = 
          new QGridLayout( stuff, 3, 3,
                           KDialog::marginHint(),
                           KDialog::spacingHint() );
      mInfoLabel = new QLabel("", stuff );
      mRootLabel = new QLabel("", stuff );
      mProgressBar = new QProgressBar( stuff );
      layout->addWidget( new QLabel( i18n("Root:" ), stuff ), 0, 0 );
      layout->addWidget( mRootLabel, 0, 1 );
      layout->addWidget( new QLabel( i18n("Usage:"), stuff ), 1, 0 );
      //layout->addWidget( new QLabel( i18n("Status:"), stuff ), 2, 0 );
      layout->addWidget( mInfoLabel, 1, 1 );
      layout->addWidget( mProgressBar, 2, 1 );
      box->addWidget( stuff );
      box->addStretch( 2 );
    }

    ~QuotaWidget() { }

    void setQuotaInfo( const QuotaInfo& info )
    {
      // we are assuming only to get STORAGE type info here, thus
      // casting to int is safe
      int current = info.current.toInt();
      int max = info.max.toInt();
      mProgressBar->setProgress( current, max );
      mInfoLabel->setText( i18n("%1 of %2 KB used").arg( current ).arg( max ) );
      mRootLabel->setText( info.root );
    }
private:
    QLabel* mInfoLabel;
    QLabel* mRootLabel;
    QProgressBar* mProgressBar;
};

///////////////////

KMail::FolderDiaQuotaTab::FolderDiaQuotaTab( KMFolderDialog* dlg, QWidget* parent, const char* name )
  : FolderDiaTab( parent, name ),
    mImapAccount( 0 ),
    mDlg( dlg )
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  // We need a widget stack to show either a label ("no qutoa support", "please wait"...)
  // or quota info
  mStack = new QWidgetStack( this );
  topLayout->addWidget( mStack );

  mLabel = new QLabel( mStack );
  mLabel->setAlignment( AlignHCenter | AlignVCenter | WordBreak );
  mStack->addWidget( mLabel );

  mQuotaWidget = new QuotaWidget( mStack );
}


void KMail::FolderDiaQuotaTab::initializeWithValuesFromFolder( KMFolder* folder )
{
  // This can be simplified once KMFolderImap and KMFolderCachedImap have a common base class
  mFolderType = folder->folderType();
  if ( mFolderType == KMFolderTypeImap ) {
    KMFolderImap* folderImap = static_cast<KMFolderImap*>( folder->storage() );
    mImapAccount = folderImap->account();
  //  mQuotaInfo = folderImap->quotaInfo();
  }
  else if ( mFolderType == KMFolderTypeCachedImap ) {
    KMFolderCachedImap* folderImap = static_cast<KMFolderCachedImap*>( folder->storage() );
    mImapAccount = folderImap->account();
    mQuotaInfo = folderImap->quotaInfo();
  }
  else
    assert( 0 ); // see KMFolderDialog constructor
}

void KMail::FolderDiaQuotaTab::load()
{
  if ( mDlg->folder() ) {
    // existing folder
    initializeWithValuesFromFolder( mDlg->folder() );
  } else if ( mDlg->parentFolder() ) {
    // new folder
    initializeWithValuesFromFolder( mDlg->parentFolder() );
  }

  if ( mFolderType == KMFolderTypeCachedImap ) {
    if ( !mQuotaInfo.isValid() ) {
      if ( mImapAccount->hasQuotaSupport() ) {
        mLabel->setText( i18n( "Information not retrieved from server yet, please use \"Check Mail\"." ) );
      } else {
        mLabel->setText( i18n( "This account does not have support for quota information." ) );
      }
    } else {
      if ( mQuotaInfo.current.isValid() ) {
        mStack->raiseWidget( mQuotaWidget );
        mQuotaWidget->setQuotaInfo( mQuotaInfo );
      } else {
        mLabel->setText( i18n( "No quota is set for this folder." ) );
      }
    }
  }
}

KMail::FolderDiaTab::AcceptStatus KMail::FolderDiaQuotaTab::accept()
{
  if ( mFolderType == KMFolderTypeCachedImap )
    return Accepted;
  else
    assert(0);
}

bool KMail::FolderDiaQuotaTab::save()
{
  // nothing to do, we are read-only
  return true;
}

bool KMail::FolderDiaQuotaTab::supports( KMFolder* refFolder )
{
  ImapAccountBase* imapAccount = 0;
  if ( refFolder->folderType() == KMFolderTypeImap )
    imapAccount = static_cast<KMFolderImap*>( refFolder->storage() )->account();
  else if ( refFolder->folderType() == KMFolderTypeCachedImap )
    imapAccount = static_cast<KMFolderCachedImap*>( refFolder->storage() )->account();
  return imapAccount && imapAccount->hasQuotaSupport(); // support for Quotas (or not tried connecting yet)
}

#include "folderdiaquotatab.moc"
