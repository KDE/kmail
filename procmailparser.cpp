/**
 *   The code here is taken from accountdialog.cpp, which is:
 *   Copyright (C) 2000 Espen Sand, espe
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "procmailparser.h"

#include <QDir>
#include <QTextStream>

#ifdef HAVE_PATHS_H
#include <paths.h>  /* defines _PATH_MAILDIR */
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif

using namespace KMail;

ProcmailRCParser::ProcmailRCParser(const QString &filename)
  : mProcmailrc(filename),
    mStream(new QTextStream(&mProcmailrc))
{
  // predefined
  mVars.insert( "HOME", QDir::homePath() );

  if( filename.isEmpty() ) {
    mProcmailrc.setFileName(QDir::homePath() + "/.procmailrc");
  }

  QRegExp lockFileGlobal("^LOCKFILE=", Qt::CaseSensitive);
  QRegExp lockFileLocal("^:0", Qt::CaseSensitive);

  if(  mProcmailrc.open(QIODevice::ReadOnly) ) {

    QString s;

    while( !mStream->atEnd() ) {

      s = mStream->readLine().trimmed();

      if(  s[0] == '#' ) continue; // skip comments

      int commentPos = -1;

      if( (commentPos = s.indexOf('#')) > -1 ) {
        // get rid of trailing comment
        s.truncate(commentPos);
        s = s.trimmed();
      }

      if(  lockFileGlobal.indexIn(s) != -1 ) {
        processGlobalLock(s);
      } else if( lockFileLocal.indexIn(s) != -1 ) {
        processLocalLock(s);
      } else if( int i = s.indexOf('=') ) {
        processVariableSetting(s,i);
      }
    }

  }
  QString default_Location = qgetenv("MAIL");

  if (default_Location.isNull()) {
    default_Location = _PATH_MAILDIR;
    default_Location += '/';
    default_Location += qgetenv("USER");
  }
  if ( !mSpoolFiles.contains(default_Location) )
    mSpoolFiles << default_Location;

  default_Location = default_Location + ".lock";
  if ( !mLockFiles.contains(default_Location) )
    mLockFiles << default_Location;
}

ProcmailRCParser::~ProcmailRCParser()
{
  delete mStream;
}

void
ProcmailRCParser::processGlobalLock(const QString &s)
{
  QString val = expandVars(s.mid(s.indexOf('=') + 1).trimmed());
  if ( !mLockFiles.contains(val) )
    mLockFiles << val;
}

void
ProcmailRCParser::processLocalLock(const QString &s)
{
  QString val;
  int colonPos = s.lastIndexOf(':');

  if (colonPos > 0) { // we don't care about the leading one
    val = s.mid(colonPos + 1).trimmed();

    if ( val.length() ) {
      // user specified a lockfile, so process it
      //
      val = expandVars(val);
      if ( val[0] != '/' && mVars.contains("MAILDIR") )
        val.insert(0, mVars["MAILDIR"] + '/');
    } // else we'll deduce the lockfile name one we
    // get the spoolfile name
  }

  // parse until we find the spoolfile
  QString line, prevLine;
  do {
    prevLine = line;
    line = mStream->readLine().trimmed();
  } while ( !mStream->atEnd() &&
             ( line[0] == '*' ||
               prevLine.length() > 0 && prevLine[prevLine.length() - 1] == '\\' ) );

  if( line[0] != '!' && line[0] != '|' &&  line[0] != '{' ) {
    // this is a filename, expand it
    //
    line =  line.trimmed();
    line = expandVars(line);

    // prepend default MAILDIR if needed
    if( line[0] != '/' && mVars.contains("MAILDIR") )
      line.insert(0, mVars["MAILDIR"] + '/');

    // now we have the spoolfile name
    if ( !mSpoolFiles.contains(line) )
      mSpoolFiles << line;

    if( colonPos > 0 && val.isEmpty() ) {
      // there is a local lockfile, but the user didn't
      // specify the name so compute it from the spoolfile's name
      val = line;

      // append lock extension
      if( mVars.contains("LOCKEXT") )
        val += mVars["LOCKEXT"];
      else
        val += ".lock";
    }

    if ( !val.isNull() && !mLockFiles.contains(val) ) {
      mLockFiles << val;
    }
  }

}

void
ProcmailRCParser::processVariableSetting(const QString &s, int eqPos)
{
  if( eqPos == -1) return;

  QString varName = s.left(eqPos),
    varValue = expandVars(s.mid(eqPos + 1).trimmed());

  mVars.insert( varName.toLatin1(), varValue );
}

QString
ProcmailRCParser::expandVars(const QString &s)
{
  if( s.isEmpty()) return s;

  QString expS = s;

  for ( QHash<QByteArray, QString>::const_iterator it = mVars.constBegin(); it != mVars.constEnd(); ++it ) {
    expS.replace( QString::fromLatin1("$") + it.key(), it.value() );
  }

  return expS;
}
