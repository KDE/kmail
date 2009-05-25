/*  -*- mode: C++; c-file-style: "gnu" -*-
    objecttreeparser_p.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2009 Klarälvdalens Datakonsult AB
    Authors: Marc Mutz <marc@kdab.net>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config.h>

#include "objecttreeparser_p.h"

#include <kleo/decryptverifyjob.h>
#include <kleo/verifydetachedjob.h>
#include <kleo/verifyopaquejob.h>
#include <kleo/keylistjob.h>

#include <gpgmepp/keylistresult.h>

#include <qtimer.h>
#include <qstringlist.h>

#include <cassert>

using namespace KMail;
using namespace Kleo;
using namespace GpgME;

DecryptVerifyBodyPartMemento::DecryptVerifyBodyPartMemento( DecryptVerifyJob * job, const QByteArray & cipherText )
  : QObject( 0 ),
    Interface::BodyPartMemento(),
    ISubject(),
    m_cipherText( cipherText ),
    m_job( job ),
    m_running( false )
{
  assert( m_job );
}

DecryptVerifyBodyPartMemento::~DecryptVerifyBodyPartMemento() {
  if ( m_job )
    m_job->slotCancel();
}

bool DecryptVerifyBodyPartMemento::start() {
  assert( m_job );
  if ( const Error err = m_job->start( m_cipherText ) ) {
    m_dr = DecryptionResult( err );
    return false;
  }
  connect( m_job, SIGNAL(result(const GpgME::DecryptionResult&,const GpgME::VerificationResult&,const QByteArray&)),
           this, SLOT(slotResult(const GpgME::DecryptionResult&,const GpgME::VerificationResult&,const QByteArray&)) );
  m_running = true;
  return true;
}

void DecryptVerifyBodyPartMemento::exec() {
  assert( m_job );
  QByteArray plainText;
  m_running = true;
  const std::pair<DecryptionResult,VerificationResult> p = m_job->exec( m_cipherText, plainText );
  saveResult( p.first, p.second, plainText );
  m_job->deleteLater(); // exec'ed jobs don't delete themselves
  m_job = 0;
}

void DecryptVerifyBodyPartMemento::saveResult( const DecryptionResult & dr,
                                               const VerificationResult & vr,
                                               const QByteArray & plainText )
{
  assert( m_job );
  m_running = false;
  m_dr = dr;
  m_vr = vr;
  m_plainText = plainText;
  m_auditLog = m_job->auditLogAsHtml();
  m_auditLogError = m_job->auditLogError();
}

void DecryptVerifyBodyPartMemento::slotResult( const DecryptionResult & dr,
                                               const VerificationResult & vr,
                                               const QByteArray & plainText )
{
  saveResult( dr, vr, plainText );
  m_job = 0;
  QTimer::singleShot( 100, this, SLOT(notify()) );
}




VerifyDetachedBodyPartMemento::VerifyDetachedBodyPartMemento( VerifyDetachedJob * job,
                                                              KeyListJob * klj,
                                                              const QByteArray & signature,
                                                              const QByteArray & plainText )
  : QObject( 0 ),
    Interface::BodyPartMemento(),
    ISubject(),
    m_signature( signature ),
    m_plainText( plainText ),
    m_job( job ),
    m_keylistjob( klj ),
    m_running( false )
{
  assert( m_job );
}

VerifyDetachedBodyPartMemento::~VerifyDetachedBodyPartMemento() {
  if ( m_job )
    m_job->slotCancel();
  if ( m_keylistjob )
    m_keylistjob->slotCancel();
}

bool VerifyDetachedBodyPartMemento::start() {
  assert( m_job );
  if ( const Error err = m_job->start( m_signature, m_plainText ) ) {
    m_vr = VerificationResult( err );
    return false;
  }
  connect( m_job, SIGNAL(result(const GpgME::VerificationResult&)),
           this, SLOT(slotResult(const GpgME::VerificationResult&)) );
  m_running = true;
  return true;
}

void VerifyDetachedBodyPartMemento::exec() {
  assert( m_job );
  m_running = true;
  saveResult( m_job->exec( m_signature, m_plainText ) );
  m_job->deleteLater(); // exec'ed jobs don't delete themselves
  m_job = 0;
  if ( canStartKeyListJob() ) {
    std::vector<GpgME::Key> keys;
    m_keylistjob->exec( keyListPattern(), /*secretOnly=*/false, keys );
    if ( !keys.empty() )
      m_key = keys.back();
  }
  if ( m_keylistjob )
    m_keylistjob->deleteLater(); // exec'ed jobs don't delete themselves
  m_keylistjob = 0;
  m_running = false;
}

bool VerifyDetachedBodyPartMemento::canStartKeyListJob() const
{
  if ( !m_keylistjob )
    return false;
  const char * const fpr = m_vr.signature( 0 ).fingerprint();
  return fpr && *fpr;
}

QStringList VerifyDetachedBodyPartMemento::keyListPattern() const
{
  assert( canStartKeyListJob() );
  return QStringList( QString::fromLatin1( m_vr.signature( 0 ).fingerprint() ) );
}

void VerifyDetachedBodyPartMemento::saveResult( const VerificationResult & vr )
{
  assert( m_job );
  m_vr = vr;
  m_auditLog = m_job->auditLogAsHtml();
  m_auditLogError = m_job->auditLogError();
}

