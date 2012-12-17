/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "createnewcontactjob.h"
#include "util.h"

#include <KABC/Addressee>
#include <KABC/ContactGroup>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentTypeDialog>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/Contact/ContactEditorDialog>

CreateNewContactJob::CreateNewContactJob(QWidget *parentWidget, QObject *parent)
    : KJob(parent),
      mParentWidget(parentWidget)
{
}

CreateNewContactJob::~CreateNewContactJob()
{
}

void CreateNewContactJob::start()
{
    Akonadi::CollectionFetchJob * const addressBookJob =
      new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                       Akonadi::CollectionFetchJob::Recursive );

    addressBookJob->fetchScope().setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
    connect( addressBookJob, SIGNAL(result(KJob*)), SLOT(slotCollectionsFetched(KJob*)) );
}


void CreateNewContactJob::slotCollectionsFetched(KJob*job)
{
    if ( job->error() ) {
      setError( job->error() );
      setErrorText( job->errorText() );
      emitResult();
      return;
    }

    const Akonadi::CollectionFetchJob *addressBookJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );

    Akonadi::Collection::List canCreateItemCollections ;

    foreach ( const Akonadi::Collection &collection, addressBookJob->collections() ) {
      if ( Akonadi::Collection::CanCreateItem & collection.rights() ) {
        canCreateItemCollections.append(collection);
      }
    }
    if ( canCreateItemCollections.isEmpty() ) {
        Akonadi::AgentTypeDialog dlg( mParentWidget );
        //TODO add caption
        dlg.agentFilterProxyModel()->addMimeTypeFilter(KABC::Addressee::mimeType());
        dlg.agentFilterProxyModel()->addMimeTypeFilter(KABC::ContactGroup::mimeType());
        dlg.agentFilterProxyModel()->addCapabilityFilter( QLatin1String( "Resource" ) );

        if ( dlg.exec() ) {
            const Akonadi::AgentType agentType = dlg.agentType();

            if ( agentType.isValid() ) {
                Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( agentType, this );
                connect( job, SIGNAL(result(KJob*)), SLOT(slotResourceCreationDone(KJob*)) );
                job->configure( mParentWidget );
                job->start();
                return;
            } else { //if agent is not valid => return error and finish job
                setError( UserDefinedError );
                emitResult();
                return;
            }
        } else { //dialog canceled => return error and finish job
            setError( UserDefinedError );
            emitResult();
            return;
        }
    }
    createContact();
    emitResult();
}

void CreateNewContactJob::slotResourceCreationDone(KJob* job)
{
    if ( job->error() ) {
      setError( job->error() );
      setErrorText( job->errorText() );
      emitResult();
      return;
    }

    createContact();
    emitResult();
}

void CreateNewContactJob::createContact()
{
    Akonadi::ContactEditorDialog dlg( Akonadi::ContactEditorDialog::CreateMode, mParentWidget );
    dlg.exec();
}

#include "createnewcontactjob.moc"
