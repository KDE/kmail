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


using namespace KMail;


FilterLog * FilterLog::self = NULL;


FilterLog::FilterLog()
{ 
  self = this;
  // start with logging enabled by default
  logging = true;
  // better limit the log to 512 KByte to avoid out of memory situations
  // when the log i sgoing to become very long
  maxLogSize = 512 * 1024;
  currentLogSize = 0;
  allowedTypes =  meta | patternDesc | ruleResult | 
                  patternResult | appliedAction;
};


FilterLog::~FilterLog()
{}


FilterLog * FilterLog::instance()
{
  if ( !self ) self = new FilterLog();
  return self;
}


void FilterLog::add( QString logEntry, ContentType contentType )
{
#ifndef NDEBUG
  kdDebug(5006) << "New filter log entry: " << logEntry << endl;
#endif
  if ( isLogging() && ( allowedTypes & contentType ) )
  {
    logEntries.append( logEntry );
    emit logEntryAdded( logEntry );
    currentLogSize += logEntry.length();
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
  maxLogSize = size; 
  checkLogSize(); 
};

      
void FilterLog::dump()
{
#ifndef NDEBUG
  kdDebug(5006) << "----- starting filter log -----" << endl;
  for ( QStringList::Iterator it = logEntries.begin();
        it != logEntries.end(); ++it )
  {
    kdDebug(5006) << *it << endl;
  }
  kdDebug(5006) << "------ end of filter log ------" << endl;
#endif
}


void FilterLog::checkLogSize()
{
  if ( currentLogSize > maxLogSize && maxLogSize >= -1 )
  {
    kdDebug(5006) << "Filter log: memory limit reached, starting to discard old items, size = "
                  << QString::number( currentLogSize ) << endl;
    // avoid some kind of hysteresis, shrink the log to 90% of its maximum
    while ( currentLogSize > ( maxLogSize * 0.9 ) )
    {
      QValueListIterator<QString> it = logEntries.begin();
      if ( it != logEntries.end())
      {
        currentLogSize -= (*it).length();
        logEntries.remove( it );
        kdDebug(5006) << "Filter log: new size = " << QString::number( currentLogSize ) << endl;
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


#include "filterlog.moc"
