/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

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
*/
// KMail broadcast status message and related classes

#ifndef __km_broadcast_status_h
#define __km_broadcast_status_h

#include <qobject.h>
#include <qmap.h>

#undef None

namespace KPIM {
  class ProgressItem;
}

/** When KMail is running it is possible to have multiple KMMainWin open
    at the same time. We want certain messages/information to be displayed
    in all active KMMainWins. KMBroadcastStatus make this possible, it
    defines a singleton object that broadcasts status messages by emitting
    signals. All KMMainWins connect up these signals to appropriate slots
    for updating their status bar. */

class KMBroadcastStatus : public QObject
{

  Q_OBJECT

public:

  /** Return the instance of the singleton object for this class */
  static KMBroadcastStatus *instance();
  /** Emit an update status bar signal */
  void setStatusMsg( const QString& message );
  /** Return the last status message from setStatusMsg() */
  QString statusMsg() const { return mStatusMsg; }
  /** Sets a status bar message with timestamp */
  void setStatusMsgWithTimestamp( const QString& message );
  /** Sets a transmission completed status bar message */
  void setStatusMsgTransmissionCompleted( int numMessages,
                                          int numBytes = -1,
                                          int numBytesRead = -1,
                                          int numBytesToRead = -1,
                                          bool mLeaveOnServer = false,
                                          KPIM::ProgressItem* progressItem = 0 ); // set the same status in this progress item
  void setStatusMsgTransmissionCompleted( const QString& account,
                                          int numMessages,
                                          int numBytes = -1,
                                          int numBytesRead = -1,
                                          int numBytesToRead = -1,
                                          bool mLeaveOnServer = false,
                                          KPIM::ProgressItem* progressItem = 0 ); // set the same status in this progress item

signals:

  /** Emitted when setStatusMsg is called. */
  void statusMsg( const QString& );

protected:

  KMBroadcastStatus();
  QString mStatusMsg;
  static KMBroadcastStatus* instance_;
};



#endif
