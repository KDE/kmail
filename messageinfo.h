/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>

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

#ifndef KMAIL_MESSAGEINFO_H
#define KMAIL_MESSAGEINFO_H

#include <QList>
#include <QMap>
#include <QSet>
#include <kiconloader.h>


namespace KMime {
  class Content;
  class Message;
}


/**
  This class keeps some extra information for KMime::Message objects used by KMail (somewhat like KMMsgInfo did)
  @author Andras Mantia <andras@kdab.net>
*/

/** Flags for the "MDN sent" state. */
typedef enum
{
    KMMsgMDNStateUnknown = ' ',
    KMMsgMDNNone = 'N',
    KMMsgMDNIgnore = 'I',
    KMMsgMDNDisplayed = 'R',
    KMMsgMDNDeleted = 'D',
    KMMsgMDNDispatched = 'F',
    KMMsgMDNProcessed = 'P',
    KMMsgMDNDenied = 'X',
    KMMsgMDNFailed = 'E'
} KMMsgMDNSentState;

namespace KMail {

class MessageInfo {
public:
    static MessageInfo * instance();

    ~MessageInfo();

    void clear();

    void setMDNSentState( KMime::Content* node, const KMMsgMDNSentState state );
    KMMsgMDNSentState mdnSentState( KMime::Content *node ) const;

private:
    MessageInfo();

    static MessageInfo * mSelf;
    QMap<KMime::Content *, KMMsgMDNSentState> mMDNSentState;
};

}

#endif
