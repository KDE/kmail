/*  -*- c++ -*-
    kmservertest.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2001-2002 Michael Haeckel <haeckel@kde.org>
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#include "kmservertest.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kio/scheduler.h>
#include <kio/slave.h>
#include <kio/job.h>
#include <kio/global.h>

//-----------------------------------------------------------------------------
KMServerTest::KMServerTest(const QString &aProtocol, const QString &aHost,
  const QString &aPort)
  : mSSL( false )
{
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveResult(KIO::Slave *, int, const QString &)));
  mUrl.setProtocol(aProtocol);
  mUrl.setHost(aHost);
  if (aPort != "993" && aPort != "995" && aPort != "465")
    mUrl.setPort(aPort.toInt());

  startOffSlave();
}

//-----------------------------------------------------------------------------
KMServerTest::~KMServerTest()
{
  if (mJob) mJob->kill(TRUE);
}


KIO::MetaData KMServerTest::slaveConfig() const {
  KIO::MetaData md;
  md.insert( "nologin", "on" );
  return md;
}

void KMServerTest::startOffSlave() {
  mSlave = KIO::Scheduler::getConnectedSlave( mUrl, slaveConfig() );
  if ( !mSlave ) {
    slotSlaveResult( 0, 1 );
    return;
  }
  connect( mSlave, SIGNAL(metaData(const KIO::MetaData&)),
	   SLOT(slotMetaData(const KIO::MetaData&)) );

  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );

  stream << (int) 'c';

  mJob = KIO::special( mUrl, packedArgs, false );
  KIO::Scheduler::assignJobToSlave( mSlave, mJob );
  connect( mJob, SIGNAL(result(KIO::Job*)), SLOT(slotResult(KIO::Job*)) );
  connect( mJob, SIGNAL(infoMessage(KIO::Job*,const QString&)),
	   SLOT(slotData(KIO::Job*,const QString&)) );
}


//-----------------------------------------------------------------------------
void KMServerTest::slotData(KIO::Job *, const QString &data)
{
  if ( mList.empty() )
    mList = QStringList::split(' ', data);
kdDebug(5006) << data << endl;
kdDebug(5006) << "count = " << mList.count() << endl;
}


void KMServerTest::slotMetaData( const KIO::MetaData & md ) {
  KIO::MetaData::const_iterator it = md.find( "PLAIN AUTH METHODS" );
  if ( it != md.end() )
    mAuthNone = it.data();
  it = md.find( "TLS AUTH METHODS" );
  if ( it != md.end() )
    mAuthTLS = it.data();
  it = md.find( "SSL AUTH METHODS" );
  if ( it != md.end() )
    mAuthSSL = it.data();
}

//-----------------------------------------------------------------------------
void KMServerTest::slotResult(KIO::Job *job)
{
  slotSlaveResult(mSlave, job->error());
}

//-----------------------------------------------------------------------------
void KMServerTest::slotSlaveResult(KIO::Slave *aSlave, int error,
  const QString &)
{
  if (aSlave != mSlave) return;
  if (error != KIO::ERR_SLAVE_DIED && mSlave)
  {
    KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
  }
  if (!mSSL) {
    mSSL = true;
    if ( error )
      mList.clear();
    else
      mList.append("NORMAL-CONNECTION");
    mUrl.setProtocol(mUrl.protocol() + 's');
    mUrl.setPort(0);
    startOffSlave();
  } else {
    mJob = 0;
    if (!error) mList.append("SSL");
    if (mList.isEmpty())
      KMessageBox::error(0, i18n("<qt>Could not connect to server <b>%1</b>.</qt>")
      .arg(mUrl.host()));
    emit capabilities(mList);
    emit capabilities(mList, mAuthNone, mAuthSSL, mAuthTLS);
  }
}


#include "kmservertest.moc"
