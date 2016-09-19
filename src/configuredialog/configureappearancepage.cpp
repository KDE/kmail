/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

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

#include "configureappearancepage.h"
#include "PimCommon/ConfigureImmutableWidgetUtils"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "configuredialog/colorlistbox.h"
#include "messagelist/aggregationcombobox.h"
#include "messagelist/aggregationconfigbutton.h"
#include "messagelist/themecombobox.h"
#include "messagelist/themeconfigbutton.h"
#include "messagelistsettings.h"
#include "MailCommon/TagWidget"
#include "MailCommon/Tag"
#include "kmkernel.h"
#include "util.h"
#include "MailCommon/FolderTreeWidget"

#include "kmmainwidget.h"

#include "mailcommonsettings_base.h"

#include "MessageViewer/ConfigureWidget"
#include "messageviewer/messageviewersettings.h"

#include "messagelist/messagelistutil.h"
#include <MessageCore/MessageCoreSettings>
#include "MessageCore/MessageCoreUtil"
#include "settings/kmailsettings.h"

#include "MailCommon/MailUtil"

#include <AkonadiCore/Tag>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>
#include <AkonadiCore/TagDeleteJob>
#include <AkonadiCore/TagCreateJob>
#include <AkonadiCore/TagAttribute>
#include <AkonadiCore/TagModifyJob>

#include <KIconButton>
#include <KLocalizedString>
#include <KColorScheme>
#include <KSeparator>
#include <KFontChooser>
#include <QHBoxLayout>
#include <KMessageBox>
#include <KKeySequenceWidget>
#include <KLineEdit>
#include <QDialog>
#include <QIcon>
#include "kmail_debug.h"

#include <kmime/kmime_dateformatter.h>
using KMime::DateFormatter;

#include <QWhatsThis>
#include <QButtonGroup>
#include <QSpinBox>
#include <QLabel>
#include <QFontDatabase>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <gravatar/gravatarconfigwidget.h>
using namespace MailCommon;

QString AppearancePage::helpAnchor() const
{
    return QStringLiteral("configure-appearance");
}

AppearancePage::AppearancePage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    //
    // "Fonts" tab:
    //
    FontsTab *fontsTab = new FontsTab();
    addTab(fontsTab, i18n("Fonts"));

    //
    // "Colors" tab:
    //
    ColorsTab *colorsTab = new ColorsTab();
    addTab(colorsTab, i18n("Colors"));

    //
    // "Layout" tab:
    //
    LayoutTab *layoutTab = new LayoutTab();
    addTab(layoutTab, i18n("Layout"));

    //
    // "Headers" tab:
    //
    HeadersTab *headersTab = new HeadersTab();
    addTab(headersTab, i18n("Message List"));

    //
    // "Reader window" tab:
    //
    ReaderTab *readerTab = new ReaderTab();
    addTab(readerTab, i18n("Message Window"));
    addConfig(MessageViewer::MessageViewerSettings::self(), readerTab);

    //
    // "System Tray" tab:
    //
    SystemTrayTab *systemTrayTab = new SystemTrayTab();
    addTab(systemTrayTab, i18n("System Tray"));

    //
    // "Message Tag" tab:
    //
    MessageTagTab *messageTagTab = new MessageTagTab();
    addTab(messageTagTab, i18n("Message Tags"));
}

QString AppearancePage::FontsTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-fonts");
}

static const struct {
    const char *configName;
    const char *displayName;
    bool   enableFamilyAndSize;
    bool   onlyFixed;
} fontNames[] = {
    { "body-font", I18N_NOOP("Message Body"), true, false },
    { "MessageListFont", I18N_NOOP("Message List"), true, false },
    { "UnreadMessageFont", I18N_NOOP("Message List - Unread Messages"), false, false },
    { "ImportantMessageFont", I18N_NOOP("Message List - Important Messages"), false, false },
    { "TodoMessageFont", I18N_NOOP("Message List - Action Item Messages"), false, false },
    { "folder-font", I18N_NOOP("Folder List"), true, false },
    { "quote1-font", I18N_NOOP("Quoted Text - First Level"), false, false },
    { "quote2-font", I18N_NOOP("Quoted Text - Second Level"), false, false },
    { "quote3-font", I18N_NOOP("Quoted Text - Third Level"), false, false },
    { "fixed-font", I18N_NOOP("Fixed Width Font"), true, true },
    { "composer-font", I18N_NOOP("Composer"), true, false },
    { "print-font",  I18N_NOOP("Printing Output"), true, false },
};
static const int numFontNames = sizeof fontNames / sizeof * fontNames;

