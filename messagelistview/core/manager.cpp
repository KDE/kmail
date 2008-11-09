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

#include "messagelistview/core/manager.h"

#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/configureaggregationsdialog.h"
#include "messagelistview/core/skin.h"
#include "messagelistview/core/view.h"
#include "messagelistview/core/configureskinsdialog.h"
#include "messagelistview/core/widgetbase.h"
#include "messagelistview/core/storagemodelbase.h"

#include <QPixmap>

#include <kmime/kmime_dateformatter.h> // kdepimlibs

#include <KConfig>
#include <KIconLoader>
#include <KGlobalSettings>
#include <KApplication>
#include <KLocale>

#include "kmkernel.h"       // These includes add a soft dependancy on stuff outside MessageListView::Core.
#include "globalsettings.h" // It should be quite simple to remove it if we need this stuff to become a library.

namespace KMail
{

namespace MessageListView
{

namespace Core
{

Manager * Manager::mInstance = 0;

Manager::Manager()
{
  mInstance = this;

  mDateFormatter = new KMime::DateFormatter();

  mDisplayMessageToolTips = true;

  mPixmapMessageNew = new QPixmap( SmallIcon( "mail-unread-new" ) );
  mPixmapMessageUnread = new QPixmap( SmallIcon( "mail-unread" ) );
  mPixmapMessageRead = new QPixmap( SmallIcon( "mail-read" ) );
  mPixmapMessageDeleted = new QPixmap( SmallIcon( "mail-deleted" ) );
  mPixmapMessageReplied = new QPixmap( SmallIcon( "mail-replied" ) );
  mPixmapMessageRepliedAndForwarded = new QPixmap( SmallIcon( "mail-forwarded-replied" ) );
  mPixmapMessageQueued = new QPixmap( SmallIcon( "mail-queued" ) ); // mail-queue ?
  mPixmapMessageToDo = new QPixmap( SmallIcon( "mail-task" ) );
  mPixmapMessageSent = new QPixmap( SmallIcon( "mail-sent" ) );
  mPixmapMessageForwarded = new QPixmap( SmallIcon( "mail-forwarded" ) );
  mPixmapMessageImportant = new QPixmap( SmallIcon( "emblem-important" ) ); // "flag"
  mPixmapMessageWatched = new QPixmap( SmallIcon( "mail-thread-watch" ) );
  mPixmapMessageIgnored = new QPixmap( SmallIcon( "mail-thread-ignored" ) );
  mPixmapMessageSpam = new QPixmap( SmallIcon( "mail-mark-junk" ) );
  mPixmapMessageHam = new QPixmap( SmallIcon( "mail-mark-notjunk" ) );
  mPixmapMessageFullySigned = new QPixmap( SmallIcon( "mail-signed-verified" ) );
  mPixmapMessagePartiallySigned = new QPixmap( SmallIcon( "mail-signed-part" ) );
  mPixmapMessageUndefinedSigned = new QPixmap( SmallIcon( "mail-signed" ) );
  mPixmapMessageNotSigned = new QPixmap( SmallIcon( "text-plain" ) );
  mPixmapMessageFullyEncrypted = new QPixmap( SmallIcon( "mail-encrypted-full" ) );
  mPixmapMessagePartiallyEncrypted = new QPixmap( SmallIcon( "mail-encrypted-part" ) );
  mPixmapMessageUndefinedEncrypted = new QPixmap( SmallIcon( "mail-encrypted" ) );
  mPixmapMessageNotEncrypted = new QPixmap( SmallIcon( "text-plain" ) );
  mPixmapMessageAttachment = new QPixmap( SmallIcon( "mail-attachment" ) );
  //mPixmapShowMore = new QPixmap( SmallIcon( "list-add.png" ) );
  //mPixmapShowLess = new QPixmap( SmallIcon( "list-remove.png" ) );
  if ( KApplication::isRightToLeft() )
    mPixmapShowMore = new QPixmap( SmallIcon( "arrow-left" ) );
  else
    mPixmapShowMore = new QPixmap( SmallIcon( "arrow-right" ) );
  mPixmapShowLess = new QPixmap( SmallIcon( "arrow-down" ) );
  mPixmapVerticalLine = new QPixmap( SmallIcon( "mail-vertical-separator-line" ) );
  mPixmapHorizontalSpacer = new QPixmap( SmallIcon( "mail-horizontal-space" ) );

  mCachedLocalizedUnknownText = i18nc( "Unknown date", "Unknown" ) ;

  loadConfiguration();
}

Manager::~Manager()
{
  saveConfiguration();
  removeAllAggregations();

  delete mPixmapMessageNew;
  delete mPixmapMessageUnread;
  delete mPixmapMessageRead;
  delete mPixmapMessageDeleted;
  delete mPixmapMessageReplied;
  delete mPixmapMessageRepliedAndForwarded;
  delete mPixmapMessageQueued;
  delete mPixmapMessageToDo;
  delete mPixmapMessageSent;
  delete mPixmapMessageForwarded;
  delete mPixmapMessageImportant; // "flag"
  delete mPixmapMessageWatched;
  delete mPixmapMessageIgnored;
  delete mPixmapMessageSpam;
  delete mPixmapMessageHam;
  delete mPixmapMessageFullySigned;
  delete mPixmapMessagePartiallySigned;
  delete mPixmapMessageUndefinedSigned;
  delete mPixmapMessageNotSigned;
  delete mPixmapMessageFullyEncrypted;
  delete mPixmapMessagePartiallyEncrypted;
  delete mPixmapMessageUndefinedEncrypted;
  delete mPixmapMessageNotEncrypted;
  delete mPixmapMessageAttachment;
  delete mPixmapShowMore;
  delete mPixmapShowLess;
  delete mPixmapVerticalLine;
  delete mPixmapHorizontalSpacer;

  delete mDateFormatter;

  ConfigureAggregationsDialog::cleanup(); // make sure it's dead
  ConfigureSkinsDialog::cleanup(); // make sure it's dead


  mInstance = 0;
}

void Manager::showConfigureAggregationsDialog( Widget *requester, const QString &preselectAggregationId )
{
  ConfigureAggregationsDialog::display( requester ? requester->window() : 0, preselectAggregationId );
}

void Manager::showConfigureSkinsDialog( Widget *requester, const QString &preselectSkinId )
{
  ConfigureSkinsDialog::display( requester ? requester->window() : 0, preselectSkinId );
}


void Manager::registerWidget( Widget *pWidget )
{
  if ( !mInstance )
    mInstance = new Manager();

  mInstance->mWidgetList.append( pWidget );
}

void Manager::unregisterWidget( Widget *pWidget )
{
  if ( !mInstance )
  {
    qWarning("ERROR: MessageListView::Manager::unregisterWidget() called when Manager::mInstance is 0");
    return;
  }

  mInstance->mWidgetList.removeAll( pWidget );

  if ( mInstance->mWidgetList.count() < 1 )
  {
    delete mInstance;
    mInstance = 0;
  }
}

unsigned long Manager::preSelectedMessageForStorageModel( const StorageModel *storageModel )
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelSelectedMessages" );

