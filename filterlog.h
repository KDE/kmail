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
#ifndef KMAIL_FILTERLOG_H
#define KMAIL_FILTERLOG_H

#include <qobject.h>
#include <qstringlist.h>

namespace KMail {

  /**
    @short KMail Filter Log Collector.
    @author Andreas Gungl <a.gungl@gmx.de>

    The filter log helps to collect log information about the
    filter process in KMail. It's implemented as singleton,
    so it's easy to direct pieces of information to a unique
    instance.
    It's possible to activate / deactivate logging. All
    collected log information can get thrown away, the
    next added log entry is the first one until another 
    clearing.
    A signal is emitted whenever a new logentry is added.
  */
  class FilterLog : public QObject
  {
    Q_OBJECT

    public:
      /** access to the singleton instance */
      static FilterLog * instance();
      
      /** check the logging state */
      bool isLogging() { return logging; };
      /** set the logging state */
      void setLogging( bool active ) { logging = active; };

      /** add a log entry */
      void add( QString logEntry );
      /** discard collected log data */
      void clear() { logEntries.clear(); };
      
      /** get access to the log entries */
      const QStringList & getLogEntries() { return logEntries; };
      /** dump the log - for testing purposes */
      void dump();
      
      /** destructor */
      virtual ~FilterLog();
      
    signals:
      void logEntryAdded( QString );

    protected:
      /** Non-public constructor needed by the singleton implementation */
      FilterLog() { self = this; logging = true; };
      
      /** The list contains the single log pieces */
      QStringList logEntries;
      /** the log status */
      bool logging;
      
    private:
      static FilterLog * self;
  };

} // namespace KMail

#endif // KMAIL_FILTERLOG_H
