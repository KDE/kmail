/* -*- mode: C++; c-file-style: "gnu" -*-
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


#ifndef COLLECTIONGENERALPAGE_H
#define COLLECTIONGENERALPAGE_H

#include <akonadi/collectionpropertiespage.h>
class KComboBox;
class QCheckBox;
class KLineEdit;
class FolderCollection;
template <typename T> class QSharedPointer;

namespace KPIMIdentities {
  class IdentityCombo;
}

class CollectionGeneralPage : public Akonadi::CollectionPropertiesPage
{
  Q_OBJECT
public:
  explicit CollectionGeneralPage( QWidget *parent = 0 );
  ~CollectionGeneralPage();

  void load( const Akonadi::Collection & col );
  void save( Akonadi::Collection & col );

  enum FolderContentsType
  {
    ContentsTypeMail = 0,
    ContentsTypeCalendar,
    ContentsTypeContact,
    ContentsTypeNote,
    ContentsTypeTask,
    ContentsTypeJournal,
    ContentsTypeLast = ContentsTypeJournal
  };

  enum IncidencesFor {
    IncForNobody,
    IncForAdmins,
    IncForReaders
  };

protected:
  void init(const Akonadi::Collection&);

private slots:
  void slotFolderNameChanged(const QString & );
  void slotIdentityCheckboxChanged();
private:
  KComboBox *mContentsComboBox;
  KComboBox *mIncidencesForComboBox;
  QCheckBox *mAlarmsBlockedCheckBox;
  QCheckBox *mSharedSeenFlagsCheckBox;
  QCheckBox   *mNewMailCheckBox;
  QCheckBox   *mNotifyOnNewMailCheckBox;
  QCheckBox   *mKeepRepliesInSameFolderCheckBox;
  QCheckBox   *mHideInSelectionDialogCheckBox;
  QCheckBox   *mUseDefaultIdentityCheckBox;
  KLineEdit   *mNameEdit;
  KPIMIdentities::IdentityCombo *mIdentityComboBox;
  QSharedPointer<FolderCollection> mFolderCollection;
  bool mIsLocalSystemFolder;
  bool mIsResourceFolder;
  bool mIsImapFolder;

};


#endif /* COLLECTIONGENERALPAGE_H */

