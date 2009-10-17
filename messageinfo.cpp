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
#include "messageinfo.h"

#include <kmime/kmime_content.h>
#include <kmime/kmime_message.h>
#include <kdebug.h>

MessageInfo * MessageInfo::mSelf = 0;

MessageInfo * MessageInfo::instance()
{
  if ( !mSelf )
    mSelf = new MessageInfo();
  return mSelf;
}

MessageInfo::MessageInfo()
{
  mSelf = this;
}

MessageInfo::~MessageInfo()
{
  mSelf = 0;
}


void MessageInfo::clear()
{
  mMDNSentState.clear();
}


void MessageInfo::setMDNSentState( KMime::Content* node, const KMMsgMDNSentState state )
{
  mMDNSentState[node] = state;
}

KMMsgMDNSentState MessageInfo::mdnSentState( KMime::Content *node ) const
{
  if ( mMDNSentState.contains( node ) )
    return mMDNSentState[node];

  return KMMsgMDNStateUnknown;
}

