/*
  Copyright (c) 2001 Heiko Hund <heiko@ist.eigentlich.net>
  Copyright (c) 2001 Thorsten Zachmann <t.zachmann@zagge.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KMPopHeaders_H
#define KMPopHeaders_H

#include <QByteArray>

class KMMessage;

//Keep these corresponding to the column numbers in the dialog for easier coding
//or change mapToAction and mapToColumn in PopHeadersView
enum KMPopFilterAction { Down=0, Later=1, Delete=2, NoAction=3 };

class KMPopHeaders {
public:

  KMPopHeaders();
  ~KMPopHeaders();

  KMPopHeaders( const QByteArray & id, const QByteArray & uid,
                KMPopFilterAction action );

  /** returns the id of the message */
  QByteArray id() const;

  /** returns the uid of the message */
  QByteArray uid() const;

  /** returns the header of the message */
  KMMessage * header() const;

  /**
   * Set the header of the message.
   * The header is then managed by this class, which
   * deletes the header in the destructor.
   */
  void setHeader( KMMessage *header );

  KMPopFilterAction action() const;

  void setAction( KMPopFilterAction action );

  bool ruleMatched() const;

  void setRuleMatched( bool matched );

private:

  KMPopFilterAction mAction;
  QByteArray mId;
  QByteArray mUid;
  bool mRuleMatched;
  KMMessage *mHeader;
};

#endif
