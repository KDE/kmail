/*  -*- c++ -*-
    kmservertest.h

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

#ifndef kmservertest_h
#define kmservertest_h

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>

namespace KIO {
  class Job;
  class Slave;
  class SimpleJob;
  class MetaData;
}

class KMServerTest : public QObject
{
  Q_OBJECT
  
public:
  KMServerTest( const QString & protocol, const QString & host, int port );
  virtual ~KMServerTest();

signals:
  void capabilities(const QStringList &);
  void capabilities(const QStringList & caps, const QString & authNone,
		    const QString & authSSL, const QString & authTLS );

protected slots:
  void slotData(KIO::Job *job, const QString &data);
  void slotResult(KIO::Job *job);
  void slotMetaData( const KIO::MetaData & );
  void slotSlaveResult(KIO::Slave *aSlave, int error,
    const QString &errorText = QString::null);

protected:
  KIO::MetaData slaveConfig() const;
  void startOffSlave( int port=0 );

protected:
  const QString  mProtocol;
  const QString  mHost;
  bool           mSSL;
  QStringList    mList;
  QString        mAuthNone;
  QString        mAuthSSL;
  QString        mAuthTLS;
  KIO::SimpleJob *mJob;
  KIO::Slave     *mSlave;
};

#endif
