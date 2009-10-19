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

#include "mailinglist-magic.h"
#include "kmglobal.h"
using KMail::MailingList;

class FolderCollection : public QObject
{
  Q_OBJECT
public:
  FolderCollection( const Akonadi::Collection & col );
  ~FolderCollection();

  Akonadi::Collection collection();

  void writeConfig() const;
  void readConfig();

  QString idString() const;


  bool isValid() const;

  Akonadi::Collection::Rights rights() const;

  Akonadi::CollectionStatistics statistics() const;

  enum ExpireAction { ExpireDelete, ExpireMove };


  const KShortcut &shortcut() const { return mShortcut; }
  void setShortcut( const KShortcut& );


  /** Get / set the user-settings for the WhoField (From/To/Empty) */
  QString userWhoField(void) { return mUserWhoField; }
  void setUserWhoField(const QString &whoField,bool writeConfig=true);

  /** Get / set whether the default identity should be used instead of the
   *  identity specified by setIdentity(). */
  void setUseDefaultIdentity( bool useDefaultIdentity );
  bool useDefaultIdentity() const { return mUseDefaultIdentity; }

  void setIdentity(uint identity);
  uint identity() const;


protected slots:
  void slotIdentitiesChanged();

signals:
  /** Emitted when the shortcut associated with this folder changes. */
  void shortcutChanged( const Akonadi::Collection & );

private:

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

};


#endif /* FOLDERCOLLECTION_H */

