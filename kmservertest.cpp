/**
 * kmservertest.cpp
 *
 * Copyright (c) 2001 Michael Haeckel <Michael@Haeckel.Net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <klocale.h>
#include <kmessagebox.h>
#include <kio/scheduler.h>

#include "kmservertest.h"

//-----------------------------------------------------------------------------
KMServerTest::KMServerTest(const QString &aProtocol, const QString &aHost,
  const QString &aPort)
{
  mFirstTry = TRUE;
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveResult(KIO::Slave *, int, const QString &)));
  mSlaveConfig.insert("nologin", "on");
  mUrl.setProtocol(aProtocol);
  mUrl.setHost(aHost);
  if (aPort != "993" && aPort != "995") mUrl.setPort(aPort.toInt());
  mSlave = KIO::Scheduler::getConnectedSlave(mUrl, mSlaveConfig);
  mJob = KIO::special(mUrl, QCString("capa"), FALSE);
  KIO::Scheduler::assignJobToSlave(mSlave, mJob);
  connect(mJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
          SLOT(slotData(KIO::Job *, const QString &)));
  connect(mJob, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
}


//-----------------------------------------------------------------------------
KMServerTest::~KMServerTest()
{
  if (mJob) mJob->kill(TRUE);
}


//-----------------------------------------------------------------------------
void KMServerTest::slotData(KIO::Job *, const QString &data)
{
  mList = QStringList::split(' ', data);
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
  if (error != KIO::ERR_SLAVE_DIED) KIO::Scheduler::disconnectSlave(mSlave);
  if (mFirstTry)
  {
    mFirstTry = FALSE;
    mUrl.setProtocol(mUrl.protocol() + "s");
    mUrl.setPort(0);
    mSlave = KIO::Scheduler::getConnectedSlave(mUrl, mSlaveConfig);
    mJob = KIO::special(mUrl, QCString("capa"), FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, mJob);
    connect(mJob, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
    if (error) 
    {
      connect(mJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
              SLOT(slotData(KIO::Job *, const QString &)));
    } else {
      mList.append("NORMAL-CONNECTION");
    }
  } else {
    mJob = NULL;
    if (!error) mList.append("SSL");
    if (mList.isEmpty())
      KMessageBox::error(0, i18n("Could not connect to server %1")
      .arg(mUrl.host()));
    emit capabilities(mList);   
    delete this;
  }
}


#include "kmservertest.moc"
