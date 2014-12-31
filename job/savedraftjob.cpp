/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "savedraftjob.h"
#include <akonadi/kmime/messagestatus.h>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>

SaveDraftJob::SaveDraftJob(const KMime::Message::Ptr &msg, const Akonadi::Collection &col, QObject *parent)
    : KJob(parent),
      mMsg(msg),
      mCollection(col)
{
}

SaveDraftJob::~SaveDraftJob()
{
}

void SaveDraftJob::start()
{
    Akonadi::Item item;
    item.setPayload( mMsg );
    item.setMimeType( KMime::Message::mimeType() );
    Akonadi::MessageStatus status;
    status.setRead();
    item.setFlags( status.statusFlags() );
    Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob( item, mCollection );
    connect( createJob, SIGNAL(result(KJob*)), SLOT(slotStoreDone(KJob*)) );
}

void SaveDraftJob::slotStoreDone(KJob *job)
{
    if ( job->error() ) {
        qDebug()<<" job->errorString() : "<<job->errorString();
        setError( job->error() );
        setErrorText( job->errorText() );
    }
    emitResult();
}
