/**
 * annotationjobs.cpp
 *
 * Copyright (c) 2004 David Faure <faure@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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


#include "annotationjobs.h"
#include <kio/scheduler.h>
#include <kdebug.h>

using namespace KMail;

KIO::SimpleJob* AnnotationJobs::setAnnotation(
    KIO::Slave* slave, const KUrl& url, const QString& entry,
    const QMap<QString,QString>& attributes )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'M' << (int)'S' << url << entry << attributes;

  KIO::SimpleJob* job = KIO::special( url, packedArgs, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

AnnotationJobs::GetAnnotationJob* AnnotationJobs::getAnnotation(
    KIO::Slave* slave, const KUrl& url, const QString& entry,
    const QStringList& attributes )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'M' << (int)'G' << url << entry << attributes;

  GetAnnotationJob* job = new GetAnnotationJob( url, entry, packedArgs);
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

AnnotationJobs::GetAnnotationJob::GetAnnotationJob( const KUrl& url, const QString& entry,
                                                    const QByteArray &packedArgs)
  : KIO::SpecialJob( url, packedArgs),
    mEntry( entry )
{
  connect( this, SIGNAL(infoMessage(KJob*,const QString&,const QString&)),
           SLOT(slotInfoMessage(KJob*,const QString&,const QString&)) );
}

void AnnotationJobs::GetAnnotationJob::slotInfoMessage( KJob*, const QString& str,const QString& )
{
  // Parse the result
  QStringList lst = str.split( "\r", QString::SkipEmptyParts );
  while ( lst.count() >= 2 ) // we take items 2 by 2
  {
    QString name = lst.front(); lst.pop_front();
    QString value = lst.front(); lst.pop_front();
    mAnnotations.append( AnnotationAttribute( mEntry, name, value ) );
  }
}

AnnotationJobs::MultiGetAnnotationJob::MultiGetAnnotationJob(
  KIO::Slave* slave, const KUrl& url, const QStringList& entries )
  : KIO::Job(),
    mSlave( slave ),
    mUrl( url ), mEntryList( entries ), mEntryListIterator( mEntryList.begin() )
{
  QTimer::singleShot(0, this, SLOT(slotStart()));
}


void AnnotationJobs::MultiGetAnnotationJob::slotStart()
{
  if ( mEntryListIterator != mEntryList.end() ) {
    QStringList attributes;
    attributes << "value";
    KIO::Job* job = getAnnotation( mSlave, mUrl, *mEntryListIterator, attributes );
    addSubjob( job );
  } else { // done!
    emitResult();
  }
}

void AnnotationJobs::MultiGetAnnotationJob::slotResult( KJob *job )
{
  if ( job->error() ) {
    KIO::Job::slotResult( job ); // will set the error and emit result(this)
    return;
  }
  removeSubjob( job );
  const QString& entry = *mEntryListIterator;
  QString value;
  bool found = false;
  GetAnnotationJob* getJob = static_cast<GetAnnotationJob *>( job );
  const AnnotationList& lst = getJob->annotations();
  for ( int i = 0 ; i < lst.size() ; ++ i ) {
    kDebug(5006) <<"found annotation" << lst[i].name <<" =" << lst[i].value;
    if ( lst[i].name.startsWith( "value." ) ) { // value.priv or value.shared
      found = true;
      value = lst[i].value;
      break;
    }
  }
  emit annotationResult( entry, value, found );
  // Move on to next one
  ++mEntryListIterator;
  slotStart();
}

AnnotationJobs::MultiGetAnnotationJob* AnnotationJobs::multiGetAnnotation( KIO::Slave* slave, const KUrl& url, const QStringList& entries )
{
  return new MultiGetAnnotationJob( slave, url, entries );
}

////

AnnotationJobs::MultiSetAnnotationJob::MultiSetAnnotationJob(
  KIO::Slave* slave, const KUrl& url, const AnnotationList& annotations )
  : KIO::Job(),
    mSlave( slave ),
    mUrl( url ), mAnnotationList( annotations ), mAnnotationListIterator( mAnnotationList.begin() )
{
  QTimer::singleShot(0, this, SLOT(slotStart()));
}


void AnnotationJobs::MultiSetAnnotationJob::slotStart()
{
  if ( mAnnotationListIterator != mAnnotationList.end() ) {
    const AnnotationAttribute& attr = *mAnnotationListIterator;
    // setAnnotation can set multiple attributes for a given entry.
    // So in theory we could group entries coming from our list. Bah.
    QMap<QString, QString> attributes;
    attributes.insert( attr.name, attr.value );
    kDebug(5006) <<" setting annotation" << attr.entry << attr.name << attr.value;
    KIO::Job* job = setAnnotation( mSlave, mUrl, attr.entry, attributes );
    addSubjob( job );
  } else { // done!
    emitResult();
  }
}

void AnnotationJobs::MultiSetAnnotationJob::slotResult( KJob *job )
{
  if ( job->error() ) {
    KIO::Job::slotResult( job ); // will set the error and emit result(this)
    return;
  }
  removeSubjob( job );
  const AnnotationAttribute& attr = *mAnnotationListIterator;
  emit annotationChanged( attr.entry, attr.name, attr.value );

  // Move on to next one
  ++mAnnotationListIterator;
  slotStart();
}

AnnotationJobs::MultiSetAnnotationJob* AnnotationJobs::multiSetAnnotation(
  KIO::Slave* slave, const KUrl& url, const AnnotationList& annotations )
{
  return new MultiSetAnnotationJob( slave, url, annotations );
}


AnnotationJobs::MultiUrlGetAnnotationJob::MultiUrlGetAnnotationJob( KIO::Slave* slave,
                                                                    const KUrl& baseUrl,
                                                                    const QStringList& paths,
                                                                    const QString& annotation )
  : KIO::Job(),
    mSlave( slave ),
    mUrl( baseUrl ),
    mPathList( paths ),
    mPathListIterator( mPathList.begin() ),
    mAnnotation( annotation )
{
  QTimer::singleShot(0, this, SLOT(slotStart()));
}


void AnnotationJobs::MultiUrlGetAnnotationJob::slotStart()
{
  if ( mPathListIterator != mPathList.end() ) {
    QStringList attributes;
    attributes << "value";
    KUrl url(mUrl);
    url.setPath( *mPathListIterator );
    KIO::Job* job = getAnnotation( mSlave, url, mAnnotation, attributes );
    addSubjob( job );
  } else { // done!
    emitResult();
  }
}

void AnnotationJobs::MultiUrlGetAnnotationJob::slotResult( KJob *job )
{
  if ( job->error() ) {
    KIO::Job::slotResult( job ); // will set the error and emit result(this)
    return;
  }
  removeSubjob( job );
  const QString& path = *mPathListIterator;
  GetAnnotationJob* getJob = static_cast<GetAnnotationJob *>( job );
  const AnnotationList& lst = getJob->annotations();
  for ( int i = 0 ; i < lst.size() ; ++ i ) {
    kDebug(5006) <<"MultiURL: found annotation" << lst[i].name <<" =" << lst[i].value <<" for path:" << path;
    if ( lst[i].name.startsWith( "value." ) ) { // value.priv or value.shared
      mAnnotations.insert( path, lst[i].value );
      break;
    }
  }
  // Move on to next one
  ++mPathListIterator;
  slotStart();
}

QMap<QString, QString> AnnotationJobs::MultiUrlGetAnnotationJob::annotations() const
{
  return mAnnotations;
}

AnnotationJobs::MultiUrlGetAnnotationJob* AnnotationJobs::multiUrlGetAnnotation( KIO::Slave* slave,
                                                                                 const KUrl& baseUrl,
                                                                                 const QStringList& paths,
                                                                                 const QString& annotation )
{
  return new MultiUrlGetAnnotationJob( slave, baseUrl, paths, annotation );
}


#include "annotationjobs.moc"
