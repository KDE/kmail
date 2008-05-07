/**
 * acljobs.cpp
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


#include "acljobs.h"
#include <kio/scheduler.h>
#include <kdebug.h>

using namespace KMail;

// Convert str to an ACLPermissions value.
// url and user are there only for the error message
static unsigned int IMAPRightsToPermission( const QString& str, const KUrl& url, const QString& user ) {
  unsigned int perm = 0;
  uint len = str.length();
  for (uint i = 0; i < len; ++i) {
    QChar ch = str[i];
    switch ( ch.toLatin1() ) {
    case 'l': perm |= ACLJobs::List; break;
    case 'r': perm |= ACLJobs::Read; break;
    case 's': perm |= ACLJobs::WriteSeenFlag; break;
    case 'w': perm |= ACLJobs::WriteFlags; break;
    case 'i': perm |= ACLJobs::Insert; break;
    case 'p': perm |= ACLJobs::Post; break;
    case 'c': perm |= ACLJobs::Create; break;
    case 'd': perm |= ACLJobs::Delete; break;
    case 'a': perm |= ACLJobs::Administer; break;
    default: break;
    }
  }
  if ( ( perm & ACLJobs::Read ) && !( perm & ACLJobs::WriteSeenFlag )  ) {
    // Reading without 'seen' is, well, annoying. Unusable, even.
    // So we treat 'rs' as a single one.
    // But if the permissions were set out of kmail, better check that both are set
    kWarning(5006) <<"IMAPRightsToPermission: found read (r) but not seen (s). Things will not work well for folder" << url <<" and user" << ( user.isEmpty() ?"myself" : user );
    if ( perm & ACLJobs::Administer )
      kWarning(5006) <<"You can change this yourself in the ACL dialog";
    else
      kWarning(5006) <<"Ask your admin for 's' permissions.";
    // Is the above correct enough to be turned into a KMessageBox?
  }

  return perm;
}

static QByteArray permissionsToIMAPRights( unsigned int permissions ) {
  QByteArray str = "";
  if ( permissions & ACLJobs::List )
    str += 'l';
  if ( permissions & ACLJobs::Read )
    str += 'r';
  if ( permissions & ACLJobs::WriteSeenFlag )
    str += 's';
  if ( permissions & ACLJobs::WriteFlags )
    str += 'w';
  if ( permissions & ACLJobs::Insert )
    str += 'i';
  if ( permissions & ACLJobs::Post )
    str += 'p';
  if ( permissions & ACLJobs::Create )
    str += 'c';
  if ( permissions & ACLJobs::Delete )
    str += 'd';
  if ( permissions & ACLJobs::Administer )
    str += 'a';
  return str;
}

#ifndef NDEBUG
QString ACLJobs::permissionsToString( unsigned int permissions )
{
  QString str;
  if ( permissions & ACLJobs::List )
    str += "List ";
  if ( permissions & ACLJobs::Read )
    str += "Read ";
  if ( permissions & ACLJobs::WriteFlags )
    str += "Write ";
  if ( permissions & ACLJobs::Insert )
    str += "Insert ";
  if ( permissions & ACLJobs::Post )
    str += "Post ";
  if ( permissions & ACLJobs::Create )
    str += "Create ";
  if ( permissions & ACLJobs::Delete )
    str += "Delete ";
  if ( permissions & ACLJobs::Administer )
    str += "Administer ";
  if ( !str.isEmpty() )
    str.truncate( str.length() - 1 );
  return str;
}
#endif

KIO::SimpleJob* ACLJobs::setACL( KIO::Slave* slave, const KUrl& url, const QString& user, unsigned int permissions )
{
  QString perm = QString::fromLatin1( permissionsToIMAPRights( permissions ) );

  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'A' << (int)'S' << url << user << perm;

  KIO::SimpleJob* job = KIO::special( url, packedArgs, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::DeleteACLJob* ACLJobs::deleteACL( KIO::Slave* slave, const KUrl& url, const QString& user )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'A' << (int)'D' << url << user;

  ACLJobs::DeleteACLJob* job = new ACLJobs::DeleteACLJob( url, user, packedArgs);
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetACLJob* ACLJobs::getACL( KIO::Slave* slave, const KUrl& url )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'A' << (int)'G' << url;

  ACLJobs::GetACLJob* job = new ACLJobs::GetACLJob( url, packedArgs);
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetUserRightsJob* ACLJobs::getUserRights( KIO::Slave* slave, const KUrl& url )
{
  QByteArray packedArgs;
  QDataStream stream( &packedArgs, QIODevice::WriteOnly );
  stream << (int)'A' << (int)'M' << url;

  ACLJobs::GetUserRightsJob* job = new ACLJobs::GetUserRightsJob( url, packedArgs );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetACLJob::GetACLJob( const KUrl& url, const QByteArray &packedArgs)
  : KIO::SpecialJob( url, packedArgs )
{
  connect( this, SIGNAL(infoMessage(KJob*,const QString&,const QString&)),
           SLOT(slotInfoMessage(KJob*,const QString&,const QString&)) );
}

void ACLJobs::GetACLJob::slotInfoMessage( KJob*, const QString& str,const QString& )
{
  // Parse the result
  QStringList lst = str.split( "\"", QString::KeepEmptyParts );
  while ( lst.count() >= 2 ) // we take items 2 by 2
  {
    QString user = lst.front(); lst.pop_front();
    QString imapRights = lst.front(); lst.pop_front();
    unsigned int perm = IMAPRightsToPermission( imapRights, url(), user );
    m_entries.append( ACLListEntry( user, imapRights, perm ) );
  }
}

ACLJobs::GetUserRightsJob::GetUserRightsJob( const KUrl& url, const QByteArray &packedArgs)
  : KIO::SpecialJob( url, packedArgs)
{
  connect( this, SIGNAL(infoMessage(KJob*,const QString&,const QString&)),
           SLOT(slotInfoMessage(KJob*,const QString&,const QString&)) );
}

void ACLJobs::GetUserRightsJob::slotInfoMessage( KJob*, const QString& str,const QString& )
{
  // Parse the result
  m_permissions = IMAPRightsToPermission( str, url(), QString() );
}

ACLJobs::DeleteACLJob::DeleteACLJob( const KUrl& url, const QString& userId,
                                     const QByteArray &packedArgs)
  : KIO::SpecialJob( url, packedArgs ),
    mUserId( userId )
{
}

////

ACLJobs::MultiSetACLJob::MultiSetACLJob( KIO::Slave* slave, const KUrl& url, const ACLList& acl )
  : KIO::Job(),
    mSlave( slave ),
    mUrl( url ), mACLList( acl ), mACLListIterator( mACLList.begin() )
{
  QTimer::singleShot(0, this, SLOT(slotStart()));
}

void ACLJobs::MultiSetACLJob::slotStart()
{
  // Skip over unchanged entries
  while ( mACLListIterator != mACLList.end() && !(*mACLListIterator).changed )
    ++mACLListIterator;

  if ( mACLListIterator != mACLList.end() )
  {
    const ACLListEntry& entry = *mACLListIterator;
    KIO::Job* job = 0;
    if ( entry.permissions > -1 )
      job = setACL( mSlave, mUrl, entry.userId, entry.permissions );
    else
      job = deleteACL( mSlave, mUrl, entry.userId );

    addSubjob( job );
  } else { // done!
    emitResult();
  }
}

void ACLJobs::MultiSetACLJob::slotResult( KJob *job )
{
  if ( job->error() ) {
    KIO::Job::slotResult( job ); // will set the error and emit result(this)
    return;
  }
  removeSubjob(job);
  const ACLListEntry& entry = *mACLListIterator;
  emit aclChanged( entry.userId, entry.permissions );

  // Move on to next one
  ++mACLListIterator;
  slotStart();
}

ACLJobs::MultiSetACLJob* ACLJobs::multiSetACL( KIO::Slave* slave, const KUrl& url, const ACLList& acl )
{
  return new MultiSetACLJob( slave, url, acl);
}

#include "acljobs.moc"
