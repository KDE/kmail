/**
 * acljobs.h
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

#ifndef KMACLJOBS_H
#define KMACLJOBS_H

#include <kio/job.h>
#include <qvaluevector.h>

namespace KMail {

  /// One entry in the ACL list: user and permissions
  struct ACLListEntry {
    ACLListEntry() {} // for QValueVector
    ACLListEntry( const QString& u, const QString& irl, int p )
      : userId( u ), internalRightsList( irl ), permissions( p ), changed( false ) {}
    QString userId;
    QString internalRightsList; ///< protocol-dependent string (e.g. IMAP rights list)
    int permissions; ///< based on the ACLPermissions enum
    bool changed; ///< special flag for KMFolderCachedImap
  };

  typedef QValueVector<ACLListEntry> ACLList;

/**
 * This namespace contains functions that return jobs for ACL operations.
 *
 * The current implementation is tied to IMAP.
 * If someone wants to extend this to other protocols, turn the class into a namespace
 * and use virtual methods.
 */
namespace ACLJobs {

  /// Bitfield modelling the possible permissions.
  /// This is modelled after the imap4 permissions except that Read is "rs".
  /// The semantics of the bits is protocol-dependent.
  enum ACLPermissions {
    List = 1,
    Read = 2,
    WriteFlags = 4,
    Insert = 8,
    Create = 16,
    Delete = 32,
    Administer = 64,
    Post = 128,
    // alias for "all read/write permissions except admin"
    AllWrite = List | Read | WriteFlags | Insert | Post | Create | Delete,
    // alias for "all permissions"
    All = List | Read | WriteFlags | Insert | Post | Create | Delete | Administer
  };
  /// Set the permissions for a given user on a given url
  KIO::SimpleJob* setACL( KIO::Slave* slave, const KURL& url, const QString& user, unsigned int permissions );

  class DeleteACLJob;
  /// Delete the permissions for a given user on a given url
  DeleteACLJob* deleteACL( KIO::Slave* slave, const KURL& url, const QString& user );

  class GetACLJob;
  /// List all ACLs for a given url
  GetACLJob* getACL( KIO::Slave* slave, const KURL& url );

  class GetUserRightsJob;
  /// Get the users' rights for a given url
  GetUserRightsJob* getUserRights( KIO::Slave* slave, const KURL& url );

  class MultiSetACLJob;
  /// Set and delete a list of permissions for different users on a given url
  MultiSetACLJob* multiSetACL( KIO::Slave* slave, const KURL& url, const ACLList& acl );

  /// List all ACLs for a given url
  class GetACLJob : public KIO::SimpleJob
  {
    Q_OBJECT
  public:
    GetACLJob( const KURL& url, const QByteArray &packedArgs,
               bool showProgressInfo );

    const ACLList& entries() const { return m_entries; }

  protected slots:
    void slotInfoMessage( KIO::Job*, const QString& );
  private:
    ACLList m_entries;
  };

  /// Get the users' rights for a given url
  class GetUserRightsJob : public KIO::SimpleJob
  {
    Q_OBJECT
  public:
    GetUserRightsJob( const KURL& url, const QByteArray &packedArgs,
                      bool showProgressInfo );
    unsigned int permissions() const { return m_permissions; }

  protected slots:
    void slotInfoMessage( KIO::Job*, const QString& );
  private:
    unsigned int m_permissions;
  };

  /// Delete the permissions for a given user on a given url
  /// This class only exists to store the userid in the job
  class DeleteACLJob : public KIO::SimpleJob
  {
    Q_OBJECT
  public:
    DeleteACLJob( const KURL& url, const QString& userId,
                  const QByteArray &packedArgs,
                  bool showProgressInfo );

    QString userId() const { return mUserId; }

  private:
    QString mUserId;
  };

  /// Set and delete a list of permissions for different users on a given url
  class MultiSetACLJob : public KIO::Job {
    Q_OBJECT

  public:
    MultiSetACLJob( KIO::Slave* slave, const KURL& url, const ACLList& acl, bool showProgressInfo );

  signals:
    // Emitted when a given user's permissions were successfully changed.
    // This allows the caller to keep track of what exactly was done (and handle errors better)
    void aclChanged( const QString& userId, int permissions );

  protected slots:
    virtual void slotStart();
    virtual void slotResult( KIO::Job *job );

  private:
    KIO::Slave* mSlave;
    const KURL mUrl;
    const ACLList mACLList;
    ACLList::const_iterator mACLListIterator;
  };


#ifndef NDEBUG
  QString permissionsToString( unsigned int permissions );
#endif
}

} // namespace

#endif /* KMACLJOBS_H */