AppearancePageFontsTab::AppearancePageFontsTab(QWidget *parent)
    : ConfigModuleTab(parent), mActiveFontIndex(-1)
{
    assert(numFontNames == sizeof mFont / sizeof * mFont);

    // "Use custom fonts" checkbox, followed by <hr>
    QVBoxLayout *vlay = new QVBoxLayout(this);
    mCustomFontCheck = new QCheckBox(i18n("&Use custom fonts"), this);
    vlay->addWidget(mCustomFontCheck);
    vlay->addWidget(new KSeparator(Qt::Horizontal, this));
    connect(mCustomFontCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    // "font location" combo box and label:
    QHBoxLayout *hlay = new QHBoxLayout(); // inherites spacing
    vlay->addLayout(hlay);
    mFontLocationCombo = new KComboBox(this);
    mFontLocationCombo->setEditable(false);
    mFontLocationCombo->setEnabled(false);   // !mCustomFontCheck->isChecked()

    QStringList fontDescriptions;
    fontDescriptions.reserve(numFontNames);
    for (int i = 0; i < numFontNames; ++i) {
        fontDescriptions << i18n(fontNames[i].displayName);
    }
    mFontLocationCombo->addItems(fontDescriptions);

    QLabel *label = new QLabel(i18n("Apply &to:"), this);
    label->setBuddy(mFontLocationCombo);
    label->setEnabled(false);   // since !mCustomFontCheck->isChecked()
    hlay->addWidget(label);

    hlay->addWidget(mFontLocationCombo);
    hlay->addStretch(10);
    mFontChooser = new KFontChooser(this, KFontChooser::DisplayFrame,
                                    QStringList(), 4);
    mFontChooser->setEnabled(false);   // since !mCustomFontCheck->isChecked()
    vlay->addWidget(mFontChooser);
    connect(mFontChooser, &KFontChooser::fontSelected,
            this, &ConfigModuleTab::slotEmitChanged);

    // {en,dis}able widgets depending on the state of mCustomFontCheck:
    connect(mCustomFontCheck, &QAbstractButton::toggled,
            label, &QWidget::setEnabled);
    connect(mCustomFontCheck, &QAbstractButton::toggled,
            mFontLocationCombo, &QWidget::setEnabled);
    connect(mCustomFontCheck, &QAbstractButton::toggled,
            mFontChooser, &QWidget::setEnabled);
    // load the right font settings into mFontChooser:
    connect(mFontLocationCombo, SIGNAL(activated(int)),
            this, SLOT(slotFontSelectorChanged(int)));
}

void AppearancePage::FontsTab::slotFontSelectorChanged(int index)
{
    qCDebug(KMAIL_LOG) << "slotFontSelectorChanged() called";
    if (index < 0 || index >= mFontLocationCombo->count()) {
        return;    // Should never happen, but it is better to check.
    }

    // Save current fontselector setting before we install the new:
    if (mActiveFontIndex == 0) {
        mFont[0] = mFontChooser->font();
        // hardcode the family and size of "message body" dependant fonts:
        for (int i = 0; i < numFontNames; ++i)
            if (!fontNames[i].enableFamilyAndSize) {
                // ### shall we copy the font and set the save and re-set
                // {regular,italic,bold,bold italic} property or should we
                // copy only family and pointSize?
                mFont[i].setFamily(mFont[0].family());
                mFont[i].setPointSize/*Float?*/(mFont[0].pointSize/*Float?*/());
            }
    } else if (mActiveFontIndex > 0) {
        mFont[ mActiveFontIndex ] = mFontChooser->font();
    }
    mActiveFontIndex = index;

    // Disonnect so the "Apply" button is not activated by the change
    disconnect(mFontChooser, &KFontChooser::fontSelected,
               this, &ConfigModuleTab::slotEmitChanged);

    // Display the new setting:
    mFontChooser->setFont(mFont[index], fontNames[index].onlyFixed);

    connect(mFontChooser, &KFontChooser::fontSelected,
            this, &ConfigModuleTab::slotEmitChanged);

    // Disable Family and Size list if we have selected a quote font:
    mFontChooser->enableColumn(KFontChooser::FamilyList | KFontChooser::SizeList,
                               fontNames[ index ].enableFamilyAndSize);
}

void AppearancePage::FontsTab::doLoadOther()
{
    if (KMKernel::self()) {
        KConfigGroup fonts(KMKernel::self()->config(), "Fonts");

        mFont[0] = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

        for (int i = 0; i < numFontNames; ++i) {
            const QString configName = QLatin1String(fontNames[i].configName);
            if (configName == QLatin1String("MessageListFont")) {
                mFont[i] = MessageList::MessageListSettings::self()->messageListFont();
            } else if (configName == QLatin1String("UnreadMessageFont")) {
                mFont[i] = MessageList::MessageListSettings::self()->unreadMessageFont();
            } else if (configName == QLatin1String("ImportantMessageFont")) {
                mFont[i] = MessageList::MessageListSettings::self()->importantMessageFont();
            } else if (configName == QLatin1String("TodoMessageFont")) {
                mFont[i] = MessageList::MessageListSettings::self()->todoMessageFont();
            } else {
                mFont[i] = fonts.readEntry(configName,
                                           (fontNames[i].onlyFixed) ? fixedFont : mFont[0]);
            }
        }
        mCustomFontCheck->setChecked(!MessageCore::MessageCoreSettings::self()->useDefaultFonts());
        mFontLocationCombo->setCurrentIndex(0);
        slotFontSelectorChanged(0);
    } else {
        setEnabled(false);
    }
}

void AppearancePage::FontsTab::save()
{
    if (KMKernel::self()) {
        KConfigGroup fonts(KMKernel::self()->config(), "Fonts");

        // read the current font (might have been modified)
        if (mActiveFontIndex >= 0) {
            mFont[ mActiveFontIndex ] = mFontChooser->font();
        }

        const bool customFonts = mCustomFontCheck->isChecked();
        MessageCore::MessageCoreSettings::self()->setUseDefaultFonts(!customFonts);

        for (int i = 0; i < numFontNames; ++i) {
            const QString configName = QLatin1String(fontNames[i].configName);
            if (customFonts && configName == QLatin1String("MessageListFont")) {
                MessageList::MessageListSettings::self()->setMessageListFont(mFont[i]);
            } else if (customFonts && configName == QLatin1String("UnreadMessageFont")) {
                MessageList::MessageListSettings::self()->setUnreadMessageFont(mFont[i]);
            } else if (customFonts && configName == QLatin1String("ImportantMessageFont")) {
                MessageList::MessageListSettings::self()->setImportantMessageFont(mFont[i]);
            } else if (customFonts && configName == QLatin1String("TodoMessageFont")) {
                MessageList::MessageListSettings::self()->setTodoMessageFont(mFont[i]);
            } else {
                if (customFonts || fonts.hasKey(configName))
                    // Don't write font info when we use default fonts, but write
                    // if it's already there:
                {
                    fonts.writeEntry(configName, mFont[i]);
                }
            }
        }
    }
}

void AppearancePage::FontsTab::doResetToDefaultsOther()
{
    mCustomFontCheck->setChecked(false);
}

QString AppearancePage::ColorsTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-colors");
}

static const struct {
    const char *configName;
    const char *displayName;
} colorNames[] = { // adjust doLoadOther if you change this:
    { "QuotedText1", I18N_NOOP("Quoted Text - First Level") },
    { "QuotedText2", I18N_NOOP("Quoted Text - Second Level") },
    { "QuotedText3", I18N_NOOP("Quoted Text - Third Level") },
    { "LinkColor", I18N_NOOP("Link") },
    { "UnreadMessageColor", I18N_NOOP("Unread Message") },
    { "ImportantMessageColor", I18N_NOOP("Important Message") },
    { "TodoMessageColor", I18N_NOOP("Action Item Message") },
    { "HTMLWarningColor", I18N_NOOP("Border Around Warning Prepending HTML Messages") },
    { "CloseToQuotaColor", I18N_NOOP("Folder Name and Size When Close to Quota") },
    { "ColorbarBackgroundPlain", I18N_NOOP("HTML Status Bar Background - No HTML Message") },
    { "ColorbarForegroundPlain", I18N_NOOP("HTML Status Bar Foreground - No HTML Message") },
    { "ColorbarBackgroundHTML",  I18N_NOOP("HTML Status Bar Background - HTML Message") },
    { "ColorbarForegroundHTML",  I18N_NOOP("HTML Status Bar Foreground - HTML Message") },
    { "BrokenAccountColor",  I18N_NOOP("Broken Account - Folder Text Color") }
};
static const int numColorNames = sizeof colorNames / sizeof * colorNames;

