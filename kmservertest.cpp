/**
 * kmservertest.cpp
 *
 * Copyright (c) 2001-2002 Michael Haeckel <haeckel@kde.org>
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

#include <config.h>
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
  if (aPort != "993" && aPort != "995" && aPort != "465")
    mUrl.setPort(aPort.toInt());
  mSlave = KIO::Scheduler::getConnectedSlave(mUrl, mSlaveConfig);
  if (!mSlave)
  {
    slotSlaveResult(0, 1);
    return;
  }
  
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly);
    
  stream << (int) 'c';

  mJob = KIO::special(mUrl, packedArgs, FALSE);
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


#include <kdebug.h>
//-----------------------------------------------------------------------------
void KMServerTest::slotData(KIO::Job *, const QString &data)
{
  mList = QStringList::split(' ', data);
kdDebug(5006) << data << endl;
kdDebug(5006) << "count = " << mList.count() << endl;
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
  if (mFirstTry)
  {
    mFirstTry = FALSE;
    if (!error) mList.append("NORMAL-CONNECTION");
    mUrl.setProtocol(mUrl.protocol() + "s");
    mUrl.setPort(0);
    mSlave = KIO::Scheduler::getConnectedSlave(mUrl, mSlaveConfig);
    if (!mSlave)
    {
      slotSlaveResult(0, 1);
      return;
    }

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly);
    
    stream << (int) 'c';

    mJob = KIO::special(mUrl, packedArgs, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, mJob);
    connect(mJob, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
    if (error)
    {
      connect(mJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
              SLOT(slotData(KIO::Job *, const QString &)));
    }
  } else {
    mJob = 0;
    if (!error) mList.append("SSL");
    if (mList.isEmpty())
      KMessageBox::error(0, i18n("<qt>Could not connect to server <b>%1</b>.</qt>")
      .arg(mUrl.host()));
    emit capabilities(mList);
  }
}


#include "kmservertest.moc"
