/*
  Copyright (c) 2013-2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef CONFIGUREAPPEARANCEPAGE_H
#define CONFIGUREAPPEARANCEPAGE_H

#include "kmail_export.h"
#include "configuredialog_p.h"
#include "MailCommon/Tag"
#include <QListWidgetItem>

class QPushButton;
class QCheckBox;
class KComboBox;
class KFontChooser;
class ColorListBox;
class QButtonGroup;
class QGroupBox;
class QSpinBox;
class KLineEdit;
class QModelIndex;
class KJob;
namespace MessageViewer {
class ConfigureWidget;
}
namespace Gravatar {
class GravatarConfigWidget;
}

namespace MessageList {
namespace Utils {
class AggregationComboBox;
class ThemeComboBox;
}
}

namespace MailCommon {
class Tag;
typedef QSharedPointer<Tag> TagPtr;
}

namespace MailCommon {
class TagWidget;
}

class AppearancePageFontsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageFontsTab(QWidget *parent = nullptr);
    QString helpAnchor() const;
    void save() override;

private Q_SLOTS:
    void slotFontSelectorChanged(int);

private:
    void doLoadOther() override;
    void updateFontSelector();
    void doResetToDefaultsOther() override;

private:
    QCheckBox *mCustomFontCheck = nullptr;
    KComboBox *mFontLocationCombo = nullptr;
    KFontChooser *mFontChooser = nullptr;

    int mActiveFontIndex;
    QFont mFont[8];
};

class AppearancePageColorsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageColorsTab(QWidget *parent = nullptr);
    QString helpAnchor() const;
    void save() override;

private:
    void doLoadOther() override;
    void doResetToDefaultsOther() override;
    void loadColor(bool loadFromConfig);

private:
    QCheckBox *mCustomColorCheck = nullptr;
    ColorListBox *mColorList = nullptr;
    QCheckBox *mRecycleColorCheck = nullptr;
    QSpinBox *mCloseToQuotaThreshold = nullptr;
};

class AppearancePageLayoutTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageLayoutTab(QWidget *parent = nullptr);
    QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;

private: // data
    QButtonGroup *mFolderListGroup = nullptr;
    QGroupBox *mFolderListGroupBox = nullptr;
    QButtonGroup *mReaderWindowModeGroup = nullptr;
    QGroupBox *mReaderWindowModeGroupBox = nullptr;
    QButtonGroup *mFolderToolTipsGroup = nullptr;
    QGroupBox *mFolderToolTipsGroupBox = nullptr;
    QButtonGroup *mFavoriteFoldersViewGroup = nullptr;
    QGroupBox *mFavoriteFoldersViewGroupBox = nullptr;
    QCheckBox *mFolderQuickSearchCB = nullptr;
};

class AppearancePageHeadersTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageHeadersTab(QWidget *parent = nullptr);

    QString helpAnchor() const;

    void save() override;

private: // methods
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;
    // virtual void doResetToDefaultsOther();
    void setDateDisplay(int id, const QString &format);

private: // data
    QCheckBox *mDisplayMessageToolTips = nullptr;
    MessageList::Utils::AggregationComboBox *mAggregationComboBox = nullptr;
    MessageList::Utils::ThemeComboBox *mThemeComboBox = nullptr;
    QButtonGroup *mDateDisplay = nullptr;
    QGroupBox *mDateDisplayBox = nullptr;
    KLineEdit *mCustomDateFormatEdit = nullptr;
    QString mCustomDateWhatsThis;

private Q_SLOTS:
    void slotLinkClicked(const QString &link);
    void slotSelectDefaultAggregation();
    void slotSelectDefaultTheme();
};

class AppearancePageReaderTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageReaderTab(QWidget *parent = nullptr);

    QString helpAnchor() const;

    void save() override;

private:
    void doLoadOther() override;
    void doResetToDefaultsOther() override;

private: // data
    QCheckBox *mCloseAfterReplyOrForwardCheck = nullptr;
    MessageViewer::ConfigureWidget *mViewerSettings = nullptr;
    Gravatar::GravatarConfigWidget *mGravatarConfigWidget = nullptr;
};

class AppearancePageSystemTrayTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageSystemTrayTab(QWidget *parent = nullptr);

    QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;

private:
    QCheckBox *mSystemTrayCheck = nullptr;
};

class TagListWidgetItem : public QListWidgetItem
{
public:
    explicit TagListWidgetItem(QListWidget *parent = nullptr);
    explicit TagListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent = nullptr);

    ~TagListWidgetItem();
    void setKMailTag(const MailCommon::Tag::Ptr &tag);
    MailCommon::Tag::Ptr kmailTag() const;
private:
    MailCommon::Tag::Ptr mTag;
};

/**Configuration tab in the appearance page for modifying the available set of
+message tags*/
class AppearancePageMessageTagTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AppearancePageMessageTagTab(QWidget *parent = nullptr);
    ~AppearancePageMessageTagTab();

    QString helpAnchor() const;

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

private Q_SLOTS:
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
private:
    void doLoadFromGlobalSettings() override;
    void swapTagsInListBox(const int first, const int second);
    void updateButtons();

private: // data
    KLineEdit *mTagAddLineEdit = nullptr;
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
    explicit AppearancePage(QWidget *parent = nullptr);

    QString helpAnchor() const override;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AppearancePageFontsTab FontsTab;
    typedef AppearancePageColorsTab ColorsTab;
    typedef AppearancePageLayoutTab LayoutTab;
    typedef AppearancePageHeadersTab HeadersTab;
    typedef AppearancePageReaderTab ReaderTab;
    typedef AppearancePageSystemTrayTab SystemTrayTab;
    typedef AppearancePageMessageTagTab MessageTagTab;
};

#endif // CONFIGUREAPPEARANCEPAGE_H
