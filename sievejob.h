/*  -*- c++ -*-
    sievejob.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_SIEVE_JOB_H__
#define __KMAIL_SIEVE_JOB_H__

#include <qobject.h>
#include <qvaluestack.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>

#include <kurl.h>
#include <kio/global.h>

class QTextDecoder;
namespace KIO {
  class Job;
}

namespace KMail {

  class SieveJob : public QObject {
    Q_OBJECT
  protected:
    enum Command { Get, Put, Activate, Deactivate, SearchActive };
    SieveJob( const KURL & url, const QString & script,
	      const QValueStack<Command> & commands,
	      QObject * parent=0, const char * name=0 );
    virtual ~SieveJob();

  public:
    enum Existence { DontKnow, Yes, No };

    static SieveJob * put( const KURL & dest, const QString & script,
			   bool makeActive, bool wasActive );
    static SieveJob * get( const KURL & src );

    void kill( bool quiet=true );

    const QStringList & sieveCapabilities() const {
      return mSieveCapabilities;
    }

    bool fileExists() const {
      return mFileExists;
    }

  signals:
    void result( KMail::SieveJob * job, bool success,
		 const QString & script, bool active );

  protected:
    void schedule( Command command );

  protected slots:
    void slotData( KIO::Job *, const QByteArray & ); // for get
    void slotDataReq( KIO::Job *, QByteArray & ); // for put
    void slotEntries( KIO::Job *, const KIO::UDSEntryList & ); // for listDir
    void slotResult( KIO::Job * ); // for all commands

  protected:
    KURL mUrl;
    KIO::Job * mJob;
    QTextDecoder * mDec;
    QString mScript;
    QString mActiveScriptName;
    Existence mFileExists;
    QStringList mSieveCapabilities;
    QValueStack<Command> mCommands;
  };

} // namespace KMail

#endif // __KMAIL_SIEVE_JOB_H__