  // QVariant supports unsigned int OR unsigned long long int, NOT unsigned long int... doh...
  qulonglong defValue = 0;

  return conf.readEntry( QString( "MessageUniqueIdForStorageModel%1" ).arg( storageModel->id() ), defValue );
}

void Manager::savePreSelectedMessageForStorageModel( const StorageModel * storageModel, unsigned long uniqueIdOfMessage )
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelSelectedMessages" );


  if ( uniqueIdOfMessage )
  {
    // QVariant supports unsigned int OR unsigned long long int, NOT unsigned long int... doh...
    qulonglong val = uniqueIdOfMessage;

    conf.writeEntry( QString( "MessageUniqueIdForStorageModel%1" ).arg( storageModel->id() ), val );
  } else
    conf.deleteEntry( QString( "MessageUniqueIdForStorageModel%1" ).arg( storageModel->id() ) );
}

const Aggregation * Manager::aggregation( const QString &id )
{
  Aggregation * opt = mAggregations.value( id );
  if ( opt )
    return opt;
    
  return defaultAggregation();
}

const Aggregation * Manager::defaultAggregation()
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelAggregations" );

  QString aggregationId = conf.readEntry( QString( "DefaultSet" ), "" );
   
  Aggregation * opt = 0;

  if ( !aggregationId.isEmpty() )
    opt = mAggregations.value( aggregationId );

