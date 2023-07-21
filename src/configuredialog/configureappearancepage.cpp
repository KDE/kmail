/*
  SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configureappearancepage.h"
#include <PimCommon/ConfigureImmutableWidgetUtils>
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "configuredialog/colorlistbox.h"
#include "kmkernel.h"
#include <Libkdepim/LineEditCatchReturnKey>
#include <MailCommon/TagWidget>
#include <MessageList/AggregationComboBox>
#include <MessageList/AggregationConfigButton>
#include <MessageList/ThemeComboBox>
#include <MessageList/ThemeConfigButton>
#include <messagelist/messagelistsettings.h>

#include <MailCommon/FolderTreeWidget>

#include "kmmainwidget.h"

#include <mailcommon/mailcommonsettings_base.h>

#include <MessageViewer/ConfigureWidget>
#include <MessageViewer/MessageViewerSettings>

#include "settings/kmailsettings.h"
#include <MessageCore/ColorUtil>
#include <MessageCore/MessageCoreSettings>
#include <MessageList/MessageListUtil>

#include <MailCommon/MailUtil>

#include <Akonadi/Tag>
#include <Akonadi/TagAttribute>
#include <Akonadi/TagCreateJob>
#include <Akonadi/TagDeleteJob>
#include <Akonadi/TagFetchJob>
#include <Akonadi/TagFetchScope>
#include <Akonadi/TagModifyJob>

#include "kmail_debug.h"
#include <KFontChooser>
#include <KIconButton>
#include <KKeySequenceWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSeparator>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>

#include <KMime/DateFormatter>

#include <KLazyLocalizedString>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWhatsThis>

using KMime::DateFormatter;
using namespace MailCommon;

QString AppearancePage::helpAnchor() const
{
    return QStringLiteral("configure-appearance");
}

#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
AppearancePage::AppearancePage(QWidget *parent, const QVariantList &args)
    : ConfigModuleWithTabs(parent, args)
#else
AppearancePage::AppearancePage(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : ConfigModuleWithTabs(parent, data, args)
#endif
{
    //
    // "General" tab:
    //
    auto readerTab = new AppearancePageGeneralTab();
    addTab(readerTab, i18n("General"));
    addConfig(MessageViewer::MessageViewerSettings::self(), readerTab);

    //
    // "Fonts" tab:
    //
    auto fontsTab = new AppearancePageFontsTab();
    addTab(fontsTab, i18n("Fonts"));

    //
    // "Colors" tab:
    //
    auto colorsTab = new AppearancePageColorsTab();
    addTab(colorsTab, i18n("Colors"));

    //
    // "Layout" tab:
    //
    auto layoutTab = new AppearancePageLayoutTab();
    addTab(layoutTab, i18n("Layout"));

    //
    // "Headers" tab:
    //
    auto headersTab = new AppearancePageHeadersTab();
    addTab(headersTab, i18n("Message List"));

    //
    // "Message Tag" tab:
    //
    auto messageTagTab = new AppearancePageMessageTagTab();
    addTab(messageTagTab, i18n("Message Tags"));
}

QString AppearancePageFontsTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-fonts");
}

static const struct {
    const char *configName;
    const KLazyLocalizedString displayName;
    bool enableFamilyAndSize;
    bool onlyFixed;
} fontNames[] = {
    {"body-font", kli18n("Message Body"), true, false},
    {"MessageListFont", kli18n("Message List"), true, false},
    {"UnreadMessageFont", kli18n("Message List - Unread Messages"), false, false},
    {"ImportantMessageFont", kli18n("Message List - Important Messages"), false, false},
    {"TodoMessageFont", kli18n("Message List - Action Item Messages"), false, false},
    {"fixed-font", kli18n("Fixed Width Font"), true, true},
    {"composer-font", kli18n("Composer"), true, false},
    {"print-font", kli18n("Printing Output"), true, false},
};
static const int numFontNames = sizeof fontNames / sizeof *fontNames;

AppearancePageFontsTab::AppearancePageFontsTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    assert(numFontNames == sizeof mFont / sizeof *mFont);

    // "Use custom fonts" checkbox, followed by <hr>
    auto vlay = new QVBoxLayout(this);
    mCustomFontCheck = new QCheckBox(i18n("&Use custom fonts"), this);
    vlay->addWidget(mCustomFontCheck);
    vlay->addWidget(new KSeparator(Qt::Horizontal, this));
    connect(mCustomFontCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    // "font location" combo box and label:
    auto hlay = new QHBoxLayout(); // inherites spacing
    vlay->addLayout(hlay);
    mFontLocationCombo = new QComboBox(this);
    mFontLocationCombo->setEnabled(false); // !mCustomFontCheck->isChecked()

    QStringList fontDescriptions;
    fontDescriptions.reserve(numFontNames);
    for (int i = 0; i < numFontNames; ++i) {
        fontDescriptions << fontNames[i].displayName.toString();
    }
    mFontLocationCombo->addItems(fontDescriptions);

    auto label = new QLabel(i18n("Apply &to:"), this);
    label->setBuddy(mFontLocationCombo);
    label->setEnabled(false); // since !mCustomFontCheck->isChecked()
    hlay->addWidget(label);

    hlay->addWidget(mFontLocationCombo);
    hlay->addStretch(10);
    mFontChooser = new KFontChooser(KFontChooser::DisplayFrame, this);
    mFontChooser->setMinVisibleItems(4);
    mFontChooser->setEnabled(false); // since !mCustomFontCheck->isChecked()
    vlay->addWidget(mFontChooser);
    connect(mFontChooser, &KFontChooser::fontSelected, this, &ConfigModuleTab::slotEmitChanged);

    // {en,dis}able widgets depending on the state of mCustomFontCheck:
    connect(mCustomFontCheck, &QAbstractButton::toggled, label, &QWidget::setEnabled);
    connect(mCustomFontCheck, &QAbstractButton::toggled, mFontLocationCombo, &QWidget::setEnabled);
    connect(mCustomFontCheck, &QAbstractButton::toggled, mFontChooser, &QWidget::setEnabled);
    // load the right font settings into mFontChooser:
    connect(mFontLocationCombo, &QComboBox::activated, this, &AppearancePageFontsTab::slotFontSelectorChanged);
}

void AppearancePageFontsTab::slotFontSelectorChanged(int index)
{
    qCDebug(KMAIL_LOG) << "slotFontSelectorChanged() called";
    if (index < 0 || index >= mFontLocationCombo->count()) {
        return; // Should never happen, but it is better to check.
    }

    // Save current fontselector setting before we install the new:
    if (mActiveFontIndex == 0) {
        mFont[0] = mFontChooser->font();
        // hardcode the family and size of "message body" dependent fonts:
        for (int i = 0; i < numFontNames; ++i) {
            if (!fontNames[i].enableFamilyAndSize) {
                // ### shall we copy the font and set the save and re-set
                // {regular,italic,bold,bold italic} property or should we
                // copy only family and pointSize?
                mFont[i].setFamily(mFont[0].family());
                mFont[i].setPointSize /*Float?*/ (mFont[0].pointSize /*Float?*/ ());
            }
        }
    } else if (mActiveFontIndex > 0) {
        mFont[mActiveFontIndex] = mFontChooser->font();
    }
    mActiveFontIndex = index;

    // Disonnect so the "Apply" button is not activated by the change
    disconnect(mFontChooser, &KFontChooser::fontSelected, this, &ConfigModuleTab::slotEmitChanged);

    // Display the new setting:
    mFontChooser->setFont(mFont[index], fontNames[index].onlyFixed);

    connect(mFontChooser, &KFontChooser::fontSelected, this, &ConfigModuleTab::slotEmitChanged);

    // Disable Family and Size list if we have selected a quote font:
    mFontChooser->enableColumn(KFontChooser::FamilyList | KFontChooser::SizeList, fontNames[index].enableFamilyAndSize);
}

