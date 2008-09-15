/**
 * quotajobs.cpp
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
#include "quotajobs.h"
#include <kio/scheduler.h>
#include <kdebug.h>

using namespace KMail;

QuotaJobs::GetQuotarootJob* QuotaJobs::getQuotaroot(
    KIO::Slave* slave, const KUrl& url )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, IO_WriteOnly );
  stream << (int)'Q' << (int)'R' << url;

  GetQuotarootJob* job = new GetQuotarootJob( url, packedArgs);
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

QuotaJobs::GetQuotarootJob::GetQuotarootJob( const KUrl& url,
                                             const QByteArray &packedArgs)
  : KIO::SpecialJob( url, packedArgs)
{
  connect( this, SIGNAL(infoMessage(KJob*,const QString&,const QString&)),
           SLOT(slotInfoMessage(KJob*,const QString&,const QString&)) );
}

void QuotaJobs::GetQuotarootJob::slotInfoMessage( KJob*, const QString& str,
                                                  const QString& )
{
  // Parse the result
  QStringList results = str.split('\r');
  QStringList roots;
  QuotaInfoList quotas;
  if ( results.size() > 0 ) {
    // the first line is the available roots
    roots = results.front().split( ' ' );
    results.pop_front();
    // the rest are pairs of root -> list of triplets
    while ( results.size() > 0 ) {
      QString root = results.front(); results.pop_front();
      // and the quotas
      if ( results.size() > 0 ) {
        QStringList triplets = results.front().split( ' ' );
        results.pop_front();
        while ( triplets.size() >= 3 ) {
          // there's always three, the label, current and max values
          QString name = triplets.front(); triplets.pop_front();
          QString current = triplets.front(); triplets.pop_front();
          QString max = triplets.front(); triplets.pop_front();
          QuotaInfo info( name, root, current, max );
          quotas.append( info );
        }
      }
    }
  }
  if ( !quotas.isEmpty() ) {
    emit quotaInfoReceived( quotas );
  }
  emit quotaRootResult( roots );
}

QuotaJobs::GetStorageQuotaJob* QuotaJobs::getStorageQuota(
    KIO::Slave* slave, const KUrl& url )
{
  GetStorageQuotaJob* job = new GetStorageQuotaJob( slave, url );
  return job;
}


QuotaJobs::GetStorageQuotaJob::GetStorageQuotaJob( KIO::Slave* slave, const KUrl& url )
  : KIO::Job()
{
    QByteArray packedArgs;
    QDataStream stream( &packedArgs, IO_WriteOnly );
    stream << (int)'Q' << (int)'R' << url;

    QuotaJobs::GetQuotarootJob *job =
        new QuotaJobs::GetQuotarootJob( url, packedArgs);
    connect(job, SIGNAL(quotaInfoReceived(const QuotaInfoList&)),
            SLOT(slotQuotaInfoReceived(const QuotaInfoList&)));
    connect(job, SIGNAL(quotaRootResult(const QStringList&)),
            SLOT(slotQuotarootResult(const QStringList&)));
    KIO::Scheduler::assignJobToSlave( slave, job );
    addSubjob( job );
}

void QuotaJobs::GetStorageQuotaJob::slotQuotarootResult( const QStringList& roots )
{
    Q_UNUSED(roots); // we only support one for now
    if ( !mStorageQuotaInfo.isValid() && !error() ) {
      // No error, so the account supports quota, but no usable info
      // was transmitted => no quota set on the folder. Make the info
      // valid, bit leave it empty.
      mStorageQuotaInfo.setName( "STORAGE" );
    }
    if ( mStorageQuotaInfo.isValid() )
      emit storageQuotaResult( mStorageQuotaInfo );
    // Our subjob is done for all we care although it technically isn't yet (we would need to wait
    // for its finished() signal), however our dtor will cancel all still running subjobs which kills
    // the slave, so detach our subjob and let it cleanup itself.
    clearSubjobs();
    emitResult();
}

void QuotaJobs::GetStorageQuotaJob::slotQuotaInfoReceived( const QuotaInfoList& infos )
{
    QuotaInfoList::ConstIterator it( infos.begin() );
    while ( it != infos.end() ) {
      // FIXME we only use the first storage quota, for now
      if ( it->name() == "STORAGE" && !mStorageQuotaInfo.isValid() ) {
          mStorageQuotaInfo = *it;
      }
      ++it;
    }
}

QuotaInfo QuotaJobs::GetStorageQuotaJob::storageQuotaInfo() const
{
  return mStorageQuotaInfo;
}

#include "quotajobs.moc"
