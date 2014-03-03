/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "configuredialog/colorlistbox.h"
#include "messagelist/utils/aggregationcombobox.h"
#include "messagelist/utils/aggregationconfigbutton.h"
#include "messagelist/utils/themecombobox.h"
#include "messagelist/utils/themeconfigbutton.h"
#include "mailcommon/tag/tagwidget.h"
#include "mailcommon/tag/tag.h"
#include "kmkernel.h"
#include "util.h"
#include "foldertreewidget.h"

#include "kmmainwidget.h"

#include "foldertreewidget.h"

#include "mailcommon/mailcommonsettings_base.h"

#include "messageviewer/widgets/configurewidget.h"
#include "messageviewer/settings/globalsettings.h"

#include "messagelist/core/settings.h"
#include "messagelist/messagelistutil.h"
#include "messagecore/settings/globalsettings.h"
#include "settings/globalsettings.h"

#include "mailcommon/util/mailutil.h"

#include <Akonadi/Tag>
#include <Akonadi/TagFetchJob>
#include <Akonadi/TagFetchScope>
#include <Akonadi/TagDeleteJob>
#include <Akonadi/TagCreateJob>
#include <Akonadi/TagAttribute>
#include <Akonadi/TagModifyJob>

#include <KIconButton>
#include <KButtonGroup>
#include <KLocalizedString>
#include <KColorScheme>
#include <KSeparator>
#include <KFontChooser>
#include <KHBox>
#include <KMessageBox>
#include <KKeySequenceWidget>
#include <KLineEdit>
#include <KDialog>

#include <kmime/kmime_dateformatter.h>
using KMime::DateFormatter;

#include <QWhatsThis>
#include <QButtonGroup>
#include <QSpinBox>
#include <QLabel>
using namespace MailCommon;

QString AppearancePage::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance");
}

AppearancePage::AppearancePage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "Fonts" tab:
    //
    mFontsTab = new FontsTab();
    addTab( mFontsTab, i18n("Fonts") );

    //
    // "Colors" tab:
    //
    mColorsTab = new ColorsTab();
    addTab( mColorsTab, i18n("Colors") );

    //
    // "Layout" tab:
    //
    mLayoutTab = new LayoutTab();
    addTab( mLayoutTab, i18n("Layout") );

    //
    // "Headers" tab:
    //
    mHeadersTab = new HeadersTab();
    addTab( mHeadersTab, i18n("Message List") );

    //
    // "Reader window" tab:
    //
    mReaderTab = new ReaderTab();
    addTab( mReaderTab, i18n("Message Window") );
    addConfig( MessageViewer::GlobalSettings::self(), mReaderTab );

    //
    // "System Tray" tab:
    //
    mSystemTrayTab = new SystemTrayTab();
    addTab( mSystemTrayTab, i18n("System Tray") );

    //
    // "Message Tag" tab:
    //
    mMessageTagTab = new MessageTagTab();
    addTab( mMessageTagTab, i18n("Message Tags") );
}


QString AppearancePage::FontsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-fonts");
}

static const struct {
    const char * configName;
    const char * displayName;
    bool   enableFamilyAndSize;
    bool   onlyFixed;
} fontNames[] = {
    { "body-font", I18N_NOOP("Message Body"), true, false },
    { "MessageListFont", I18N_NOOP("Message List"), true, false },
    { "UnreadMessageFont", I18N_NOOP("Message List - Unread Messages"), true, false },
    { "ImportantMessageFont", I18N_NOOP("Message List - Important Messages"), true, false },
    { "TodoMessageFont", I18N_NOOP("Message List - Action Item Messages"), true, false },
    { "folder-font", I18N_NOOP("Folder List"), true, false },
    { "quote1-font", I18N_NOOP("Quoted Text - First Level"), false, false },
    { "quote2-font", I18N_NOOP("Quoted Text - Second Level"), false, false },
    { "quote3-font", I18N_NOOP("Quoted Text - Third Level"), false, false },
    { "fixed-font", I18N_NOOP("Fixed Width Font"), true, true },
    { "composer-font", I18N_NOOP("Composer"), true, false },
    { "print-font",  I18N_NOOP("Printing Output"), true, false },
};
static const int numFontNames = sizeof fontNames / sizeof *fontNames;

AppearancePageFontsTab::AppearancePageFontsTab( QWidget * parent )
    : ConfigModuleTab( parent ), mActiveFontIndex( -1 )
{
    assert( numFontNames == sizeof mFont / sizeof *mFont );

    // "Use custom fonts" checkbox, followed by <hr>
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    mCustomFontCheck = new QCheckBox( i18n("&Use custom fonts"), this );
    vlay->addWidget( mCustomFontCheck );
    vlay->addWidget( new KSeparator( Qt::Horizontal, this ) );
    connect ( mCustomFontCheck, SIGNAL(stateChanged(int)),
              this, SLOT(slotEmitChanged()) );

    // "font location" combo box and label:
    QHBoxLayout *hlay = new QHBoxLayout(); // inherites spacing
    vlay->addLayout( hlay );
    mFontLocationCombo = new KComboBox( this );
    mFontLocationCombo->setEditable( false );
    mFontLocationCombo->setEnabled( false ); // !mCustomFontCheck->isChecked()

    QStringList fontDescriptions;
    for ( int i = 0 ; i < numFontNames ; ++i )
        fontDescriptions << i18n( fontNames[i].displayName );
    mFontLocationCombo->addItems( fontDescriptions );

    QLabel *label = new QLabel( i18n("Apply &to:"), this );
    label->setBuddy( mFontLocationCombo );
    label->setEnabled( false ); // since !mCustomFontCheck->isChecked()
    hlay->addWidget( label );

    hlay->addWidget( mFontLocationCombo );
    hlay->addStretch( 10 );
    vlay->addSpacing( KDialog::spacingHint() );
    mFontChooser = new KFontChooser( this, KFontChooser::DisplayFrame,
                                     QStringList(), 4 );
    mFontChooser->setEnabled( false ); // since !mCustomFontCheck->isChecked()
    vlay->addWidget( mFontChooser );
    connect ( mFontChooser, SIGNAL(fontSelected(QFont)),
              this, SLOT(slotEmitChanged()) );


    // {en,dis}able widgets depending on the state of mCustomFontCheck:
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             mFontLocationCombo, SLOT(setEnabled(bool)) );
    connect( mCustomFontCheck, SIGNAL(toggled(bool)),
             mFontChooser, SLOT(setEnabled(bool)) );
    // load the right font settings into mFontChooser:
    connect( mFontLocationCombo, SIGNAL(activated(int)),
             this, SLOT(slotFontSelectorChanged(int)) );
}


void AppearancePage::FontsTab::slotFontSelectorChanged( int index )
{
    kDebug() << "slotFontSelectorChanged() called";
    if( index < 0 || index >= mFontLocationCombo->count() )
        return; // Should never happen, but it is better to check.

    // Save current fontselector setting before we install the new:
    if( mActiveFontIndex == 0 ) {
        mFont[0] = mFontChooser->font();
        // hardcode the family and size of "message body" dependant fonts:
        for ( int i = 0 ; i < numFontNames ; ++i )
            if ( !fontNames[i].enableFamilyAndSize ) {
                // ### shall we copy the font and set the save and re-set
                // {regular,italic,bold,bold italic} property or should we
                // copy only family and pointSize?
                mFont[i].setFamily( mFont[0].family() );
                mFont[i].setPointSize/*Float?*/( mFont[0].pointSize/*Float?*/() );
            }
    } else if ( mActiveFontIndex > 0 )
        mFont[ mActiveFontIndex ] = mFontChooser->font();
    mActiveFontIndex = index;

    // Disonnect so the "Apply" button is not activated by the change
    disconnect ( mFontChooser, SIGNAL(fontSelected(QFont)),
                 this, SLOT(slotEmitChanged()) );

    // Display the new setting:
    mFontChooser->setFont( mFont[index], fontNames[index].onlyFixed );

    connect ( mFontChooser, SIGNAL(fontSelected(QFont)),
              this, SLOT(slotEmitChanged()) );

    // Disable Family and Size list if we have selected a quote font:
    mFontChooser->enableColumn( KFontChooser::FamilyList|KFontChooser::SizeList,
                                fontNames[ index ].enableFamilyAndSize );
}

void AppearancePage::FontsTab::doLoadOther()
{
    KConfigGroup fonts( KMKernel::self()->config(), "Fonts" );
    KConfigGroup messagelistFont( KMKernel::self()->config(), "MessageListView::Fonts" );

    mFont[0] = KGlobalSettings::generalFont();
    QFont fixedFont = KGlobalSettings::fixedFont();

    for ( int i = 0 ; i < numFontNames ; ++i ) {
        const QString configName = QLatin1String(fontNames[i].configName);
        if ( configName == QLatin1String( "MessageListFont" ) ||
             configName == QLatin1String( "UnreadMessageFont" ) ||
             configName == QLatin1String( "ImportantMessageFont" ) ||
             configName == QLatin1String( "TodoMessageFont" ) ) {
            mFont[i] = messagelistFont.readEntry( configName,
                                                  (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
        } else {
            mFont[i] = fonts.readEntry( configName,
                                        (fontNames[i].onlyFixed) ? fixedFont : mFont[0] );
        }
    }
    mCustomFontCheck->setChecked( !MessageCore::GlobalSettings::self()->useDefaultFonts() );
    mFontLocationCombo->setCurrentIndex( 0 );
    slotFontSelectorChanged( 0 );
}

void AppearancePage::FontsTab::save()
{
    KConfigGroup fonts( KMKernel::self()->config(), "Fonts" );
    KConfigGroup messagelistFont( KMKernel::self()->config(), "MessageListView::Fonts" );

    // read the current font (might have been modified)
    if ( mActiveFontIndex >= 0 )
        mFont[ mActiveFontIndex ] = mFontChooser->font();

    const bool customFonts = mCustomFontCheck->isChecked();
    MessageCore::GlobalSettings::self()->setUseDefaultFonts( !customFonts );

    for ( int i = 0 ; i < numFontNames ; ++i ) {
        const QString configName = QLatin1String(fontNames[i].configName);
        if ( configName == QLatin1String( "MessageListFont" ) ||
             configName == QLatin1String( "UnreadMessageFont" ) ||
             configName == QLatin1String( "ImportantMessageFont" ) ||
             configName == QLatin1String( "TodoMessageFont" ) ) {
            if ( customFonts || messagelistFont.hasKey( configName ) ) {
                // Don't write font info when we use default fonts, but write
                // if it's already there:
                messagelistFont.writeEntry( configName, mFont[i] );
            }
        } else {
            if ( customFonts || fonts.hasKey( configName ) )
                // Don't write font info when we use default fonts, but write
                // if it's already there:
                fonts.writeEntry( configName, mFont[i] );
        }
    }
}

void AppearancePage::FontsTab::doResetToDefaultsOther()
{
    mCustomFontCheck->setChecked( false );
}


QString AppearancePage::ColorsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-colors");
}

static const struct {
    const char * configName;
    const char * displayName;
} colorNames[] = { // adjust doLoadOther if you change this:
                   { "QuotedText1", I18N_NOOP("Quoted Text - First Level") },
                   { "QuotedText2", I18N_NOOP("Quoted Text - Second Level") },
                   { "QuotedText3", I18N_NOOP("Quoted Text - Third Level") },
                   { "LinkColor", I18N_NOOP("Link") },
                   { "FollowedColor", I18N_NOOP("Followed Link") },
                   { "MisspelledColor", I18N_NOOP("Misspelled Words") },
                   { "UnreadMessageColor", I18N_NOOP("Unread Message") },
                   { "ImportantMessageColor", I18N_NOOP("Important Message") },
                   { "TodoMessageColor", I18N_NOOP("Action Item Message") },
                   { "PGPMessageEncr", I18N_NOOP("OpenPGP Message - Encrypted") },
                   { "PGPMessageOkKeyOk", I18N_NOOP("OpenPGP Message - Valid Signature with Trusted Key") },
                   { "PGPMessageOkKeyBad", I18N_NOOP("OpenPGP Message - Valid Signature with Untrusted Key") },
                   { "PGPMessageWarn", I18N_NOOP("OpenPGP Message - Unchecked Signature") },
                   { "PGPMessageErr", I18N_NOOP("OpenPGP Message - Bad Signature") },
                   { "HTMLWarningColor", I18N_NOOP("Border Around Warning Prepending HTML Messages") },
                   { "CloseToQuotaColor", I18N_NOOP("Folder Name and Size When Close to Quota") },
                   { "ColorbarBackgroundPlain", I18N_NOOP("HTML Status Bar Background - No HTML Message") },
                   { "ColorbarForegroundPlain", I18N_NOOP("HTML Status Bar Foreground - No HTML Message") },
                   { "ColorbarBackgroundHTML",  I18N_NOOP("HTML Status Bar Background - HTML Message") },
                   { "ColorbarForegroundHTML",  I18N_NOOP("HTML Status Bar Foreground - HTML Message") },
                   { "BrokenAccountColor",  I18N_NOOP("Broken Account - Folder Text Color") },
                   { "BackgroundColor",  I18N_NOOP("Background Color") },
                 };
static const int numColorNames = sizeof colorNames / sizeof *colorNames;

AppearancePageColorsTab::AppearancePageColorsTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // "use custom colors" check box
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );
    mCustomColorCheck = new QCheckBox( i18n("&Use custom colors"), this );
    vlay->addWidget( mCustomColorCheck );
    connect( mCustomColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // color list box:
    mColorList = new ColorListBox( this );
    mColorList->setEnabled( false ); // since !mCustomColorCheck->isChecked()
    for ( int i = 0 ; i < numColorNames ; ++i )
        mColorList->addColor( i18n(colorNames[i].displayName) );
    vlay->addWidget( mColorList, 1 );

    // "recycle colors" check box:
    mRecycleColorCheck =
            new QCheckBox( i18n("Recycle colors on deep &quoting"), this );
    mRecycleColorCheck->setEnabled( false );
    vlay->addWidget( mRecycleColorCheck );
    connect( mRecycleColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // close to quota threshold
    QHBoxLayout *hbox = new QHBoxLayout();
    vlay->addLayout( hbox );
    QLabel *l = new QLabel( i18n("Close to quota threshold:"), this );
    hbox->addWidget( l );
    mCloseToQuotaThreshold = new QSpinBox( this );
    mCloseToQuotaThreshold->setRange( 0, 100 );
    mCloseToQuotaThreshold->setSingleStep( 1 );
    connect( mCloseToQuotaThreshold, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    mCloseToQuotaThreshold->setSuffix( i18n("%"));

    hbox->addWidget( mCloseToQuotaThreshold );
    hbox->addWidget( new QWidget(this), 2 );

    // {en,dir}able widgets depending on the state of mCustomColorCheck:
    connect( mCustomColorCheck, SIGNAL(toggled(bool)),
             mColorList, SLOT(setEnabled(bool)) );
    connect( mCustomColorCheck, SIGNAL(toggled(bool)),
             mRecycleColorCheck, SLOT(setEnabled(bool)) );
    connect( mCustomColorCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mColorList, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
}

void AppearancePage::ColorsTab::doLoadOther()
{
    mCustomColorCheck->setChecked( !MessageCore::GlobalSettings::self()->useDefaultColors() );
    mRecycleColorCheck->setChecked( MessageViewer::GlobalSettings::self()->recycleQuoteColors() );
    mCloseToQuotaThreshold->setValue( GlobalSettings::self()->closeToQuotaThreshold() );
    loadColor( true );
}

void AppearancePage::ColorsTab::loadColor( bool loadFromConfig )
{
    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    KConfigGroup reader( KMKernel::self()->config(), "Reader" );

    KConfigGroup messageListView( KMKernel::self()->config(), "MessageListView::Colors" );

    KConfigGroup collectionFolderView( KMKernel::self()->config(), "CollectionFolderView" );

    static const QColor defaultColor[ numColorNames ] = {
        KMail::Util::quoteL1Color(),
        KMail::Util::quoteL2Color(),
        KMail::Util::quoteL3Color(),
        scheme.foreground( KColorScheme::LinkText ).color(), // link
        scheme.foreground( KColorScheme::VisitedText ).color(),// visited link
        scheme.foreground( KColorScheme::NegativeText ).color(), // misspelled words
        MessageList::Util::unreadDefaultMessageColor(), // unread mgs
        MessageList::Util::importantDefaultMessageColor(), // important msg
        MessageList::Util::todoDefaultMessageColor(), // action item mgs
        QColor( 0x00, 0x80, 0xFF ), // pgp encrypted
        scheme.background( KColorScheme::PositiveBackground ).color(), // pgp ok, trusted key
        QColor( 0xFF, 0xFF, 0x40 ), // pgp ok, untrusted key
        QColor( 0xFF, 0xFF, 0x40 ), // pgp unchk
        Qt::red, // pgp bad
        QColor( 0xFF, 0x40, 0x40 ), // warning text color
        MailCommon::Util::defaultQuotaColor(), // close to quota
        Qt::lightGray, // colorbar plain bg
        Qt::black,     // colorbar plain fg
        Qt::black,     // colorbar html  bg
        Qt::white,     // colorbar html  fg
        scheme.foreground(KColorScheme::NegativeText).color(),  //Broken Account Color
        scheme.background().color() // reader background color
    };

    for ( int i = 0 ; i < numColorNames ; ++i ) {
        if ( loadFromConfig ) {
            const QString configName = QLatin1String(colorNames[i].configName);
            if ( configName == QLatin1String( "UnreadMessageColor" ) ||
                 configName == QLatin1String( "ImportantMessageColor" ) ||
                 configName == QLatin1String( "TodoMessageColor" ) ) {
                mColorList->setColorSilently( i, messageListView.readEntry( configName, defaultColor[i] ) );
            } else if( configName == QLatin1String("BrokenAccountColor")) {
                mColorList->setColorSilently( i, collectionFolderView.readEntry(configName,defaultColor[i]));
            } else {
                mColorList->setColorSilently( i, reader.readEntry( configName, defaultColor[i] ) );
            }
        } else {
            mColorList->setColorSilently( i, defaultColor[i] );
        }
    }
}

void AppearancePage::ColorsTab::doResetToDefaultsOther()
{
    mCustomColorCheck->setChecked( false );
    mRecycleColorCheck->setChecked( false );
    mCloseToQuotaThreshold->setValue( 80 );
    loadColor( false );
}

void AppearancePage::ColorsTab::save()
{
    KConfigGroup reader( KMKernel::self()->config(), "Reader" );
    KConfigGroup messageListView( KMKernel::self()->config(), "MessageListView::Colors" );
    KConfigGroup collectionFolderView( KMKernel::self()->config(), "CollectionFolderView" );
    bool customColors = mCustomColorCheck->isChecked();
    MessageCore::GlobalSettings::self()->setUseDefaultColors( !customColors );

    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    for ( int i = 0 ; i < numColorNames ; ++i ) {
        // Don't write color info when we use default colors, but write
        // if it's already there:
        const QString configName = QLatin1String(colorNames[i].configName);
        if ( configName == QLatin1String( "UnreadMessageColor" ) ||
             configName == QLatin1String( "ImportantMessageColor" ) ||
             configName == QLatin1String( "TodoMessageColor" ) ) {
            if ( customColors || messageListView.hasKey( configName ) )
                messageListView.writeEntry( configName, mColorList->color(i) );

        } else if( configName == QLatin1String("BrokenAccountColor")) {
            if ( customColors || collectionFolderView.hasKey( configName ) )
                collectionFolderView.writeEntry(configName,mColorList->color(i));
        } else if (configName == QLatin1String("BackgroundColor")) {
            if (customColors && (mColorList->color(i) != scheme.background().color() ))
                reader.writeEntry(configName, mColorList->color(i));
        } else {
            if ( customColors || reader.hasKey( configName ) )
                reader.writeEntry( configName, mColorList->color(i) );
        }
    }
    MessageViewer::GlobalSettings::self()->setRecycleQuoteColors( mRecycleColorCheck->isChecked() );
    GlobalSettings::self()->setCloseToQuotaThreshold( mCloseToQuotaThreshold->value() );
}

QString AppearancePage::LayoutTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-layout");
}

AppearancePageLayoutTab::AppearancePageLayoutTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "folder list" radio buttons:
    populateButtonGroup( mFolderListGroupBox = new QGroupBox( this ),
                         mFolderListGroup = new QButtonGroup( this ),
                         Qt::Horizontal, GlobalSettings::self()->folderListItem() );
    vlay->addWidget( mFolderListGroupBox );
    connect( mFolderListGroup, SIGNAL (buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QHBoxLayout* folderCBHLayout = new QHBoxLayout();
    mFolderQuickSearchCB = new QCheckBox( i18n("Show folder quick search field"), this );
    connect( mFolderQuickSearchCB, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()) );
    folderCBHLayout->addWidget( mFolderQuickSearchCB );
    vlay->addLayout( folderCBHLayout );
    vlay->addSpacing( KDialog::spacingHint() );   // space before next box

    // "favorite folders view mode" radio buttons:
    mFavoriteFoldersViewGroupBox = new QGroupBox( this );
    mFavoriteFoldersViewGroupBox->setTitle( i18n( "Show Favorite Folders View" ) );
    mFavoriteFoldersViewGroupBox->setLayout( new QHBoxLayout() );
    mFavoriteFoldersViewGroupBox->layout()->setSpacing( KDialog::spacingHint() );
    mFavoriteFoldersViewGroup = new QButtonGroup( this );
    connect( mFavoriteFoldersViewGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QRadioButton* favoriteFoldersViewHiddenRadio = new QRadioButton( i18n( "Never" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewHiddenRadio, static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::HiddenMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewHiddenRadio );

    QRadioButton* favoriteFoldersViewIconsRadio = new QRadioButton( i18n( "As icons" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewIconsRadio, static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::IconMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewIconsRadio );

    QRadioButton* favoriteFoldersViewListRadio = new QRadioButton( i18n( "As list" ), mFavoriteFoldersViewGroupBox );
    mFavoriteFoldersViewGroup->addButton( favoriteFoldersViewListRadio,  static_cast<int>( MailCommon::MailCommonSettings::EnumFavoriteCollectionViewMode::ListMode ) );
    mFavoriteFoldersViewGroupBox->layout()->addWidget( favoriteFoldersViewListRadio );

    vlay->addWidget( mFavoriteFoldersViewGroupBox );


    // "folder tooltips" radio buttons:
    mFolderToolTipsGroupBox = new QGroupBox( this );
    mFolderToolTipsGroupBox->setTitle( i18n( "Folder Tooltips" ) );
    mFolderToolTipsGroupBox->setLayout( new QHBoxLayout() );
    mFolderToolTipsGroupBox->layout()->setSpacing( KDialog::spacingHint() );
    mFolderToolTipsGroup = new QButtonGroup( this );
    connect( mFolderToolTipsGroup, SIGNAL(buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    QRadioButton* folderToolTipsAlwaysRadio = new QRadioButton( i18n( "Always" ), mFolderToolTipsGroupBox );
    mFolderToolTipsGroup->addButton( folderToolTipsAlwaysRadio, static_cast< int >( FolderTreeWidget::DisplayAlways ) );
    mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsAlwaysRadio );

    QRadioButton* folderToolTipsNeverRadio = new QRadioButton( i18n( "Never" ), mFolderToolTipsGroupBox );
    mFolderToolTipsGroup->addButton( folderToolTipsNeverRadio, static_cast< int >( FolderTreeWidget::DisplayNever ) );
    mFolderToolTipsGroupBox->layout()->addWidget( folderToolTipsNeverRadio );

    vlay->addWidget( mFolderToolTipsGroupBox );

    // "show reader window" radio buttons:
    populateButtonGroup( mReaderWindowModeGroupBox = new QGroupBox( this ),
                         mReaderWindowModeGroup = new QButtonGroup( this ),
                         Qt::Vertical, GlobalSettings::self()->readerWindowModeItem() );
    vlay->addWidget( mReaderWindowModeGroupBox );
    connect( mReaderWindowModeGroup, SIGNAL (buttonClicked(int)),
             this, SLOT(slotEmitChanged()) );

    vlay->addStretch( 10 ); // spacer
}

void AppearancePage::LayoutTab::doLoadOther()
{
    loadWidget( mFolderListGroupBox, mFolderListGroup, GlobalSettings::self()->folderListItem() );
    loadWidget( mReaderWindowModeGroupBox, mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
    loadWidget( mFavoriteFoldersViewGroupBox, mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem() );
    loadWidget( mFolderQuickSearchCB, GlobalSettings::self()->enableFolderQuickSearchItem() );
    const int checkedFolderToolTipsPolicy = GlobalSettings::self()->toolTipDisplayPolicy();
    if ( checkedFolderToolTipsPolicy < mFolderToolTipsGroup->buttons().size() && checkedFolderToolTipsPolicy >= 0 )
        mFolderToolTipsGroup->buttons()[ checkedFolderToolTipsPolicy ]->setChecked( true );
}

void AppearancePage::LayoutTab::save()
{
    saveButtonGroup( mFolderListGroup, GlobalSettings::self()->folderListItem() );
    saveButtonGroup( mReaderWindowModeGroup, GlobalSettings::self()->readerWindowModeItem() );
    saveButtonGroup( mFavoriteFoldersViewGroup, MailCommon::MailCommonSettings::self()->favoriteCollectionViewModeItem() );
    saveCheckBox( mFolderQuickSearchCB, GlobalSettings::self()->enableFolderQuickSearchItem() );
    GlobalSettings::self()->setToolTipDisplayPolicy( mFolderToolTipsGroup->checkedId() );
}

//
// Appearance Message List
//

QString AppearancePage::HeadersTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-headers");
}

static const struct {
    const char * displayName;
    DateFormatter::FormatType dateDisplay;
} dateDisplayConfig[] = {
    { I18N_NOOP("Sta&ndard format (%1)"), KMime::DateFormatter::CTime },
    { I18N_NOOP("Locali&zed format (%1)"), KMime::DateFormatter::Localized },
    { I18N_NOOP("Smart for&mat (%1)"), KMime::DateFormatter::Fancy },
    { I18N_NOOP("C&ustom format:"), KMime::DateFormatter::Custom }
};
static const int numDateDisplayConfig =
        sizeof dateDisplayConfig / sizeof *dateDisplayConfig;

AppearancePageHeadersTab::AppearancePageHeadersTab( QWidget * parent )
    : ConfigModuleTab( parent ),
      mCustomDateFormatEdit( 0 )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "General Options" group:
    QGroupBox *group = new QGroupBox( i18nc( "General options for the message list.", "General" ), this );
    //  group->layout()->setSpacing( KDialog::spacingHint() );
    QVBoxLayout *gvlay = new QVBoxLayout( group );
    gvlay->setSpacing( KDialog::spacingHint() );

    mDisplayMessageToolTips = new QCheckBox(
                MessageList::Core::Settings::self()->messageToolTipEnabledItem()->label(), group );
    gvlay->addWidget( mDisplayMessageToolTips );

    connect( mDisplayMessageToolTips, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mHideTabBarWithSingleTab = new QCheckBox(
                MessageList::Core::Settings::self()->autoHideTabBarWithSingleTabItem()->label(), group );
    gvlay->addWidget( mHideTabBarWithSingleTab );

    connect( mHideTabBarWithSingleTab, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mTabsHaveCloseButton = new QCheckBox(
                MessageList::Core::Settings::self()->tabsHaveCloseButtonItem()->label(), group );
    gvlay->addWidget(  mTabsHaveCloseButton );

    connect( mTabsHaveCloseButton, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    // "Aggregation"
    using MessageList::Utils::AggregationComboBox;
    mAggregationComboBox = new AggregationComboBox( group );

    QLabel* aggregationLabel = new QLabel( i18n( "Default aggregation:" ), group );
    aggregationLabel->setBuddy( mAggregationComboBox );

    using MessageList::Utils::AggregationConfigButton;
    AggregationConfigButton * aggregationConfigButton = new AggregationConfigButton( group, mAggregationComboBox );

    QHBoxLayout * aggregationLayout = new QHBoxLayout();
    aggregationLayout->addWidget( aggregationLabel, 1 );
    aggregationLayout->addWidget( mAggregationComboBox, 1 );
    aggregationLayout->addWidget( aggregationConfigButton, 0 );
    gvlay->addLayout( aggregationLayout );

    connect( aggregationConfigButton, SIGNAL(configureDialogCompleted()),
             this, SLOT(slotSelectDefaultAggregation()) );
    connect( mAggregationComboBox, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    // "Theme"
    using MessageList::Utils::ThemeComboBox;
    mThemeComboBox = new ThemeComboBox( group );

    QLabel *themeLabel = new QLabel( i18n( "Default theme:" ), group );
    themeLabel->setBuddy( mThemeComboBox );

    using MessageList::Utils::ThemeConfigButton;
    ThemeConfigButton * themeConfigButton = new ThemeConfigButton( group, mThemeComboBox );

    QHBoxLayout * themeLayout = new QHBoxLayout();
    themeLayout->addWidget( themeLabel, 1 );
    themeLayout->addWidget( mThemeComboBox, 1 );
    themeLayout->addWidget( themeConfigButton, 0 );
    gvlay->addLayout( themeLayout );

    connect( themeConfigButton, SIGNAL(configureDialogCompleted()),
             this, SLOT(slotSelectDefaultTheme()) );
    connect( mThemeComboBox, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    vlay->addWidget( group );

    // "Date Display" group:
    mDateDisplay = new KButtonGroup( this );
    mDateDisplay->setTitle( i18n("Date Display") );
    gvlay = new QVBoxLayout( mDateDisplay );
    gvlay->setSpacing( KDialog::spacingHint() );

    for ( int i = 0 ; i < numDateDisplayConfig ; ++i ) {
        const char *label = dateDisplayConfig[i].displayName;
        QString buttonLabel;
        if ( QString::fromLatin1(label).contains(QLatin1String("%1")) )
            buttonLabel = i18n( label, DateFormatter::formatCurrentDate( dateDisplayConfig[i].dateDisplay) );
        else
            buttonLabel = i18n( label );
        QRadioButton *radio = new QRadioButton( buttonLabel, mDateDisplay );
        gvlay->addWidget( radio );

        if ( dateDisplayConfig[i].dateDisplay == DateFormatter::Custom ) {
            KHBox *hbox = new KHBox( mDateDisplay );
            hbox->setSpacing( KDialog::spacingHint() );

            mCustomDateFormatEdit = new KLineEdit( hbox );
            mCustomDateFormatEdit->setEnabled( false );
            hbox->setStretchFactor( mCustomDateFormatEdit, 1 );

            connect( radio, SIGNAL(toggled(bool)),
                     mCustomDateFormatEdit, SLOT(setEnabled(bool)) );
            connect( mCustomDateFormatEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(slotEmitChanged()) );

            QLabel *formatHelp = new QLabel(
                        i18n( "<qt><a href=\"whatsthis1\">Custom format information...</a></qt>"), hbox );
            connect( formatHelp, SIGNAL(linkActivated(QString)),
                     SLOT(slotLinkClicked(QString)) );

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
            mCustomDateFormatEdit->setWhatsThis( mCustomDateWhatsThis );
            radio->setWhatsThis( mCustomDateWhatsThis );
            gvlay->addWidget( hbox );
        }  } // end for loop populating mDateDisplay

    vlay->addWidget( mDateDisplay );
    connect( mDateDisplay, SIGNAL(clicked(int)),
             this, SLOT(slotEmitChanged()) );


    vlay->addStretch( 10 ); // spacer
}

void AppearancePageHeadersTab::slotLinkClicked( const QString & link )
{
    if ( link == QLatin1String( "whatsthis1" ) )
        QWhatsThis::showText( QCursor::pos(), mCustomDateWhatsThis );
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
    loadWidget(mDisplayMessageToolTips, MessageList::Core::Settings::self()->messageToolTipEnabledItem());
    loadWidget(mHideTabBarWithSingleTab, MessageList::Core::Settings::self()->autoHideTabBarWithSingleTabItem() );
    loadWidget(mTabsHaveCloseButton, MessageList::Core::Settings::self()->tabsHaveCloseButtonItem() );

    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    // "Date Display":
    setDateDisplay( MessageCore::GlobalSettings::self()->dateFormat(),
                    MessageCore::GlobalSettings::self()->customDateFormat() );
}

void AppearancePage::HeadersTab::doLoadFromGlobalSettings()
{
    loadWidget(mDisplayMessageToolTips, MessageList::Core::Settings::self()->messageToolTipEnabledItem());
    loadWidget(mHideTabBarWithSingleTab, MessageList::Core::Settings::self()->autoHideTabBarWithSingleTabItem() );
    loadWidget(mTabsHaveCloseButton, MessageList::Core::Settings::self()->tabsHaveCloseButtonItem() );
    // "Aggregation":
    slotSelectDefaultAggregation();

    // "Theme":
    slotSelectDefaultTheme();

    setDateDisplay( MessageCore::GlobalSettings::self()->dateFormat(),
                    MessageCore::GlobalSettings::self()->customDateFormat() );
}


void AppearancePage::HeadersTab::setDateDisplay( int num, const QString & format )
{
    DateFormatter::FormatType dateDisplay =
            static_cast<DateFormatter::FormatType>( num );

    // special case: needs text for the line edit:
    if ( dateDisplay == DateFormatter::Custom )
        mCustomDateFormatEdit->setText( format );

    for ( int i = 0 ; i < numDateDisplayConfig ; ++i )
        if ( dateDisplay == dateDisplayConfig[i].dateDisplay ) {
            mDateDisplay->setSelected( i );
            return;
        }
    // fell through since none found:
    mDateDisplay->setSelected( numDateDisplayConfig - 2 ); // default
}

void AppearancePage::HeadersTab::save()
{
    saveCheckBox(mDisplayMessageToolTips, MessageList::Core::Settings::self()->messageToolTipEnabledItem());
    saveCheckBox(mHideTabBarWithSingleTab, MessageList::Core::Settings::self()->autoHideTabBarWithSingleTabItem() );
    saveCheckBox(mTabsHaveCloseButton, MessageList::Core::Settings::self()->tabsHaveCloseButtonItem() );

    KMKernel::self()->savePaneSelection();
    // "Aggregation"
    mAggregationComboBox->writeDefaultConfig();

    // "Theme"
    mThemeComboBox->writeDefaultConfig();

    const int dateDisplayID = mDateDisplay->selected();
    // check bounds:
    if ( ( dateDisplayID >= 0 ) && ( dateDisplayID < numDateDisplayConfig ) ) {
        MessageCore::GlobalSettings::self()->setDateFormat(
                    static_cast<int>( dateDisplayConfig[ dateDisplayID ].dateDisplay ) );
    }
    MessageCore::GlobalSettings::self()->setCustomDateFormat( mCustomDateFormatEdit->text() );
}


//
// Message Window
//

QString AppearancePage::ReaderTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-reader");
}

AppearancePageReaderTab::AppearancePageReaderTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setSpacing( KDialog::spacingHint() );
    topLayout->setMargin( KDialog::marginHint() );

    // "Close message window after replying or forwarding" check box:
    populateCheckBox( mCloseAfterReplyOrForwardCheck = new QCheckBox( this ),
                      GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mCloseAfterReplyOrForwardCheck->setToolTip(
                i18n( "Close the standalone message window after replying or forwarding the message" ) );
    topLayout->addWidget( mCloseAfterReplyOrForwardCheck );
    connect( mCloseAfterReplyOrForwardCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mViewerSettings = new MessageViewer::ConfigureWidget;
    connect( mViewerSettings, SIGNAL(settingsChanged()),
             this, SLOT(slotEmitChanged()) );
    topLayout->addWidget( mViewerSettings );

    topLayout->addStretch( 100 ); // spacer
}

void AppearancePage::ReaderTab::doResetToDefaultsOther()
{
}

void AppearancePage::ReaderTab::doLoadOther()
{
    loadWidget( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mViewerSettings->readConfig();
}


void AppearancePage::ReaderTab::save()
{
    saveCheckBox( mCloseAfterReplyOrForwardCheck, GlobalSettings::self()->closeAfterReplyOrForwardItem() );
    mViewerSettings->writeConfig();
}

QString AppearancePage::SystemTrayTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-systemtray");
}

AppearancePageSystemTrayTab::AppearancePageSystemTrayTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout * vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Enable system tray applet" check box
    mSystemTrayCheck = new QCheckBox( i18n("Enable system tray icon"), this );
    vlay->addWidget( mSystemTrayCheck );
    connect( mSystemTrayCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );

    mSystemTrayShowUnreadMail = new QCheckBox( i18n("Show unread mail in tray icon"), this );
    vlay->addWidget( mSystemTrayShowUnreadMail );
    mSystemTrayShowUnreadMail->setEnabled(false);
    connect( mSystemTrayShowUnreadMail, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mSystemTrayCheck, SIGNAL(toggled(bool)),
             mSystemTrayShowUnreadMail, SLOT(setEnabled(bool)) );


    // System tray modes
    mSystemTrayGroup = new KButtonGroup( this );
    mSystemTrayGroup->setTitle( i18n("System Tray Mode" ) );
    QVBoxLayout *gvlay = new QVBoxLayout( mSystemTrayGroup );
    gvlay->setSpacing( KDialog::spacingHint() );

    connect( mSystemTrayGroup, SIGNAL(clicked(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mSystemTrayCheck, SIGNAL(toggled(bool)),
             mSystemTrayGroup, SLOT(setEnabled(bool)) );

    gvlay->addWidget( new QRadioButton( i18n("Always show KMail in system tray"), mSystemTrayGroup ) );
    gvlay->addWidget( new QRadioButton( i18n("Only show KMail in system tray if there are unread messages"), mSystemTrayGroup ) );

    vlay->addWidget( mSystemTrayGroup );
    vlay->addStretch( 10 ); // spacer
}

void AppearancePage::SystemTrayTab::doLoadFromGlobalSettings()
{
    loadWidget(mSystemTrayCheck, GlobalSettings::self()->systemTrayEnabledItem() );
    loadWidget(mSystemTrayShowUnreadMail, GlobalSettings::self()->systemTrayShowUnreadItem() );
    mSystemTrayGroup->setSelected( GlobalSettings::self()->systemTrayPolicy() );
    mSystemTrayGroup->setEnabled( mSystemTrayCheck->isChecked() );
}

void AppearancePage::SystemTrayTab::save()
{
    saveCheckBox(mSystemTrayCheck, GlobalSettings::self()->systemTrayEnabledItem() );
    GlobalSettings::self()->setSystemTrayPolicy( mSystemTrayGroup->selected() );
    saveCheckBox(mSystemTrayShowUnreadMail,GlobalSettings::self()->systemTrayShowUnreadItem());
    GlobalSettings::self()->writeConfig();
}

QString AppearancePage::MessageTagTab::helpAnchor() const
{
    return QString::fromLatin1("configure-appearance-messagetag");
}


TagListWidgetItem::TagListWidgetItem(QListWidget *parent)
    : QListWidgetItem(parent), mTag( 0 )
{
}

TagListWidgetItem::TagListWidgetItem(const QIcon & icon, const QString & text, QListWidget * parent )
    : QListWidgetItem(icon, text, parent), mTag( 0 )
{
}

TagListWidgetItem::~TagListWidgetItem()
{
}

void TagListWidgetItem::setKMailTag( const MailCommon::Tag::Ptr& tag )
{
    mTag = tag;
}

MailCommon::Tag::Ptr TagListWidgetItem::kmailTag() const
{
    return mTag;
}


AppearancePageMessageTagTab::AppearancePageMessageTagTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    mPreviousTag = -1;
    QHBoxLayout *maingrid = new QHBoxLayout( this );
    maingrid->setMargin( KDialog::marginHint() );
    maingrid->setSpacing( KDialog::spacingHint() );


    //Lefthand side Listbox and friends

    //Groupbox frame
    mTagsGroupBox = new QGroupBox( i18n("A&vailable Tags"), this );
    maingrid->addWidget( mTagsGroupBox );
    QVBoxLayout *tageditgrid = new QVBoxLayout( mTagsGroupBox );
    tageditgrid->setMargin( KDialog::marginHint() );
    tageditgrid->setSpacing( KDialog::spacingHint() );
    tageditgrid->addSpacing( 2 * KDialog::spacingHint() );

    //Listbox, add, remove row
    QHBoxLayout *addremovegrid = new QHBoxLayout();
    tageditgrid->addLayout( addremovegrid );

    mTagAddLineEdit = new KLineEdit( mTagsGroupBox );
    mTagAddLineEdit->setTrapReturnKey( true );
    addremovegrid->addWidget( mTagAddLineEdit );

    mTagAddButton = new KPushButton( mTagsGroupBox );
    mTagAddButton->setToolTip( i18n("Add new tag") );
    mTagAddButton->setIcon( KIcon( QLatin1String("list-add") ) );
    addremovegrid->addWidget( mTagAddButton );

    mTagRemoveButton = new KPushButton( mTagsGroupBox );
    mTagRemoveButton->setToolTip( i18n("Remove selected tag") );
    mTagRemoveButton->setIcon( KIcon( QLatin1String("list-remove") ) );
    addremovegrid->addWidget( mTagRemoveButton );

    //Up and down buttons
    QHBoxLayout *updowngrid = new QHBoxLayout();
    tageditgrid->addLayout( updowngrid );

    mTagUpButton = new KPushButton( mTagsGroupBox );
    mTagUpButton->setToolTip( i18n("Increase tag priority") );
    mTagUpButton->setIcon( KIcon( QLatin1String("arrow-up") ) );
    mTagUpButton->setAutoRepeat( true );
    updowngrid->addWidget( mTagUpButton );

    mTagDownButton = new KPushButton( mTagsGroupBox );
    mTagDownButton->setToolTip( i18n("Decrease tag priority") );
    mTagDownButton->setIcon( KIcon( QLatin1String("arrow-down") ) );
    mTagDownButton->setAutoRepeat( true );
    updowngrid->addWidget( mTagDownButton );

    //Listbox for tag names
    QHBoxLayout *listboxgrid = new QHBoxLayout();
    tageditgrid->addLayout( listboxgrid );
    mTagListBox = new QListWidget( mTagsGroupBox );
    mTagListBox->setDragDropMode(QAbstractItemView::InternalMove);
    connect( mTagListBox->model(),SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),SLOT(slotRowsMoved(QModelIndex,int,int,QModelIndex,int)) );

    mTagListBox->setMinimumWidth( 150 );
    listboxgrid->addWidget( mTagListBox );

    //RHS for individual tag settings

    //Extra VBoxLayout for stretchers around settings
    QVBoxLayout *tagsettinggrid = new QVBoxLayout();
    maingrid->addLayout( tagsettinggrid );
    //tagsettinggrid->addStretch( 10 );

    //Groupbox frame
    mTagSettingGroupBox = new QGroupBox( i18n("Ta&g Settings"),
                                          this );
    tagsettinggrid->addWidget( mTagSettingGroupBox );
    QList<KActionCollection *> actionCollections;
    if( kmkernel->getKMMainWidget() )
        actionCollections = kmkernel->getKMMainWidget()->actionCollections();

    QHBoxLayout *lay = new QHBoxLayout(mTagSettingGroupBox);
    mTagWidget = new MailCommon::TagWidget(actionCollections,this);
    lay->addWidget(mTagWidget);

    connect(mTagWidget,SIGNAL(changed()),this, SLOT(slotEmitChangeCheck()));

    //For enabling the add button in case box is non-empty
    connect( mTagAddLineEdit, SIGNAL(textChanged(QString)),
              this, SLOT(slotAddLineTextChanged(QString)) );

    //For on-the-fly updating of tag name in editbox
    connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
              this, SLOT(slotNameLineTextChanged(QString)) );

    connect( mTagWidget, SIGNAL(iconNameChanged(QString)), SLOT(slotIconNameChanged(QString)) );

    connect(  mTagAddLineEdit, SIGNAL(returnPressed()),
              this, SLOT(slotAddNewTag()) );


    connect( mTagAddButton, SIGNAL(clicked()),
              this, SLOT(slotAddNewTag()) );

    connect( mTagRemoveButton, SIGNAL(clicked()),
              this, SLOT(slotRemoveTag()) );

    connect( mTagUpButton, SIGNAL(clicked()),
              this, SLOT(slotMoveTagUp()) );

    connect( mTagDownButton, SIGNAL(clicked()),
              this, SLOT(slotMoveTagDown()) );

    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
              this, SLOT(slotSelectionChanged()) );
    //Adjust widths for columns
    maingrid->setStretchFactor( mTagsGroupBox, 1 );
    maingrid->setStretchFactor( lay, 1 );
    tagsettinggrid->addStretch( 10 );
}

AppearancePageMessageTagTab::~AppearancePageMessageTagTab()
{
}

void AppearancePage::MessageTagTab::slotEmitChangeCheck()
{
    slotEmitChanged();
}

void AppearancePage::MessageTagTab::slotRowsMoved( const QModelIndex &,
                                                   int sourcestart, int sourceEnd,
                                                   const QModelIndex &, int destinationRow )
{
    Q_UNUSED( sourceEnd );
    Q_UNUSED( sourcestart );
    Q_UNUSED( destinationRow );
    updateButtons();
    slotEmitChangeCheck();
}


void AppearancePage::MessageTagTab::updateButtons()
{
    const int currentIndex = mTagListBox->currentRow();

    const bool theFirst = ( currentIndex == 0 );
    const bool theLast = ( currentIndex >= (int)mTagListBox->count() - 1 );
    const bool aFilterIsSelected = ( currentIndex >= 0 );

    mTagUpButton->setEnabled( aFilterIsSelected && !theFirst );
    mTagDownButton->setEnabled( aFilterIsSelected && !theLast );
}

void AppearancePage::MessageTagTab::slotMoveTagUp()
{
    const int tmp_index = mTagListBox->currentRow();
    if ( tmp_index <= 0 )
        return;
    swapTagsInListBox( tmp_index, tmp_index - 1 );
    updateButtons();
}

void AppearancePage::MessageTagTab::slotMoveTagDown()
{
    const int tmp_index = mTagListBox->currentRow();
    if ( ( tmp_index < 0 )
         || ( tmp_index >= int( mTagListBox->count() ) - 1 ) )
        return;
    swapTagsInListBox( tmp_index, tmp_index + 1 );
    updateButtons();
}

void AppearancePage::MessageTagTab::swapTagsInListBox( const int first,
                                                       const int second )
{
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );
    QListWidgetItem *item = mTagListBox->takeItem( first );
    // now selected item is at idx(idx-1), so
    // insert the other item at idx, ie. above(below).
    mPreviousTag = second;
    mTagListBox->insertItem( second, item );
    mTagListBox->setCurrentRow( second );
    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );
    slotEmitChangeCheck();
}

void AppearancePage::MessageTagTab::slotRecordTagSettings( int aIndex )
{
    if ( ( aIndex < 0 ) || ( aIndex >= int( mTagListBox->count() ) )  )
        return;
    QListWidgetItem *item = mTagListBox->item( aIndex );
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );

    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    tmp_desc->tagName = tagItem->text();
    mTagWidget->recordTagSettings(tmp_desc);
}

void AppearancePage::MessageTagTab::slotUpdateTagSettingWidgets( int aIndex )
{
    //Check if selection is valid
    if ( ( aIndex < 0 ) || ( mTagListBox->currentRow() < 0 )  ) {
        mTagRemoveButton->setEnabled( false );
        mTagUpButton->setEnabled( false );
        mTagDownButton->setEnabled( false );

        mTagWidget->setEnabled(false);
        return;
    }
    mTagWidget->setEnabled(true);

    mTagRemoveButton->setEnabled( true );
    mTagUpButton->setEnabled( ( 0 != aIndex ) );
    mTagDownButton->setEnabled(( ( int( mTagListBox->count() ) - 1 ) != aIndex ) );
    QListWidgetItem * item = mTagListBox->currentItem();
    TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );
    MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();

    disconnect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                this, SLOT(slotNameLineTextChanged(QString)) );

    mTagWidget->tagNameLineEdit()->setEnabled( !tmp_desc->isImmutable );
    mTagWidget->tagNameLineEdit()->setText( tmp_desc->tagName );
    connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
             this, SLOT(slotNameLineTextChanged(QString)) );


    mTagWidget->setTagTextColor(tmp_desc->textColor);

    mTagWidget->setTagBackgroundColor(tmp_desc->backgroundColor);

    mTagWidget->setTagTextFont(tmp_desc->textFont);

    mTagWidget->iconButton()->setEnabled( !tmp_desc->isImmutable );
    mTagWidget->iconButton()->setIcon( tmp_desc->iconName );

    mTagWidget->keySequenceWidget()->setEnabled( true );
    mTagWidget->keySequenceWidget()->setKeySequence( tmp_desc->shortcut.primary(),
                                                     KKeySequenceWidget::NoValidate );

    mTagWidget->inToolBarCheck()->setEnabled( true );
    mTagWidget->inToolBarCheck()->setChecked( tmp_desc->inToolbar );
}

void AppearancePage::MessageTagTab::slotSelectionChanged()
{
    mEmitChanges = false;
    slotRecordTagSettings( mPreviousTag );
    slotUpdateTagSettingWidgets( mTagListBox->currentRow() );
    mPreviousTag = mTagListBox->currentRow();
    mEmitChanges = true;
}

void AppearancePage::MessageTagTab::slotRemoveTag()
{
    const int tmp_index = mTagListBox->currentRow();
    if ( tmp_index >= 0 ) {
        QListWidgetItem * item = mTagListBox->takeItem( mTagListBox->currentRow() );
        TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( item );
        MailCommon::Tag::Ptr tmp_desc = tagItem->kmailTag();
        if ( tmp_desc->tag().isValid() ) {
          new Akonadi::TagDeleteJob(tmp_desc->tag());
        } else {
          kWarning() << "Can't remove tag with invalid akonadi tag";
        }
        mPreviousTag = -1;

        //Before deleting the current item, make sure the selectionChanged signal
        //is disconnected, so that the widgets will not get updated while the
        //deletion takes place.
        disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                    this, SLOT(slotSelectionChanged()) );

        delete item;
        connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                 this, SLOT(slotSelectionChanged()) );

        slotSelectionChanged();
        slotEmitChangeCheck();
    }
}

void AppearancePage::MessageTagTab::slotNameLineTextChanged( const QString
                                                             &aText )
{
    //If deleted all, leave the first character for the sake of not having an
    //empty tag name
    if ( aText.isEmpty() && !mTagListBox->currentItem())
        return;

    const int count = mTagListBox->count();
    for ( int i=0; i < count; ++i ) {
        if(mTagListBox->item(i)->text() == aText) {
            KMessageBox::error(this,i18n("We cannot create tag. A tag with same name already exists."));
            disconnect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                        this, SLOT(slotNameLineTextChanged(QString)) );
            mTagWidget->tagNameLineEdit()->setText(mTagListBox->currentItem()->text());
            connect( mTagWidget->tagNameLineEdit(), SIGNAL(textChanged(QString)),
                     this, SLOT(slotNameLineTextChanged(QString)) );
            return;
        }
    }


    //Disconnect so the tag information is not saved and reloaded with every
    //letter
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );

    mTagListBox->currentItem()->setText( aText );
    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );
}

void AppearancePage::MessageTagTab::slotIconNameChanged( const QString &iconName )
{
    mTagListBox->currentItem()->setIcon( KIcon( iconName ) );
}

void AppearancePage::MessageTagTab::slotAddLineTextChanged( const QString &aText )
{
    mTagAddButton->setEnabled( !aText.trimmed().isEmpty() );
}

void AppearancePage::MessageTagTab::slotAddNewTag()
{
    const QString newTagName = mTagAddLineEdit->text();
    const int count = mTagListBox->count();
    for ( int i=0; i < count; ++i ) {
        if(mTagListBox->item(i)->text() == newTagName) {
            KMessageBox::error(this,i18n("We cannot create tag. A tag with same name already exists."));
            return;
        }
    }

    const int tmp_priority = mTagListBox->count();

    MailCommon::Tag::Ptr tag( Tag::createDefaultTag( newTagName ) );
    tag->priority = tmp_priority;

    slotEmitChangeCheck();
    TagListWidgetItem *newItem = new TagListWidgetItem( KIcon( tag->iconName ), newTagName,  mTagListBox );
    newItem->setKMailTag( tag );
    mTagListBox->addItem( newItem );
    mTagListBox->setCurrentItem( newItem );
    mTagAddLineEdit->setText( QString() );
}

void AppearancePage::MessageTagTab::doLoadFromGlobalSettings()
{
    mTagListBox->clear();

    Akonadi::TagFetchJob *fetchJob = new Akonadi::TagFetchJob(this);
    fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(slotTagsFetched(KJob*)));
}

void AppearancePage::MessageTagTab::slotTagsFetched(KJob *job)
{
    if (job->error()) {
        kWarning() << "Failed to load tags " << job->errorString();
        return;
    }
    Akonadi::TagFetchJob *fetchJob = static_cast<Akonadi::TagFetchJob*>(job);

    QList<MailCommon::TagPtr> msgTagList;
    foreach( const Akonadi::Tag &akonadiTag, fetchJob->tags() ) {
        MailCommon::Tag::Ptr tag = MailCommon::Tag::fromAkonadi( akonadiTag );
        msgTagList.append( tag );
    }

    qSort( msgTagList.begin(), msgTagList.end(), MailCommon::Tag::compare );

    foreach( const MailCommon::Tag::Ptr& tag, msgTagList ) {
        TagListWidgetItem *newItem = new TagListWidgetItem( KIcon( tag->iconName ), tag->tagName, mTagListBox );
        newItem->setKMailTag( tag );
        if ( tag->priority == -1 )
            tag->priority = mTagListBox->count() - 1;
    }

    //Disconnect so that insertItem's do not trigger an update procedure
    disconnect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(slotSelectionChanged()) );

    connect( mTagListBox, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this, SLOT(slotSelectionChanged()) );

    slotUpdateTagSettingWidgets( -1 );
    //Needed since the previous function doesn't affect add button
    mTagAddButton->setEnabled( false );

    // Save the original list
    mOriginalMsgTagList.clear();
    foreach( const MailCommon::TagPtr &tag, msgTagList ) {
        mOriginalMsgTagList.append( MailCommon::TagPtr( new MailCommon::Tag( *tag ) ) );
    }

}

void AppearancePage::MessageTagTab::save()
{
    const int currentRow = mTagListBox->currentRow();
    if ( currentRow < 0 ) {
        return;
    }

    const int count = mTagListBox->count();
    if ( !count ) {
        return;
    }

    QListWidgetItem *item = mTagListBox->currentItem();
    if ( !item ) {
        return;
    }
    slotRecordTagSettings( currentRow );
    const int numberOfMsgTagList = count;
    for ( int i=0; i < numberOfMsgTagList; ++i ) {
        TagListWidgetItem *tagItem = static_cast<TagListWidgetItem*>( mTagListBox->item(i) );
        if ( ( i>=mOriginalMsgTagList.count() ) || *(tagItem->kmailTag()) != *(mOriginalMsgTagList[i]) ) {
            MailCommon::Tag::Ptr tag = tagItem->kmailTag();
            tag->priority = i;

            MailCommon::Tag::SaveFlags saveFlags = mTagWidget->saveFlags();
            Akonadi::Tag akonadiTag = tag->saveToAkonadi( saveFlags );
            if ((*tag).id()> 0) {
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


