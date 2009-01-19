/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/aggregation.h"

#include <QDataStream>

#include <klocale.h>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

static const int gAggregationCurrentVersion = 0x1009; // increase if you add new fields of change the meaning of some


Aggregation::Aggregation(
      const QString &name,
      const QString &description,
      Grouping grouping,
      GroupExpandPolicy groupExpandPolicy,
      Threading threading,
      ThreadLeader threadLeader,
      ThreadExpandPolicy threadExpandPolicy,
      FillViewStrategy fillViewStrategy
    )
  : OptionSet( name, description ),
    mGrouping( grouping ),
    mGroupExpandPolicy( groupExpandPolicy ),
    mThreading( threading ),
    mThreadLeader( threadLeader ),
    mThreadExpandPolicy( threadExpandPolicy ),
    mFillViewStrategy( fillViewStrategy )
{
}

Aggregation::Aggregation(
      const Aggregation &opt
    )
  : OptionSet( opt ),
    mGrouping( opt.mGrouping ),
    mGroupExpandPolicy( opt.mGroupExpandPolicy ),
    mThreading( opt.mThreading ),
    mThreadLeader( opt.mThreadLeader ),
    mThreadExpandPolicy( opt.mThreadExpandPolicy ),
    mFillViewStrategy( opt.mFillViewStrategy )
{
}

Aggregation::Aggregation()
  : OptionSet(),
    mGrouping( NoGrouping ),
    mGroupExpandPolicy( NeverExpandGroups ),
    mThreading( NoThreading ),
    mThreadLeader( TopmostMessage ),
    mThreadExpandPolicy( NeverExpandThreads ),
    mFillViewStrategy( FavorInteractivity )
{
}

bool Aggregation::load( QDataStream &stream )
{
  int val;

  stream >> val;
  if ( val != gAggregationCurrentVersion )
    return false; // b0rken (invalid version)

  stream >> val;
  mGrouping = (Grouping)val;
  switch( mGrouping )
  {
    case NoGrouping:
    case GroupByDate:
    case GroupByDateRange:
    case GroupBySenderOrReceiver:
    case GroupBySender:
    case GroupByReceiver:
      // ok
    break;
    default:
      // b0rken
      return false;
    break;
  };

  stream >> val; // Formerly contained group sorting
  stream >> val; // Formerly contained group sorting direction

  stream >> val;
  mGroupExpandPolicy = (GroupExpandPolicy)val;
  switch( mGroupExpandPolicy )
  {
    case NeverExpandGroups:
    case ExpandRecentGroups:
    case AlwaysExpandGroups:
      // ok
    break;
    default:
      // b0rken
      return false;
    break;
  };

  stream >> val;
  mThreading = (Threading)val;
  switch( mThreading )
  {
    case NoThreading:
    case PerfectOnly:
    case PerfectAndReferences:
    case PerfectReferencesAndSubject:
      // ok
    break;
    default:
      // b0rken
      return false;
    break;
  }

  stream >> val;
  mThreadLeader = (ThreadLeader)val;
  switch( mThreadLeader )
  {
    case MostRecentMessage:
    case TopmostMessage:
      // ok
      // FIXME: Should check that thread leaders setting matches grouping and threading settings.
    break;
    default:
      // b0rken
      return false;
    break;
  }

  stream >> val;
  mThreadExpandPolicy = (ThreadExpandPolicy)val;
  switch( mThreadExpandPolicy )
  {
    case NeverExpandThreads:
    case ExpandThreadsWithNewMessages:
    case ExpandThreadsWithUnreadMessages:
    case ExpandThreadsWithUnreadOrImportantMessages:
    case AlwaysExpandThreads:
      // ok
    break;
    default:
      // b0rken
      return false;
    break;
  }

  stream >> val; // Formely contained message sorting
  stream >> val; // Formely contained message sort direction

  stream >> val;
  mFillViewStrategy = (FillViewStrategy)val;
  switch( mFillViewStrategy )
  {
    case FavorSpeed:
    case FavorInteractivity:
    case BatchNoInteractivity:
      // ok
    break;
    default:
      // b0rken
      return false;
    break;
  }

  return true;
}

