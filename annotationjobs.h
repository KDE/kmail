/**
 * annotationjobs.h
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

#ifndef ANNOTATIONJOBS_H
#define ANNOTATIONJOBS_H

#include <kio/job.h>
#include <qvaluevector.h>

namespace KMail {

/// One entry in the annotation list: attribute name and attribute value
struct AnnotationAttribute {
  AnnotationAttribute() {} // for QValueVector
  AnnotationAttribute( const QString& e, const QString& n, const QString& v )
    : entry( e ), name( n ), value( v ) {}
  QString entry; // e.g. /comment
  QString name;  // e.g. value.shared
  QString value;
};

typedef QValueVector<AnnotationAttribute> AnnotationList;

/**
 * This namespace contains functions that return jobs for annotation operations.
 *
 * The current implementation is tied to IMAP.
 * If someone wants to extend this to other protocols, turn the namespace into a class
 * and use virtual methods.
 */
namespace AnnotationJobs {

/**
 * Set an annotation entry (note that it can have multiple attributes)
 * @param entry the name of the annotation entry
 * @param attributes attribute name+value pairs
 */
KIO::SimpleJob* setAnnotation( KIO::Slave* slave, const KURL& url, const QString& entry,
                               const QMap<QString,QString>& attributes );

class MultiSetAnnotationJob;
/**
 * Set multiple annotation entries
 */
MultiSetAnnotationJob* multiSetAnnotation( KIO::Slave* slave, const KURL& url, const AnnotationList& annotations );

class GetAnnotationJob;
/**
 * Get an annotation entry
 * @param entry the name of the annotation entry
 * @param attributes attribute names
 */
GetAnnotationJob* getAnnotation( KIO::Slave* slave, const KURL& url, const QString& entry,
                                 const QStringList& attributes );

class MultiGetAnnotationJob;
/**
 * Get multiple annotation entries
 * Currently we assume we want to get the "value" for each, to simplify the data structure.
 */
MultiGetAnnotationJob* multiGetAnnotation( KIO::Slave* slave, const KURL& url, const QStringList& entries );

/// for getAnnotation()
class GetAnnotationJob : public KIO::SimpleJob
{
  Q_OBJECT
public:
  GetAnnotationJob( const KURL& url, const QString& entry, const QByteArray &packedArgs,
                    bool showProgressInfo );

  const AnnotationList& annotations() const { return mAnnotations; }

protected slots:
  void slotInfoMessage( KIO::Job*, const QString& );
private:
  AnnotationList mAnnotations;
  QString mEntry;
};

/// for multiGetAnnotation
class MultiGetAnnotationJob : public KIO::Job
{
  Q_OBJECT

public:
  MultiGetAnnotationJob( KIO::Slave* slave, const KURL& url, const QStringList& entries, bool showProgressInfo );

signals:
  // Emitted when a given annotation was found - or not found
  void annotationResult( const QString& entry, const QString& value, bool found );

protected slots:
  virtual void slotStart();
  virtual void slotResult( KIO::Job *job );

private:
  KIO::Slave* mSlave;
  const KURL mUrl;
  const QStringList mEntryList;
  QStringList::const_iterator mEntryListIterator;
};

/// for multiSetAnnotation
class MultiSetAnnotationJob : public KIO::Job
{
  Q_OBJECT

public:
  MultiSetAnnotationJob( KIO::Slave* slave, const KURL& url, const AnnotationList& annotations, bool showProgressInfo );

signals:
  // Emitted when a given annotation was successfully changed
  void annotationChanged( const QString& entry, const QString& attribute, const QString& value );

protected slots:
  virtual void slotStart();
  virtual void slotResult( KIO::Job *job );

private:
  KIO::Slave* mSlave;
  const KURL mUrl;
  const AnnotationList mAnnotationList;
  AnnotationList::const_iterator mAnnotationListIterator;
};

} // AnnotationJobs namespace

} // KMail namespace

#endif /* ANNOTATIONJOBS_H */