  if ( opt )
    return opt;

  // try just the first one
  QHash< QString, Aggregation * >::Iterator it = mAggregations.begin();
  if ( it != mAggregations.end() )
    return *it;

  // aargh
  createDefaultAggregations();

  it = mAggregations.begin();
  return *it;
}

void Manager::saveAggregationForStorageModel( const StorageModel *storageModel, const QString &id, bool storageUsesPrivateAggregation )
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelAggregations" );

  if ( storageUsesPrivateAggregation )
    conf.writeEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ), id );
  else
    conf.deleteEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ) );

  // This always becomes the "current" default aggregation
  conf.writeEntry( QString( "DefaultSet" ), id );
}


const Aggregation * Manager::aggregationForStorageModel( const StorageModel *storageModel, bool *storageUsesPrivateAggregation )
{
  Q_ASSERT( storageUsesPrivateAggregation );

  *storageUsesPrivateAggregation = false; // this is by default

  if ( !storageModel )
    return defaultAggregation();

  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelAggregations" );

  QString aggregationId = conf.readEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ), "" );

  Aggregation * opt = 0;

  if ( !aggregationId.isEmpty() )
  {
    // a private aggregation was stored
    opt = mAggregations.value( aggregationId );
    *storageUsesPrivateAggregation = ( opt != 0 );
  }

  if ( opt )
    return opt;

  // FIXME: If the storageModel is a mailing list, maybe suggest a mailing-list like preset...
  //        We could even try to guess if the storageModel is a mailing list

  return defaultAggregation();
}

void Manager::addAggregation( Aggregation *set )
{
  Aggregation * old = mAggregations.value( set->id() );
  if ( old )
    delete old;
  mAggregations.insert( set->id(), set );
}

