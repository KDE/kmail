/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "manageshowcollectionproperties.h"
#include "kmmainwidget.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <akonadi/collectionattributessynchronizationjob.h>
#include <Solid/Networking>


#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionpropertiesdialog.h>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>

Q_DECLARE_METATYPE(KPIM::ProgressItem*)
Q_DECLARE_METATYPE(Akonadi::Job*)
Q_DECLARE_METATYPE(QPointer<KPIM::ProgressItem>)

ManageShowCollectionProperties::ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent)
    : QObject(parent),
      mMainWidget(mainWidget)
{

}

ManageShowCollectionProperties::~ManageShowCollectionProperties()
{

}

void ManageShowCollectionProperties::slotCollectionProperties()
{
    showCollectionProperties( QString() );
}

void ManageShowCollectionProperties::slotShowExpiryProperties()
{
    showCollectionProperties( QLatin1String( "MailCommon::CollectionExpiryPage" ) );
}

void ManageShowCollectionProperties::slotFolderMailingListProperties()
{
    showCollectionProperties( QLatin1String( "KMail::CollectionMailingListPage" ) );
}

void ManageShowCollectionProperties::slotShowFolderShortcutDialog()
{
    showCollectionProperties( QLatin1String( "KMail::CollectionShortcutPage" ) );
}


void ManageShowCollectionProperties::showCollectionProperties( const QString &pageToShow )
{
    if ( !mMainWidget->currentFolder() )
        return;

    if ( Solid::Networking::status() == Solid::Networking::Unconnected ) {

        KMessageBox::information(
                    mMainWidget,
                    i18n( "Network is unconnected. Folder information cannot be updated." ) );
        showCollectionPropertiesContinued( pageToShow, QPointer<KPIM::ProgressItem>() );
    } else {
        const Akonadi::AgentInstance agentInstance = Akonadi::AgentManager::self()->instance( mMainWidget->currentFolder()->collection().resource() );
        bool isOnline = agentInstance.isOnline();
        if (!isOnline) {
            showCollectionPropertiesContinued( pageToShow, QPointer<KPIM::ProgressItem>() );
        } else {
            QPointer<KPIM::ProgressItem> progressItem( KPIM::ProgressManager::createProgressItem( i18n( "Retrieving folder properties" ) ) );
            progressItem->setUsesBusyIndicator( true );
            progressItem->setCryptoStatus(KPIM::ProgressItem::Unknown);

            Akonadi::CollectionAttributesSynchronizationJob *sync
                    = new Akonadi::CollectionAttributesSynchronizationJob( mMainWidget->currentFolder()->collection() );
            sync->setProperty( "collectionId", mMainWidget->currentFolder()->collection().id() );
            sync->setProperty( "pageToShow", pageToShow );        // note for dialog later
            sync->setProperty( "progressItem", QVariant::fromValue( progressItem ) );
            connect( sync, SIGNAL(result(KJob*)),
                     this, SLOT(slotCollectionPropertiesContinued(KJob*)) );
            connect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                     sync, SLOT(kill()) );
            connect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                     KPIM::ProgressManager::instance(), SLOT(slotStandardCancelHandler(KPIM::ProgressItem*)) );
            sync->start();
        }
    }
}

void ManageShowCollectionProperties::slotCollectionPropertiesContinued( KJob* job )
{
    QString pageToShow;
    QPointer<KPIM::ProgressItem> progressItem;

    if ( job ) {
        Akonadi::CollectionAttributesSynchronizationJob *sync
                = dynamic_cast<Akonadi::CollectionAttributesSynchronizationJob *>( job );
        Q_ASSERT( sync );
        if ( sync->property( "collectionId" ) != mMainWidget->currentFolder()->collection().id() )
            return;
        pageToShow = sync->property( "pageToShow" ).toString();
        progressItem = sync->property( "progressItem" ).value< QPointer<KPIM::ProgressItem> >();
        if ( progressItem ) {
            disconnect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                        sync, SLOT(kill()) );
        } else {
            // progressItem does not exist anymore, operation has been canceled
            return;
        }
    }

    showCollectionPropertiesContinued( pageToShow, progressItem );
}

void ManageShowCollectionProperties::showCollectionPropertiesContinued( const QString &pageToShow, QPointer<KPIM::ProgressItem> progressItem )
{
    if ( !progressItem ) {
        progressItem = KPIM::ProgressManager::createProgressItem( i18n( "Retrieving folder properties" ) );
        progressItem->setUsesBusyIndicator( true );
        progressItem->setCryptoStatus(KPIM::ProgressItem::Unknown);
        connect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
                 KPIM::ProgressManager::instance(), SLOT(slotStandardCancelHandler(KPIM::ProgressItem*)) );
    }

    Akonadi::CollectionFetchJob *fetch = new Akonadi::CollectionFetchJob( mMainWidget->currentFolder()->collection(),
                                                                          Akonadi::CollectionFetchJob::Base );
    connect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)), fetch, SLOT(kill()) );
    fetch->fetchScope().setIncludeStatistics( true );
    fetch->setProperty( "pageToShow", pageToShow );
    fetch->setProperty( "progressItem", QVariant::fromValue( progressItem ) );
    connect( fetch, SIGNAL(result(KJob*)),
             this, SLOT(slotCollectionPropertiesFinished(KJob*)) );
    connect( progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
             fetch, SLOT(kill()) );
}

void ManageShowCollectionProperties::slotCollectionPropertiesFinished( KJob *job )
{
    if ( !job )
        return;

    QPointer<KPIM::ProgressItem> progressItem = job->property( "progressItem" ).value< QPointer<KPIM::ProgressItem> >();
    // progressItem does not exist anymore, operation has been canceled
    if ( !progressItem ) {
        return;
    }

    progressItem->setComplete();
    progressItem->setStatus( i18n( "Done" ) );

    Akonadi::CollectionFetchJob *fetch = dynamic_cast<Akonadi::CollectionFetchJob *>( job );
    Q_ASSERT( fetch );
    if ( fetch->collections().isEmpty() )
    {
        kWarning() << "no collection";
        return;
    }

    const Akonadi::Collection collection = fetch->collections().first();

    const QStringList pages = QStringList() << QLatin1String( "MailCommon::CollectionGeneralPage" )
                                            << QLatin1String( "KMail::CollectionViewPage" )
                                            << QLatin1String( "Akonadi::CachePolicyPage" )
                                            << QLatin1String( "KMail::CollectionTemplatesPage" )
                                            << QLatin1String( "MailCommon::CollectionExpiryPage" )
                                            << QLatin1String( "PimCommon::CollectionAclPage" )
                                            << QLatin1String( "KMail::CollectionMailingListPage" )
                                            << QLatin1String( "KMail::CollectionQuotaPage" )
                                            << QLatin1String( "KMail::CollectionShortcutPage" )
                                            << QLatin1String( "KMail::CollectionMaintenancePage" );

    Akonadi::CollectionPropertiesDialog *dlg = new Akonadi::CollectionPropertiesDialog( collection, pages, mMainWidget );
    dlg->setCaption( i18nc( "@title:window", "Properties of Folder %1", collection.name() ) );


    const QString pageToShow = fetch->property( "pageToShow" ).toString();
    if ( !pageToShow.isEmpty() ) {                        // show a specific page
        dlg->setCurrentPage( pageToShow );
    }
    dlg->show();
}
