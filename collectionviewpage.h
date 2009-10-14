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

#ifndef COLLECTIONVIEWPAGE_H
#define COLLECTIONVIEWPAGE_H

#include <akonadi/collectionpropertiespage.h>
class QCheckBox;
class QLabel;
class KComboBox;
class KIconButton;

namespace MessageList {
  namespace Utils {
    class AggregationComboBox;
    class ThemeComboBox;
  }
}

class CollectionViewPage : public Akonadi::CollectionPropertiesPage
{
  Q_OBJECT
public:
  explicit CollectionViewPage( QWidget *parent = 0 );

  void load( const Akonadi::Collection & col );
  void save( Akonadi::Collection & col );
protected:
  void init();
#if 0
public slots:
  void slotChangeIcon( const QString & icon );
  void slotAggregationCheckboxChanged();
  void slotThemeCheckboxChanged();
  void slotSelectFolderAggregation();
  void slotSelectFolderTheme();

private:
  void initializeWithValuesFromFolder( KMFolder * folder );
#endif
private:

  bool mIsLocalSystemFolder;
  bool mIsResourceFolder;
  QCheckBox   *mIconsCheckBox;
  QLabel      *mNormalIconLabel;
  KIconButton *mNormalIconButton;
  QLabel      *mUnreadIconLabel;
  KIconButton *mUnreadIconButton;
  KComboBox *mShowSenderReceiverComboBox;
  QCheckBox *mUseDefaultAggregationCheckBox;
  MessageList::Utils::AggregationComboBox *mAggregationComboBox;
  QCheckBox *mUseDefaultThemeCheckBox;
  MessageList::Utils::ThemeComboBox *mThemeComboBox;

};


#endif /* COLLECTIONVIEWPAGE_H */

