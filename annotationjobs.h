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
    AnnotationAttribute( const QString& n, const QString& v )
      : name( n ), value( v ) {}
    QString name;
    QString value;
  };

  typedef QValueVector<AnnotationAttribute> AnnotationList;

/**
 * This namespace contains functions that return jobs for annotation operations.
 *
 * The current implementation is tied to IMAP.
 * If someone wants to extend this to other protocols, turn the class into a namespace
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

  class GetAnnotationJob;
  /** Get an annotation entry
   * @param entry the name of the annotation entry
   * @param attributes attribute names
   */
  GetAnnotationJob* getAnnotation( KIO::Slave* slave, const KURL& url, const QString& entry,
                                   const QStringList& attributes );

  class GetAnnotationJob : public KIO::SimpleJob
  {
    Q_OBJECT
  public:
    GetAnnotationJob( const KURL& url, const QByteArray &packedArgs,
                      bool showProgressInfo );

    const AnnotationList& annotations() const { return m_entries; }

  protected slots:
    void slotInfoMessage( KIO::Job*, const QString& );
  private:
    AnnotationList m_entries;
  };

}

} // namespace

#endif /* ANNOTATIONJOBS_H */