AppearancePageColorsTab::AppearancePageColorsTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    // "use custom colors" check box
    QVBoxLayout *vlay = new QVBoxLayout(this);
    mCustomColorCheck = new QCheckBox(i18n("&Use custom colors"), this);
    vlay->addWidget(mCustomColorCheck);
    connect(mCustomColorCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    // color list box:
    mColorList = new ColorListBox(this);
    mColorList->setEnabled(false);   // since !mCustomColorCheck->isChecked()
    for (int i = 0; i < numColorNames; ++i) {
        mColorList->addColor(i18n(colorNames[i].displayName));
    }
    vlay->addWidget(mColorList, 1);

    // "recycle colors" check box:
    mRecycleColorCheck =
        new QCheckBox(i18n("Recycle colors on deep &quoting"), this);
    mRecycleColorCheck->setEnabled(false);
    vlay->addWidget(mRecycleColorCheck);
    connect(mRecycleColorCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    // close to quota threshold
    QHBoxLayout *hbox = new QHBoxLayout();
    vlay->addLayout(hbox);
    QLabel *l = new QLabel(i18n("Close to quota threshold:"), this);
    hbox->addWidget(l);
    mCloseToQuotaThreshold = new QSpinBox(this);
    mCloseToQuotaThreshold->setRange(0, 100);
    mCloseToQuotaThreshold->setSingleStep(1);
    connect(mCloseToQuotaThreshold, SIGNAL(valueChanged(int)),
            this, SLOT(slotEmitChanged()));
    mCloseToQuotaThreshold->setSuffix(i18n("%"));

    hbox->addWidget(mCloseToQuotaThreshold);
    hbox->addWidget(new QWidget(this), 2);

    // {en,dir}able widgets depending on the state of mCustomColorCheck:
    connect(mCustomColorCheck, &QAbstractButton::toggled,
            mColorList, &QWidget::setEnabled);
    connect(mCustomColorCheck, &QAbstractButton::toggled,
            mRecycleColorCheck, &QWidget::setEnabled);
    connect(mCustomColorCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);
    connect(mColorList, &ColorListBox::changed,
            this, &ConfigModuleTab::slotEmitChanged);
}

void AppearancePage::ColorsTab::doLoadOther()
{
    mCustomColorCheck->setChecked(!MessageCore::MessageCoreSettings::self()->useDefaultColors());
    mRecycleColorCheck->setChecked(MessageViewer::MessageViewerSettings::self()->recycleQuoteColors());
    mCloseToQuotaThreshold->setValue(KMailSettings::self()->closeToQuotaThreshold());
    loadColor(true);
}

void AppearancePage::ColorsTab::loadColor(bool loadFromConfig)
{
    if (KMKernel::self()) {
        KColorScheme scheme(QPalette::Active, KColorScheme::View);

        KConfigGroup reader(KMKernel::self()->config(), "Reader");

        KConfigGroup collectionFolderView(KMKernel::self()->config(), "CollectionFolderView");

        static const QColor defaultColor[ numColorNames ] = {
            MessageCore::Util::quoteLevel1DefaultTextColor(),
            MessageCore::Util::quoteLevel2DefaultTextColor(),
            MessageCore::Util::quoteLevel3DefaultTextColor(),
            scheme.foreground(KColorScheme::LinkText).color(),   // link
            MessageList::Util::unreadDefaultMessageColor(), // unread mgs
            MessageList::Util::importantDefaultMessageColor(), // important msg
            MessageList::Util::todoDefaultMessageColor(), // action item mgs
            QColor(0xFF, 0x40, 0x40),   // warning text color
            MailCommon::Util::defaultQuotaColor(), // close to quota
            Qt::lightGray, // colorbar plain bg
            Qt::black,     // colorbar plain fg
            Qt::black,     // colorbar html  bg
            Qt::white,     // colorbar html  fg
            scheme.foreground(KColorScheme::NegativeText).color()  //Broken Account Color
        };

        for (int i = 0; i < numColorNames; ++i) {
            if (loadFromConfig) {
                const QString configName = QLatin1String(colorNames[i].configName);
                if (configName == QLatin1String("UnreadMessageColor")) {
                    mColorList->setColorSilently(i, MessageList::MessageListSettings::self()->unreadMessageColor());
                } else if (configName == QLatin1String("ImportantMessageColor")) {
                    mColorList->setColorSilently(i, MessageList::MessageListSettings::self()->importantMessageColor());
                } else if (configName == QLatin1String("TodoMessageColor")) {
                    mColorList->setColorSilently(i, MessageList::MessageListSettings::self()->todoMessageColor());
                } else if (configName == QLatin1String("BrokenAccountColor")) {
                    mColorList->setColorSilently(i, collectionFolderView.readEntry(configName, defaultColor[i]));
                } else {
                    mColorList->setColorSilently(i, reader.readEntry(configName, defaultColor[i]));
                }
            } else {
                mColorList->setColorSilently(i, defaultColor[i]);
            }
        }
    } else {
        setEnabled(false);
    }
}

void AppearancePage::ColorsTab::doResetToDefaultsOther()
{
    mCustomColorCheck->setChecked(false);
    mRecycleColorCheck->setChecked(false);
    mCloseToQuotaThreshold->setValue(80);
    loadColor(false);
}

void AppearancePage::ColorsTab::save()
{
    if (!KMKernel::self()) {
        return;
    }
    KConfigGroup reader(KMKernel::self()->config(), "Reader");
    KConfigGroup collectionFolderView(KMKernel::self()->config(), "CollectionFolderView");
    bool customColors = mCustomColorCheck->isChecked();
    MessageCore::MessageCoreSettings::self()->setUseDefaultColors(!customColors);

    KColorScheme scheme(QPalette::Active, KColorScheme::View);

    for (int i = 0; i < numColorNames; ++i) {
        const QString configName = QLatin1String(colorNames[i].configName);
        if (customColors && configName == QLatin1String("UnreadMessageColor")) {
            MessageList::MessageListSettings::self()->setUnreadMessageColor(mColorList->color(i));
        } else if (customColors && configName == QLatin1String("ImportantMessageColor")) {
            MessageList::MessageListSettings::self()->setImportantMessageColor(mColorList->color(i));
        } else if (customColors && configName == QLatin1String("TodoMessageColor")) {
            MessageList::MessageListSettings::self()->setTodoMessageColor(mColorList->color(i));
        } else if (configName == QLatin1String("BrokenAccountColor")) {
            if (customColors || collectionFolderView.hasKey(configName)) {
                collectionFolderView.writeEntry(configName, mColorList->color(i));
            }
        } else {
            if (customColors || reader.hasKey(configName)) {
                reader.writeEntry(configName, mColorList->color(i));
            }
        }
    }
    MessageViewer::MessageViewerSettings::self()->setRecycleQuoteColors(mRecycleColorCheck->isChecked());
    KMailSettings::self()->setCloseToQuotaThreshold(mCloseToQuotaThreshold->value());
}

QString AppearancePage::LayoutTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-layout");
}

AppearancePageLayoutTab::AppearancePageLayoutTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout(this);

    // "folder list" radio buttons:
    populateButtonGroup(mFolderListGroupBox = new QGroupBox(this),
                        mFolderListGroup = new QButtonGroup(this),
                        Qt::Vertical, KMailSettings::self()->folderListItem());
    vlay->addWidget(mFolderListGroupBox);
    connect(mFolderListGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(slotEmitChanged()));

    QHBoxLayout *folderCBHLayout = new QHBoxLayout;
    mFolderQuickSearchCB = new QCheckBox(i18n("Show folder quick search field"), this);
    connect(mFolderQuickSearchCB, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    folderCBHLayout->addWidget(mFolderQuickSearchCB);
    vlay->addLayout(folderCBHLayout);

    // "favorite folders view mode" radio buttons:
    mFavoriteFoldersViewGroupBox = new QGroupBox(this);
    mFavoriteFoldersViewGroupBox->setTitle(i18n("Show Favorite Folders View"));
    mFavoriteFoldersViewGroupBox->setLayout(new QVBoxLayout());
    mFavoriteFoldersViewGroup = new QButtonGroup(this);
    connect(mFavoriteFoldersViewGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(slotEmitChanged()));

    QRadioButton *favoriteFoldersViewHiddenRadio = new QRadioButton(i18n("Never"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewHiddenRadio, static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewHiddenRadio);

    QRadioButton *favoriteFoldersViewIconsRadio = new QRadioButton(i18n("As icons"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewIconsRadio, static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewIconsRadio);

    QRadioButton *favoriteFoldersViewListRadio = new QRadioButton(i18n("As list"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewListRadio,  static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewListRadio);

    vlay->addWidget(mFavoriteFoldersViewGroupBox);

    // "folder tooltips" radio buttons:
    mFolderToolTipsGroupBox = new QGroupBox(this);
    mFolderToolTipsGroupBox->setTitle(i18n("Folder Tooltips"));
    mFolderToolTipsGroupBox->setLayout(new QVBoxLayout());
    mFolderToolTipsGroup = new QButtonGroup(this);
    connect(mFolderToolTipsGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(slotEmitChanged()));

    QRadioButton *folderToolTipsAlwaysRadio = new QRadioButton(i18n("Always"), mFolderToolTipsGroupBox);
    mFolderToolTipsGroup->addButton(folderToolTipsAlwaysRadio, static_cast< int >(FolderTreeWidget::DisplayAlways));
    mFolderToolTipsGroupBox->layout()->addWidget(folderToolTipsAlwaysRadio);

    QRadioButton *folderToolTipsNeverRadio = new QRadioButton(i18n("Never"), mFolderToolTipsGroupBox);
    mFolderToolTipsGroup->addButton(folderToolTipsNeverRadio, static_cast< int >(FolderTreeWidget::DisplayNever));
    mFolderToolTipsGroupBox->layout()->addWidget(folderToolTipsNeverRadio);

    vlay->addWidget(mFolderToolTipsGroupBox);

    // "show reader window" radio buttons:
    populateButtonGroup(mReaderWindowModeGroupBox = new QGroupBox(this),
                        mReaderWindowModeGroup = new QButtonGroup(this),
                        Qt::Vertical, KMailSettings::self()->readerWindowModeItem());
    vlay->addWidget(mReaderWindowModeGroupBox);
    connect(mReaderWindowModeGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(slotEmitChanged()));

    vlay->addStretch(10);   // spacer
}

void AppearancePage::LayoutTab::doLoadOther()
{
    loadWidget(mFolderListGroupBox, mFolderListGroup, KMailSettings::self()->folderListItem());
    loadWidget(mReaderWindowModeGroupBox, mReaderWindowModeGroup, KMailSettings::self()->readerWindowModeItem());
    loadWidget(mFavoriteFoldersViewGroupBox, mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem());
    loadWidget(mFolderQuickSearchCB, KMailSettings::self()->enableFolderQuickSearchItem());
    const int checkedFolderToolTipsPolicy = KMailSettings::self()->toolTipDisplayPolicy();
    if (checkedFolderToolTipsPolicy >= 0) {
        mFolderToolTipsGroup->button(checkedFolderToolTipsPolicy)->setChecked(true);
    }
}

void AppearancePage::LayoutTab::save()
{
    saveButtonGroup(mFolderListGroup, KMailSettings::self()->folderListItem());
    saveButtonGroup(mReaderWindowModeGroup, KMailSettings::self()->readerWindowModeItem());
    saveButtonGroup(mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem());
    saveCheckBox(mFolderQuickSearchCB, KMailSettings::self()->enableFolderQuickSearchItem());
    KMailSettings::self()->setToolTipDisplayPolicy(mFolderToolTipsGroup->checkedId());
}

//
// Appearance Message List
//

QString AppearancePage::HeadersTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-headers");
}

static const struct {
    const char *displayName;
    DateFormatter::FormatType dateDisplay;
} dateDisplayConfig[] = {
    { I18N_NOOP("Sta&ndard format (%1)"), KMime::DateFormatter::CTime },
    { I18N_NOOP("Locali&zed format (%1)"), KMime::DateFormatter::Localized },
    { I18N_NOOP("Smart for&mat (%1)"), KMime::DateFormatter::Fancy },
    { I18N_NOOP("C&ustom format:"), KMime::DateFormatter::Custom }
};
static const int numDateDisplayConfig =
    sizeof dateDisplayConfig / sizeof * dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab(QWidget *parent)
    : ConfigModuleTab(parent),
      mCustomDateFormatEdit(Q_NULLPTR)
{
    QVBoxLayout *vlay = new QVBoxLayout(this);

    // "General Options" group:
    QGroupBox *group = new QGroupBox(i18nc("General options for the message list.", "General"), this);
    QVBoxLayout *gvlay = new QVBoxLayout(group);

    mDisplayMessageToolTips = new QCheckBox(
        MessageList::MessageListSettings::self()->messageToolTipEnabledItem()->label(), group);
    gvlay->addWidget(mDisplayMessageToolTips);

    connect(mDisplayMessageToolTips, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    mTabsHaveCloseButton = new QCheckBox(
        MessageList::MessageListSettings::self()->tabsHaveCloseButtonItem()->label(), group);
    gvlay->addWidget(mTabsHaveCloseButton);

    connect(mTabsHaveCloseButton, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    // "Aggregation"
    using MessageList::Utils::AggregationComboBox;
    mAggregationComboBox = new AggregationComboBox(group);

    QLabel *aggregationLabel = new QLabel(i18n("Default aggregation:"), group);
    aggregationLabel->setBuddy(mAggregationComboBox);

    using MessageList::Utils::AggregationConfigButton;
    AggregationConfigButton *aggregationConfigButton = new AggregationConfigButton(group, mAggregationComboBox);

    QHBoxLayout *aggregationLayout = new QHBoxLayout();
    aggregationLayout->addWidget(aggregationLabel, 1);
    aggregationLayout->addWidget(mAggregationComboBox, 1);
    aggregationLayout->addWidget(aggregationConfigButton, 0);
    gvlay->addLayout(aggregationLayout);

    connect(aggregationConfigButton, &MessageList::Utils::AggregationConfigButton::configureDialogCompleted,
            this, &AppearancePageHeadersTab::slotSelectDefaultAggregation);
    connect(mAggregationComboBox, SIGNAL(activated(int)),
            this, SLOT(slotEmitChanged()));

    // "Theme"
    using MessageList::Utils::ThemeComboBox;
    mThemeComboBox = new ThemeComboBox(group);

    QLabel *themeLabel = new QLabel(i18n("Default theme:"), group);
    themeLabel->setBuddy(mThemeComboBox);

    using MessageList::Utils::ThemeConfigButton;
    ThemeConfigButton *themeConfigButton = new ThemeConfigButton(group, mThemeComboBox);

    QHBoxLayout *themeLayout = new QHBoxLayout();
    themeLayout->addWidget(themeLabel, 1);
    themeLayout->addWidget(mThemeComboBox, 1);
    themeLayout->addWidget(themeConfigButton, 0);
    gvlay->addLayout(themeLayout);

    connect(themeConfigButton, &MessageList::Utils::ThemeConfigButton::configureDialogCompleted,
            this, &AppearancePageHeadersTab::slotSelectDefaultTheme);
    connect(mThemeComboBox, SIGNAL(activated(int)),
            this, SLOT(slotEmitChanged()));

    vlay->addWidget(group);

    // "Date Display" group:
    mDateDisplayBox = new QGroupBox(this);
    mDateDisplayBox->setTitle(i18n("Date Display"));
    mDateDisplay = new QButtonGroup(this);
    mDateDisplay->setExclusive(true);
    gvlay = new QVBoxLayout(mDateDisplayBox);

    for (int i = 0; i < numDateDisplayConfig; ++i) {
        const char *label = dateDisplayConfig[i].displayName;
        QString buttonLabel;
        if (QString::fromLatin1(label).contains(QStringLiteral("%1"))) {
            buttonLabel = i18n(label, DateFormatter::formatCurrentDate(dateDisplayConfig[i].dateDisplay));
        } else {
            buttonLabel = i18n(label);
        }
        if (dateDisplayConfig[i].dateDisplay == DateFormatter::Custom) {

            QWidget *hbox = new QWidget(mDateDisplayBox);
            QHBoxLayout *hboxHBoxLayout = new QHBoxLayout(hbox);
            hboxHBoxLayout->setMargin(0);
            QRadioButton *radio = new QRadioButton(buttonLabel, hbox);
            hboxHBoxLayout->addWidget(radio);
            mDateDisplay->addButton(radio, dateDisplayConfig[i].dateDisplay);

            mCustomDateFormatEdit = new KLineEdit(hbox);
            hboxHBoxLayout->addWidget(mCustomDateFormatEdit);
            mCustomDateFormatEdit->setEnabled(false);
            hboxHBoxLayout->setStretchFactor(mCustomDateFormatEdit, 1);

            connect(radio, &QAbstractButton::toggled,
                    mCustomDateFormatEdit, &QWidget::setEnabled);
            connect(mCustomDateFormatEdit, &QLineEdit::textChanged,
                    this, &ConfigModuleTab::slotEmitChanged);

            QLabel *formatHelp = new QLabel(
                i18n("<qt><a href=\"whatsthis1\">Custom format information...</a></qt>"), hbox);
            formatHelp->setContextMenuPolicy(Qt::NoContextMenu);
            connect(formatHelp, &QLabel::linkActivated,
                    this, &AppearancePageHeadersTab::slotLinkClicked);
            hboxHBoxLayout->addWidget(formatHelp);

            mCustomDateWhatsThis =
                i18n("<qt><p><strong>These expressions may be used for the date:"
                     "</strong></p>"
                     "<ul>"
                     "<li>d - the day as a number without a leading zero (1-31)</li>"
                     "<li>dd - the day as a number with a leading zero (01-31)</li>"
                     "<li>ddd - the abbreviated day name (Mon - Sun)</li>"
                     "<li>dddd - the long day name (Monday - Sunday)</li>"
                     "<li>M - the month as a number without a leading zero (1-12)</li>"
                     "<li>MM - the month as a number with a leading zero (01-12)</li>"
                     "<li>MMM - the abbreviated month name (Jan - Dec)</li>"
                     "<li>MMMM - the long month name (January - December)</li>"
                     "<li>yy - the year as a two digit number (00-99)</li>"
                     "<li>yyyy - the year as a four digit number (0000-9999)</li>"
                     "</ul>"
                     "<p><strong>These expressions may be used for the time:"
                     "</strong></p> "
                     "<ul>"
                     "<li>h - the hour without a leading zero (0-23 or 1-12 if AM/PM display)</li>"
                     "<li>hh - the hour with a leading zero (00-23 or 01-12 if AM/PM display)</li>"
                     "<li>m - the minutes without a leading zero (0-59)</li>"
                     "<li>mm - the minutes with a leading zero (00-59)</li>"
                     "<li>s - the seconds without a leading zero (0-59)</li>"
                     "<li>ss - the seconds with a leading zero (00-59)</li>"
                     "<li>z - the milliseconds without leading zeroes (0-999)</li>"
                     "<li>zzz - the milliseconds with leading zeroes (000-999)</li>"
                     "<li>AP - switch to AM/PM display. AP will be replaced by either \"AM\" or \"PM\".</li>"
                     "<li>ap - switch to AM/PM display. ap will be replaced by either \"am\" or \"pm\".</li>"
                     "<li>Z - time zone in numeric form (-0500)</li>"
                     "</ul>"
                     "<p><strong>All other input characters will be ignored."
                     "</strong></p></qt>");
            mCustomDateFormatEdit->setWhatsThis(mCustomDateWhatsThis);
            radio->setWhatsThis(mCustomDateWhatsThis);
            gvlay->addWidget(hbox);
        } else {
            QRadioButton *radio = new QRadioButton(buttonLabel, mDateDisplayBox);
            gvlay->addWidget(radio);
            mDateDisplay->addButton(radio, dateDisplayConfig[i].dateDisplay);
        }
    } // end for loop populating mDateDisplay

    vlay->addWidget(mDateDisplayBox);
    connect(mDateDisplay, SIGNAL(buttonClicked(int)), this, SLOT(slotEmitChanged()));

    vlay->addStretch(10);   // spacer
}

void AppearancePageHeadersTab::slotLinkClicked(const QString &link)
{
    if (link == QLatin1String("whatsthis1")) {
        QWhatsThis::showText(QCursor::pos(), mCustomDateWhatsThis);
    }
}

void AppearancePage::HeadersTab::slotSelectDefaultAggregation()
{
    // Select current default aggregation.
    mAggregationComboBox->selectDefault();
}

void AppearancePage::HeadersTab::slotSelectDefaultTheme()
{
    // Select current default theme.
    mThemeComboBox->selectDefault();
}

void AppearancePage::HeadersTab::doLoadOther()
{
    // "General Options":
    loadWidget(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());
    loadWidget(mTabsHaveCloseButton, MessageList::MessageListSettings::self()->tabsHaveCloseButtonItem());

    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    // "Date Display":
    setDateDisplay(MessageCore::MessageCoreSettings::self()->dateFormat(),
                   MessageCore::MessageCoreSettings::self()->customDateFormat());
}

void AppearancePage::HeadersTab::doLoadFromGlobalSettings()
{
    loadWidget(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());
    loadWidget(mTabsHaveCloseButton, MessageList::MessageListSettings::self()->tabsHaveCloseButtonItem());
    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    setDateDisplay(MessageCore::MessageCoreSettings::self()->dateFormat(),
                   MessageCore::MessageCoreSettings::self()->customDateFormat());
}

void AppearancePage::HeadersTab::setDateDisplay(int num, const QString &format)
{
    DateFormatter::FormatType dateDisplay =
        static_cast<DateFormatter::FormatType>(num);

    // special case: needs text for the line edit:
    if (dateDisplay == DateFormatter::Custom) {
        mCustomDateFormatEdit->setText(format);
    }

    for (int i = 0; i < numDateDisplayConfig; ++i)
        if (dateDisplay == dateDisplayConfig[i].dateDisplay) {
            mDateDisplay->button(dateDisplay)->setChecked(true);
            return;
        }
    // fell through since none found:
    mDateDisplay->button(numDateDisplayConfig - 2)->setChecked(true);   // default
}

void AppearancePage::HeadersTab::save()
{
    saveCheckBox(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());
    saveCheckBox(mTabsHaveCloseButton, MessageList::MessageListSettings::self()->tabsHaveCloseButtonItem());

    KMKernel::self()->savePaneSelection();
    // "Aggregation"
    mAggregationComboBox->writeDefaultConfig();

    // "Theme"
    mThemeComboBox->writeDefaultConfig();

    const int dateDisplayID = mDateDisplay->checkedId();
    MessageCore::MessageCoreSettings::self()->setDateFormat(dateDisplayID);
    MessageCore::MessageCoreSettings::self()->setCustomDateFormat(mCustomDateFormatEdit->text());
}

//
// Message Window
//

QString AppearancePage::ReaderTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-reader");
}

AppearancePageReaderTab::AppearancePageReaderTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    // "Close message window after replying or forwarding" check box:
    populateCheckBox(mCloseAfterReplyOrForwardCheck = new QCheckBox(this),
                     KMailSettings::self()->closeAfterReplyOrForwardItem());
    mCloseAfterReplyOrForwardCheck->setToolTip(
        i18n("Close the standalone message window after replying or forwarding the message"));
    topLayout->addWidget(mCloseAfterReplyOrForwardCheck);
    connect(mCloseAfterReplyOrForwardCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    mViewerSettings = new MessageViewer::ConfigureWidget;
    connect(mViewerSettings, &MessageViewer::ConfigureWidget::settingsChanged,
            this, &ConfigModuleTab::slotEmitChanged);
    topLayout->addWidget(mViewerSettings);

    mGravatarConfigWidget = new Gravatar::GravatarConfigWidget;
    connect(mGravatarConfigWidget, &Gravatar::GravatarConfigWidget::configChanged, this, &ConfigModuleTab::slotEmitChanged);
    topLayout->addWidget(mGravatarConfigWidget);
    topLayout->addStretch(100);   // spacer
}

void AppearancePage::ReaderTab::doResetToDefaultsOther()
{
    mGravatarConfigWidget->doResetToDefaultsOther();
}

void AppearancePage::ReaderTab::doLoadOther()
{
    loadWidget(mCloseAfterReplyOrForwardCheck, KMailSettings::self()->closeAfterReplyOrForwardItem());
    mViewerSettings->readConfig();
    mGravatarConfigWidget->doLoadFromGlobalSettings();
}

void AppearancePage::ReaderTab::save()
{
    saveCheckBox(mCloseAfterReplyOrForwardCheck, KMailSettings::self()->closeAfterReplyOrForwardItem());
    mViewerSettings->writeConfig();
    mGravatarConfigWidget->save();
}

QString AppearancePage::SystemTrayTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-systemtray");
}

AppearancePageSystemTrayTab::AppearancePageSystemTrayTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout(this);

    // "Enable system tray applet" check box
    mSystemTrayCheck = new QCheckBox(i18n("Enable system tray icon"), this);
    vlay->addWidget(mSystemTrayCheck);
    connect(mSystemTrayCheck, &QCheckBox::stateChanged,
            this, &ConfigModuleTab::slotEmitChanged);

    vlay->addStretch(10);
}

void AppearancePage::SystemTrayTab::doLoadFromGlobalSettings()
{
    loadWidget(mSystemTrayCheck, KMailSettings::self()->systemTrayEnabledItem());
}

void AppearancePage::SystemTrayTab::save()
{
    saveCheckBox(mSystemTrayCheck, KMailSettings::self()->systemTrayEnabledItem());
    KMailSettings::self()->save();
}

QString AppearancePage::MessageTagTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-messagetag");
}

TagListWidgetItem::TagListWidgetItem(QListWidget *parent)
    : QListWidgetItem(parent), mTag(Q_NULLPTR)
{
}

TagListWidgetItem::TagListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent)
    : QListWidgetItem(icon, text, parent), mTag(Q_NULLPTR)
{
}

TagListWidgetItem::~TagListWidgetItem()
{
}

void TagListWidgetItem::setKMailTag(const MailCommon::Tag::Ptr &tag)
{
    mTag = tag;
}

MailCommon::Tag::Ptr TagListWidgetItem::kmailTag() const
{
    return mTag;
}

AppearancePageMessageTagTab::AppearancePageMessageTagTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mPreviousTag = -1;
    QHBoxLayout *maingrid = new QHBoxLayout(this);

    //Lefthand side Listbox and friends

    //Groupbox frame
    mTagsGroupBox = new QGroupBox(i18n("A&vailable Tags"), this);
    maingrid->addWidget(mTagsGroupBox);
    QVBoxLayout *tageditgrid = new QVBoxLayout(mTagsGroupBox);

    //Listbox, add, remove row
    QHBoxLayout *addremovegrid = new QHBoxLayout();
    tageditgrid->addLayout(addremovegrid);

    mTagAddLineEdit = new KLineEdit(mTagsGroupBox);
    mTagAddLineEdit->setTrapReturnKey(true);
    addremovegrid->addWidget(mTagAddLineEdit);

    mTagAddButton = new QPushButton(mTagsGroupBox);
    mTagAddButton->setToolTip(i18n("Add new tag"));
    mTagAddButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addremovegrid->addWidget(mTagAddButton);

    mTagRemoveButton = new QPushButton(mTagsGroupBox);
    mTagRemoveButton->setToolTip(i18n("Remove selected tag"));
    mTagRemoveButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    addremovegrid->addWidget(mTagRemoveButton);

    //Up and down buttons
    QHBoxLayout *updowngrid = new QHBoxLayout();
    tageditgrid->addLayout(updowngrid);

    mTagUpButton = new QPushButton(mTagsGroupBox);
    mTagUpButton->setToolTip(i18n("Increase tag priority"));
    mTagUpButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    mTagUpButton->setAutoRepeat(true);
    updowngrid->addWidget(mTagUpButton);

    mTagDownButton = new QPushButton(mTagsGroupBox);
    mTagDownButton->setToolTip(i18n("Decrease tag priority"));
    mTagDownButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    mTagDownButton->setAutoRepeat(true);
    updowngrid->addWidget(mTagDownButton);

    //Listbox for tag names
    QHBoxLayout *listboxgrid = new QHBoxLayout();
    tageditgrid->addLayout(listboxgrid);
    mTagListBox = new QListWidget(mTagsGroupBox);
    mTagListBox->setDragDropMode(QAbstractItemView::InternalMove);
    connect(mTagListBox->model(), &QAbstractItemModel::rowsMoved, this, &AppearancePageMessageTagTab::slotRowsMoved);

    mTagListBox->setMinimumWidth(150);
    listboxgrid->addWidget(mTagListBox);

    //RHS for individual tag settings

    //Extra VBoxLayout for stretchers around settings
    QVBoxLayout *tagsettinggrid = new QVBoxLayout();
    maingrid->addLayout(tagsettinggrid);
    //tagsettinggrid->addStretch( 10 );

    //Groupbox frame
    mTagSettingGroupBox = new QGroupBox(i18n("Ta&g Settings"),
                                        this);
    tagsettinggrid->addWidget(mTagSettingGroupBox);
    QList<KActionCollection *> actionCollections;
    if (kmkernel->getKMMainWidget()) {
        actionCollections = kmkernel->getKMMainWidget()->actionCollections();
    }

    QHBoxLayout *lay = new QHBoxLayout(mTagSettingGroupBox);
    mTagWidget = new MailCommon::TagWidget(actionCollections, this);
    lay->addWidget(mTagWidget);

    connect(mTagWidget, &TagWidget::changed, this, &AppearancePageMessageTagTab::slotEmitChangeCheck);

    //For enabling the add button in case box is non-empty
    connect(mTagAddLineEdit, &KLineEdit::textChanged,
            this, &AppearancePage::MessageTagTab::slotAddLineTextChanged);

    //For on-the-fly updating of tag name in editbox
    connect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged,
            this, &AppearancePageMessageTagTab::slotNameLineTextChanged);

    connect(mTagWidget, &TagWidget::iconNameChanged, this, &AppearancePageMessageTagTab::slotIconNameChanged);

    connect(mTagAddLineEdit, &KLineEdit::returnPressed,
            this, &AppearancePageMessageTagTab::slotAddNewTag);

    connect(mTagAddButton, &QAbstractButton::clicked,
            this, &AppearancePageMessageTagTab::slotAddNewTag);

    connect(mTagRemoveButton, &QAbstractButton::clicked,
            this, &AppearancePageMessageTagTab::slotRemoveTag);

    connect(mTagUpButton, &QAbstractButton::clicked,
            this, &AppearancePageMessageTagTab::slotMoveTagUp);

    connect(mTagDownButton, &QAbstractButton::clicked,
            this, &AppearancePageMessageTagTab::slotMoveTagDown);

    connect(mTagListBox, &QListWidget::currentItemChanged,
            this, &AppearancePageMessageTagTab::slotSelectionChanged);
    //Adjust widths for columns
    maingrid->setStretchFactor(mTagsGroupBox, 1);
    maingrid->setStretchFactor(lay, 1);
    tagsettinggrid->addStretch(10);
}

AppearancePageMessageTagTab::~AppearancePageMessageTagTab()
{
}

void AppearancePage::MessageTagTab::slotEmitChangeCheck()
{
    slotEmitChanged();
}

void AppearancePage::MessageTagTab::slotRowsMoved(const QModelIndex &,
        int sourcestart, int sourceEnd,
        const QModelIndex &, int destinationRow)
{
    Q_UNUSED(sourceEnd);
    Q_UNUSED(sourcestart);
    Q_UNUSED(destinationRow);
    updateButtons();
    slotEmitChangeCheck();
}

void AppearancePage::MessageTagTab::updateButtons()
{
    const int currentIndex = mTagListBox->currentRow();

    const bool theFirst = (currentIndex == 0);
    const bool theLast = (currentIndex >= (int)mTagListBox->count() - 1);
    const bool aFilterIsSelected = (currentIndex >= 0);

    mTagUpButton->setEnabled(aFilterIsSelected && !theFirst);
    mTagDownButton->setEnabled(aFilterIsSelected && !theLast);
}

void AppearancePage::MessageTagTab::slotMoveTagUp()
{
    const int tmp_index = mTagListBox->currentRow();
    if (tmp_index <= 0) {
        return;
    }
    swapTagsInListBox(tmp_index, tmp_index - 1);
    updateButtons();
}

void AppearancePage::MessageTagTab::slotMoveTagDown()
{
    const int tmp_index = mTagListBox->currentRow();
    if ((tmp_index < 0)
            || (tmp_index >= int(mTagListBox->count()) - 1)) {
        return;
    }
    swapTagsInListBox(tmp_index, tmp_index + 1);
    updateButtons();
}

void AppearancePage::MessageTagTab::swapTagsInListBox(const int first,
        const int second)
{
    disconnect(mTagListBox, &QListWidget::currentItemChanged,
               this, &AppearancePageMessageTagTab::slotSelectionChanged);
    QListWidgetItem *item = mTagListBox->takeItem(first);
    // now selected item is at idx(idx-1), so
    // insert the other item at idx, ie. above(below).
    mPreviousTag = second;
    mTagListBox->insertItem(second, item);
    mTagListBox->setCurrentRow(second);
    connect(mTagListBox, &QListWidget::currentItemChanged,
            this, &AppearancePageMessageTagTab::slotSelectionChanged);
    slotEmitChangeCheck();
}

void AppearancePage::MessageTagTab::slotRecordTagSettings(int aIndex)
{
    if ((aIndex < 0) || (aIndex >= int(mTagListBox->count()))) {
        return;
    }
    QListWidgetItem *item = mTagListBox->item(aIndex);
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem *>(item);

    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    tmp_desc->tagName = tagItem->text();
    mTagWidget->recordTagSettings(tmp_desc);
}

void AppearancePage::MessageTagTab::slotUpdateTagSettingWidgets(int aIndex)
{
    //Check if selection is valid
    if ((aIndex < 0) || (mTagListBox->currentRow() < 0)) {
        mTagRemoveButton->setEnabled(false);
        mTagUpButton->setEnabled(false);
        mTagDownButton->setEnabled(false);

        mTagWidget->setEnabled(false);
        return;
    }
    mTagWidget->setEnabled(true);

    mTagRemoveButton->setEnabled(true);
    mTagUpButton->setEnabled((0 != aIndex));
    mTagDownButton->setEnabled(((int(mTagListBox->count()) - 1) != aIndex));
    QListWidgetItem *item = mTagListBox->currentItem();
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem *>(item);
    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    disconnect(mTagWidget->tagNameLineEdit(), &KLineEdit::textChanged,
               this, &AppearancePage::MessageTagTab::slotNameLineTextChanged);

    mTagWidget->tagNameLineEdit()->setEnabled(!tmp_desc->isImmutable);
    mTagWidget->tagNameLineEdit()->setText(tmp_desc->tagName);
    connect(mTagWidget->tagNameLineEdit(), &KLineEdit::textChanged,
            this, &AppearancePage::MessageTagTab::slotNameLineTextChanged);

    mTagWidget->setTagTextColor(tmp_desc->textColor);

    mTagWidget->setTagBackgroundColor(tmp_desc->backgroundColor);

    mTagWidget->setTagTextFormat(tmp_desc->isBold, tmp_desc->isItalic);

    mTagWidget->iconButton()->setEnabled(!tmp_desc->isImmutable);
    mTagWidget->iconButton()->setIcon(tmp_desc->iconName);

    mTagWidget->keySequenceWidget()->setEnabled(true);
    mTagWidget->keySequenceWidget()->setKeySequence(tmp_desc->shortcut,
            KKeySequenceWidget::NoValidate);

    mTagWidget->inToolBarCheck()->setEnabled(true);
    mTagWidget->inToolBarCheck()->setChecked(tmp_desc->inToolbar);
}

void AppearancePage::MessageTagTab::slotSelectionChanged()
{
    mEmitChanges = false;
    slotRecordTagSettings(mPreviousTag);
    slotUpdateTagSettingWidgets(mTagListBox->currentRow());
    mPreviousTag = mTagListBox->currentRow();
    mEmitChanges = true;
}

