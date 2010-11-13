/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Montel Laurent <montel@kde.org>

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

#include "kmcollectionpropertiesdialog.h"
#include "collectiontemplatespage.h"
#include "collectionmaintenancepage.h"
#include "collectiongeneralpage.h"
#include "collectionviewpage.h"
#include "collectionquotapage.h"
#include "collectionaclpage.h"

#include <akonadi/cachepolicypage.h>
#include <akonadi/collectionmodifyjob.h>

#include <ktabwidget.h>

#include <QtGui/QBoxLayout>


KMCollectionPropertiesDialog::KMCollectionPropertiesDialog( const Akonadi::Collection &collection, QWidget *parent ) :
  KDialog( parent )
{
  mCollection = collection;
  QBoxLayout *layout = new QHBoxLayout( mainWidget() );
  layout->setMargin( 0 );
  mTabWidget = new KTabWidget( mainWidget() );
  layout->addWidget( mTabWidget );

  Akonadi::CollectionPropertiesPage *page = new CollectionGeneralPage( mTabWidget );
  insertPage( page );
  page = new CollectionViewPage( mTabWidget );
  insertPage( page );
  page = new Akonadi::CachePolicyPage( mTabWidget );
  insertPage( page );
  page = new CollectionTemplatesPage( mTabWidget );
  insertPage( page );
  page = new CollectionAclPage( mTabWidget );
  insertPage( page );
  page = new CollectionQuotaPage( mTabWidget );
  insertPage( page );
  page = new CollectionMaintenancePage( mTabWidget );
  insertPage( page );
  connect( this, SIGNAL( okClicked() ), SLOT( save() ) );
  connect( this, SIGNAL( cancelClicked() ), SLOT( deleteLater() ) );

}

void KMCollectionPropertiesDialog::insertPage( Akonadi::CollectionPropertiesPage * page )
{
  if ( page->canHandle( mCollection ) ) {
    mTabWidget->addTab( page, page->pageTitle() );
    page->load( mCollection );
  } else {
    delete page;
  }
}

void KMCollectionPropertiesDialog::save()
{
  for ( int i = 0; i < mTabWidget->count(); ++i ) {
    Akonadi::CollectionPropertiesPage *page = static_cast<Akonadi::CollectionPropertiesPage*>( mTabWidget->widget( i ) );
    page->save( mCollection );
  }

  Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( mCollection, this );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( saveResult( KJob* ) ) );
}

void KMCollectionPropertiesDialog::saveResult( KJob *job )
{
  if ( job->error() ) {
    // TODO
    kWarning() << job->errorString();
  }
  deleteLater();
}

#include "kmcollectionpropertiesdialog.moc"
