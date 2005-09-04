/*  -*- c++ -*-
    sievejob.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __KMAIL_SIEVE_JOB_H__
#define __KMAIL_SIEVE_JOB_H__

#include <qobject.h>
#include <q3valuestack.h>
#include <qstring.h>
#include <qstringlist.h>
#include <q3cstring.h>

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
    enum Command { Get, Put, Activate, Deactivate, SearchActive, List, Delete };
    SieveJob( const KURL & url, const QString & script,
	      const Q3ValueStack<Command> & commands,
	      QObject * parent=0, const char * name=0 );
    virtual ~SieveJob();

  public:
    enum Existence { DontKnow, Yes, No };

    /**
     * Store a Sieve script. If @param makeActive is set, also mark the
     * script active
     */
    static SieveJob * put( const KURL & dest, const QString & script,
			   bool makeActive, bool wasActive );

    /**
     * Get a specific Sieve script
     */
    static SieveJob * get( const KURL & src );

    /**
     * List all available scripts
     */
    static SieveJob * list( const KURL & url );

    static SieveJob * del( const KURL & url );

    static SieveJob * activate( const KURL & url );

    void kill( bool quiet=true );

    const QStringList & sieveCapabilities() const {
      return mSieveCapabilities;
    }

    bool fileExists() const {
      return mFileExists;
    }

  signals:
    void gotScript( KMail::SieveJob * job, bool success,
		    const QString & script, bool active );

    /**
     * We got the list of available scripts
     *
     * @param scriptList is the list of script filenames
     * @param activeScript lists the filename of the active script, or an
     *        empty string if no script is active.
     */
    void gotList( KMail::SieveJob *job, bool success,
                  const QStringList &scriptList, const QString &activeScript );
    
    void result(  KMail::SieveJob * job, bool success,
                  const QString & script, bool active );

    void item( KMail::SieveJob * job, const QString & filename, bool active );

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
    Q3ValueStack<Command> mCommands;

    // List of Sieve scripts on the server, used by @ref list()
    QStringList mAvailableScripts;
  };

} // namespace KMail

#endif // __KMAIL_SIEVE_JOB_H__

// vim: set noet sts=2 ts=8 sw=2:

