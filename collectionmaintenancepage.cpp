/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "collectionmaintenancepage.h"
#include <akonadi/collectionstatistics.h>
#include <akonadi/collection.h>
#include <klineedit.h>
#include <QLabel>
#include <QHBoxLayout>
#include <KDialog>
#include <QGroupBox>
#include <KLocale>
#include <QFormLayout>
#include <kio/global.h>


using namespace Akonadi;

CollectionMaintenancePage::CollectionMaintenancePage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle(  i18n("Maintenance") );
  init();
}

void CollectionMaintenancePage::init()
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( 0 );
  topLayout->setSpacing( KDialog::spacingHint() );

  QGroupBox *filesGroup = new QGroupBox( i18n("Files"), this );
  QFormLayout *box = new QFormLayout( filesGroup );
  box->setSpacing( KDialog::spacingHint() );

  QString contentsDesc ;
#if 0
  = folderContentDesc( mFolder->storage()->contentsType() );
#endif
  QLabel *label = new QLabel( contentsDesc, filesGroup );
  // Passing a QLabel rather than QString to addRow(), so that it doesn't
  // get a buddy set (except in the cases where we do want one).
  box->addRow( new QLabel( i18nc( "@label:textbox Folder content type (eg. Mail)", "Contents:" ),
                           filesGroup ), label );
#if 0
  KMFolderType folderType = mFolder->folderType();
  QString folderDesc = folderTypeDesc( folderType );
#endif
  QString folderDesc;
  label = new QLabel( folderDesc, filesGroup );
  box->addRow( new QLabel( i18n("Folder type:"), filesGroup ), label );

  mCollectionLocation = new KLineEdit( filesGroup );
  mCollectionLocation->setReadOnly( true );
  box->addRow( i18n("Location:"), mCollectionLocation );

  mFolderSizeLabel = new QLabel( i18nc( "folder size", "Not available" ), filesGroup );
  box->addRow( new QLabel( i18n("Size:"), filesGroup ), mFolderSizeLabel );

#if 0
  if ( folderType == KMFolderTypeMaildir )	// see KMMainWidget::slotTroubleshootMaildir()
  {
    mRebuildIndexButton = new KPushButton( i18n("Recreate Index"), filesGroup );
    QObject::connect( mRebuildIndexButton, SIGNAL(clicked()), SLOT(slotRebuildIndex()) );

    QHBoxLayout *hbl = new QHBoxLayout();	// to get an unstretched, right aligned button
    hbl->addStretch( 1 );
    hbl->addWidget( mRebuildIndexButton );
    box->addRow( QString(), hbl );
  }

  if ( folderType == KMFolderTypeCachedImap )
  {
    mRebuildImapButton = new KPushButton( i18n("Rebuild Local IMAP Cache"), filesGroup );
    QObject::connect( mRebuildImapButton, SIGNAL(clicked()), SLOT(slotRebuildImap()) );

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->addStretch( 1 );
    hbl->addWidget( mRebuildImapButton );
    box->addRow( QString(), hbl );
  }
#endif
  topLayout->addWidget( filesGroup );

  QGroupBox *messagesGroup = new QGroupBox( i18n("Messages"), this);
  box = new QFormLayout( messagesGroup );
  box->setSpacing( KDialog::spacingHint() );

  mCollectionCount = new QLabel( messagesGroup );
  box->addRow( new QLabel( i18n("Total messages:"), messagesGroup ), mCollectionCount );

  mCollectionUnread = new QLabel( messagesGroup );
  box->addRow( new QLabel( i18n("Unread messages:"), messagesGroup ), mCollectionUnread );

  // Compaction is only sensible and currently supported for mbox folders.
  //
  // FIXME: should "compaction supported" be an attribute of the folder
  // storage type (mFolder->storage()->isCompactionSupported() or something
  // like that)?
