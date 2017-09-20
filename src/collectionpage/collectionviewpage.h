/*
   Copyright (C) 2009-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef COLLECTIONVIEWPAGE_H
#define COLLECTIONVIEWPAGE_H

#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>
#include <MailCommon/FolderSettings>
class QCheckBox;
class QLabel;
class KComboBox;
class KIconButton;
class QRadioButton;
template<typename T> class QSharedPointer;

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
    explicit CollectionViewPage(QWidget *parent = nullptr);
    ~CollectionViewPage();

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private:
    void slotChangeIcon(const QString &icon);
    void slotAggregationCheckboxChanged();
    void slotThemeCheckboxChanged();
    void slotSelectFolderAggregation();
    void slotSelectFolderTheme();

    void init(const Akonadi::Collection &);
    QSharedPointer<MailCommon::FolderSettings> mFolderCollection;
    QCheckBox *mIconsCheckBox = nullptr;
    QLabel *mNormalIconLabel = nullptr;
    KIconButton *mNormalIconButton = nullptr;
    QLabel *mUnreadIconLabel = nullptr;
    KIconButton *mUnreadIconButton = nullptr;
    KComboBox *mShowSenderReceiverComboBox = nullptr;
    QCheckBox *mUseDefaultAggregationCheckBox = nullptr;
    MessageList::Utils::AggregationComboBox *mAggregationComboBox = nullptr;
    QCheckBox *mUseDefaultThemeCheckBox = nullptr;
    MessageList::Utils::ThemeComboBox *mThemeComboBox = nullptr;
    QRadioButton *mPreferHtmlToText = nullptr;
    QRadioButton *mPreferTextToHtml = nullptr;
    QRadioButton *mUseGlobalSettings = nullptr;
    Akonadi::Collection mCurrentCollection;
    int mShowSenderReceiverValue = -1;
    bool mIsLocalSystemFolder = false;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionViewPageFactory, CollectionViewPage)

#endif /* COLLECTIONVIEWPAGE_H */
