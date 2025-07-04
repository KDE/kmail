/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "MailCommon/Tag"
#include "config-kmail.h"
#include "configuredialog_p.h"
#include "kmail_export.h"
#include <QListWidgetItem>

class QPushButton;
class QCheckBox;
class QComboBox;
class KFontChooser;
class ColorListBox;
class QButtonGroup;
class QGroupBox;
class QSpinBox;
class QLineEdit;
class QModelIndex;
class KJob;
namespace MessageViewer
{
class ConfigureWidget;
}

namespace MessageList
{
namespace Utils
{
class AggregationComboBox;
class ThemeComboBox;
}
}

namespace MailCommon
{
class Tag;
using TagPtr = QSharedPointer<Tag>;
}

namespace MailCommon
{
class TagWidget;
}

class AppearancePageFontsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageFontsTab(QWidget *parent = nullptr);
    [[nodiscard]] QString helpAnchor() const;
    void save() override;

private:
    void slotFontSelectorChanged(int);
    void doLoadOther() override;
    void doResetToDefaultsOther() override;

private:
    QCheckBox *const mCustomFontCheck;
    QComboBox *const mFontLocationCombo;
    KFontChooser *const mFontChooser;

    int mActiveFontIndex{-1};
    QFont mFont[8];
};

class AppearancePageColorsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageColorsTab(QWidget *parent = nullptr);
    [[nodiscard]] QString helpAnchor() const;
    void save() override;

private:
    void doLoadOther() override;
    void doResetToDefaultsOther() override;
    void loadColor(bool loadFromConfig);

private:
    QCheckBox *const mCustomColorCheck;
    ColorListBox *const mColorList;
    QCheckBox *const mRecycleColorCheck;
    QSpinBox *const mCloseToQuotaThreshold;
    QCheckBox *const mUseInlineStyle;
};

class AppearancePageLayoutTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageLayoutTab(QWidget *parent = nullptr);
    [[nodiscard]] QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private: // data
    QButtonGroup *mFolderListGroup = nullptr;
    QButtonGroup *mReaderWindowModeGroup = nullptr;
    QButtonGroup *mFolderToolTipsGroup = nullptr;
    QButtonGroup *mFavoriteFoldersViewGroup = nullptr;
    QCheckBox *mFolderQuickSearchCB = nullptr;
};

class AppearancePageHeadersTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageHeadersTab(QWidget *parent = nullptr);

    [[nodiscard]] QString helpAnchor() const;

    void save() override;

private: // methods
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;
    // virtual void doResetToDefaultsOther();
    void setDateDisplay(int id, const QString &format);
    void slotLinkClicked(const QString &link);
    void slotSelectDefaultAggregation();
    void slotSelectDefaultTheme();

private: // data
    QCheckBox *mDisplayMessageToolTips = nullptr;
    MessageList::Utils::AggregationComboBox *mAggregationComboBox = nullptr;
    MessageList::Utils::ThemeComboBox *mThemeComboBox = nullptr;
    QButtonGroup *mDateDisplay = nullptr;
    QGroupBox *mDateDisplayBox = nullptr;
    QLineEdit *mCustomDateFormatEdit = nullptr;
    QString mCustomDateWhatsThis;
};

class AppearancePageGeneralTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageGeneralTab(QWidget *parent = nullptr);

    [[nodiscard]] QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private: // data
    QCheckBox *mCloseAfterReplyOrForwardCheck = nullptr;
    MessageViewer::ConfigureWidget *mViewerSettings = nullptr;
    QCheckBox *mSystemTrayCheck = nullptr;
    QCheckBox *mStartInTrayCheck = nullptr;
    QCheckBox *mShowNumberInTaskBar = nullptr;
    QCheckBox *const mDisplayOwnIdentity;
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    QCheckBox *mEnablePlasmaActivities = nullptr;
#endif
};

class TagListWidgetItem : public QListWidgetItem
{
public:
    explicit TagListWidgetItem(QListWidget *parent = nullptr);
    explicit TagListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent = nullptr);

    ~TagListWidgetItem() override;
    void setKMailTag(const MailCommon::Tag::Ptr &tag);
    [[nodiscard]] MailCommon::Tag::Ptr kmailTag() const;

private:
    MailCommon::Tag::Ptr mTag = nullptr;
};

/**Configuration tab in the appearance page for modifying the available set of
+message tags*/
class AppearancePageMessageTagTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageMessageTagTab(QWidget *parent = nullptr);
    ~AppearancePageMessageTagTab() override;

    [[nodiscard]] QString helpAnchor() const;

    void save() override;

public Q_SLOTS:
    /**Enables/disables Add button according to whether @p aText is empty.
    Connected to signal of the line edit widget for adding tags
    @param aText String to change add button according to
    */
    void slotAddLineTextChanged(const QString &aText);
    /**Creates a generic tag with the visible name from the line edit widget for
    adding tags. Adds it to the end of the list and selects. Empties the line
    edit widget*/
    void slotAddNewTag();
    /**Removes the currently selected text in the list box.*/
    void slotRemoveTag();
    /**Increases the currently selected tag's priority and handles related visual
    changes*/
    void slotMoveTagUp();
    /**Decreases the currently selected tag's priority and handles related visual
    changes*/
    void slotMoveTagDown();

private:
    /*Handles necessary processing when the selection in the edit box changes.
    Records the unselected tag's information, and applies visual changes
    necessary depending on the description of the new tag. Private since doesn't
    change the selection of the edit box itself*/
    void slotSelectionChanged();
    /*This slot is necessary so that apply button is not activated when we are
    only applying visual changes after selecting a new tag in the list box*/
    void slotEmitChangeCheck();
    /*Transfers the tag settings from the widgets to the internal data structures.
    Private since passing a wrong parameter modifies another tag's data*/
    void slotRecordTagSettings(int aIndex);
    /*Transfers the tag settings from the internal data structures to the widgets.
    Private since passing a wrong parameter visualizes another tag's data*/
    void slotUpdateTagSettingWidgets(int aIndex);
    /*Transfers changes in the tag name edit box to the list box for tags. Private
    since calling externally decouples the name in the list box from name edit box*/
    void slotNameLineTextChanged(const QString &);
    void slotIconNameChanged(const QString &iconName);
    void slotRowsMoved(const QModelIndex &, int sourcestart, int sourceEnd, const QModelIndex &, int destinationRow);
    void slotTagsFetched(KJob *job);

    void slotDeleteTagJob(KJob *job);

    void doLoadFromGlobalSettings() override;
    void swapTagsInListBox(const int first, const int second);
    void updateButtons();
    void slotCustomMenuRequested(const QPoint &pos);

private: // data
    QLineEdit *mTagAddLineEdit = nullptr;
    QPushButton *mTagAddButton = nullptr;
    QPushButton *mTagRemoveButton = nullptr;
    QPushButton *mTagUpButton = nullptr;
    QPushButton *mTagDownButton = nullptr;

    QListWidget *mTagListBox = nullptr;

    QGroupBox *mTagsGroupBox = nullptr;
    QGroupBox *mTagSettingGroupBox = nullptr;

    MailCommon::TagWidget *mTagWidget = nullptr;

    // So we can compare to mMsgTagList and see if the user changed tags
    QList<MailCommon::TagPtr> mOriginalMsgTagList;

    /*Used to safely call slotRecordTagSettings when the selection in
    list box changes*/
    int mPreviousTag;
};

class KMAIL_EXPORT AppearancePage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit AppearancePage(QObject *parent, const KPluginMetaData &data);

    [[nodiscard]] QString helpAnchor() const override;
};
