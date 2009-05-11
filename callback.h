/*  -*- c++ -*-
    callback.h

    This file is used by KMail's plugin interface.
    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <qstring.h>

class KMMessage;
class KMReaderWin;

#include <kdepimmacros.h>


#include <libkcal/attendee.h> // only for an enum, we are not linking


namespace KMail {

/** This class is used for callback hooks needed by bodypart
    formatter plugins. It also holds a pointer to the message we are
    working on.
    Feel free to put whatever you want in here. It's not supposed to be
    a nicely formatted class, but simply include everything necessary
    for the plugins. */
class KDE_EXPORT Callback {
public:
  Callback( KMMessage* msg, KMReaderWin* readerWin );

  /** Get the full message */
  KMMessage* getMsg() const { return mMsg; }

  /** Mail a message
   * @ param status can be accepted/cancel/tentative/delegated
   */
  bool mailICal( const QString &to, const QString &iCal,
                 const QString &subject, const QString &status,
                 bool delMessage = true ) const;

  /** Get the receiver of the mail */
  QString receiver() const;

  /** Returns the sender of the mail. */
  QString sender() const;

  /** Close the main window showing this message, if it's a secondary window. */
  void closeIfSecondaryWindow() const;

  bool askForComment( KCal::Attendee::PartStat status ) const;
  bool deleteInvitationAfterReply() const;
  bool exchangeCompatibleInvitations() const;

private:
  KMMessage* mMsg;
  KMReaderWin* mReaderWin;
  mutable QString mReceiver;
  mutable bool mReceiverSet;
};

}

#endif /* CALLBACK_H */

