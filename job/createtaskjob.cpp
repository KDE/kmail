/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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


#include "createtaskjob.h"
#include "attributes/taskattribute.h"
#include <akonadi/kmime/messagestatus.h>

#include <KLocalizedString>

#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>

#include <QDebug>

CreateTaskJob::CreateTaskJob(const Akonadi::Item::List &items, bool revert, QObject *parent)
    : KJob(parent),
      mListItem(items),
      mRevertStatus(revert)
{
}

CreateTaskJob::~CreateTaskJob()
{

}

void CreateTaskJob::start()
{
    if (mListItem.isEmpty()) {
        Q_EMIT emitResult();
        return;
    }
    fetchItems();
}

void CreateTaskJob::fetchItems()
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mListItem, this );
    job->fetchScope().fetchFullPayload( true );
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().fetchAttribute<TaskAttribute>();
    connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchJobDone(KJob*)) );
}

void CreateTaskJob::itemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qDebug() << job->errorString();
        Q_EMIT emitResult();
        return;
    }
    Akonadi::ItemFetchJob *fetchjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List lst = fetchjob->items();
    if (lst.isEmpty()) {
        Q_EMIT emitResult();
        return;
    }
    //TODO
}
