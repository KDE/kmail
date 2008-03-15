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

#include "sievejob.h"

#include <kio/job.h>
#include <kio/deletejob.h>
#include <kio/jobuidelegate.h>
using KIO::Job;
// <kio/global.h>
using KIO::UDSEntryList;
using KIO::UDSEntry;
#include <kdebug.h>

#include <QTextCodec>

#include <cassert>

namespace KMail {

  SieveJob::SieveJob( const KUrl & url, const QString & script,
		      const QStack<Command> & commands,
		      QObject * parent, const char * name )
    : QObject( parent ),
      mUrl( url ), mJob( 0 ), mDec( 0 ),
      mScript( script ), mFileExists( DontKnow ), mCommands( commands )
  {
    setObjectName( name );
    assert( !commands.isEmpty() );
    schedule( commands.top() );
  }

  SieveJob::~SieveJob() {
    kill();
    delete mDec;
    kDebug(5006) <<"~SieveJob()";
  }

  void SieveJob::kill( KJob::KillVerbosity verbosity ) {
    if ( mJob ) mJob->kill( verbosity );
  }

  void SieveJob::schedule( Command command ) {
    switch ( command ) {
    case Get:
      kDebug(5006) << "get(" << mUrl.prettyUrl() <<" )";
      mJob = KIO::get( mUrl );
      connect( mJob, SIGNAL(data(KIO::Job*,const QByteArray&)),
               SLOT(slotData(KIO::Job*,const QByteArray&)) );
      break;
    case Put:
      kDebug(5006) << "put(" << mUrl.prettyUrl() <<" )";
      mJob = KIO::put( mUrl, 0600, KIO::Overwrite );
      connect( mJob, SIGNAL(dataReq(KIO::Job*,QByteArray&)),
               SLOT(slotDataReq(KIO::Job*,QByteArray&)) );
      break;
    case Activate:
      kDebug(5006) << "chmod(" << mUrl.prettyUrl() <<", 0700 )";
      mJob = KIO::chmod( mUrl, 0700 );
      break;
    case Deactivate:
      kDebug(5006) << "chmod(" << mUrl.prettyUrl() <<", 0600 )";
      mJob = KIO::chmod( mUrl, 0600 );
      break;
    case SearchActive:
      kDebug(5006) << "listDir(" << mUrl.prettyUrl() <<" )";
      {
        KUrl url = mUrl;
        QString query = url.query(); //save query part, because KUrl::cd() erases it
        if ( !url.fileName().isEmpty() )
          url.cd("..");
        url.setQuery( query );
        kDebug(5006) << "listDir's real URL:" << url.prettyUrl();
        mJob = KIO::listDir( url );
        connect( mJob, SIGNAL(entries(KIO::Job*,const KIO::UDSEntryList&)),
                 SLOT(slotEntries(KIO::Job*,const KIO::UDSEntryList&)) );
        break;
      }
    case List:
      kDebug(5006) << "listDir(" << mUrl.prettyUrl() <<" )";
      {
        mJob = KIO::listDir( mUrl );
        connect( mJob, SIGNAL( entries(KIO::Job *, const KIO::UDSEntryList & ) ),
                 SLOT( slotEntries( KIO::Job *, const KIO::UDSEntryList & ) ) );
        break;
      }
    case Delete:
      kDebug(5006) << "delete(" << mUrl.prettyUrl() <<" )";
      mJob = KIO::del( mUrl );
      break;
    default:
      assert( 0 );
    }
    // common to all jobs:
    connect( mJob, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)) );
  }

  void SieveJob::slotData( Job *, const QByteArray & data ) {
    // check for end-of-data marker:
    if ( data.size() == 0 )
      return;

    // make sure we have a textdecoder;
    if ( !mDec )
      mDec = QTextCodec::codecForMib( 106 /*utf8*/ )->makeDecoder();

    // decode utf8; add to mScript:
    mScript += mDec->toUnicode( data.data(), data.size() );
  }

  void SieveJob::slotDataReq( Job *, QByteArray & data ) {
    // check whether we have already sent our data:
    if ( mScript.isEmpty() ) {
      data = QByteArray(); // end-of-data marker
      return;
    }

    // Convert mScript into UTF-8:
    data = mScript.toUtf8();

    // "data" contains a trailing NUL, remove:
    if ( data.size() > 0 && data[(int)data.size() - 1] == '\0' )
      data.resize( data.size() - 1 );

    // mark mScript sent:
    mScript.clear();
  }

  void SieveJob::slotEntries( Job *, const UDSEntryList & l ) {
    // loop over entries:
    for ( UDSEntryList::const_iterator it = l.begin() ; it != l.end() ; ++it ) {
      // Loop over all UDS atoms to find the UDSEntry::UDS_ACCESS and UDS_NAME atoms;
      // note if we find an exec'able file ( == active script )
      // or the requested filename (mUrl.fileName()).
      const QString filename = it->stringValue( KIO::UDSEntry::UDS_NAME );
      mAvailableScripts.append( filename );
      bool isActive = ( it->numberValue( KIO::UDSEntry::UDS_ACCESS ) == 0700 );

      if ( isActive )
	mActiveScriptName = filename;

      if ( mFileExists == DontKnow && filename == mUrl.fileName() )
	mFileExists = Yes;
      emit item( this, filename, isActive );
      if ( mFileExists == Yes && !mActiveScriptName.isEmpty() )
	return; // early return if we have all information
    }
  }

  void SieveJob::slotResult( KJob * job ) {
    Command lastCmd = mCommands.top();

    // First, let's see if we come back from a SearchActive. If so, set
    // mFileExists to No if we didn't see the mUrl.fileName() during
    // listDir...
    if ( lastCmd == SearchActive && mFileExists == DontKnow && !job->error() )
      mFileExists = No;
    // prepare for next round:
    mCommands.pop();
    delete mDec; mDec = 0;

    if ( mSieveCapabilities.empty() ) {
      mSieveCapabilities = static_cast<KIO::Job*>(job)->queryMetaData( "sieveExtensions" ).split(' ', QString::SkipEmptyParts );
      kDebug(5006) <<"Received Sieve extensions supported:\n" << mSieveCapabilities.join("\n");
    }

    // check for errors:
    if ( job->error() ) {
      static_cast<KIO::Job*>(job)->ui()->setWindow( 0 );
      static_cast<KIO::Job*>(job)->ui()->showErrorMessage();

      emit result( this, false, mScript, mUrl.fileName() == mActiveScriptName );

      if ( lastCmd == List )
	emit gotList( this, false, mAvailableScripts, mActiveScriptName );
      else
	emit gotScript( this, false, mScript, mUrl.fileName() == mActiveScriptName );

      mJob = 0;
      delete this;
      return;
    }

    // check for new tasks:
    if ( !mCommands.empty() ) {
      // Don't fail get'ting a non-existant script:
      if ( mCommands.top() == Get && mFileExists == No ) {
	mScript.clear();
	mCommands.pop();
      }
    }

    if ( mCommands.empty() ) {
      // was last command; report success and delete this object:
      emit result( this, true, mScript, mUrl.fileName() == mActiveScriptName );
      if ( lastCmd == List )
	emit gotList( this, true, mAvailableScripts, mActiveScriptName );
      else
	emit gotScript( this, true, mScript, mUrl.fileName() == mActiveScriptName );

      mJob = 0; // deletes itself on returning from this slot
      delete this;
      return;
    } else {
      // schedule the next command:
      schedule( mCommands.top() );
    }
  }

  SieveJob * SieveJob::put( const KUrl & dest, const QString & script,
			    bool makeActive, bool wasActive ) {
    QStack<Command> commands;
    if ( makeActive )
      commands.push( Activate );
    if ( wasActive )
      commands.push( Deactivate );
    commands.push( Put );
    return new SieveJob( dest, script, commands );
  }

  SieveJob * SieveJob::get( const KUrl & src ) {
    QStack<Command> commands;
    commands.push( Get );
    commands.push( SearchActive );
    return new SieveJob( src, QString(), commands );
  }

  SieveJob * SieveJob::list( const KUrl & src ) {
    QStack<Command> commands;
    commands.push( List );
    return new SieveJob( src, QString(), commands );
  }
  SieveJob * SieveJob::del( const KUrl & url ) {
    QStack<Command> commands;
    commands.push( Delete );
    return new SieveJob( url, QString(), commands );
  }

  SieveJob * SieveJob::activate( const KUrl & url ) {
    QStack<Command> commands;
    commands.push( Activate );
    return new SieveJob( url, QString(), commands );
  }

} // namespace KMail

#include "sievejob.moc"

// vim: set noet sts=2 ts=8 sw=2:

