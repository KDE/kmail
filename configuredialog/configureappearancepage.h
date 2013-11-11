/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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
#include <config-enterprise.h>
#include "configuredialog_p.h"
#include "tag.h"
#include <QListWidgetItem>

class QPushButton;
class QCheckBox;
class KComboBox;
class KFontChooser;
class ColorListBox;
class QButtonGroup;
class QGroupBox;
class QSpinBox;
class KButtonGroup;
class KLineEdit;
class QModelIndex;
namespace MessageViewer {
class ConfigureWidget;
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

class AppearancePageFontsTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageFontsTab( QWidget * parent=0 );
    QString helpAnchor() const;
    void save();

private slots:
    void slotFontSelectorChanged( int );

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void updateFontSelector();
    void doResetToDefaultsOther();

private:
    QCheckBox    *mCustomFontCheck;
    KComboBox    *mFontLocationCombo;
    KFontChooser *mFontChooser;

    int          mActiveFontIndex;
    QFont        mFont[12];
};

class AppearancePageColorsTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageColorsTab( QWidget * parent=0 );
    QString helpAnchor() const;
    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();
    void loadColor( bool loadFromConfig );

private:
    QCheckBox    *mCustomColorCheck;
    ColorListBox *mColorList;
    QCheckBox    *mRecycleColorCheck;
    QSpinBox     *mCloseToQuotaThreshold;
};

class AppearancePageLayoutTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageLayoutTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private: // data
    QButtonGroup  *mFolderListGroup;
    QGroupBox     *mFolderListGroupBox;
    QButtonGroup  *mReaderWindowModeGroup;
    QGroupBox     *mReaderWindowModeGroupBox;
    QCheckBox     *mFolderQuickSearchCB;
    QButtonGroup  *mFolderToolTipsGroup;
    QGroupBox     *mFolderToolTipsGroupBox;
    QButtonGroup  *mFavoriteFoldersViewGroup;
    QGroupBox     *mFavoriteFoldersViewGroupBox;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageHeadersTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private: // methods
    void doLoadFromGlobalSettings();
    void doLoadOther();
    // virtual void doResetToDefaultsOther();
    void setDateDisplay( int id, const QString & format );

private: // data
    QCheckBox    *mDisplayMessageToolTips;
    QCheckBox    *mHideTabBarWithSingleTab;
    QCheckBox    *mTabsHaveCloseButton;
    MessageList::Utils::AggregationComboBox *mAggregationComboBox;
    MessageList::Utils::ThemeComboBox *mThemeComboBox;
    KButtonGroup *mDateDisplay;
    KLineEdit    *mCustomDateFormatEdit;
    QString       mCustomDateWhatsThis;

private slots:
    void slotLinkClicked( const QString & link );
    void slotSelectDefaultAggregation();
    void slotSelectDefaultTheme();
};

class AppearancePageReaderTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageReaderTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private:
    void doLoadOther();
    void doResetToDefaultsOther();

private: // data
    QCheckBox *mCloseAfterReplyOrForwardCheck;
    MessageViewer::ConfigureWidget *mViewerSettings;
};


class AppearancePageSystemTrayTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageSystemTrayTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();

private: // data
    QCheckBox    *mSystemTrayCheck;
    QCheckBox    *mSystemTrayShowUnreadMail;
    KButtonGroup *mSystemTrayGroup;
};


class TagListWidgetItem : public QListWidgetItem
{
public:
    explicit TagListWidgetItem( QListWidget *parent = 0);
    explicit TagListWidgetItem( const QIcon & icon, const QString & text, QListWidget * parent = 0);

    ~TagListWidgetItem();
    void setKMailTag( const MailCommon::Tag::Ptr& tag );
    MailCommon::Tag::Ptr kmailTag() const;
private:
    MailCommon::Tag::Ptr mTag;
};

/**Configuration tab in the appearance page for modifying the available set of
+message tags*/
class AppearancePageMessageTagTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageMessageTagTab( QWidget * parent=0);
    ~AppearancePageMessageTagTab();

    QString helpAnchor() const;

    void save();

public slots:
    /**Enables/disables Add button according to whether @p aText is empty.
  Connected to signal of the line edit widget for adding tags
  @param aText String to change add button according to
  */
    void slotAddLineTextChanged( const QString &aText );
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

private slots:
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
    void slotRecordTagSettings( int aIndex );
    /*Transfers the tag settings from the internal data structures to the widgets.
  Private since passing a wrong parameter visualizes another tag's data*/
    void slotUpdateTagSettingWidgets( int aIndex );
    /*Transfers changes in the tag name edit box to the list box for tags. Private
  since calling externally decouples the name in the list box from name edit box*/
    void slotNameLineTextChanged( const QString & );
    void slotIconNameChanged( const QString &iconName );
    void slotRowsMoved( const QModelIndex &,
                        int sourcestart, int sourceEnd,
                        const QModelIndex &, int destinationRow );

private:
    void doLoadFromGlobalSettings();
    void swapTagsInListBox( const int first, const int second );
    void updateButtons();

private: // data

    KLineEdit *mTagAddLineEdit;
    QPushButton *mTagAddButton, *mTagRemoveButton,
    *mTagUpButton, *mTagDownButton;

    QListWidget *mTagListBox;

    QGroupBox *mTagsGroupBox, *mTagSettingGroupBox;

    MailCommon::TagWidget *mTagWidget;

    // So we can compare to mMsgTagList and see if the user changed tags
    QList<MailCommon::TagPtr> mOriginalMsgTagList;

    /*Used to safely call slotRecordTagSettings when the selection in
    list box changes*/
    int mPreviousTag;
    bool mNepomukActive;
};

class KMAIL_EXPORT AppearancePage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit AppearancePage( const KComponentData &instance, QWidget *parent=0 );

    QString helpAnchor() const;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AppearancePageFontsTab FontsTab;
    typedef AppearancePageColorsTab ColorsTab;
    typedef AppearancePageLayoutTab LayoutTab;
    typedef AppearancePageHeadersTab HeadersTab;
    typedef AppearancePageReaderTab ReaderTab;
    typedef AppearancePageSystemTrayTab SystemTrayTab;
    typedef AppearancePageMessageTagTab MessageTagTab;

private:
    FontsTab      *mFontsTab;
    ColorsTab     *mColorsTab;
    LayoutTab     *mLayoutTab;
    HeadersTab    *mHeadersTab;
    ReaderTab     *mReaderTab;
    SystemTrayTab *mSystemTrayTab;
    MessageTagTab *mMessageTagTab;
};



#endif // CONFIGUREAPPEARANCEPAGE_H
