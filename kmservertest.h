/**
 * kmservertest.h
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef kmservertest_h
#define kmservertest_h

#include <qobject.h>
#include <kio/job.h>

class KURL;
class QStringList;

class KMServerTest : public QObject
{
  Q_OBJECT
  
public:
  KMServerTest(const QString &aProtocol, const QString &aHost,
    const QString &aPort);
  ~KMServerTest();

signals:
  void capabilities(const QStringList &);

protected slots:
  void slotData(KIO::Job *job, const QString &data);
  void slotResult(KIO::Job *job);
  void slotSlaveResult(KIO::Slave *aSlave, int error,
    const QString &errorText = QString::null);

protected:
  bool           mFirstTry;
  KURL           mUrl;
  QStringList    mList;
  KIO::SimpleJob *mJob;
  KIO::Slave     *mSlave;
  KIO::MetaData  mSlaveConfig;
};

#endif
