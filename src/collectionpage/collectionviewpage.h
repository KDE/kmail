/*
   SPDX-FileCopyrightText: 2009-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONVIEWPAGE_H
#define COLLECTIONVIEWPAGE_H

#include <AkonadiWidgets/collectionpropertiespage.h>
#include <AkonadiCore/collection.h>
#include <MailCommon/FolderSettings>
class QCheckBox;
class QLabel;
class QComboBox;
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
    ~CollectionViewPage() override;

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
    QComboBox *mShowSenderReceiverComboBox = nullptr;
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
