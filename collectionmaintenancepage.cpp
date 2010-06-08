/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009-2010 Montel Laurent <montel@kde.org>

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
#include <Akonadi/AgentType>
#include <Akonadi/AgentManager>
#include <Akonadi/ChangeRecorder>
#include <klineedit.h>
#include <QLabel>
#include <KDialog>
#include <QGroupBox>
#include <KLocale>
#include <QFormLayout>
#include <kio/global.h>
#include "kmkernel.h"

using namespace Akonadi;


CollectionMaintenancePage::CollectionMaintenancePage(QWidget * parent) :
  CollectionPropertiesPage( parent ), mIsNotAVirtualCollection( true )
{
  setPageTitle(  i18n("Maintenance") );
}

void CollectionMaintenancePage::init(const Akonadi::Collection & col)
{
  mCurrentCollection = col;

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( KDialog::marginHint() );
  QGroupBox *filesGroup = new QGroupBox( i18n("Files"), this );
  QFormLayout *box = new QFormLayout( filesGroup );
  box->setSpacing( KDialog::spacingHint() );
  mIsNotAVirtualCollection = ( col.resource() != QLatin1String( "akonadi_search_resource" ) ) && ( col.resource() != QLatin1String( "akonadi_nepomuktag_resource" ) );
  connect( KMKernel::self()->monitor(), SIGNAL( collectionStatisticsChanged( Akonadi::Collection::Id , const Akonadi::CollectionStatistics & ) ), this, SLOT( updateCollectionStatistic( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ) );



#if 0 //TODO remove it ?
  QString contentsDesc = folderContentDesc( mFolder->storage()->contentsType() );
  QLabel *label = new QLabel( contentsDesc, filesGroup );
  // Passing a QLabel rather than QString to addRow(), so that it doesn't
  // get a buddy set (except in the cases where we do want one).
  box->addRow( new QLabel( i18nc( "@label:textbox Folder content type (eg. Mail)", "Contents:" ),
                           filesGroup ), label );
#endif

  const AgentInstance instance = Akonadi::AgentManager::self()->instance( col.resource() );
  const QString folderDesc = instance.type().name();

  if ( mIsNotAVirtualCollection ) {
    QLabel *label = new QLabel( folderDesc, filesGroup );
    box->addRow( new QLabel( i18n("Folder type:"), filesGroup ), label );

    mCollectionLocation = new KLineEdit( filesGroup );
    mCollectionLocation->setReadOnly( true );
    box->addRow( i18n("Location:"), mCollectionLocation );
  }

  mFolderSizeLabel = new QLabel( i18nc( "folder size", "Not available" ), filesGroup );
  box->addRow( new QLabel( i18n("Size:"), filesGroup ), mFolderSizeLabel );

  topLayout->addWidget( filesGroup );

  QGroupBox *messagesGroup = new QGroupBox( i18n("Messages"), this);
  box = new QFormLayout( messagesGroup );
  box->setSpacing( KDialog::spacingHint() );

  mCollectionCount = new QLabel( messagesGroup );
  box->addRow( new QLabel( i18n("Total messages:"), messagesGroup ), mCollectionCount );

  mCollectionUnread = new QLabel( messagesGroup );
  box->addRow( new QLabel( i18n("Unread messages:"), messagesGroup ), mCollectionUnread );

  topLayout->addWidget( messagesGroup );

  topLayout->addStretch( 100 );
}

void CollectionMaintenancePage::load(const Collection & col)
{
  init( col );
  if ( col.isValid() ) {
    updateLabel( col.statistics().count(), col.statistics().unreadCount(), col.statistics().size() );
    if ( mIsNotAVirtualCollection )
      mCollectionLocation->setText( col.remoteId() );
  }
}

void CollectionMaintenancePage::updateLabel( qint64 nbMail, qint64 nbUnreadMail, qint64 size )
{
  mCollectionCount->setText( QString::number( qMax( 0LL, nbMail ) ) );
  mCollectionUnread->setText( QString::number( qMax( 0LL, nbUnreadMail ) ) );
  mFolderSizeLabel->setText( KGlobal::locale()->formatByteSize( qMax( 0LL, size ) ) );

}

void CollectionMaintenancePage::save(Collection & )
{
  //Nothing (read only)
}

void CollectionMaintenancePage::updateCollectionStatistic(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics& statistic)
{
  if ( id == mCurrentCollection.id() ) {
    updateLabel( statistic.count(), statistic.unreadCount(), statistic.size() );
  }
}

