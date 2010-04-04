/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FOLDERCOLLECTION_H
#define FOLDERCOLLECTION_H

#include <kshortcut.h>
#include <akonadi/collection.h>
#include <akonadi/collectionstatistics.h>
#include <KConfigGroup>
#include <KIO/Job>

#include "mailinglist-magic.h"
using KMail::MailingList;

class KMMainWidget;

class FolderCollection : public QObject
{
  Q_OBJECT
public:
  static QSharedPointer<FolderCollection> forCollection( const Akonadi::Collection& coll );

  ~FolderCollection();

  enum CompactOptions { CompactLater, CompactNow, CompactSilentlyNow };

  /*
   * Define the possible units to use for measuring message expiry.
   * expireNever is used to switch off message expiry, and expireMaxUnits
   * must always be the last in the list (for bounds checking).
   */
  enum ExpireUnits { ExpireNever, ExpireDays, ExpireWeeks, ExpireMonths, ExpireMaxUnits };

  Akonadi::Collection collection();


  QString configGroupName() const;

  void writeConfig() const;
  void readConfig();

  QString name() const;

  QString idString() const;

  bool isReadOnly() const;

  bool noContent() const;

  bool isSystemFolder() const;

  qint64 count() const;

  bool canDeleteMessages() const;

  bool isValid() const;

  Akonadi::Collection::Rights rights() const;

  Akonadi::CollectionStatistics statistics() const;

  enum ExpireAction { ExpireDelete, ExpireMove };


  const KShortcut &shortcut() const { return mShortcut; }

  void setShortcut( const KShortcut&, KMMainWidget * );


  /** Get / set the user-settings for the WhoField (From/To/Empty) */
  QString userWhoField(void) { return mUserWhoField; }
  void setUserWhoField(const QString &whoField,bool writeConfig=true);

  /** Get / set whether the default identity should be used instead of the
   *  identity specified by setIdentity(). */
  void setUseDefaultIdentity( bool useDefaultIdentity );
  bool useDefaultIdentity() const { return mUseDefaultIdentity; }

  void setIdentity(uint identity);
  uint identity() const;

  /**
   * Set whether this folder automatically expires messages.
   */
  void setAutoExpire(bool enabled);

  /**
   * Does this folder automatically expire old messages?
   */
  bool isAutoExpire() const { return mExpireMessages; }

  /**
   * Set the maximum age for unread messages in this folder.
   * Age should not be negative. Units are set using
   * setUnreadExpireUnits().
   */
  void setUnreadExpireAge(int age);

  /**
   * Set units to use for expiry of unread messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void setUnreadExpireUnits(ExpireUnits units);

  /**
   * Set the maximum age for read messages in this folder.
   * Age should not be negative. Units are set using
   * setReadExpireUnits().
   */
  void setReadExpireAge(int age);

  /**
   * Set units to use for expiry of read messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void setReadExpireUnits(ExpireUnits units);

  /**
   * Get the age at which unread messages are expired.
   * Units are determined by getUnreadExpireUnits().
   */
  int getUnreadExpireAge() const { return mUnreadExpireAge; }

  /**
   * Get the age at which read messages are expired.
   * Units are determined by getReadExpireUnits().
   */
  int getReadExpireAge() const { return mReadExpireAge; }

  /**
   * What should expiry do? Delete or move to another folder?
   */
  ExpireAction expireAction() const { return mExpireAction; }
  void setExpireAction( ExpireAction a );

  /**
   * If expiry should move to folder, return the ID of that folder
   */
  QString expireToFolderId() const { return mExpireToFolderId; }
  void setExpireToFolderId( const QString& id );

  /**
   * Units getUnreadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getUnreadExpireUnits() const { return mUnreadExpireUnits; }

  /**
   * Units getReadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getReadExpireUnits() const { return mReadExpireUnits; }



  /** Returns true if this folder is associated with a mailing-list. */
  void setMailingListEnabled( bool enabled );
  bool isMailingListEnabled() const { return mMailingListEnabled; }

  void setMailingList( const MailingList& mlist );
  MailingList mailingList() const
  { return mMailingList; }

  void daysToExpire( int& unreadDays, int& readDays );

  /**
   * Returns true if the replies to mails from this folder should be
   * put in the same folder.
   */
  bool putRepliesInSameFolder() const { return mPutRepliesInSameFolder; }
  void setPutRepliesInSameFolder( bool b ) { mPutRepliesInSameFolder = b; }

  /**
   * Returns true if this folder should be hidden from all folder selection dialogs
   */
  bool hideInSelectionDialog() const { return mHideInSelectionDialog; }
  void setHideInSelectionDialog( bool hide ) { mHideInSelectionDialog = hide; }

  /**
   * Returns true if the user doesn't want to get notified about new mail
   * in this folder.
   */
  bool ignoreNewMail() const { return mIgnoreNewMail; }
  void setIgnoreNewMail( bool b ) { mIgnoreNewMail = b; }

  QString mailingListPostAddress() const;

  /** Mark all new messages as unread. */
  void markNewAsUnread();

  /** Mark all new and unread messages as read. */
  void markUnreadAsRead();

  void removeCollection();
  void expireOldMessages( bool immediate );

  /**
   * Compact this folder. Options:
   * CompactLater: schedule it as a background task
   * CompactNow: do it now, and inform the user of the result (manual compaction)
   * CompactSilentlyNow: do it now, and keep silent about it (e.g. for outbox)
   */
  void compact( CompactOptions options );

protected slots:
  void slotIdentitiesChanged();
  void slotMarkNewAsUnreadfetchDone( KJob * job );
  void slotMarkNewAsReadfetchDone( KJob * job);
  void slotDeletionCollectionResult( KJob *job );

signals:
  void viewConfigChanged();

private:

  explicit FolderCollection( const Akonadi::Collection & col, bool writeConfig = true );

  Akonadi::Collection mCollection;
  bool         mExpireMessages;          // true if old messages are expired
  int          mUnreadExpireAge;         // Given in unreadExpireUnits
  int          mReadExpireAge;           // Given in readExpireUnits
  ExpireUnits  mUnreadExpireUnits;
  ExpireUnits  mReadExpireUnits;
  ExpireAction mExpireAction;
  QString      mExpireToFolderId;

  /** Mailing list attributes */
  bool                mMailingListEnabled;
  MailingList         mMailingList;

  bool mUseDefaultIdentity;
  uint mIdentity;

  /** Should new mail in this folder be ignored? */
  bool mIgnoreNewMail;
  /** name of the field that is used for "From" in listbox */
  QString mWhoField, mUserWhoField;

  /** Should replies to messages in this folder be put in here? */
  bool mPutRepliesInSameFolder;

  /** Should this folder be hidden in the folder selection dialog? */
  bool mHideInSelectionDialog;

  /** shortcut associated with this folder or null, if none is configured. */
  KShortcut mShortcut;
  bool mWriteConfig;
};


#endif /* FOLDERCOLLECTION_H */