#if 0
  if ( folderType == KMFolderTypeMbox )			// compaction only sensible for this
  {
    mCompactStatusLabel = new QLabel( i18nc( "compaction status", "Unknown" ), messagesGroup );
    box->addRow( new QLabel( i18n("Compaction:"), messagesGroup ), mCompactStatusLabel );

    mCompactNowButton = new KPushButton( i18n("Compact Now"), messagesGroup );
    mCompactNowButton->setEnabled( false );
    connect( mCompactNowButton, SIGNAL(clicked()), SLOT(slotCompactNow()) );

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->addStretch( 1 );
    hbl->addWidget( mCompactNowButton );
    box->addRow( QString(), hbl );
  }
#endif
  topLayout->addWidget( messagesGroup );

  topLayout->addStretch( 100 );
}


#if 0

void FolderDialogMaintenanceTab::updateControls()
{
  if ( mCompactStatusLabel )
  {
    QString s;
    if ( mFolder->needsCompacting() )
    {
      if ( mFolder->storage()->compactable() ) s = i18nc( "compaction status", "Possible");
      else s = i18nc( "compaction status", "Possible, but unsafe");
    }
    else s = i18nc( "compaction status", "Not required");
    mCompactStatusLabel->setText( s );
  }

  if ( mCompactNowButton )
    mCompactNowButton->setEnabled( mFolder->needsCompacting() );
}

void FolderDialogMaintenanceTab::updateFolderIndexSizes()
{
  qint64 folderSize = mFolder->storage()->folderSize();
  if ( folderSize != -1 )
  {
    mFolderSizeLabel->setText( convertSizeWithBytes( folderSize ) );
  }

  KUrl u;
  u.setPath( mFolder->indexLocation() );
  if ( u.isValid() && u.isLocalFile() )
  {
    // the index should always be a file
    QFileInfo fi( u.toLocalFile() );
    mIndexSizeLabel->setText( convertSizeWithBytes( fi.size() ) );
  }
}
#endif

void CollectionMaintenancePage::slotCompactNow()
{
#if 0
  if ( !mFolder->needsCompacting() ) return;

  if ( !mFolder->storage()->compactable() )
  {
    if ( KMessageBox::warningContinueCancel( this,
                                             i18nc( "@info",
      "Compacting folder <resource>%1</resource> may not be safe.<nl/>"
      "<warning>This may result in index or mailbox corruption.</warning><nl/>"
      "Ensure that you have a recent backup of the mailbox and messages.", mFolder->label() ),
                                             i18nc( "@title", "Really compact folder?" ),
                                             KGuiItem( i18nc( "@action:button", "Compact Folder" ) ),
                                             KStandardGuiItem::cancel(), QString(),
                                             KMessageBox::Notify | KMessageBox::Dangerous )
                                             != KMessageBox::Continue ) return;
    mFolder->storage()->enableCompaction();
  }

  // finding and activating the action, because
  // KMMainWidget::slotCompactFolder() and similar actions are protected
  QAction *act = kmkernel->getKMMainWidget()->action( "compact" );
  if ( act ) act->activate( QAction::Trigger );

  updateControls();
  updateFolderIndexSizes();
#endif
}


void CollectionMaintenancePage::slotRebuildIndex()
{
#if 0
  QAction *act = kmkernel->getKMMainWidget()->action( "troubleshoot_maildir" );
  if ( !act ) return;

  act->activate( QAction::Trigger );
  updateFolderIndexSizes();
#endif
}


void CollectionMaintenancePage::slotRebuildImap()
{
#if 0
  QAction *act = kmkernel->getKMMainWidget()->action( "troubleshoot_folder" );
  if ( act ) act->activate( QAction::Trigger );
#endif
}


void CollectionMaintenancePage::load(const Collection & col)
{
  if ( col.isValid() ) {
    mCollectionCount->setText( QString::number( col.statistics().count() ) );
    mCollectionUnread->setText( QString::number( col.statistics().unreadCount() ));
    mFolderSizeLabel->setText( KGlobal::locale()->formatByteSize( col.statistics().size() ) );

    mCollectionLocation->setText( col.remoteId() );

  }
}

void CollectionMaintenancePage::save(Collection & )
{
  //Nothing (read only)
}

