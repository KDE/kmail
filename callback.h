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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <qstring.h>

class KMMessage;

#include <kdepimmacros.h>

namespace KMail {

/** This class is used for callback hooks needed by bodypart
    formatter plugins. It also holds a pointer to the message we are
    working on.
    Feel free to put whatever you want in here. It's not supposed to be
    a nicely formatted class, but simply include everything necessary
    for the plugins. */
class KDE_EXPORT Callback {
public:
  Callback( KMMessage* msg );

  /** Get the full message */
  KMMessage* getMsg() const { return mMsg; }

  /** Mail a message */
  bool mailICal( const QString& to, const QString iCal,
                 const QString& subject ) const;

  /** Get the receiver of the mail */
  QString receiver() const;

private:
  KMMessage* mMsg;
  mutable QString mReceiver;
  mutable bool mReceiverSet;
};

}

#endif /* CALLBACK_H */

