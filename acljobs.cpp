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
#include "acljobs.h"
#include <kio/scheduler.h>
#include <kdebug.h>

using namespace KMail;

static unsigned int IMAPRightsToPermission( const QString& str ) {
  unsigned int perm = 0;
  bool foundSeenPerm = false;
  uint len = str.length();
  for (uint i = 0; i < len; ++i) {
    QChar ch = str[i];
    switch ( ch.latin1() ) {
    case 'l': perm |= ACLJobs::List; break;
    case 'r': perm |= ACLJobs::Read; break;
    case 's': foundSeenPerm = true; break;
    case 'w': perm |= ACLJobs::WriteFlags; break;
    case 'i': perm |= ACLJobs::Insert; break;
    case 'p': perm |= ACLJobs::Post; break;
    case 'c': perm |= ACLJobs::Create; break;
    case 'd': perm |= ACLJobs::Delete; break;
    case 'a': perm |= ACLJobs::Administer; break;
    default: break;
    }
  }
  if ( ( perm & ACLJobs::Read ) && str.find( 's' ) == -1 ) {
    // Reading without 'seen' is, well, annoying. Unusable, even.
    // So we treat 'rs' as a single one.
    // But if the permissions were set out of kmail, better check that both are set
    kdWarning(5006) << "IMAPRightsToPermission: found read (r) but not seen (s). Things will not work well." << endl;
    if ( perm & ACLJobs::Administer )
      kdWarning(5006) << "You can change this yourself in the ACL dialog" << endl;
    else
      kdWarning(5006) << "Ask your admin for 's' permissions." << endl;
    // Is the above correct enough to be turned into a KMessageBox?
  }

  return perm;
}

static QCString permissionsToIMAPRights( unsigned int permissions ) {
  QCString str = "";
  if ( permissions & ACLJobs::List )
    str += 'l';
  if ( permissions & ACLJobs::Read )
    str += "rs";
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

KIO::SimpleJob* ACLJobs::setACL( KIO::Slave* slave, const KURL& url, const QString& user, unsigned int permissions )
{
  QString perm = QString::fromLatin1( permissionsToIMAPRights( permissions ) );

  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'A' << (int)'S' << url << user << perm;

  KIO::SimpleJob* job = KIO::special( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::DeleteACLJob* ACLJobs::deleteACL( KIO::Slave* slave, const KURL& url, const QString& user )
{
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'A' << (int)'D' << url << user;

  ACLJobs::DeleteACLJob* job = new ACLJobs::DeleteACLJob( url, user, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetACLJob* ACLJobs::getACL( KIO::Slave* slave, const KURL& url )
{
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'A' << (int)'G' << url;

  ACLJobs::GetACLJob* job = new ACLJobs::GetACLJob( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetUserRightsJob* ACLJobs::getUserRights( KIO::Slave* slave, const KURL& url )
{
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int)'A' << (int)'M' << url;

  ACLJobs::GetUserRightsJob* job = new ACLJobs::GetUserRightsJob( url, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( slave, job );
  return job;
}

ACLJobs::GetACLJob::GetACLJob( const KURL& url, const QByteArray &packedArgs,
                                 bool showProgressInfo )
  : KIO::SimpleJob( url, KIO::CMD_SPECIAL, packedArgs, showProgressInfo )
{
  connect( this, SIGNAL(infoMessage(KIO::Job*,const QString&)),
           SLOT(slotInfoMessage(KIO::Job*,const QString&)) );
}

void ACLJobs::GetACLJob::slotInfoMessage( KIO::Job*, const QString& str )
{
  // Parse the result
  QStringList lst = QStringList::split( " ", str );
  while ( lst.count() >= 2 ) // we take items 2 by 2
  {
    QString user = lst.front(); lst.pop_front();
    QString imapRights = lst.front(); lst.pop_front();
    unsigned int perm = IMAPRightsToPermission( imapRights );
    m_entries.append( ACLListEntry( user, imapRights, perm ) );
  }
}

ACLJobs::GetUserRightsJob::GetUserRightsJob( const KURL& url, const QByteArray &packedArgs,
                                               bool showProgressInfo )
  : KIO::SimpleJob( url, KIO::CMD_SPECIAL, packedArgs, showProgressInfo )
{
  connect( this, SIGNAL(infoMessage(KIO::Job*,const QString&)),
           SLOT(slotInfoMessage(KIO::Job*,const QString&)) );
}

void ACLJobs::GetUserRightsJob::slotInfoMessage( KIO::Job*, const QString& str )
{
  // Parse the result
  m_permissions = IMAPRightsToPermission( str );
}

ACLJobs::DeleteACLJob::DeleteACLJob( const KURL& url, const QString& userId,
                                     const QByteArray &packedArgs,
                                     bool showProgressInfo )
  : KIO::SimpleJob( url, KIO::CMD_SPECIAL, packedArgs, showProgressInfo ),
    mUserId( userId )
{
}

////

ACLJobs::MultiSetACLJob::MultiSetACLJob( KIO::Slave* slave, const KURL& url, const ACLList& acl, bool showProgressInfo )
  : KIO::Job( showProgressInfo ),
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

void ACLJobs::MultiSetACLJob::slotResult( KIO::Job *job )
{
  if ( job->error() ) {
    KIO::Job::slotResult( job ); // will set the error and emit result(this)
    return;
  }
  subjobs.remove(job);
  const ACLListEntry& entry = *mACLListIterator;
  emit aclChanged( entry.userId, entry.permissions );

  // Move on to next one
  ++mACLListIterator;
  slotStart();
}

ACLJobs::MultiSetACLJob* ACLJobs::multiSetACL( KIO::Slave* slave, const KURL& url, const ACLList& acl )
{
  return new MultiSetACLJob( slave, url, acl, false /*showProgressInfo*/ );
}

#include "acljobs.moc"