void VerifyDetachedBodyPartMemento::slotResult( const VerificationResult & vr )
{
  saveResult( vr );
  m_job = 0;
  if ( canStartKeyListJob() && startKeyListJob() )
    return;
  if ( m_keylistjob )
    m_keylistjob->deleteLater();
  m_keylistjob = 0;
  m_running = false;
  QTimer::singleShot( 100, this, SLOT(notify()) );
}

bool VerifyDetachedBodyPartMemento::startKeyListJob()
{
  assert( canStartKeyListJob() );
  if ( const GpgME::Error err = m_keylistjob->start( keyListPattern() ) )
    return false;
  connect( m_keylistjob, SIGNAL(done()), this, SLOT(slotKeyListJobDone()) );
  connect( m_keylistjob, SIGNAL(nextKey(const GpgME::Key&)),
           this, SLOT(slotNextKey(const GpgME::Key&)) );
  return true;
}

void VerifyDetachedBodyPartMemento::slotNextKey( const GpgME::Key & key )
{
  m_key = key;
}

void VerifyDetachedBodyPartMemento::slotKeyListJobDone()
{
  m_keylistjob = 0;
  m_running = false;
  QTimer::singleShot( 100, this, SLOT(notify()) );
}


VerifyOpaqueBodyPartMemento::VerifyOpaqueBodyPartMemento( VerifyOpaqueJob * job,
                                                          KeyListJob *  klj,
                                                          const QByteArray & signature )
  : QObject( 0 ),
    Interface::BodyPartMemento(),
    ISubject(),
    m_signature( signature ),
    m_job( job ),
    m_keylistjob( klj ),
    m_running( false )
{
  assert( m_job );
}

VerifyOpaqueBodyPartMemento::~VerifyOpaqueBodyPartMemento() {
  if ( m_job )
    m_job->slotCancel();
  if ( m_keylistjob )
    m_keylistjob->slotCancel();
}

bool VerifyOpaqueBodyPartMemento::start() {
  assert( m_job );
  if ( const Error err = m_job->start( m_signature ) ) {
    m_vr = VerificationResult( err );
    return false;
  }
  connect( m_job, SIGNAL(result(const GpgME::VerificationResult&,const QByteArray&)),
           this, SLOT(slotResult(const GpgME::VerificationResult&,const QByteArray&)) );
  m_running = true;
  return true;
}

void VerifyOpaqueBodyPartMemento::exec() {
  assert( m_job );
  m_running = true;
  QByteArray plainText;
  saveResult( m_job->exec( m_signature, plainText ), plainText );
  m_job->deleteLater(); // exec'ed jobs don't delete themselves
  m_job = 0;
  if ( canStartKeyListJob() ) {
    std::vector<GpgME::Key> keys;
    m_keylistjob->exec( keyListPattern(), /*secretOnly=*/false, keys );
    if ( !keys.empty() )
      m_key = keys.back();
  }
  if ( m_keylistjob )
    m_keylistjob->deleteLater(); // exec'ed jobs don't delete themselves
  m_keylistjob = 0;
  m_running = false;
}

bool VerifyOpaqueBodyPartMemento::canStartKeyListJob() const
{
  if ( !m_keylistjob )
    return false;
  const char * const fpr = m_vr.signature( 0 ).fingerprint();
  return fpr && *fpr;
}

QStringList VerifyOpaqueBodyPartMemento::keyListPattern() const
{
  assert( canStartKeyListJob() );
  return QStringList( QString::fromLatin1( m_vr.signature( 0 ).fingerprint() ) );
}

void VerifyOpaqueBodyPartMemento::saveResult( const VerificationResult & vr,
                                              const QByteArray & plainText )
{
  assert( m_job );
  m_vr = vr;
  m_plainText = plainText;
  m_auditLog = m_job->auditLogAsHtml();
  m_auditLogError = m_job->auditLogError();
}

void VerifyOpaqueBodyPartMemento::slotResult( const VerificationResult & vr,
                                              const QByteArray & plainText )
{
  saveResult( vr, plainText );
  m_job = 0;
  if ( canStartKeyListJob() && startKeyListJob() )
    return;
  if ( m_keylistjob )
    m_keylistjob->deleteLater();
  m_keylistjob = 0;
  m_running = false;
  QTimer::singleShot( 100, this, SLOT(notify()) );
}

bool VerifyOpaqueBodyPartMemento::startKeyListJob()
{
  assert( canStartKeyListJob() );
  if ( const GpgME::Error err = m_keylistjob->start( keyListPattern() ) )
    return false;
  connect( m_keylistjob, SIGNAL(done()), this, SLOT(slotKeyListJobDone()) );
  connect( m_keylistjob, SIGNAL(nextKey(const GpgME::Key&)),
           this, SLOT(slotNextKey(const GpgME::Key&)) );
  return true;
}

void VerifyOpaqueBodyPartMemento::slotNextKey( const GpgME::Key & key )
{
  m_key = key;
}

void VerifyOpaqueBodyPartMemento::slotKeyListJobDone()
{
  m_keylistjob = 0;
  m_running = false;
  QTimer::singleShot( 100, this, SLOT(notify()) );
}


#include "objecttreeparser_p.moc"