void Manager::createDefaultAggregations()
{
  addAggregation(
      new Aggregation(
          i18n( "Current Activity, Threaded" ),
          i18n( "This view uses smart date range groups with the most recent group displayed on top. " \
                "Messages are threaded and the most recent is displayed again on top. " \
                "The group for a thread is selected by the most recent message in it. " \
                "So for example, in \"Today\" you will find all the messages arrived today " \
                "and all the threads that have been active today."
            ),
          Aggregation::GroupByDateRange,
          Aggregation::SortGroupsByDateTime,
          Aggregation::Descending,
          Aggregation::ExpandRecentGroups,
          Aggregation::PerfectReferencesAndSubject,
          Aggregation::MostRecentMessage,
          Aggregation::ExpandThreadsWithUnreadMessages,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending,
          Aggregation::FavorInteractivity
       )
    );

  addAggregation(
      new Aggregation(
          i18n( "Current Activity, Flat" ),
          i18n( "This view uses smart date range groups with the most recent group displayed on top. " \
                "Messages are not threaded and the most recent message is displayed on top. " \
                "So for example, in \"Today\" you will simply find all the messages arrived today."
            ),
          Aggregation::GroupByDateRange,
          Aggregation::SortGroupsByDateTime,
          Aggregation::Descending,
          Aggregation::ExpandRecentGroups,
          Aggregation::NoThreading,
          Aggregation::MostRecentMessage,
          Aggregation::NeverExpandThreads,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending,
          Aggregation::FavorInteractivity
       )
    );

  addAggregation(
      new Aggregation(
          i18n( "Activity by Date, Threaded" ),
          i18n( "This view uses day-by-day groups with the most recent group displayed on top. " \
                "Messages are threaded and the most recent is displayed again on top. " \
                "The group for a thread is selected by the most recent message in it. " \
                "So for example, in \"Today\" you will find all the messages arrived today " \
                "and all the threads that have been active today."
            ),
          Aggregation::GroupByDate,
          Aggregation::SortGroupsByDateTime,
          Aggregation::Descending,
          Aggregation::ExpandRecentGroups,
          Aggregation::PerfectReferencesAndSubject,
          Aggregation::MostRecentMessage,
          Aggregation::ExpandThreadsWithUnreadMessages,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending,
          Aggregation::FavorInteractivity
       )
    );

  addAggregation(
      new Aggregation(
          i18n( "Activity by Date, Flat" ),
          i18n( "This view uses day-by-day groups with the most recent group displayed on top. " \
                "Messages are not threaded and the most recent message is displayed on top. " \
                "So for example, in \"Today\" you will simply find all the messages arrived today."
            ),
          Aggregation::GroupByDate,
          Aggregation::SortGroupsByDateTime,
          Aggregation::Descending,
          Aggregation::ExpandRecentGroups,
          Aggregation::NoThreading,
          Aggregation::MostRecentMessage,
          Aggregation::NeverExpandThreads,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending,
          Aggregation::FavorInteractivity
       )
    );

  addAggregation(
      new Aggregation(
          i18n( "Standard Mailing List" ),
          i18n( "This is a plain and old mailing list view: no groups, heavy threading and " \
                "messages displayed with the most recent on bottom"
            ),
          Aggregation::NoGrouping,
          Aggregation::SortGroupsByDateTimeOfMostRecent,
          Aggregation::Ascending,
          Aggregation::NeverExpandGroups,
          Aggregation::PerfectReferencesAndSubject,
          Aggregation::TopmostMessage,
          Aggregation::ExpandThreadsWithUnreadMessages,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Ascending,
          Aggregation::FavorInteractivity
       )
    );

  addAggregation(
      new Aggregation(
          i18n( "Flat Date View" ),
          i18n( "This is a plain and old list of messages sorted by date: no groups, no threading " \
                "and messages displayed with the most recent on top by default"
            ),
          Aggregation::NoGrouping,
          Aggregation::SortGroupsByDateTimeOfMostRecent,
          Aggregation::Ascending,
          Aggregation::NeverExpandGroups,
          Aggregation::NoThreading,
          Aggregation::TopmostMessage,
          Aggregation::NeverExpandThreads,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending,
          Aggregation::FavorInteractivity
        )
    );

  addAggregation(
      new Aggregation(
          i18n( "Senders/Receivers, Flat" ),
          i18n( "This view groups the messages by senders or receivers (depending on the folder " \
                "type). Groups are sorted by the sender/receiver name or e-mail. " \
                "Messages are not threaded and the most recent message is displayed on bottom."
            ),
          Aggregation::GroupBySenderOrReceiver,
          Aggregation::SortGroupsBySenderOrReceiver,
          Aggregation::Ascending,
          Aggregation::NeverExpandGroups,
          Aggregation::NoThreading,
          Aggregation::TopmostMessage,
          Aggregation::NeverExpandThreads,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Ascending,
          Aggregation::FavorSpeed
        )
    );

  addAggregation(
      new Aggregation(
          i18n( "Thread Starters" ),
          i18n( "This view groups the messages in threads and then groups the threads by the starting user. " \
                "Groups are sorted by the user name or e-mail. "
            ),
          Aggregation::GroupBySenderOrReceiver,
          Aggregation::SortGroupsBySenderOrReceiver,
          Aggregation::Ascending,
          Aggregation::NeverExpandGroups,
          Aggregation::PerfectReferencesAndSubject,
          Aggregation::TopmostMessage,
          Aggregation::NeverExpandThreads,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Ascending,
          Aggregation::FavorSpeed
        )
    );

/*
  FIX THIS
  addAggregation(
      new Aggregation(
          i18n( "Recent Thread Starters" ),
          i18n( "This view groups the messages in threads and then groups the threads by the starting user. " \
                "Groups are sorted by the date of the first thread start. "
            ),
          Aggregation::GroupBySenderOrReceiver,
          Aggregation::SortGroupsByDateTimeOfMostRecent,
          Aggregation::Descending,
          Aggregation::PerfectReferencesAndSubject,
          Aggregation::TopmostMessage,
          Aggregation::SortMessagesByDateTime,
          Aggregation::Descending
        )
    );
*/
}