void AppearancePage::MessageTagTab::slotRemoveTag()
{
    const int tmp_index = mTagListBox->currentRow();
    if (tmp_index >= 0) {
        if (KMessageBox::Yes == KMessageBox::questionYesNo(this, i18n("Do you want to remove tag \'%1\'?", mTagListBox->item(mTagListBox->currentRow())->text()))) {
            QListWidgetItem *item = mTagListBox->takeItem(mTagListBox->currentRow());
            TagListWidgetItem *tagItem = static_cast<TagListWidgetItem *>(item);
            MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();
            if (tmp_desc->tag().isValid()) {
                new Akonadi::TagDeleteJob(tmp_desc->tag());
            } else {
                qCWarning(KMAIL_LOG) << "Can't remove tag with invalid akonadi tag";
            }
            mPreviousTag = -1;

            //Before deleting the current item, make sure the selectionChanged signal
            //is disconnected, so that the widgets will not get updated while the
            //deletion takes place.
            disconnect(mTagListBox, &QListWidget::currentItemChanged,
                       this, &AppearancePageMessageTagTab::slotSelectionChanged);

            delete item;
            connect(mTagListBox, &QListWidget::currentItemChanged,
                    this, &AppearancePageMessageTagTab::slotSelectionChanged);

            slotSelectionChanged();
            slotEmitChangeCheck();
        }
    }
}

void AppearancePage::MessageTagTab::slotDeleteTagJob(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "Failed to delete tag " << job->errorString();
    }
}

void AppearancePage::MessageTagTab::slotNameLineTextChanged(const QString &aText)
{
    //If deleted all, leave the first character for the sake of not having an
    //empty tag name
    if (aText.isEmpty() && !mTagListBox->currentItem()) {
        return;
    }

    const int count = mTagListBox->count();
    for (int i = 0; i < count; ++i) {
        if (mTagListBox->item(i)->text() == aText) {
            KMessageBox::error(this, i18n("We cannot create tag. A tag with same name already exists."));
            disconnect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged,
                       this, &AppearancePageMessageTagTab::slotNameLineTextChanged);
            mTagWidget->tagNameLineEdit()->setText(mTagListBox->currentItem()->text());
            connect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged,
                    this, &AppearancePageMessageTagTab::slotNameLineTextChanged);
            return;
        }
    }

    //Disconnect so the tag information is not saved and reloaded with every
    //letter
    disconnect(mTagListBox, &QListWidget::currentItemChanged,
               this, &AppearancePageMessageTagTab::slotSelectionChanged);

    mTagListBox->currentItem()->setText(aText);
    connect(mTagListBox, &QListWidget::currentItemChanged,
            this, &AppearancePageMessageTagTab::slotSelectionChanged);
}

void AppearancePage::MessageTagTab::slotIconNameChanged(const QString &iconName)
{
    mTagListBox->currentItem()->setIcon(QIcon::fromTheme(iconName));
}

void AppearancePage::MessageTagTab::slotAddLineTextChanged(const QString &aText)
{
    mTagAddButton->setEnabled(!aText.trimmed().isEmpty());
}

void AppearancePage::MessageTagTab::slotAddNewTag()
{
    const QString newTagName = mTagAddLineEdit->text();
    const int count = mTagListBox->count();
    for (int i = 0; i < count; ++i) {
        if (mTagListBox->item(i)->text() == newTagName) {
            KMessageBox::error(this, i18n("We cannot create tag. A tag with same name already exists."));
            return;
        }
    }

    const int tmp_priority = mTagListBox->count();

    MailCommon::Tag::Ptr tag(Tag::createDefaultTag(newTagName));
    tag->priority = tmp_priority;

    slotEmitChangeCheck();
    TagListWidgetItem *newItem = new TagListWidgetItem(QIcon::fromTheme(tag->iconName), newTagName,  mTagListBox);
    newItem->setKMailTag(tag);
    mTagListBox->addItem(newItem);
    mTagListBox->setCurrentItem(newItem);
    mTagAddLineEdit->clear();
}

void AppearancePage::MessageTagTab::doLoadFromGlobalSettings()
{
    mTagListBox->clear();

    Akonadi::TagFetchJob *fetchJob = new Akonadi::TagFetchJob(this);
    fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(fetchJob, &KJob::result, this, &AppearancePageMessageTagTab::slotTagsFetched);
}

void AppearancePage::MessageTagTab::slotTagsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "Failed to load tags " << job->errorString();
        return;
    }
    Akonadi::TagFetchJob *fetchJob = static_cast<Akonadi::TagFetchJob *>(job);

    QList<MailCommon::TagPtr> msgTagList;
    const Akonadi::Tag::List tagList = fetchJob->tags();
    msgTagList.reserve(tagList.count());
    foreach (const Akonadi::Tag &akonadiTag, tagList) {
        MailCommon::Tag::Ptr tag = MailCommon::Tag::fromAkonadi(akonadiTag);
        msgTagList.append(tag);
    }

    std::sort(msgTagList.begin(), msgTagList.end(), MailCommon::Tag::compare);

    foreach (const MailCommon::Tag::Ptr &tag, msgTagList) {
        TagListWidgetItem *newItem = new TagListWidgetItem(QIcon::fromTheme(tag->iconName), tag->tagName, mTagListBox);
        newItem->setKMailTag(tag);
        if (tag->priority == -1) {
            tag->priority = mTagListBox->count() - 1;
        }
    }

    //Disconnect so that insertItem's do not trigger an update procedure
    disconnect(mTagListBox, &QListWidget::currentItemChanged,
               this, &AppearancePageMessageTagTab::slotSelectionChanged);

    connect(mTagListBox, &QListWidget::currentItemChanged,
            this, &AppearancePageMessageTagTab::slotSelectionChanged);

    slotUpdateTagSettingWidgets(-1);
    //Needed since the previous function doesn't affect add button
    mTagAddButton->setEnabled(false);

    // Save the original list
    mOriginalMsgTagList.clear();
    foreach (const MailCommon::TagPtr &tag, msgTagList) {
        mOriginalMsgTagList.append(MailCommon::TagPtr(new MailCommon::Tag(*tag)));
    }

}

void AppearancePage::MessageTagTab::save()
{
    const int currentRow = mTagListBox->currentRow();
    if (currentRow < 0) {
        return;
    }

    const int count = mTagListBox->count();
    if (!count) {
        return;
    }

    QListWidgetItem *item = mTagListBox->currentItem();
    if (!item) {
        return;
    }
    slotRecordTagSettings(currentRow);
    const int numberOfMsgTagList = count;
    for (int i = 0; i < numberOfMsgTagList; ++i) {
        TagListWidgetItem *tagItem = static_cast<TagListWidgetItem *>(mTagListBox->item(i));
        if ((i >= mOriginalMsgTagList.count()) || *(tagItem->kmailTag()) != *(mOriginalMsgTagList[i])) {
            MailCommon::Tag::Ptr tag = tagItem->kmailTag();
            tag->priority = i;
            Akonadi::Tag akonadiTag = tag->saveToAkonadi();
            if ((*tag).id() > 0) {
                akonadiTag.setId((*tag).id());
            }
            if (akonadiTag.isValid()) {
                new Akonadi::TagModifyJob(akonadiTag);
            } else {
                new Akonadi::TagCreateJob(akonadiTag);
            }
        }
    }
}

