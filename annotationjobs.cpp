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
#include "annotationjobs.h"
#include <kio/scheduler.h>
#include <kdebug.h>

using namespace KMail;

KIO::SimpleJob* AnnotationJobs::setAnnotation(
    KIO::Slave* slave, const KURL& url, const QString& entry,
    const QMap<QString,QString>& attributes )
{
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'M' << (int)'S' << url << entry << attributes;

  KIO::SimpleJob* job = KIO::special( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

AnnotationJobs::GetAnnotationJob* AnnotationJobs::getAnnotation(
    KIO::Slave* slave, const KURL& url, const QString& entry,
    const QStringList& attributes )
{
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'M' << (int)'G' << url << entry << attributes;

  GetAnnotationJob* job = new GetAnnotationJob( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

AnnotationJobs::GetAnnotationJob::GetAnnotationJob( const KURL& url, const QByteArray &packedArgs,
                                                    bool showProgressInfo )
  : KIO::SimpleJob( url, KIO::CMD_SPECIAL, packedArgs, showProgressInfo )
{
  connect( this, SIGNAL(infoMessage(KIO::Job*,const QString&)),
           SLOT(slotInfoMessage(KIO::Job*,const QString&)) );
}

void AnnotationJobs::GetAnnotationJob::slotInfoMessage( KIO::Job*, const QString& str )
{
  // Parse the result
  QStringList lst = QStringList::split( "\r", str );
  while ( lst.count() >= 2 ) // we take items 2 by 2
  {
    QString name = lst.front(); lst.pop_front();
    QString value = lst.front(); lst.pop_front();
    m_entries.append( AnnotationAttribute( name, value ) );
  }
}

#include "annotationjobs.moc"