void Manager::removeAllAggregations()
{
  for( QHash< QString, Aggregation * >::Iterator it = mAggregations.begin(); it != mAggregations.end(); ++it )
    delete ( *it );

  mAggregations.clear();
}

void Manager::aggregationsConfigurationCompleted()
{
  if ( mAggregations.count() < 1 )
    createDefaultAggregations(); // panic

  saveConfiguration(); // just to be sure :)

  // notify all the widgets that they should reload the option set combos
  for( QList< Widget * >::Iterator it = mWidgetList.begin(); it != mWidgetList.end(); ++it )
    ( *it )->aggregationsChanged();
}


const Skin * Manager::skin( const QString &id )
{
  Skin * opt = mSkins.value( id );
  if ( opt )
    return opt;
    
  return defaultSkin();
}

const Skin * Manager::defaultSkin()
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelSkins" );

  QString skinId = conf.readEntry( QString( "DefaultSet" ), "" );
   
  Skin * opt = 0;

  if ( !skinId.isEmpty() )
    opt = mSkins.value( skinId );

  if ( opt )
    return opt;

  // try just the first one
  QHash< QString, Skin * >::Iterator it = mSkins.begin();
  if ( it != mSkins.end() )
    return *it;

  // aargh
  createDefaultSkins();

  it = mSkins.begin();

  Q_ASSERT( it != mSkins.end() );

  return *it;
}

void Manager::saveSkinForStorageModel( const StorageModel *storageModel, const QString &id, bool storageUsesPrivateSkin )
{
  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelSkins" );

  if ( storageUsesPrivateSkin )
    conf.writeEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ), id );
  else
    conf.deleteEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ) );

  // This always becomes the "current" default skin
  conf.writeEntry( QString( "DefaultSet" ), id );
}


const Skin * Manager::skinForStorageModel( const StorageModel *storageModel, bool *storageUsesPrivateSkin )
{
  Q_ASSERT( storageUsesPrivateSkin );

  *storageUsesPrivateSkin = false; // this is by default

  if ( !storageModel )
    return defaultSkin();

  KConfigGroup conf( KMKernel::config(), "MessageListView::StorageModelSkins" );
  QString skinId = conf.readEntry( QString( "SetForStorageModel%1" ).arg( storageModel->id() ), "" );

  Skin * opt = 0;

  if ( !skinId.isEmpty() )
  {
    // a private skin was stored
    opt = mSkins.value( skinId );
    *storageUsesPrivateSkin = (opt != 0);
  }

  if ( opt )
    return opt;

  // FIXME: If the storageModel is a mailing list, maybe suggest a mailing-list like preset...
  //        We could even try to guess if the storageModel is a mailing list

  // FIXME: Prefer right-to-left skins when application layout is RTL.

  return defaultSkin();
}

void Manager::addSkin( Skin *set )
{
  Skin * old = mSkins.value( set->id() );
  if ( old )
    delete old;
  mSkins.insert( set->id(), set );
}

static Skin::Column * add_skin_simple_text_column( Skin * s, const QString &name, Skin::ContentItem::Type type, bool visibleByDefault, Aggregation::MessageSorting messageSorting, bool alignRight, bool addGroupHeaderItem )
{
  Skin::Column * c = new Skin::Column();
  c->setLabel( name );
  c->setVisibleByDefault( visibleByDefault );
  c->setMessageSorting( messageSorting );

  Skin::Row * r = new Skin::Row();

  Skin::ContentItem * i = new Skin::ContentItem( type );
  i->setFont( KGlobalSettings::generalFont() );

  if ( alignRight )
    r->addRightItem( i );
  else
    r->addLeftItem( i );

  c->addMessageRow( r );

  if ( addGroupHeaderItem )
  {
    Skin::Row * r = new Skin::Row();

    Skin::ContentItem * i = new Skin::ContentItem( type );
    i->setFont( KGlobalSettings::generalFont() );

    if ( alignRight )
      r->addRightItem( i );
    else
      r->addLeftItem( i );

    c->addGroupHeaderRow( r );
  }


  s->addColumn( c );

  return c;
}

