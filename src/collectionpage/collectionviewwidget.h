/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONVIEWWIDGET_H
#define COLLECTIONVIEWWIDGET_H

#include <QSharedPointer>
#include <QWidget>
#include <MailCommon/FolderSettings>
class QCheckBox;
class QComboBox;
class QRadioButton;
namespace MessageList {
namespace Utils {
class AggregationComboBox;
class ThemeComboBox;
}
}
class CollectionViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionViewWidget(QWidget *parent = nullptr);
    ~CollectionViewWidget();
    void load(const Akonadi::Collection &col);
    void save(Akonadi::Collection &col);
private:
    void slotSelectFolderAggregation();
    void slotSelectFolderTheme();
    void slotThemeCheckboxChanged();
    void slotAggregationCheckboxChanged();
    QSharedPointer<MailCommon::FolderSettings> mFolderCollection;
    QCheckBox *mIconsCheckBox = nullptr;
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
};

#endif // COLLECTIONVIEWWIDGET_H
