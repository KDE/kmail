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

#include "kmpopheaders.h"
#include "kmmessage.h"

KMPopHeaders::KMPopHeaders()
  : mAction( NoAction ),
    mId(),
    mUid(),
    mRuleMatched( false ),
    mHeader( 0 )
{
}

KMPopHeaders::~KMPopHeaders()
{
  if ( mHeader )
    delete mHeader;
}

KMPopHeaders::KMPopHeaders( const QByteArray & id, const QByteArray & uid,
                            KMPopFilterAction action )
  : mAction( action ),
    mId( id ),
    mUid( uid ),
    mRuleMatched( false ),
    mHeader( 0 )
{
}

QByteArray KMPopHeaders::id() const
{
  return mId;
}

QByteArray KMPopHeaders::uid() const
{
  return mUid;
}

KMMessage * KMPopHeaders::header() const
{
  return mHeader;
}

void KMPopHeaders::setHeader( KMMessage *header )
{
  mHeader = header;
}

KMPopFilterAction KMPopHeaders::action() const
{
  return mAction;
}

void KMPopHeaders::setAction( KMPopFilterAction action )
{
  mAction = action;
}

void KMPopHeaders::setRuleMatched( bool matched )
{
  mRuleMatched = matched;
}

bool KMPopHeaders::ruleMatched() const
{
  return mRuleMatched;
}