static Skin::Column * add_skin_simple_icon_column( Skin * s, const QString &name, Skin::ContentItem::Type type, bool visibleByDefault, Aggregation::MessageSorting messageSorting )
{
  Skin::Column * c = new Skin::Column();
  c->setLabel( name );
  c->setVisibleByDefault( visibleByDefault );
  c->setMessageSorting( messageSorting );

  Skin::Row * r = new Skin::Row();

  Skin::ContentItem * i = new Skin::ContentItem( type );
  i->setSoftenByBlendingWhenDisabled( true );

  r->addLeftItem( i );

  c->addMessageRow( r );

  s->addColumn( c );

  return c;
}


void Manager::createDefaultSkins()
{
  Skin * s;
  Skin::Column * c;
  Skin::Row * r;
  Skin::ContentItem * i;

  // The "Simple" backward compatible skin

  s = new Skin(
      i18n( "Simple" ),
      i18n( "A simple, backward compatible, single row skin" )
    );

    c = new Skin::Column();
    c->setLabel( i18n( "Subject" ) );
    c->setMessageSorting( Aggregation::SortMessagesBySubject );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::ExpandedStateIcon );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::GroupHeaderLabel );
        QFont bigFont = KGlobalSettings::generalFont();
        //bigFont.setPointSize( 16 );
        bigFont.setBold( true );
        i->setFont( bigFont );
        i->setUseCustomFont( true );
      r->addLeftItem( i );
    c->addGroupHeaderRow( r );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::CombinedReadRepliedStateIcon );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::AttachmentStateIcon );
        i->setHideWhenDisabled( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::SignatureStateIcon );
        i->setHideWhenDisabled( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::EncryptionStateIcon );
        i->setHideWhenDisabled( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::Subject );
      r->addLeftItem( i );
    c->addMessageRow( r );

  s->addColumn( c );

  c = add_skin_simple_text_column( s, i18n( "Sender/Receiver" ), Skin::ContentItem::SenderOrReceiver, true, Aggregation::SortMessagesBySenderOrReceiver, false, false);
  c->setIsSenderOrReceiver( true );
  add_skin_simple_text_column( s, i18n( "Sender" ), Skin::ContentItem::Sender, false, Aggregation::SortMessagesBySender, false, false );
  add_skin_simple_text_column( s, i18n( "Receiver" ), Skin::ContentItem::Receiver, false, Aggregation::SortMessagesByReceiver, false, false );
  add_skin_simple_text_column( s, i18n( "Date" ), Skin::ContentItem::Date, true, Aggregation::SortMessagesByDateTime, false, false );
  add_skin_simple_text_column( s, i18n( "Most Recent Date" ), Skin::ContentItem::MostRecentDate, false, Aggregation::SortMessagesByDateTimeOfMostRecent, false, true );
  add_skin_simple_text_column( s, i18n( "Size" ), Skin::ContentItem::Size, false, Aggregation::SortMessagesBySize, false, false );
  add_skin_simple_icon_column( s, i18n( "Attachment" ), Skin::ContentItem::AttachmentStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "New/Unread" ), Skin::ContentItem::ReadStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Important" ), Skin::ContentItem::ImportantStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "To Do" ), Skin::ContentItem::ToDoStateIcon, false, Aggregation::SortMessagesByToDoStatus );
  add_skin_simple_icon_column( s, i18n( "Replied" ), Skin::ContentItem::RepliedStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Spam/Ham" ), Skin::ContentItem::SpamHamStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Watched/Ignored" ), Skin::ContentItem::WatchedIgnoredStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Encryption" ), Skin::ContentItem::EncryptionStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Signature" ), Skin::ContentItem::SignatureStateIcon, false, Aggregation::NoMessageSorting );
  add_skin_simple_icon_column( s, i18n( "Tag List" ), Skin::ContentItem::TagList, false, Aggregation::NoMessageSorting );

  addSkin( s );

  // The Fancy skin

  s = new Skin(
      i18n( "Fancy" ),
      i18n( "A fancy multiline and multi item skin" )
    );

    c = new Skin::Column();
    c->setLabel( i18n( "Message" ) );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::ExpandedStateIcon );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::GroupHeaderLabel );
        QFont aBigFont = KGlobalSettings::generalFont();
        aBigFont.setPointSize( 16 );
        aBigFont.setBold( true );
        i->setFont( aBigFont );
        i->setUseCustomFont( true );
      r->addLeftItem( i );
    c->addGroupHeaderRow( r );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::Subject );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::ReadStateIcon );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::RepliedStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::AttachmentStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::EncryptionStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::SignatureStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::TagList );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
    c->addMessageRow( r );

  Skin::Row * firstFancyRow = r; // save it so we can continue adding stuff below (after cloning the skin)

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::SenderOrReceiver );
        i->setSoftenByBlending( true );
        QFont aItalicFont = KGlobalSettings::generalFont();
        aItalicFont.setItalic( true );
        i->setFont( aItalicFont );
        i->setUseCustomFont( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::Date );
        i->setSoftenByBlending( true );
        i->setFont( aItalicFont );
        i->setUseCustomFont( true );
      r->addRightItem( i );
    c->addMessageRow( r );

  s->addColumn( c );

  // clone the "Fancy skin" here so we'll use it as starting point for the "Fancy with clickable status"
  Skin * fancyWithClickableStatus = new Skin( *s );
  fancyWithClickableStatus->generateUniqueId();

  // and continue the "Fancy" specific settings
  r = firstFancyRow;

        i = new Skin::ContentItem( Skin::ContentItem::ToDoStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::ImportantStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::SpamHamStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::WatchedIgnoredStateIcon );
        i->setHideWhenDisabled( true );
      r->addRightItem( i );

  s->setViewHeaderPolicy( Skin::NeverShowHeader );

  addSkin( s );


  // The "Fancy with Clickable Status" skin

  s = fancyWithClickableStatus;

  s->setName( i18n( "Fancy with Clickable Status" ) );
  s->setDescription( i18n( "A fancy multiline and multi item skin with a clickable status column" ) );

    c = new Skin::Column();
    c->setLabel( i18n( "Status" ) );
    c->setVisibleByDefault( true );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::ToDoStateIcon );
        i->setSoftenByBlendingWhenDisabled( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::ImportantStateIcon );
        i->setSoftenByBlendingWhenDisabled( true );
      r->addLeftItem( i );
    c->addMessageRow( r );

      r = new Skin::Row();
        i = new Skin::ContentItem( Skin::ContentItem::SpamHamStateIcon );
        i->setSoftenByBlendingWhenDisabled( true );
      r->addLeftItem( i );
        i = new Skin::ContentItem( Skin::ContentItem::WatchedIgnoredStateIcon );
        i->setSoftenByBlendingWhenDisabled( true );
      r->addLeftItem( i );
    c->addMessageRow( r );

  s->addColumn( c );

  addSkin( s );
}

