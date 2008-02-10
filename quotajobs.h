/**
 * quotajobs.h
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

#ifndef QUOTAJOBS_H
#define QUOTAJOBS_H

#include <qvariant.h>

#include <kio/job.h>
#include <klocale.h>
#include <qvector.h>

#include <math.h>

#include "globalsettings.h"

#undef max

namespace KIO { class Slave; }

namespace KMail {

// One quota entry consisting of a name, the quota root,
// the current value and the maximal value
class QuotaInfo {
  public :
  QuotaInfo() {} // for mStorageQuotaInfo
  QuotaInfo( const QString& _name, const QString& _root, const QVariant& _current, const QVariant& _max )
    : mName( _name ), mRoot( _root ), mCurrent( _current ),mMax( _max )  {}
  bool operator==( const QuotaInfo & other ) const {
    return mName == other.mName && mRoot == other.mRoot && mMax == other.mMax && mCurrent == other.mCurrent;
  }
  bool operator!=( const QuotaInfo & other ) const {
    return !(operator==(other) );
  }
  bool isValid() const { return !mName.isEmpty(); }
  bool isEmpty() const { return mName.isEmpty() || ( mRoot.isEmpty() && !mCurrent.isValid() && !mMax.isValid() ); }

  QString name() const { return mName; }
  void setName( const QString& n ) { mName = n; }
  QString root() const { return mRoot; }
  void setRoot( const QString& r ) { mRoot = r; }
  QVariant max() const { return mMax; }
  void setMax( const QVariant& m ) { mMax = m; }
  QVariant current() const { return mCurrent; }
  void setCurrent( const QVariant& c ) { mCurrent = c; }

  QString toString() const {
    if ( isValid() && !isEmpty() ) {
      readConfig();
      int factor = static_cast<int> ( pow( 1000.0, mFactor ) );
      return i18n("%1 of %2 %3 used", mCurrent.toInt() / factor,
                                      mMax.toInt() / factor, mUnits );
    }
    return QString();
  }

 private:
  void readConfig() const {
      if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::KB )
      {
            mUnits = i18n("KB");
            mFactor = 0;
      }
      else if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::MB )
           {
                mUnits = i18n("MB");
                mFactor = 1;
           }
      else if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::GB )
           {
               mUnits = i18n("GB");
               mFactor = 2;
           }
   }

  QString mName;  // e.g. STORAGE
  QString mRoot; /// e.g. INBOX
  QVariant mCurrent;
  QVariant mMax;
  mutable QString mUnits; //used by readConfig (const) privately and is modified
  mutable int mFactor;
};

typedef QVector<QuotaInfo> QuotaInfoList;

/**
 * This namespace contains functions that return jobs for quota operations.
 *
 * The current implementation is tied to IMAP.
 * If someone wants to extend this to other protocols, turn the namespace into a class
 * and use virtual methods.
 */
namespace QuotaJobs {

class GetQuotarootJob;
/**
 * Get the quotaroots for a mailbox
 * @param slave Slave object the job should be assigned to
 * @param url URL for which to get the quotaroot
 */
GetQuotarootJob* getQuotaroot( KIO::Slave* slave, const KUrl& url );

class GetStorageQuotaJob;
/**
 * Get the storage quota for a mailbox, if there is one.
 * @param slave Slave object the job should be assigned to
 * @param url URL for which to get the storage quota
 */
GetStorageQuotaJob* getStorageQuota( KIO::Slave* slave, const KUrl& url );

/// for getQuotaroot()
class GetQuotarootJob : public KIO::SpecialJob
{
  Q_OBJECT
public:
  GetQuotarootJob( const KUrl& url, const QByteArray &packedArgs);

signals:
  /** Emitted when the server returns a (potentially empty) list of
   * quota roots for the specified mailbox.
   * @param roots List of quota roots for the mailbox
   */
  void quotaRootResult( const QStringList& roots );

  /**
   * Emitted when the server returns a list of quota infos for the specified
   * mailbox. This is an aggregate of all quotas for all applicable roots for
   * the mailbox. It happens as a side effect of root listing.
   * @param info List of quota infos for the mailbox
   */
  void quotaInfoReceived( const QuotaInfoList& info );

protected slots:
  void slotInfoMessage( KJob*, const QString& );

protected:

  // FIXME: this using statement is incorrect, I just want to get rid of the
  //        compiler warnings generated by this. The warnings are right, this
  //        is probably the cause of bug 154960.
  //        Instead, there now is a #warning in the cpp file.
  using KIO::SpecialJob::slotInfoMessage;

};

/// for getStorageQuota()
class GetStorageQuotaJob : public KIO::Job
{
  Q_OBJECT
public:
  GetStorageQuotaJob( KIO::Slave* slave, const KUrl& url );

  /**  Returns the storage quota info, if any, can be queried on result(). */
  QuotaInfo storageQuotaInfo() const;

signals:
  /** Emitted to indicate that storage quota information has
   * been received. Is not emitted if there is no such info
   * on the server, so users need to rely on the normal
   * result() signal to be informed when the job is done.
   */
  void storageQuotaResult( const QuotaInfo& info );


protected slots:
  void slotQuotarootResult( const QStringList& roots );
  void slotQuotaInfoReceived( const QuotaInfoList& roots );
private:
  QuotaInfo mStorageQuotaInfo;
};

} // QuotaJobs namespace

} // KMail namespace


#endif /* QUOTAJOBS_H */

