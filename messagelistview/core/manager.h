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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_MANAGER_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_MANAGER_H__

#include <QList>
#include <QHash>

class QPixmap;

namespace KMime
{
  class DateFormatter;
}

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Aggregation;
class Skin;
class StorageModel;
class Widget;

/**
 * @brief: The manager for all the existing MessageListView::Widget objects.
 *
 * This class is the "central" object of the whole MessageListView framework.
 * It's a singleton that can be accessed only by the means of static methods,
 * is created automatically when the first MessageListView::Widget object is created
 * and destroyed automatically when the last MessageListView::Widget object is destroyed.
 *
 * This class takes care of loading/storing/mantaining the settings for the
 * whole MessageListView framework. It also keeps track of all the existing
 * MessageListView::Widget objects and takes care of uptdating them when settings change.
 *
 * This object relies on KMKernel existence for its whole lifetime (and in particular
 * for loading and saving settings) thus you must make sure that KMKernel is created
 * before the first MessageListView::Widget and destroyed after the last MessageListView::Widget.
 * But in KMail this is always the case :)
 */
class Manager
{
protected:
  Manager();
  ~Manager();

private:
  static Manager *mInstance;
  QList< Widget * > mWidgetList;
  QHash< QString, Aggregation * > mAggregations;
  QHash< QString, Skin * > mSkins;
  KMime::DateFormatter * mDateFormatter;
  bool mDisplayMessageToolTips;
  QString mCachedLocalizedUnknownText;

  // pixmaps, never null

  QPixmap * mPixmapMessageNew;
  QPixmap * mPixmapMessageUnread;
  QPixmap * mPixmapMessageRead;
  QPixmap * mPixmapMessageDeleted;
  QPixmap * mPixmapMessageReplied;
  QPixmap * mPixmapMessageRepliedAndForwarded;
  QPixmap * mPixmapMessageQueued;
  QPixmap * mPixmapMessageToDo;
  QPixmap * mPixmapMessageSent;
  QPixmap * mPixmapMessageForwarded;
  QPixmap * mPixmapMessageImportant; // "flag"
  QPixmap * mPixmapMessageWatched;
  QPixmap * mPixmapMessageIgnored;
  QPixmap * mPixmapMessageSpam;
  QPixmap * mPixmapMessageHam;
  QPixmap * mPixmapMessageFullySigned;
  QPixmap * mPixmapMessagePartiallySigned;
  QPixmap * mPixmapMessageUndefinedSigned;
  QPixmap * mPixmapMessageNotSigned;
  QPixmap * mPixmapMessageFullyEncrypted;
  QPixmap * mPixmapMessagePartiallyEncrypted;
  QPixmap * mPixmapMessageUndefinedEncrypted;
  QPixmap * mPixmapMessageNotEncrypted;
  QPixmap * mPixmapMessageAttachment;
  QPixmap * mPixmapShowMore;
  QPixmap * mPixmapShowLess;
  QPixmap * mPixmapVerticalLine;
  QPixmap * mPixmapHorizontalSpacer;

public:
  // instance management
  static Manager * instance()
    { return mInstance; };

  // widget registration
  static void registerWidget( Widget *pWidget );
  static void unregisterWidget( Widget *pWidget );

  const KMime::DateFormatter * dateFormatter() const
    { return mDateFormatter; };