void Manager::removeAllSkins()
{
  for( QHash< QString, Skin * >::Iterator it = mSkins.begin(); it != mSkins.end(); ++it )
    delete ( *it );

  mSkins.clear();
}

void Manager::skinsConfigurationCompleted()
{
  if ( mSkins.count() < 1 )
    createDefaultSkins(); // panic

  saveConfiguration(); // just to be sure :)

  // notify all the widgets that they should reload the option set combos
  for( QList< Widget * >::Iterator it = mWidgetList.begin(); it != mWidgetList.end(); ++it )
    ( *it )->skinsChanged();
}

void Manager::reloadAllWidgets()
{
  for( QList< Widget * >::Iterator it = mWidgetList.begin(); it != mWidgetList.end(); ++it )
  {
    if ( !( *it )->view() )
      continue;
    ( *it )->view()->reload();
  }
}


void Manager::reloadGlobalConfiguration()
{
  // This is called when configuration changes (probably edited by the options dialog)
  int oldDateFormat = (int)mDateFormatter->format();
  QString oldDateCustomFormat = mDateFormatter->customFormat();

  loadGlobalConfiguration();

  if (
       ( oldDateFormat != (int)mDateFormatter->format() ) ||
       ( oldDateCustomFormat != mDateFormatter->customFormat() )
     )
    reloadAllWidgets();
}