void Aggregation::save( QDataStream &stream ) const
{
  stream << (int)gAggregationCurrentVersion;
  stream << (int)mGrouping;
  stream << 0; // Formerly group sorting
  stream << 0; // Formerly group sort direction
  stream << (int)mGroupExpandPolicy;
  stream << (int)mThreading;
  stream << (int)mThreadLeader;
  stream << (int)mThreadExpandPolicy;
  stream << 0; // Formerly message sorting
  stream << 0; // Formerly message sort direction
  stream << (int)mFillViewStrategy;
}

QList< QPair< QString, int > > Aggregation::enumerateGroupingOptions()
{
  QList< QPair< QString, int > > ret;
  ret.append( QPair< QString, int >( i18nc( "No grouping of messages", "None" ), NoGrouping ) );
  ret.append( QPair< QString, int >( i18n( "by Exact Date (of Thread Leaders)" ), GroupByDate ) );
  ret.append( QPair< QString, int >( i18n( "by Smart Date Ranges (of Thread Leaders)" ), GroupByDateRange ) );
  ret.append( QPair< QString, int >( i18n( "by Smart Sender/Receiver" ), GroupBySenderOrReceiver ) );
  ret.append( QPair< QString, int >( i18n( "by Sender" ), GroupBySender ) );
  ret.append( QPair< QString, int >( i18n( "by Receiver" ), GroupBySender ) );
  return ret;
}


QList< QPair< QString, int > > Aggregation::enumerateGroupExpandPolicyOptions( Grouping g )
{
  QList< QPair< QString, int > > ret;
  if ( g == NoGrouping )
    return ret;
  ret.append( QPair< QString, int >( i18n( "Never Expand Groups" ), NeverExpandGroups ) );
  if ( ( g == GroupByDate ) || ( g == GroupByDateRange ) )
    ret.append( QPair< QString, int >( i18n( "Expand Recent Groups" ), ExpandRecentGroups ) );
  ret.append( QPair< QString, int >( i18n( "Always Expand Groups" ), AlwaysExpandGroups ) );
  return ret;
}

QList< QPair< QString, int > > Aggregation::enumerateThreadingOptions()
{
  QList< QPair< QString, int > > ret;
  ret.append( QPair< QString, int >( i18nc( "No threading of messages", "None" ), NoThreading ) );
  ret.append( QPair< QString, int >( i18n( "Perfect Only" ), PerfectOnly ) );
  ret.append( QPair< QString, int >( i18n( "Perfect and by References" ), PerfectAndReferences ) );
  ret.append( QPair< QString, int >( i18n( "Perfect, by References and by Subject" ), PerfectReferencesAndSubject ) );
  return ret;
}

QList< QPair< QString, int > > Aggregation::enumerateThreadLeaderOptions( Grouping g, Threading t )
{
  QList< QPair< QString, int > > ret;
  if ( t == NoThreading )
    return ret;
  ret.append( QPair< QString, int >( i18n( "Topmost Message" ), TopmostMessage ) );
  if ( ( g != GroupByDate ) && ( g != GroupByDateRange ) )
    return ret;
  ret.append( QPair< QString, int >( i18n( "Most Recent Message" ), MostRecentMessage ) );
  return ret;
}

QList< QPair< QString, int > > Aggregation::enumerateThreadExpandPolicyOptions( Threading t )
{
  QList< QPair< QString, int > > ret;
  if ( t == NoThreading )
    return ret;
  ret.append( QPair< QString, int >( i18n( "Never Expand Threads" ), NeverExpandThreads ) );
  ret.append( QPair< QString, int >( i18n( "Expand Threads With New Messages" ), ExpandThreadsWithNewMessages ) );
  ret.append( QPair< QString, int >( i18n( "Expand Threads With Unread Messages" ), ExpandThreadsWithUnreadMessages ) );
  ret.append( QPair< QString, int >( i18n( "Expand Threads With Unread or Important Messages" ), ExpandThreadsWithUnreadOrImportantMessages ) );
  ret.append( QPair< QString, int >( i18n( "Always Expand Threads" ), AlwaysExpandThreads ) );
  return ret;
}

QList< QPair< QString, int > > Aggregation::enumerateFillViewStrategyOptions()
{
  QList< QPair< QString, int > > ret;
  ret.append( QPair< QString, int >( i18n( "Favor Interactivity" ), FavorInteractivity ) );
  ret.append( QPair< QString, int >( i18n( "Favor Speed" ), FavorSpeed ) );
  ret.append( QPair< QString, int >( i18n( "Batch Job (No Interactivity)" ), BatchNoInteractivity ) );
  return ret;
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