void AppearancePageFontsTab::doLoadOther()
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
                mFont[i] = fonts.readEntry(configName, (fontNames[i].onlyFixed) ? fixedFont : mFont[0]);
            }
        }
        mCustomFontCheck->setChecked(!MessageCore::MessageCoreSettings::self()->useDefaultFonts());
        mFontLocationCombo->setCurrentIndex(0);
        slotFontSelectorChanged(0);
    } else {
        setEnabled(false);
    }
}

void AppearancePageFontsTab::save()
{
    if (KMKernel::self()) {
        KConfigGroup fonts(KMKernel::self()->config(), "Fonts");

        // read the current font (might have been modified)
        if (mActiveFontIndex >= 0) {
            mFont[mActiveFontIndex] = mFontChooser->font();
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
                if (customFonts || fonts.hasKey(configName)) {
                    // Don't write font info when we use default fonts, but write
                    // if it's already there:
                    fonts.writeEntry(configName, mFont[i]);
                }
            }
        }
    }
}

void AppearancePageFontsTab::doResetToDefaultsOther()
{
    mCustomFontCheck->setChecked(false);
}

QString AppearancePageColorsTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-colors");
}

static const struct {
    const char *configName;
    const KLazyLocalizedString displayName;
} colorNames[] = { // adjust doLoadOther if you change this:
    {"QuotedText1", kli18n("Quoted Text - First Level")},
    {"QuotedText2", kli18n("Quoted Text - Second Level")},
    {"QuotedText3", kli18n("Quoted Text - Third Level")},
    {"LinkColor", kli18n("Link")},
    {"UnreadMessageColor", kli18n("Unread Message")},
    {"ImportantMessageColor", kli18n("Important Message")},
    {"TodoMessageColor", kli18n("Action Item Message")},
    {"ColorbarBackgroundPlain", kli18n("HTML Status Bar Background - No HTML Message")},
    {"ColorbarForegroundPlain", kli18n("HTML Status Bar Foreground - No HTML Message")},
    {"ColorbarBackgroundHTML", kli18n("HTML Status Bar Background - HTML Message")},
    {"ColorbarForegroundHTML", kli18n("HTML Status Bar Foreground - HTML Message")}};
static const int numColorNames = sizeof colorNames / sizeof *colorNames;

AppearancePageColorsTab::AppearancePageColorsTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    // "use custom colors" check box
    auto vlay = new QVBoxLayout(this);
    mCustomColorCheck = new QCheckBox(i18n("&Use custom colors"), this);
    vlay->addWidget(mCustomColorCheck);
    connect(mCustomColorCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    mUseInlineStyle = new QCheckBox(i18n("&Do not change color from original HTML mail"), this);
    vlay->addWidget(mUseInlineStyle);
    connect(mUseInlineStyle, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    // color list box:
    mColorList = new ColorListBox(this);
    mColorList->setEnabled(false); // since !mCustomColorCheck->isChecked()
    for (int i = 0; i < numColorNames; ++i) {
        mColorList->addColor(colorNames[i].displayName.toString());
    }
    vlay->addWidget(mColorList, 1);

    // "recycle colors" check box:
    mRecycleColorCheck = new QCheckBox(i18n("Recycle colors on deep &quoting"), this);
    mRecycleColorCheck->setEnabled(false);
    vlay->addWidget(mRecycleColorCheck);
    connect(mRecycleColorCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    // close to quota threshold
    auto hbox = new QHBoxLayout();
    vlay->addLayout(hbox);
    auto l = new QLabel(i18n("Close to quota threshold:"), this);
    hbox->addWidget(l);
    mCloseToQuotaThreshold = new QSpinBox(this);
    mCloseToQuotaThreshold->setRange(0, 100);
    mCloseToQuotaThreshold->setSingleStep(1);
    connect(mCloseToQuotaThreshold, &QSpinBox::valueChanged, this, &ConfigModuleTab::slotEmitChanged);
    mCloseToQuotaThreshold->setSuffix(i18n("%"));

    hbox->addWidget(mCloseToQuotaThreshold);
    hbox->addWidget(new QWidget(this), 2);

    // {en,dir}able widgets depending on the state of mCustomColorCheck:
    connect(mCustomColorCheck, &QAbstractButton::toggled, mColorList, &QWidget::setEnabled);
    connect(mCustomColorCheck, &QAbstractButton::toggled, mRecycleColorCheck, &QWidget::setEnabled);
    connect(mCustomColorCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);
    connect(mColorList, &ColorListBox::changed, this, &ConfigModuleTab::slotEmitChanged);
}

void AppearancePageColorsTab::doLoadOther()
{
    mCustomColorCheck->setChecked(!MessageCore::MessageCoreSettings::self()->useDefaultColors());
    mUseInlineStyle->setChecked(MessageCore::MessageCoreSettings::self()->useRealHtmlMailColor());
    mRecycleColorCheck->setChecked(MessageViewer::MessageViewerSettings::self()->recycleQuoteColors());
    mCloseToQuotaThreshold->setValue(KMailSettings::self()->closeToQuotaThreshold());
    loadColor(true);
}

void AppearancePageColorsTab::loadColor(bool loadFromConfig)
{
    if (KMKernel::self()) {
        KConfigGroup reader(KMKernel::self()->config(), "Reader");

        static const QColor defaultColor[numColorNames] = {
            MessageCore::ColorUtil::self()->quoteLevel1DefaultTextColor(),
            MessageCore::ColorUtil::self()->quoteLevel2DefaultTextColor(),
            MessageCore::ColorUtil::self()->quoteLevel3DefaultTextColor(),
            MessageCore::ColorUtil::self()->linkColor(), // link
            MessageList::Util::unreadDefaultMessageColor(), // unread mgs
            MessageList::Util::importantDefaultMessageColor(), // important msg
            MessageList::Util::todoDefaultMessageColor(), // action item mgs
            Qt::lightGray, // colorbar plain bg
            Qt::black, // colorbar plain fg
            Qt::black, // colorbar html  bg
            Qt::white // colorbar html  fg
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

void AppearancePageColorsTab::doResetToDefaultsOther()
{
    mCustomColorCheck->setChecked(false);
    mUseInlineStyle->setChecked(false);
    mRecycleColorCheck->setChecked(false);
    mCloseToQuotaThreshold->setValue(80);
    loadColor(false);
}

void AppearancePageColorsTab::save()
{
    if (!KMKernel::self()) {
        return;
    }
    KConfigGroup reader(KMKernel::self()->config(), "Reader");
    bool customColors = mCustomColorCheck->isChecked();
    MessageCore::MessageCoreSettings::self()->setUseDefaultColors(!customColors);
    MessageCore::MessageCoreSettings::self()->setUseRealHtmlMailColor(mUseInlineStyle->isChecked());

    for (int i = 0; i < numColorNames; ++i) {
        const QString configName = QLatin1String(colorNames[i].configName);
        if (customColors && configName == QLatin1String("UnreadMessageColor")) {
            MessageList::MessageListSettings::self()->setUnreadMessageColor(mColorList->color(i));
        } else if (customColors && configName == QLatin1String("ImportantMessageColor")) {
            MessageList::MessageListSettings::self()->setImportantMessageColor(mColorList->color(i));
        } else if (customColors && configName == QLatin1String("TodoMessageColor")) {
            MessageList::MessageListSettings::self()->setTodoMessageColor(mColorList->color(i));
        } else {
            if (customColors || reader.hasKey(configName)) {
                reader.writeEntry(configName, mColorList->color(i));
            }
        }
    }
    MessageViewer::MessageViewerSettings::self()->setRecycleQuoteColors(mRecycleColorCheck->isChecked());
    KMailSettings::self()->setCloseToQuotaThreshold(mCloseToQuotaThreshold->value());
}

QString AppearancePageLayoutTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-layout");
}

AppearancePageLayoutTab::AppearancePageLayoutTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto vlay = new QVBoxLayout(this);

    // "folder list" radio buttons:
    populateButtonGroup(mFolderListGroupBox = new QGroupBox(this),
                        mFolderListGroup = new QButtonGroup(this),
                        Qt::Vertical,
                        KMailSettings::self()->folderListItem());
    vlay->addWidget(mFolderListGroupBox);
    connect(mFolderListGroup, &QButtonGroup::buttonClicked, this, &ConfigModuleTab::slotEmitChanged);

    auto folderCBHLayout = new QHBoxLayout;
    mFolderQuickSearchCB = new QCheckBox(i18n("Show folder quick search field"), this);
    connect(mFolderQuickSearchCB, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    folderCBHLayout->addWidget(mFolderQuickSearchCB);
    vlay->addLayout(folderCBHLayout);

    // "favorite folders view mode" radio buttons:
    mFavoriteFoldersViewGroupBox = new QGroupBox(this);
    mFavoriteFoldersViewGroupBox->setTitle(i18n("Show Favorite Folders View"));
    mFavoriteFoldersViewGroupBox->setLayout(new QVBoxLayout());
    mFavoriteFoldersViewGroup = new QButtonGroup(this);
    connect(mFavoriteFoldersViewGroup, &QButtonGroup::buttonClicked, this, &ConfigModuleTab::slotEmitChanged);

    auto favoriteFoldersViewHiddenRadio = new QRadioButton(i18n("Never"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewHiddenRadio,
                                         static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewHiddenRadio);

    auto favoriteFoldersViewIconsRadio = new QRadioButton(i18n("As icons"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewIconsRadio,
                                         static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewIconsRadio);

    auto favoriteFoldersViewListRadio = new QRadioButton(i18n("As list"), mFavoriteFoldersViewGroupBox);
    mFavoriteFoldersViewGroup->addButton(favoriteFoldersViewListRadio,
                                         static_cast<int>(MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode));
    mFavoriteFoldersViewGroupBox->layout()->addWidget(favoriteFoldersViewListRadio);

    vlay->addWidget(mFavoriteFoldersViewGroupBox);

    // "folder tooltips" radio buttons:
    mFolderToolTipsGroupBox = new QGroupBox(this);
    mFolderToolTipsGroupBox->setTitle(i18n("Folder Tooltips"));
    mFolderToolTipsGroupBox->setLayout(new QVBoxLayout());
    mFolderToolTipsGroup = new QButtonGroup(this);
    connect(mFolderToolTipsGroup, &QButtonGroup::buttonClicked, this, &ConfigModuleTab::slotEmitChanged);

    auto folderToolTipsAlwaysRadio = new QRadioButton(i18n("Always"), mFolderToolTipsGroupBox);
    mFolderToolTipsGroup->addButton(folderToolTipsAlwaysRadio, static_cast<int>(FolderTreeWidget::DisplayAlways));
    mFolderToolTipsGroupBox->layout()->addWidget(folderToolTipsAlwaysRadio);

    auto folderToolTipsNeverRadio = new QRadioButton(i18n("Never"), mFolderToolTipsGroupBox);
    mFolderToolTipsGroup->addButton(folderToolTipsNeverRadio, static_cast<int>(FolderTreeWidget::DisplayNever));
    mFolderToolTipsGroupBox->layout()->addWidget(folderToolTipsNeverRadio);

    vlay->addWidget(mFolderToolTipsGroupBox);

    // "show reader window" radio buttons:
    populateButtonGroup(mReaderWindowModeGroupBox = new QGroupBox(this),
                        mReaderWindowModeGroup = new QButtonGroup(this),
                        Qt::Vertical,
                        KMailSettings::self()->readerWindowModeItem());
    vlay->addWidget(mReaderWindowModeGroupBox);

    connect(mReaderWindowModeGroup, &QButtonGroup::buttonClicked, this, &ConfigModuleTab::slotEmitChanged);

    vlay->addStretch(10); // spacer
}

void AppearancePageLayoutTab::doLoadOther()
{
    loadWidget(mFolderListGroupBox, mFolderListGroup, KMailSettings::self()->folderListItem());
    loadWidget(mReaderWindowModeGroupBox, mReaderWindowModeGroup, KMailSettings::self()->readerWindowModeItem());
    if (KMKernel::self()) {
        loadWidget(mFavoriteFoldersViewGroupBox, mFavoriteFoldersViewGroup, KMKernel::self()->mailCommonSettings()->favoriteCollectionViewModeItem());
    }
    loadWidget(mFolderQuickSearchCB, KMailSettings::self()->enableFolderQuickSearchItem());
    const int checkedFolderToolTipsPolicy = KMailSettings::self()->toolTipDisplayPolicy();
    if (checkedFolderToolTipsPolicy >= 0) {
        mFolderToolTipsGroup->button(checkedFolderToolTipsPolicy)->setChecked(true);
    }
}

void AppearancePageLayoutTab::save()
{
    saveButtonGroup(mFolderListGroup, KMailSettings::self()->folderListItem());
    saveButtonGroup(mReaderWindowModeGroup, KMailSettings::self()->readerWindowModeItem());
    if (KMKernel::self()) {
        saveButtonGroup(mFavoriteFoldersViewGroup, KMKernel::self()->mailCommonSettings()->favoriteCollectionViewModeItem());
    }
    saveCheckBox(mFolderQuickSearchCB, KMailSettings::self()->enableFolderQuickSearchItem());
    KMailSettings::self()->setToolTipDisplayPolicy(mFolderToolTipsGroup->checkedId());
}

//
// Appearance Message List
//

QString AppearancePageHeadersTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-headers");
}

static const struct {
    const KLazyLocalizedString displayName;
    DateFormatter::FormatType dateDisplay;
} dateDisplayConfig[] = {{kli18n("Sta&ndard format (%1)"), KMime::DateFormatter::CTime},
                         {kli18n("Locali&zed format (%1)"), KMime::DateFormatter::Localized},
                         {kli18n("Smart for&mat (%1)"), KMime::DateFormatter::Fancy},
                         {kli18n("C&ustom format:"), KMime::DateFormatter::Custom}};
static const int numDateDisplayConfig = sizeof dateDisplayConfig / sizeof *dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto formLayout = new QFormLayout(this);

    mDisplayMessageToolTips = new QCheckBox(MessageList::MessageListSettings::self()->messageToolTipEnabledItem()->label(), this);
    formLayout->addRow(QString(), mDisplayMessageToolTips);

    connect(mDisplayMessageToolTips, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    // "Aggregation"
    using MessageList::Utils::AggregationComboBox;
    mAggregationComboBox = new AggregationComboBox(this);

    using MessageList::Utils::AggregationConfigButton;
    auto aggregationConfigButton = new AggregationConfigButton(this, mAggregationComboBox);

    auto aggregationLayout = new QHBoxLayout();
    aggregationLayout->addWidget(mAggregationComboBox, 1);
    aggregationLayout->addWidget(aggregationConfigButton, 0);
    formLayout->addRow(i18n("Default aggregation:"), aggregationLayout);

    connect(aggregationConfigButton,
            &MessageList::Utils::AggregationConfigButton::configureDialogCompleted,
            this,
            &AppearancePageHeadersTab::slotSelectDefaultAggregation);
    connect(mAggregationComboBox, &MessageList::Utils::AggregationComboBox::activated, this, &ConfigModuleTab::slotEmitChanged);

    // "Theme"
    using MessageList::Utils::ThemeComboBox;
    mThemeComboBox = new ThemeComboBox(this);

    using MessageList::Utils::ThemeConfigButton;
    auto themeConfigButton = new ThemeConfigButton(this, mThemeComboBox);

    auto themeLayout = new QHBoxLayout();
    themeLayout->addWidget(mThemeComboBox, 1);
    themeLayout->addWidget(themeConfigButton, 0);
    formLayout->addRow(i18n("Default theme:"), themeLayout);

    connect(themeConfigButton, &MessageList::Utils::ThemeConfigButton::configureDialogCompleted, this, &AppearancePageHeadersTab::slotSelectDefaultTheme);
    connect(mThemeComboBox, &MessageList::Utils::ThemeComboBox::activated, this, &ConfigModuleTab::slotEmitChanged);

    // "Date Display" group:
    mDateDisplay = new QButtonGroup(this);
    mDateDisplay->setExclusive(true);

    for (int i = 0; i < numDateDisplayConfig; ++i) {
        const char *label = KLocalizedString(dateDisplayConfig[i].displayName).untranslatedText().constData();

        QString buttonLabel;
        if (QString::fromLatin1(label).contains(QLatin1String("%1"))) {
            buttonLabel = KLocalizedString(dateDisplayConfig[i].displayName)
                              .subs(DateFormatter::formatCurrentDate(dateDisplayConfig[i].dateDisplay))
                              .toString(); // i18n(label, DateFormatter::formatCurrentDate(dateDisplayConfig[i].dateDisplay));
        } else {
            buttonLabel = KLocalizedString(dateDisplayConfig[i].displayName).toString();
        }
        if (dateDisplayConfig[i].dateDisplay == DateFormatter::Custom) {
            auto hbox = new QWidget(this);
            auto hboxHBoxLayout = new QHBoxLayout(hbox);
            hboxHBoxLayout->setContentsMargins({});
            auto radio = new QRadioButton(buttonLabel, hbox);
            hboxHBoxLayout->addWidget(radio);
            mDateDisplay->addButton(radio, dateDisplayConfig[i].dateDisplay);

            mCustomDateFormatEdit = new QLineEdit(hbox);
            new KPIM::LineEditCatchReturnKey(mCustomDateFormatEdit, this);
            hboxHBoxLayout->addWidget(mCustomDateFormatEdit);
            mCustomDateFormatEdit->setEnabled(false);
            hboxHBoxLayout->setStretchFactor(mCustomDateFormatEdit, 1);

            connect(radio, &QAbstractButton::toggled, mCustomDateFormatEdit, &QWidget::setEnabled);
            connect(mCustomDateFormatEdit, &QLineEdit::textChanged, this, &ConfigModuleTab::slotEmitChanged);

            auto formatHelp = new QLabel(i18n("<qt><a href=\"whatsthis1\">Custom format information...</a></qt>"), hbox);
            formatHelp->setContextMenuPolicy(Qt::NoContextMenu);
            connect(formatHelp, &QLabel::linkActivated, this, &AppearancePageHeadersTab::slotLinkClicked);
            hboxHBoxLayout->addWidget(formatHelp);

            mCustomDateWhatsThis = i18n(
                "<qt><p><strong>These expressions may be used for the date:"
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
            formLayout->addRow(QString(), hbox);
        } else {
            auto radio = new QRadioButton(buttonLabel, mDateDisplayBox);
            if (i == 0) {
                formLayout->addRow(i18n("Date Display:"), radio);
            } else {
                formLayout->addRow(QString(), radio);
            }
            mDateDisplay->addButton(radio, dateDisplayConfig[i].dateDisplay);
        }
    } // end for loop populating mDateDisplay

    connect(mDateDisplay, &QButtonGroup::buttonClicked, this, &ConfigModuleTab::slotEmitChanged);
}

void AppearancePageHeadersTab::slotLinkClicked(const QString &link)
{
    if (link == QLatin1String("whatsthis1")) {
        QWhatsThis::showText(QCursor::pos(), mCustomDateWhatsThis);
    }
}

void AppearancePageHeadersTab::slotSelectDefaultAggregation()
{
    // Select current default aggregation.
    mAggregationComboBox->selectDefault();
}

void AppearancePageHeadersTab::slotSelectDefaultTheme()
{
    // Select current default theme.
    mThemeComboBox->selectDefault();
}

void AppearancePageHeadersTab::doLoadOther()
{
    // "General Options":
    loadWidget(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());

    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    // "Date Display":
    setDateDisplay(MessageCore::MessageCoreSettings::self()->dateFormat(), MessageCore::MessageCoreSettings::self()->customDateFormat());
}

void AppearancePageHeadersTab::doLoadFromGlobalSettings()
{
    loadWidget(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());
    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    setDateDisplay(MessageCore::MessageCoreSettings::self()->dateFormat(), MessageCore::MessageCoreSettings::self()->customDateFormat());
}

void AppearancePageHeadersTab::setDateDisplay(int num, const QString &format)
{
    auto dateDisplay = static_cast<DateFormatter::FormatType>(num);

    // special case: needs text for the line edit:
    if (dateDisplay == DateFormatter::Custom) {
        mCustomDateFormatEdit->setText(format);
    }

    for (int i = 0; i < numDateDisplayConfig; ++i) {
        if (dateDisplay == dateDisplayConfig[i].dateDisplay) {
            mDateDisplay->button(dateDisplay)->setChecked(true);
            return;
        }
    }
    // fell through since none found:
    mDateDisplay->button(numDateDisplayConfig - 2)->setChecked(true); // default
}

void AppearancePageHeadersTab::save()
{
    saveCheckBox(mDisplayMessageToolTips, MessageList::MessageListSettings::self()->messageToolTipEnabledItem());

    if (KMKernel::self()) {
        KMKernel::self()->savePaneSelection();
    }
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

QString AppearancePageGeneralTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-reader");
}

AppearancePageGeneralTab::AppearancePageGeneralTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    auto topLayout = new QVBoxLayout(this);

    auto readerBox = new QGroupBox(i18n("Message Window"), this);
    topLayout->addWidget(readerBox);

    auto readerBoxLayout = new QVBoxLayout(readerBox);

    // "Close message window after replying or forwarding" check box:
    populateCheckBox(mCloseAfterReplyOrForwardCheck = new QCheckBox(this), MessageViewer::MessageViewerSettings::self()->closeAfterReplyOrForwardItem());
    mCloseAfterReplyOrForwardCheck->setToolTip(i18n("Close the standalone message window after replying or forwarding the message"));
    readerBoxLayout->addWidget(mCloseAfterReplyOrForwardCheck);
    connect(mCloseAfterReplyOrForwardCheck, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    mViewerSettings = new MessageViewer::ConfigureWidget;
    connect(mViewerSettings, &MessageViewer::ConfigureWidget::settingsChanged, this, &ConfigModuleTab::slotEmitChanged);
    readerBoxLayout->addWidget(mViewerSettings);

    auto systrayBox = new QGroupBox(i18n("System Tray"), this);
    topLayout->addWidget(systrayBox);

    auto systrayBoxlayout = new QVBoxLayout(systrayBox);

    // "Enable system tray applet" check box
    mSystemTrayCheck = new QCheckBox(i18n("Enable system tray icon"), this);
    systrayBoxlayout->addWidget(mSystemTrayCheck);

    // "Enable start in system tray" check box
    mStartInTrayCheck = new QCheckBox(i18n("Start minimized to tray"));
    systrayBoxlayout->addWidget(mStartInTrayCheck);

    // Dependencies between the two checkboxes
    connect(mStartInTrayCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Checked) {
            mSystemTrayCheck->setCheckState(Qt::Checked);
        }
        slotEmitChanged();
    });
    connect(mSystemTrayCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Unchecked) {
            mStartInTrayCheck->setCheckState(Qt::Unchecked);
        }
        slotEmitChanged();
    });

    // "Enable system tray applet" check box
    mShowNumberInTaskBar = new QCheckBox(i18n("Show unread email in Taskbar"), this);
    systrayBoxlayout->addWidget(mShowNumberInTaskBar);
    connect(mShowNumberInTaskBar, &QCheckBox::stateChanged, this, &ConfigModuleTab::slotEmitChanged);

    topLayout->addStretch(100); // spacer
}

void AppearancePageGeneralTab::doResetToDefaultsOther()
{
}

void AppearancePageGeneralTab::doLoadOther()
{
    loadWidget(mSystemTrayCheck, KMailSettings::self()->systemTrayEnabledItem());
    loadWidget(mStartInTrayCheck, KMailSettings::self()->startInTrayItem());
    loadWidget(mShowNumberInTaskBar, KMailSettings::self()->showUnreadInTaskbarItem());
    loadWidget(mCloseAfterReplyOrForwardCheck, MessageViewer::MessageViewerSettings::self()->closeAfterReplyOrForwardItem());
    mViewerSettings->readConfig();
}

void AppearancePageGeneralTab::save()
{
    saveCheckBox(mSystemTrayCheck, KMailSettings::self()->systemTrayEnabledItem());
    saveCheckBox(mStartInTrayCheck, KMailSettings::self()->startInTrayItem());
    saveCheckBox(mShowNumberInTaskBar, KMailSettings::self()->showUnreadInTaskbarItem());
    KMailSettings::self()->save();
    saveCheckBox(mCloseAfterReplyOrForwardCheck, MessageViewer::MessageViewerSettings::self()->closeAfterReplyOrForwardItem());
    mViewerSettings->writeConfig();
}

QString AppearancePageMessageTagTab::helpAnchor() const
{
    return QStringLiteral("configure-appearance-messagetag");
}

TagListWidgetItem::TagListWidgetItem(QListWidget *parent)
    : QListWidgetItem(parent)
    , mTag(nullptr)
{
}

TagListWidgetItem::TagListWidgetItem(const QIcon &icon, const QString &text, QListWidget *parent)
    : QListWidgetItem(icon, text, parent)
    , mTag(nullptr)
{
}

TagListWidgetItem::~TagListWidgetItem() = default;

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
    auto maingrid = new QHBoxLayout(this);

    // Lefthand side Listbox and friends

    // Groupbox frame
    mTagsGroupBox = new QGroupBox(i18n("A&vailable Tags"), this);
    maingrid->addWidget(mTagsGroupBox);
    auto tageditgrid = new QVBoxLayout(mTagsGroupBox);

    // Listbox, add, remove row
    auto addremovegrid = new QHBoxLayout();
    tageditgrid->addLayout(addremovegrid);

    mTagAddLineEdit = new QLineEdit(mTagsGroupBox);
    new KPIM::LineEditCatchReturnKey(mTagAddLineEdit, this);
    addremovegrid->addWidget(mTagAddLineEdit);

    mTagAddButton = new QPushButton(mTagsGroupBox);
    mTagAddButton->setToolTip(i18n("Add new tag"));
    mTagAddButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addremovegrid->addWidget(mTagAddButton);

    mTagRemoveButton = new QPushButton(mTagsGroupBox);
    mTagRemoveButton->setToolTip(i18n("Remove selected tag"));
    mTagRemoveButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    addremovegrid->addWidget(mTagRemoveButton);

    // Up and down buttons
    auto updowngrid = new QHBoxLayout();
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

    // Listbox for tag names
    auto listboxgrid = new QHBoxLayout();
    tageditgrid->addLayout(listboxgrid);
    mTagListBox = new QListWidget(mTagsGroupBox);
    mTagListBox->setDragDropMode(QAbstractItemView::InternalMove);
    connect(mTagListBox->model(), &QAbstractItemModel::rowsMoved, this, &AppearancePageMessageTagTab::slotRowsMoved);
    connect(mTagListBox, &QListWidget::customContextMenuRequested, this, &AppearancePageMessageTagTab::slotCustomMenuRequested);

    mTagListBox->setMinimumWidth(150);
    listboxgrid->addWidget(mTagListBox);
    mTagListBox->setContextMenuPolicy(Qt::CustomContextMenu);

    // RHS for individual tag settings

    // Extra VBoxLayout for stretchers around settings
    auto tagsettinggrid = new QVBoxLayout();
    maingrid->addLayout(tagsettinggrid);

    // Groupbox frame
    mTagSettingGroupBox = new QGroupBox(i18n("Ta&g Settings"), this);
    tagsettinggrid->addWidget(mTagSettingGroupBox);
    QList<KActionCollection *> actionCollections;
    if (kmkernel->getKMMainWidget()) {
        actionCollections = kmkernel->getKMMainWidget()->actionCollections();
    }

    auto lay = new QHBoxLayout(mTagSettingGroupBox);
    mTagWidget = new MailCommon::TagWidget(actionCollections, this);
    lay->addWidget(mTagWidget);

    connect(mTagWidget, &TagWidget::changed, this, &AppearancePageMessageTagTab::slotEmitChangeCheck);

    // For enabling the add button in case box is non-empty
    connect(mTagAddLineEdit, &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotAddLineTextChanged);

    // For on-the-fly updating of tag name in editbox
    connect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotNameLineTextChanged);

    connect(mTagWidget, &TagWidget::iconNameChanged, this, &AppearancePageMessageTagTab::slotIconNameChanged);

    connect(mTagAddLineEdit, &QLineEdit::returnPressed, this, &AppearancePageMessageTagTab::slotAddNewTag);

    connect(mTagAddButton, &QAbstractButton::clicked, this, &AppearancePageMessageTagTab::slotAddNewTag);

    connect(mTagRemoveButton, &QAbstractButton::clicked, this, &AppearancePageMessageTagTab::slotRemoveTag);

    connect(mTagUpButton, &QAbstractButton::clicked, this, &AppearancePageMessageTagTab::slotMoveTagUp);

    connect(mTagDownButton, &QAbstractButton::clicked, this, &AppearancePageMessageTagTab::slotMoveTagDown);

    connect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);
    // Adjust widths for columns
    maingrid->setStretchFactor(mTagsGroupBox, 1);
    maingrid->setStretchFactor(lay, 1);
    tagsettinggrid->addStretch(10);
}

AppearancePageMessageTagTab::~AppearancePageMessageTagTab() = default;

void AppearancePageMessageTagTab::slotEmitChangeCheck()
{
    slotEmitChanged();
}

void AppearancePageMessageTagTab::slotCustomMenuRequested(const QPoint &)
{
    const int currentIndex = mTagListBox->currentRow();
    if (currentIndex >= 0) {
        QMenu menu(this);
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"), this, &AppearancePageMessageTagTab::slotRemoveTag);
        menu.exec(QCursor::pos());
    }
}

void AppearancePageMessageTagTab::slotRowsMoved(const QModelIndex &, int sourcestart, int sourceEnd, const QModelIndex &, int destinationRow)
{
    Q_UNUSED(sourceEnd)
    Q_UNUSED(sourcestart)
    Q_UNUSED(destinationRow)
    updateButtons();
    slotEmitChangeCheck();
}

void AppearancePageMessageTagTab::updateButtons()
{
    const int currentIndex = mTagListBox->currentRow();

    const bool theFirst = (currentIndex == 0);
    const bool theLast = (currentIndex >= (int)mTagListBox->count() - 1);
    const bool aFilterIsSelected = (currentIndex >= 0);

    mTagUpButton->setEnabled(aFilterIsSelected && !theFirst);
    mTagDownButton->setEnabled(aFilterIsSelected && !theLast);
}

void AppearancePageMessageTagTab::slotMoveTagUp()
{
    const int tmp_index = mTagListBox->currentRow();
    if (tmp_index <= 0) {
        return;
    }
    swapTagsInListBox(tmp_index, tmp_index - 1);
    updateButtons();
}

void AppearancePageMessageTagTab::slotMoveTagDown()
{
    const int tmp_index = mTagListBox->currentRow();
    if ((tmp_index < 0) || (tmp_index >= int(mTagListBox->count()) - 1)) {
        return;
    }
    swapTagsInListBox(tmp_index, tmp_index + 1);
    updateButtons();
}

void AppearancePageMessageTagTab::swapTagsInListBox(const int first, const int second)
{
    disconnect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);
    QListWidgetItem *item = mTagListBox->takeItem(first);
    // now selected item is at idx(idx-1), so
    // insert the other item at idx, ie. above(below).
    mPreviousTag = second;
    mTagListBox->insertItem(second, item);
    mTagListBox->setCurrentRow(second);
    connect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);
    slotEmitChangeCheck();
}

void AppearancePageMessageTagTab::slotRecordTagSettings(int aIndex)
{
    if ((aIndex < 0) || (aIndex >= int(mTagListBox->count()))) {
        return;
    }
    QListWidgetItem *item = mTagListBox->item(aIndex);
    auto tagItem = static_cast<TagListWidgetItem *>(item);

    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    tmp_desc->tagName = tagItem->text();
    mTagWidget->recordTagSettings(tmp_desc);
}

void AppearancePageMessageTagTab::slotUpdateTagSettingWidgets(int aIndex)
{
    // Check if selection is valid
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
    auto tagItem = static_cast<TagListWidgetItem *>(item);
    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    disconnect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotNameLineTextChanged);

    mTagWidget->tagNameLineEdit()->setEnabled(!tmp_desc->isImmutable);
    mTagWidget->tagNameLineEdit()->setText(tmp_desc->tagName);
    connect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotNameLineTextChanged);

    mTagWidget->setTagTextColor(tmp_desc->textColor);

    mTagWidget->setTagBackgroundColor(tmp_desc->backgroundColor);

    mTagWidget->setTagTextFormat(tmp_desc->isBold, tmp_desc->isItalic);

    mTagWidget->iconButton()->setEnabled(!tmp_desc->isImmutable);
    mTagWidget->iconButton()->setIcon(tmp_desc->iconName);

    mTagWidget->keySequenceWidget()->setEnabled(true);
    mTagWidget->keySequenceWidget()->setKeySequence(tmp_desc->shortcut, KKeySequenceWidget::NoValidate);

    mTagWidget->inToolBarCheck()->setEnabled(true);
    mTagWidget->inToolBarCheck()->setChecked(tmp_desc->inToolbar);
}

void AppearancePageMessageTagTab::slotSelectionChanged()
{
    mEmitChanges = false;
    slotRecordTagSettings(mPreviousTag);
    slotUpdateTagSettingWidgets(mTagListBox->currentRow());
    mPreviousTag = mTagListBox->currentRow();
    mEmitChanges = true;
}

void AppearancePageMessageTagTab::slotRemoveTag()
{
    const int tmp_index = mTagListBox->currentRow();
    if (tmp_index >= 0) {
        if (KMessageBox::ButtonCode::PrimaryAction
            == KMessageBox::questionTwoActions(this,
                                               i18n("Do you want to remove tag \'%1\'?", mTagListBox->item(mTagListBox->currentRow())->text()),
                                               i18nc("@title:window", "Remove Tag"),
                                               KStandardGuiItem::remove(),
                                               KStandardGuiItem::cancel())) {
            QListWidgetItem *item = mTagListBox->takeItem(mTagListBox->currentRow());
            auto tagItem = static_cast<TagListWidgetItem *>(item);
            MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();
            if (tmp_desc->tag().isValid()) {
                new Akonadi::TagDeleteJob(tmp_desc->tag());
            } else {
                qCWarning(KMAIL_LOG) << "Can't remove tag with invalid akonadi tag";
            }
            mPreviousTag = -1;

            // Before deleting the current item, make sure the selectionChanged signal
            // is disconnected, so that the widgets will not get updated while the
            // deletion takes place.
            disconnect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);

            delete item;
            connect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);

            slotSelectionChanged();
            slotEmitChangeCheck();
        }
    }
}

void AppearancePageMessageTagTab::slotDeleteTagJob(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "Failed to delete tag " << job->errorString();
    }
}

void AppearancePageMessageTagTab::slotNameLineTextChanged(const QString &aText)
{
    // If deleted all, leave the first character for the sake of not having an
    // empty tag name
    if (aText.isEmpty() && !mTagListBox->currentItem()) {
        return;
    }

    const int count = mTagListBox->count();
    for (int i = 0; i < count; ++i) {
        if (mTagListBox->item(i)->text() == aText) {
            KMessageBox::error(this, i18n("We cannot create tag. A tag with same name already exists."));
            disconnect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotNameLineTextChanged);
            mTagWidget->tagNameLineEdit()->setText(mTagListBox->currentItem()->text());
            connect(mTagWidget->tagNameLineEdit(), &QLineEdit::textChanged, this, &AppearancePageMessageTagTab::slotNameLineTextChanged);
            return;
        }
    }

    // Disconnect so the tag information is not saved and reloaded with every
    // letter
    disconnect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);

    mTagListBox->currentItem()->setText(aText);
    connect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);
}

void AppearancePageMessageTagTab::slotIconNameChanged(const QString &iconName)
{
    mTagListBox->currentItem()->setIcon(QIcon::fromTheme(iconName));
}

void AppearancePageMessageTagTab::slotAddLineTextChanged(const QString &aText)
{
    mTagAddButton->setEnabled(!aText.trimmed().isEmpty());
}

void AppearancePageMessageTagTab::slotAddNewTag()
{
    const QString newTagName = mTagAddLineEdit->text().trimmed();
    if (newTagName.isEmpty()) {
        return;
    }
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
    auto newItem = new TagListWidgetItem(QIcon::fromTheme(tag->iconName), newTagName, mTagListBox);
    newItem->setKMailTag(tag);
    mTagListBox->addItem(newItem);
    mTagListBox->setCurrentItem(newItem);
    mTagAddLineEdit->clear();
}

void AppearancePageMessageTagTab::doLoadFromGlobalSettings()
{
    mTagListBox->clear();

    auto fetchJob = new Akonadi::TagFetchJob(this);
    fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(fetchJob, &KJob::result, this, &AppearancePageMessageTagTab::slotTagsFetched);
}

void AppearancePageMessageTagTab::slotTagsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "Failed to load tags " << job->errorString();
        return;
    }
    auto fetchJob = static_cast<Akonadi::TagFetchJob *>(job);

    QList<MailCommon::TagPtr> msgTagList;
    const Akonadi::Tag::List tagList = fetchJob->tags();
    msgTagList.reserve(tagList.count());
    for (const Akonadi::Tag &akonadiTag : tagList) {
        MailCommon::Tag::Ptr tag = MailCommon::Tag::fromAkonadi(akonadiTag);
        msgTagList.append(tag);
    }

    std::sort(msgTagList.begin(), msgTagList.end(), MailCommon::Tag::compare);

    for (const MailCommon::Tag::Ptr &tag : std::as_const(msgTagList)) {
        auto newItem = new TagListWidgetItem(QIcon::fromTheme(tag->iconName), tag->tagName, mTagListBox);
        newItem->setKMailTag(tag);
        if (tag->priority == -1) {
            tag->priority = mTagListBox->count() - 1;
        }
    }

    // Disconnect so that insertItem's do not trigger an update procedure
    disconnect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);

    connect(mTagListBox, &QListWidget::currentItemChanged, this, &AppearancePageMessageTagTab::slotSelectionChanged);

    slotUpdateTagSettingWidgets(-1);
    // Needed since the previous function doesn't affect add button
    mTagAddButton->setEnabled(false);

    // Save the original list
    mOriginalMsgTagList.clear();
    for (const MailCommon::TagPtr &tag : std::as_const(msgTagList)) {
        mOriginalMsgTagList.append(MailCommon::TagPtr(new MailCommon::Tag(*tag)));
    }
}

void AppearancePageMessageTagTab::save()
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
        auto tagItem = static_cast<TagListWidgetItem *>(mTagListBox->item(i));
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