  // global pixmaps
  const QPixmap * pixmapMessageNew() const
    { return mPixmapMessageNew; };
  const QPixmap * pixmapMessageUnread() const
    { return mPixmapMessageUnread; };
  const QPixmap * pixmapMessageRead() const
    { return mPixmapMessageRead; };
  const QPixmap * pixmapMessageDeleted() const
    { return mPixmapMessageDeleted; };
  const QPixmap * pixmapMessageReplied() const
    { return mPixmapMessageReplied; };
  const QPixmap * pixmapMessageRepliedAndForwarded() const
    { return mPixmapMessageRepliedAndForwarded; };
  const QPixmap * pixmapMessageQueued() const
    { return mPixmapMessageQueued; };
  const QPixmap * pixmapMessageToDo() const
    { return mPixmapMessageToDo; };
  const QPixmap * pixmapMessageSent() const
    { return mPixmapMessageSent; };
  const QPixmap * pixmapMessageForwarded() const
    { return mPixmapMessageForwarded; };
  const QPixmap * pixmapMessageImportant() const
    { return mPixmapMessageImportant; };
  const QPixmap * pixmapMessageWatched() const
    { return mPixmapMessageWatched; };
  const QPixmap * pixmapMessageIgnored() const
    { return mPixmapMessageIgnored; };
  const QPixmap * pixmapMessageSpam() const
    { return mPixmapMessageSpam; };
  const QPixmap * pixmapMessageHam() const
    { return mPixmapMessageHam; };
  const QPixmap * pixmapMessageFullySigned() const
    { return mPixmapMessageFullySigned; };
  const QPixmap * pixmapMessagePartiallySigned() const
    { return mPixmapMessagePartiallySigned; };
  const QPixmap * pixmapMessageUndefinedSigned() const
    { return mPixmapMessageUndefinedSigned; };
  const QPixmap * pixmapMessageNotSigned() const
    { return mPixmapMessageNotSigned; };
  const QPixmap * pixmapMessageFullyEncrypted() const
    { return mPixmapMessageFullyEncrypted; };
  const QPixmap * pixmapMessagePartiallyEncrypted() const
    { return mPixmapMessagePartiallyEncrypted; };
  const QPixmap * pixmapMessageUndefinedEncrypted() const
    { return mPixmapMessageUndefinedEncrypted; };
  const QPixmap * pixmapMessageNotEncrypted() const
    { return mPixmapMessageNotEncrypted; };
  const QPixmap * pixmapMessageAttachment() const
    { return mPixmapMessageAttachment; };
  const QPixmap * pixmapShowMore() const
    { return mPixmapShowMore; };
  const QPixmap * pixmapShowLess() const
    { return mPixmapShowLess; };
  const QPixmap * pixmapVerticalLine() const
    { return mPixmapVerticalLine; };
  const QPixmap * pixmapHorizontalSpacer() const
    { return mPixmapHorizontalSpacer; };
  
  const QString & cachedLocalizedUnknownText() const
    { return mCachedLocalizedUnknownText; };

  /**
   * Returns true if the display of tooltips in the View instances is enabled
   * in the configuration and false otherwise.
   */
  bool displayMessageToolTips() const
    { return mDisplayMessageToolTips; };

  /**
   * Returns the unique id of the last selected message for the specified StorageModel.
   * Returns 0 if this value isn't stored in the configuration.
   */
  unsigned long preSelectedMessageForStorageModel( const StorageModel *storageModel );

  /**
   * Stores in the unique id of the last selected message for the specified StorageModel.
   * The uniqueIdOfMessage may be 0 (which means that no selection shall be stored in the configuration).
   */
  void savePreSelectedMessageForStorageModel( const StorageModel * storageModel, unsigned long uniqueIdOfMessage );

  // aggregation sets management
  const Aggregation * aggregationForStorageModel( const StorageModel *storageModel, bool *storageUsesPrivateAggregation );
  void saveAggregationForStorageModel( const StorageModel *storageModel, const QString &id, bool storageUsesPrivateAggregation );
  const Aggregation * defaultAggregation();
  const Aggregation * aggregation( const QString &id );

  void addAggregation( Aggregation *set );
  void removeAllAggregations();

  const QHash< QString, Aggregation * > & aggregations() const
    { return mAggregations; };

  void showConfigureAggregationsDialog( Widget *requester, const QString &aggregationId = QString() );

  /**
   * This is called by the aggregation configuration dialog
   * once the sets have been changed.
   */
  void aggregationsConfigurationCompleted();


  // skin sets management
  const Skin * skinForStorageModel( const StorageModel *storageModel, bool *storageUsesPrivateSkin );
  void saveSkinForStorageModel( const StorageModel *storageModel, const QString &id, bool storageUsesPrivateSkin );
  const Skin * defaultSkin();
  const Skin * skin( const QString &id );

  void addSkin( Skin *set );
  void removeAllSkins();

  const QHash< QString, Skin * > & skins() const
    { return mSkins; };

  void showConfigureSkinsDialog( Widget *requester, const QString &preselectSkinId = QString() );

  /**
   * This is called by the skin configuration dialog
   * once the sets have been changed.
   */
  void skinsConfigurationCompleted();

  /**
   * Reloads the global configuration from the config files (so we assume it has changed)
   * The settings private to MessageListView (like Skins or Aggregations) aren't reloaded.
   * If the global configuration has changed then all the views are reloaded.
   */
  void reloadGlobalConfiguration();

  /**
   * Explicitly reloads the contents of all the widgets.
   */
  void reloadAllWidgets();

private:
  // internal configuration stuff
  void loadConfiguration();
  void saveConfiguration();
  void loadGlobalConfiguration();

  // internal option set management
  void createDefaultAggregations();
  void createDefaultSkins();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_MANAGER_H__