void Manager::loadGlobalConfiguration()
{
  // Load the date format
  KConfigGroup config( KMKernel::config(), "General" );

  KMime::DateFormatter::FormatType t = (KMime::DateFormatter::FormatType) config.readEntry(
      "dateFormat",
      ( int )KMime::DateFormatter::Fancy
    );
  mDateFormatter->setCustomFormat( config.readEntry( "customDateFormat", QString() ) );
  mDateFormatter->setFormat( t );  

  mDisplayMessageToolTips = GlobalSettings::self()->displayMessageToolTips();
}

void Manager::loadConfiguration()
{
  loadGlobalConfiguration();

  {
    // load Aggregations

    KConfigGroup conf( KMKernel::config(), "MessageListView::Aggregations" );

    mAggregations.clear();

    int cnt = conf.readEntry( "Count", (int)0 );

    int idx = 0;
    while ( idx < cnt )
    {
      QString data = conf.readEntry( QString( "Set%1" ).arg( idx ), QString() );
      if ( !data.isEmpty() )
      {
        Aggregation * set = new Aggregation();
        if ( set->loadFromString( data ) )
        {
          if ( Aggregation * old = mAggregations.value( set->id() ) )
            delete old;
          mAggregations.insert( set->id(), set );
        } else {
          delete set; // b0rken
        }
      }
      idx++;
    }

    if ( mAggregations.count() == 0 )
    {
      // don't allow zero configuration, create some presets
      createDefaultAggregations();
    }
  }

  {
    // load Skins

    KConfigGroup conf( KMKernel::config(), "MessageListView::Skins" );

    mSkins.clear();

    int cnt = conf.readEntry( "Count", (int)0 );

    int idx = 0;
    while ( idx < cnt )
    {
      QString data = conf.readEntry( QString( "Set%1" ).arg( idx ), QString() );
      if ( !data.isEmpty() )
      {
        Skin * set = new Skin();
        if ( set->loadFromString( data ) )
        {
          if ( Skin * old = mSkins.value( set->id() ) )
            delete old;
          mSkins.insert( set->id(), set );
        } else {
          kWarning() << "Saved skin loading failed";
          delete set; // b0rken
        }
      }
      idx++;
    }

    if ( mSkins.count() == 0 )
    {
      // don't allow zero configuration, create some presets
      createDefaultSkins();
    }
  }

}

void Manager::saveConfiguration()
{
  {
    // store aggregations

    KConfigGroup conf( KMKernel::config(), "MessageListView::Aggregations" );
    //conf.clear();

    conf.writeEntry( "Count", mAggregations.count() );

    int idx = 0;
    for( QHash< QString, Aggregation * >::Iterator it = mAggregations.begin(); it != mAggregations.end(); ++it )
    {
      conf.writeEntry( QString( "Set%1" ).arg( idx ), ( *it )->saveToString() );
      idx++;
    }
  }

  {
    // store skins

    KConfigGroup conf( KMKernel::config(), "MessageListView::Skins" );
    //conf.clear();

    conf.writeEntry( "Count", mSkins.count() );

    int idx = 0;
    for( QHash< QString, Skin * >::Iterator it = mSkins.begin(); it != mSkins.end(); ++it )
    {
      conf.writeEntry( QString( "Set%1" ).arg( idx ), ( *it )->saveToString() );
      idx++;
    }
  }
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
