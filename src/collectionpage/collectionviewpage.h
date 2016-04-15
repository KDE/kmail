/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009-2016 Montel Laurent <montel@kde.org>

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

#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>
class QCheckBox;
class QLabel;
class KComboBox;
class KIconButton;
class QRadioButton;
template <typename T> class QSharedPointer;

namespace MessageList
{
namespace Utils
{
class AggregationComboBox;
class ThemeComboBox;
}
}

class CollectionViewPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionViewPage(QWidget *parent = Q_NULLPTR);
    ~CollectionViewPage();

    void load(const Akonadi::Collection &col) Q_DECL_OVERRIDE;
    void save(Akonadi::Collection &col) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotChangeIcon(const QString &icon);
    void slotAggregationCheckboxChanged();
    void slotThemeCheckboxChanged();
    void slotSelectFolderAggregation();
    void slotSelectFolderTheme();

private:
    void init(const Akonadi::Collection &);
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
    QRadioButton *mPreferHtmlToText;
    QRadioButton *mPreferTextToHtml;
    QRadioButton *mUseGlobalSettings;
    Akonadi::Collection mCurrentCollection;
    int mShowSenderReceiverValue;
    bool mIsLocalSystemFolder;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionViewPageFactory, CollectionViewPage)

#endif /* COLLECTIONVIEWPAGE_H */

