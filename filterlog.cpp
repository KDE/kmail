/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

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

#include "filterlog.h"

#include <kdebug.h>

#include <qdatetime.h>
#include <qfile.h>

#include <sys/stat.h>


using namespace KMail;


FilterLog * FilterLog::mSelf = NULL;


FilterLog::FilterLog()
{ 
  mSelf = this;
  // start with logging disabled by default
  mLogging = false;
  // better limit the log to 512 KByte to avoid out of memory situations
  // when the log i sgoing to become very long
  mMaxLogSize = 512 * 1024;
  mCurrentLogSize = 0;
  mAllowedTypes =  meta | patternDesc | ruleResult | 
                   patternResult | appliedAction;
}


FilterLog::~FilterLog()
{}


FilterLog * FilterLog::instance()
{
  if ( !mSelf ) mSelf = new FilterLog();
  return mSelf;
}


void FilterLog::add( QString logEntry, ContentType contentType )
{
  if ( isLogging() && ( mAllowedTypes & contentType ) )
  {
    QString timedLog = "[" + QTime::currentTime().toString() + "] ";
    if ( contentType & ~meta )
      timedLog += logEntry;
    else
      timedLog = logEntry;
    mLogEntries.append( timedLog );
    emit logEntryAdded( timedLog );
    mCurrentLogSize += timedLog.length();
    checkLogSize();
  }
}


void FilterLog::setMaxLogSize( long size ) 
{
  if ( size < -1)
    size = -1;
  // do not allow less than 1 KByte except unlimited (-1)
  if ( size >= 0 && size < 1024 )
    size = 1024; 
  mMaxLogSize = size; 
  emit logStateChanged();
  checkLogSize(); 
}

      
void FilterLog::dump()
{
#ifndef NDEBUG
  kdDebug(5006) << "----- starting filter log -----" << endl;
  for ( QStringList::Iterator it = mLogEntries.begin();
        it != mLogEntries.end(); ++it )
  {
    kdDebug(5006) << *it << endl;
  }
  kdDebug(5006) << "------ end of filter log ------" << endl;
#endif
}


void FilterLog::checkLogSize()
{
  if ( mCurrentLogSize > mMaxLogSize && mMaxLogSize > -1 )
  {
    kdDebug(5006) << "Filter log: memory limit reached, starting to discard old items, size = "
                  << QString::number( mCurrentLogSize ) << endl;
    // avoid some kind of hysteresis, shrink the log to 90% of its maximum
    while ( mCurrentLogSize > ( mMaxLogSize * 0.9 ) )
    {
      QValueListIterator<QString> it = mLogEntries.begin();
      if ( it != mLogEntries.end())
      {
        mCurrentLogSize -= (*it).length();
        mLogEntries.remove( it );
        kdDebug(5006) << "Filter log: new size = " 
                      << QString::number( mCurrentLogSize ) << endl;
      }
      else
      {
        kdDebug(5006) << "Filter log: size reduction disaster!" << endl;
        clear();
      }
    }
    emit logShrinked();
  }
}


bool FilterLog::saveToFile( QString fileName )
{
    QFile file( fileName );
    if( file.open( IO_WriteOnly ) ) {
      fchmod( file.handle(), S_IRUSR | S_IWUSR );
      {
        QDataStream ds( &file );
        for ( QStringList::Iterator it = mLogEntries.begin(); 
              it != mLogEntries.end(); ++it ) 
        {
          QString tmpString = *it + '\n';
          QCString cstr( tmpString.local8Bit() );
          ds.writeRawBytes( cstr, cstr.size() );
        }
      }
      return true;
    } 
    else
      return false;
}


#include "filterlog.moc"
