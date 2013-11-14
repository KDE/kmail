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

#include "configurecomposerpage.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "pimcommon/autocorrection/composerautocorrectionwidget.h"
#include "messagecomposer/imagescaling/imagescalingwidget.h"
#include "messagecomposer/settings/messagecomposersettings.h"
#include "settings/globalsettings.h"
#include "configuredialog/configuredialoglistview.h"
#include "pimcommon/widgets/simplestringlisteditor.h"
#include "templateparser/templatesconfiguration_kfg.h"
#include "templateparser/templatesconfiguration.h"
#include "templateparser/customtemplates.h"
#include "templateparser/globalsettings_base.h"
#include "messageviewer/utils/autoqpointer.h"
#include "addressline/recentaddresses.h"
#include <libkdepim/ldap/ldapclientsearch.h>
#include "addressline/completionordereditor.h"
using KPIM::RecentAddresses;


#include <KLocale>
#include <KSeparator>
#include <KCharsets>
#include <KUrlRequester>
#include <KHBox>
#include <KMessageBox>
#include <KFile>
#include <kascii.h>
#include <KIntSpinBox>
#include <KIntNumInput>

#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTextCodec>
#include <QCheckBox>

QString ComposerPage::helpAnchor() const
{
    return QString::fromLatin1("configure-composer");
}

ComposerPage::ComposerPage( const KComponentData &instance, QWidget *parent )
    : ConfigModuleWithTabs( instance, parent )
{
    //
    // "General" tab:
    //
    mGeneralTab = new GeneralTab();
    addTab( mGeneralTab, i18nc("General settings for the composer.", "General") );
    addConfig( GlobalSettings::self(), mGeneralTab );

    //
    // "Templates" tab:
    //
    mTemplatesTab = new TemplatesTab();
    addTab( mTemplatesTab, i18n("Standard Templates") );

    //
    // "Custom Templates" tab:
    //
    mCustomTemplatesTab = new CustomTemplatesTab();
    addTab( mCustomTemplatesTab, i18n("Custom Templates") );

    //
    // "Subject" tab:
    //
    mSubjectTab = new SubjectTab();
    addTab( mSubjectTab, i18nc("Settings regarding the subject when composing a message.","Subject") );
    addConfig( GlobalSettings::self(), mSubjectTab );

    //
    // "Charset" tab:
    //
    mCharsetTab = new CharsetTab();
    addTab( mCharsetTab, i18n("Charset") );

    //
    // "Headers" tab:
    //
    mHeadersTab = new HeadersTab();
    addTab( mHeadersTab, i18n("Headers") );

    //
    // "Attachments" tab:
    //
    mAttachmentsTab = new AttachmentsTab();
    addTab( mAttachmentsTab, i18nc("Config->Composer->Attachments", "Attachments") );

    //
    // "autocorrection" tab:
    //
    mAutoCorrectionTab = new AutoCorrectionTab();
    addTab( mAutoCorrectionTab, i18n("Autocorrection") );

    //
    // "autoresize" tab:
    //
    mAutoImageResizeTab = new AutoImageResizeTab();
    addTab( mAutoImageResizeTab, i18n("Auto Resize Image") );

    //
    // "external editor" tab:
    mExternalEditorTab = new ExternalEditorTab();
    addTab( mExternalEditorTab, i18n("External Editor") );
}

QString ComposerPage::GeneralTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-general");
}

ComposerPageGeneralTab::ComposerPageGeneralTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    // Main layout
    QHBoxLayout *hb1 = new QHBoxLayout();			// box with 2 columns
    QVBoxLayout *vb1 = new QVBoxLayout();			// first with 2 groupboxes
    QVBoxLayout *vb2 = new QVBoxLayout();			// second with 1 groupbox

    // "Signature" group
    QGroupBox *groupBox = new QGroupBox( i18nc( "@title:group", "Signature" ) );
    QVBoxLayout *groupVBoxLayout = new QVBoxLayout();

    // "Automatically insert signature" checkbox
    mAutoAppSignFileCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->autoTextSignatureItem()->label(),
                this );

    QString helpText = i18n( "Automatically insert the configured signature\n"
                     "when starting to compose a message" );
    mAutoAppSignFileCheck->setToolTip( helpText );
    mAutoAppSignFileCheck->setWhatsThis( helpText );

    connect( mAutoAppSignFileCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupVBoxLayout->addWidget( mAutoAppSignFileCheck );

    // "Insert signature above quoted text" checkbox
    mTopQuoteCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->prependSignatureItem()->label(), this );
    mTopQuoteCheck->setEnabled( false );

    helpText = i18n( "Insert the signature above any quoted text" );
    mTopQuoteCheck->setToolTip( helpText );
    mTopQuoteCheck->setWhatsThis( helpText );

    connect( mTopQuoteCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mAutoAppSignFileCheck, SIGNAL(toggled(bool)),
             mTopQuoteCheck, SLOT(setEnabled(bool)) );
    groupVBoxLayout->addWidget( mTopQuoteCheck );

    // "Prepend separator to signature" checkbox
    mDashDashCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->dashDashSignatureItem()->label(), this );
    mDashDashCheck->setEnabled( false );

    helpText = i18n( "Insert the RFC-compliant signature separator\n"
                     "(two dashes and a space on a line) before the signature" );
    mDashDashCheck->setToolTip( helpText );
    mDashDashCheck->setWhatsThis( helpText );

    connect( mDashDashCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mAutoAppSignFileCheck, SIGNAL(toggled(bool)),
             mDashDashCheck, SLOT(setEnabled(bool)) );
    groupVBoxLayout->addWidget( mDashDashCheck);

    // "Remove signature when replying" checkbox
    mStripSignatureCheck = new QCheckBox( TemplateParser::GlobalSettings::self()->stripSignatureItem()->label(),
                                          this );

    helpText = i18n( "When replying, do not quote any existing signature" );
    mStripSignatureCheck->setToolTip( helpText );
    mStripSignatureCheck->setWhatsThis( helpText );

    connect( mStripSignatureCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupVBoxLayout->addWidget( mStripSignatureCheck );

    groupBox->setLayout( groupVBoxLayout );
    vb1->addWidget( groupBox );

    // "Format" group
    groupBox = new QGroupBox( i18nc( "@title:group", "Format" ) );
    QGridLayout *groupGridLayout = new QGridLayout();
    int row = 0;

    // "Only quote selected text when replying" checkbox
    mQuoteSelectionOnlyCheck = new QCheckBox( MessageComposer::MessageComposerSettings::self()->quoteSelectionOnlyItem()->label(),
                                              this );
    helpText = i18n( "When replying, only quote the selected text\n"
                     "(instead of the complete message), if\n"
                     "there is text selected in the message window." );
    mQuoteSelectionOnlyCheck->setToolTip( helpText );
    mQuoteSelectionOnlyCheck->setWhatsThis( helpText );

    connect( mQuoteSelectionOnlyCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mQuoteSelectionOnlyCheck, row, 0, 1, -1 );
    ++row;

    // "Use smart quoting" checkbox
    mSmartQuoteCheck = new QCheckBox(
                TemplateParser::GlobalSettings::self()->smartQuoteItem()->label(), this );
    helpText = i18n( "When replying, add quote signs in front of all lines of the quoted text,\n"
                     "even when the line was created by adding an additional line break while\n"
                     "word-wrapping the text." );
    mSmartQuoteCheck->setToolTip( helpText );
    mSmartQuoteCheck->setWhatsThis( helpText );

    connect( mSmartQuoteCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mSmartQuoteCheck, row, 0, 1, -1 );
    ++row;

    // "Word wrap at column" checkbox/spinbox
    mWordWrapCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->wordWrapItem()->label(), this);

    helpText = i18n( "Enable automatic word wrapping at the specified width" );
    mWordWrapCheck->setToolTip( helpText );
    mWordWrapCheck->setWhatsThis( helpText );

    mWrapColumnSpin = new KIntSpinBox( 30/*min*/, 78/*max*/, 1/*step*/,
                                       78/*init*/, this );
    mWrapColumnSpin->setEnabled( false ); // since !mWordWrapCheck->isChecked()

    helpText = i18n( "Set the text width for automatic word wrapping" );
    mWrapColumnSpin->setToolTip( helpText );
    mWrapColumnSpin->setWhatsThis( helpText );

    connect( mWordWrapCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mWrapColumnSpin, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    // only enable the spinbox if the checkbox is checked
    connect( mWordWrapCheck, SIGNAL(toggled(bool)),
             mWrapColumnSpin, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( mWordWrapCheck, row, 0 );
    groupGridLayout->addWidget( mWrapColumnSpin, row, 1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Reply/Forward using HTML if present" checkbox
    mReplyUsingHtml = new QCheckBox( TemplateParser::GlobalSettings::self()->replyUsingHtmlItem()->label(), this );
    helpText = i18n( "When replying or forwarding, quote the message\n"
                     "in the original format it was received.\n"
                     "If unchecked, the reply will be as plain text by default." );
    mReplyUsingHtml->setToolTip( helpText );
    mReplyUsingHtml->setWhatsThis( helpText );

    connect( mReplyUsingHtml, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mReplyUsingHtml, row, 0, 1, -1 );
    ++row;

    // "Improve plain text of HTML" checkbox
    mImprovePlainTextOfHtmlMessage = new QCheckBox( MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessageItem()->label(), this );

    // For what is supported see http://www.grantlee.org/apidox/classGrantlee_1_1PlainTextMarkupBuilder.html
    helpText = i18n( "Format the plain text part of a message from the HTML markup.\n"
                     "Bold, italic and underlined text, lists, and external references\n"
                     "are supported." );
    mImprovePlainTextOfHtmlMessage->setToolTip( helpText );
    mImprovePlainTextOfHtmlMessage->setWhatsThis( helpText );

    connect( mImprovePlainTextOfHtmlMessage, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mImprovePlainTextOfHtmlMessage, row, 0, 1, -1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Autosave interval" spinbox
    mAutoSave = new KIntSpinBox( 0, 60, 1, 1, this );
    mAutoSave->setObjectName( QLatin1String("kcfg_AutosaveInterval") );
    mAutoSave->setSpecialValueText( i18n("No autosave") );
    mAutoSave->setSuffix( ki18ncp( "Interval suffix", " minute", " minutes" ) );

    helpText = i18n( "Automatically save the message at this specified interval" );
    mAutoSave->setToolTip( helpText );
    mAutoSave->setWhatsThis( helpText );

    QLabel *label = new QLabel( GlobalSettings::self()->autosaveIntervalItem()->label(), this );
    label->setBuddy( mAutoSave );

    connect( mAutoSave, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0 );
    groupGridLayout->addWidget( mAutoSave, row, 1 );
    ++row;

#ifdef KDEPIM_ENTERPRISE_BUILD
    // "Default forwarding type" combobox
    mForwardTypeCombo = new KComboBox( false, this );
    mForwardTypeCombo->addItems( QStringList() << i18nc( "@item:inlistbox Inline mail forwarding",
                                                         "Inline" )
                                 << i18n( "As Attachment" ) );

    helpText = i18n( "Set the default forwarded message format" );
    mForwardTypeCombo->setToolTip( helpText );
    mForwardTypeCombo->setWhatsThis( helpText );

    label = new QLabel( i18n( "Default forwarding type:" ), this );
    label->setBuddy( mForwardTypeCombo );

    connect( mForwardTypeCombo, SIGNAL(activated(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0 );
    groupGridLayout->addWidget( mForwardTypeCombo, row, 1 );
    ++row;
#endif

    groupBox->setLayout( groupGridLayout );
    vb1->addWidget( groupBox );

    // "Recipients" group
    groupBox = new QGroupBox( i18nc( "@title:group", "Recipients" ) );
    groupGridLayout = new QGridLayout();
    row = 0;

    // "Automatically request MDNs" checkbox
    mAutoRequestMDNCheck = new QCheckBox( GlobalSettings::self()->requestMDNItem()->label(),
                                          this);

    helpText = i18n( "By default, request an MDN when starting to compose a message.\n"
                     "You can select this on a per-message basis using \"Options - Request Disposition Notification\"" );
    mAutoRequestMDNCheck->setToolTip( helpText );
    mAutoRequestMDNCheck->setWhatsThis( helpText );

    connect( mAutoRequestMDNCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mAutoRequestMDNCheck, row, 0, 1, -1 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

#ifdef KDEPIM_ENTERPRISE_BUILD
    // "Warn if too many recipients" checkbox/spinbox
    mRecipientCheck = new QCheckBox(
                GlobalSettings::self()->tooManyRecipientsItem()->label(), this);
    mRecipientCheck->setObjectName( QLatin1String("kcfg_TooManyRecipients") );
    helpText = i18n( GlobalSettings::self()->tooManyRecipientsItem()->whatsThis().toUtf8() );
    mRecipientCheck->setWhatsThis( helpText );
    mRecipientCheck->setToolTip( i18n( "Warn if too many recipients are specified" ) );

    mRecipientSpin = new KIntSpinBox( 1/*min*/, 100/*max*/, 1/*step*/,
                                      5/*init*/, this );
    mRecipientSpin->setObjectName( QLatin1String("kcfg_RecipientThreshold") );
    mRecipientSpin->setEnabled( false );
    helpText = i18n( GlobalSettings::self()->recipientThresholdItem()->whatsThis().toUtf8() );
    mRecipientSpin->setWhatsThis( helpText );
    mRecipientSpin->setToolTip( i18n( "Set the maximum number of recipients for the warning" ) );

    connect( mRecipientCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mRecipientSpin, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    // only enable the spinbox if the checkbox is checked
    connect( mRecipientCheck, SIGNAL(toggled(bool)),
             mRecipientSpin, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( mRecipientCheck, row, 0, 1, 2);
    groupGridLayout->addWidget( mRecipientSpin, row, 2 );
    ++row;
#endif

    // "Maximum Reply-to-All recipients" spinbox
    mMaximumRecipients = new KIntSpinBox( 0, 9999, 1, 1, this );

    helpText = i18n( "Only allow this many recipients to be specified for the message.\n"
                     "This applies to doing a \"Reply to All\", entering recipients manually\n"
                     "or using the \"Select...\" picker.  Setting this limit helps you to\n"
                     "avoid accidentally sending a message to too many people.  Note,\n"
                     "however, that it does not take account of distribution lists or\n"
                     "mailing lists." );
    mMaximumRecipients->setToolTip( helpText );
    mMaximumRecipients->setWhatsThis( helpText );

    label = new QLabel( MessageComposer::MessageComposerSettings::self()->maximumRecipientsItem()->label(), this );
    label->setBuddy(mMaximumRecipients);

    connect( mMaximumRecipients, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );

    groupGridLayout->addWidget( label, row, 0, 1, 2 );
    groupGridLayout->addWidget( mMaximumRecipients, row, 2 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Use recent addresses for autocompletion" checkbox
    mShowRecentAddressesInComposer = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposerItem()->label(),
                this);

    helpText = i18n( "Remember recent addresses entered,\n"
                     "and offer them for recipient completion" );
    mShowRecentAddressesInComposer->setToolTip( helpText );
    mShowRecentAddressesInComposer->setWhatsThis( helpText );

    connect( mShowRecentAddressesInComposer, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    groupGridLayout->addWidget( mShowRecentAddressesInComposer, row, 0, 1, -1 );
    ++row;

    // "Maximum recent addresses retained" spinbox
    mMaximumRecentAddress = new KIntNumInput( this );
    mMaximumRecentAddress->setSliderEnabled( false );
    mMaximumRecentAddress->setMinimum( 0 );
    mMaximumRecentAddress->setMaximum( 999 );
    mMaximumRecentAddress->setSpecialValueText( i18nc( "No addresses are retained", "No save" ) );
    mMaximumRecentAddress->setEnabled( false );

    label = new QLabel( i18n( "Maximum recent addresses retained:" ) );
    label->setBuddy( mMaximumRecentAddress );
    label->setEnabled( false );

    helpText = i18n( "The maximum number of recently entered addresses that will\n"
                     "be remembered for completion" );
    mMaximumRecentAddress->setToolTip( helpText );
    mMaximumRecentAddress->setWhatsThis( helpText );

    connect( mMaximumRecentAddress, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mShowRecentAddressesInComposer, SIGNAL(toggled(bool)),
             mMaximumRecentAddress, SLOT(setEnabled(bool)) );
    connect( mShowRecentAddressesInComposer, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );

    groupGridLayout->addWidget( label, row, 0, 1, 2 );
    groupGridLayout->addWidget( mMaximumRecentAddress, row, 2 );
    ++row;

    // "Edit Recent Addresses" button
    QPushButton *recentAddressesBtn = new QPushButton( i18n( "Edit Recent Addresses..." ), this );
    helpText = i18n( "Edit, add or remove recent addresses" );
    recentAddressesBtn->setToolTip( helpText );
    recentAddressesBtn->setWhatsThis( helpText );

    connect( recentAddressesBtn, SIGNAL(clicked()),
             this, SLOT(slotConfigureRecentAddresses()) );
    groupGridLayout->addWidget( recentAddressesBtn, row, 1, 1, 2 );
    ++row;

    // Spacing
    groupGridLayout->setRowMinimumHeight( row, KDialog::spacingHint() );
    ++row;

    // "Configure Completion Order" button
    QPushButton *completionOrderBtn = new QPushButton( i18n( "Configure Completion Order..." ), this );
    helpText = i18n( "Configure the order in which address books\n"
                     "will be used when doing address completion" );
    completionOrderBtn->setToolTip( helpText );
    completionOrderBtn->setWhatsThis( helpText );

    connect( completionOrderBtn, SIGNAL(clicked()),
             this, SLOT(slotConfigureCompletionOrder()) );
    groupGridLayout->addWidget( completionOrderBtn, row, 1, 1, 2 );

    groupBox->setLayout( groupGridLayout );
    vb2->addWidget( groupBox );

    // Finish up main layout
    vb1->addStretch( 1 );
    vb2->addStretch( 1 );

    hb1->addLayout( vb1 );
    hb1->addLayout( vb2 );
    setLayout( hb1 );
}


void ComposerPage::GeneralTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );

    const bool autoAppSignFile = MessageComposer::MessageComposerSettings::self()->autoTextSignature()==QLatin1String( "auto" );
    const bool topQuoteCheck = MessageComposer::MessageComposerSettings::self()->prependSignature();
    const bool dashDashSignature = MessageComposer::MessageComposerSettings::self()->dashDashSignature();
    const bool smartQuoteCheck = MessageComposer::MessageComposerSettings::self()->quoteSelectionOnly();
    const bool wordWrap = MessageComposer::MessageComposerSettings::self()->wordWrap();
    const int wrapColumn = MessageComposer::MessageComposerSettings::self()->lineWrapWidth();
    const bool showRecentAddress = MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposer();
    const int maximumRecipient = MessageComposer::MessageComposerSettings::self()->maximumRecipients();
    const bool improvePlainText = MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessage();

    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );

    mAutoAppSignFileCheck->setChecked( autoAppSignFile );
    mTopQuoteCheck->setChecked( topQuoteCheck );
    mDashDashCheck->setChecked( dashDashSignature );
    mQuoteSelectionOnlyCheck->setChecked( smartQuoteCheck );
    mWordWrapCheck->setChecked( wordWrap );
    mWrapColumnSpin->setValue( wrapColumn );
    mMaximumRecipients->setValue( maximumRecipient );
    mShowRecentAddressesInComposer->setChecked( showRecentAddress );
    mImprovePlainTextOfHtmlMessage->setChecked(improvePlainText);

    mMaximumRecentAddress->setValue( 40 );
}

void ComposerPage::GeneralTab::doLoadFromGlobalSettings()
{
    // various check boxes:

    mAutoAppSignFileCheck->setChecked(
                MessageComposer::MessageComposerSettings::self()->autoTextSignature()==QLatin1String( "auto" ) );
    mTopQuoteCheck->setChecked( MessageComposer::MessageComposerSettings::self()->prependSignature() );
    mDashDashCheck->setChecked( MessageComposer::MessageComposerSettings::self()->dashDashSignature() );
    mSmartQuoteCheck->setChecked( TemplateParser::GlobalSettings::self()->smartQuote() );
    mQuoteSelectionOnlyCheck->setChecked( MessageComposer::MessageComposerSettings::self()->quoteSelectionOnly() );
    mReplyUsingHtml->setChecked( TemplateParser::GlobalSettings::self()->replyUsingHtml() );
    mStripSignatureCheck->setChecked( TemplateParser::GlobalSettings::self()->stripSignature() );
    mAutoRequestMDNCheck->setChecked( GlobalSettings::self()->requestMDN() );
    mWordWrapCheck->setChecked( MessageComposer::MessageComposerSettings::self()->wordWrap() );
    mWrapColumnSpin->setValue( MessageComposer::MessageComposerSettings::self()->lineWrapWidth() );
    mMaximumRecipients->setValue( MessageComposer::MessageComposerSettings::self()->maximumRecipients() );
    mAutoSave->setValue( GlobalSettings::self()->autosaveInterval() );
    mShowRecentAddressesInComposer->setChecked( MessageComposer::MessageComposerSettings::self()->showRecentAddressesInComposer() );
    mImprovePlainTextOfHtmlMessage->setChecked(MessageComposer::MessageComposerSettings::self()->improvePlainTextOfHtmlMessage());

#ifdef KDEPIM_ENTERPRISE_BUILD
    mRecipientCheck->setChecked( GlobalSettings::self()->tooManyRecipients() );
    mRecipientSpin->setValue( GlobalSettings::self()->recipientThreshold() );
    if ( GlobalSettings::self()->forwardingInlineByDefault() )
        mForwardTypeCombo->setCurrentIndex( 0 );
    else
        mForwardTypeCombo->setCurrentIndex( 1 );
#endif

    mMaximumRecentAddress->setValue(RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->maxCount());
}

void ComposerPage::GeneralTab::save() {
    MessageComposer::MessageComposerSettings::self()->setAutoTextSignature(
                mAutoAppSignFileCheck->isChecked() ? QLatin1String("auto") : QLatin1String("manual") );
    MessageComposer::MessageComposerSettings::self()->setPrependSignature( mTopQuoteCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setDashDashSignature( mDashDashCheck->isChecked() );
    TemplateParser::GlobalSettings::self()->setSmartQuote( mSmartQuoteCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setQuoteSelectionOnly( mQuoteSelectionOnlyCheck->isChecked() );
    TemplateParser::GlobalSettings::self()->setReplyUsingHtml( mReplyUsingHtml->isChecked() );
    TemplateParser::GlobalSettings::self()->setStripSignature( mStripSignatureCheck->isChecked() );
    GlobalSettings::self()->setRequestMDN( mAutoRequestMDNCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setWordWrap( mWordWrapCheck->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setLineWrapWidth( mWrapColumnSpin->value() );
    MessageComposer::MessageComposerSettings::self()->setMaximumRecipients( mMaximumRecipients->value() );
    GlobalSettings::self()->setAutosaveInterval( mAutoSave->value() );
    MessageComposer::MessageComposerSettings::self()->setShowRecentAddressesInComposer( mShowRecentAddressesInComposer->isChecked() );
    MessageComposer::MessageComposerSettings::self()->setImprovePlainTextOfHtmlMessage( mImprovePlainTextOfHtmlMessage->isChecked() );
#ifdef KDEPIM_ENTERPRISE_BUILD
    GlobalSettings::self()->setTooManyRecipients( mRecipientCheck->isChecked() );
    GlobalSettings::self()->setRecipientThreshold( mRecipientSpin->value() );
    GlobalSettings::self()->setForwardingInlineByDefault( mForwardTypeCombo->currentIndex() == 0 );
#endif

    RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->setMaxCount( mMaximumRecentAddress->value() );

    MessageComposer::MessageComposerSettings::self()->requestSync();
}

void ComposerPage::GeneralTab::slotConfigureRecentAddresses()
{
    MessageViewer::AutoQPointer<KPIM::RecentAddressDialog> dlg( new KPIM::RecentAddressDialog( this ) );
    dlg->setAddresses( RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->addresses() );
    if ( dlg->exec() && dlg ) {
        RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->clear();
        const QStringList &addrList = dlg->addresses();
        QStringList::ConstIterator it;
        QStringList::ConstIterator end( addrList.constEnd() );

        for ( it = addrList.constBegin(); it != end; ++it )
            RecentAddresses::self(  MessageComposer::MessageComposerSettings::self()->config() )->add( *it );
    }
}

void ComposerPage::GeneralTab::slotConfigureCompletionOrder()
{
    KLDAP::LdapClientSearch search;
    KPIM::CompletionOrderEditor editor( &search, this );
    editor.exec();
}

QString ComposerPage::ExternalEditorTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-externaleditor");
}

ComposerPageExternalEditorTab::ComposerPageExternalEditorTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *layout = new QVBoxLayout( this );

    mExternalEditorCheck = new QCheckBox(
                GlobalSettings::self()->useExternalEditorItem()->label(), this);
    mExternalEditorCheck->setObjectName( QLatin1String("kcfg_UseExternalEditor") );
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             this, SLOT(slotEmitChanged()) );

    KHBox *hbox = new KHBox( this );
    QLabel *label = new QLabel( GlobalSettings::self()->externalEditorItem()->label(),
                                hbox );
    mEditorRequester = new KUrlRequester( hbox );
    //Laurent 25/10/2011 fix #Bug 256655 - A "save changes?" dialog appears ALWAYS when leaving composer settings, even when unchanged.
    //mEditorRequester->setObjectName( "kcfg_ExternalEditor" );
    connect( mEditorRequester, SIGNAL(urlSelected(KUrl)),
             this, SLOT(slotEmitChanged()) );
    connect( mEditorRequester, SIGNAL(textChanged(QString)),
             this, SLOT(slotEmitChanged()) );

    hbox->setStretchFactor( mEditorRequester, 1 );
    label->setBuddy( mEditorRequester );
    label->setEnabled( false ); // since !mExternalEditorCheck->isChecked()
    // ### FIXME: allow only executables (x-bit when available..)
    mEditorRequester->setFilter( QLatin1String("application/x-executable "
                                               "application/x-shellscript "
                                               "application/x-desktop") );
    mEditorRequester->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    mEditorRequester->setEnabled( false ); // !mExternalEditorCheck->isChecked()
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             mEditorRequester, SLOT(setEnabled(bool)) );

    label = new QLabel( i18n("<b>%f</b> will be replaced with the "
                             "filename to edit.<br />"
                             "<b>%w</b> will be replaced with the window id.<br />"
                             "<b>%l</b> will be replaced with the line number."), this );
    label->setEnabled( false ); // see above
    connect( mExternalEditorCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    layout->addWidget( mExternalEditorCheck );
    layout->addWidget( hbox );
    layout->addWidget( label );
    layout->addStretch();
}

void ComposerPage::ExternalEditorTab::doLoadFromGlobalSettings()
{
    loadWidget(mExternalEditorCheck, GlobalSettings::self()->useExternalEditorItem() );
    loadWidget(mEditorRequester, GlobalSettings::self()->externalEditorItem() );
}

void ComposerPage::ExternalEditorTab::save()
{
    saveCheckBox(mExternalEditorCheck, GlobalSettings::self()->useExternalEditorItem() );
    saveUrlRequester(mEditorRequester, GlobalSettings::self()->externalEditorItem() );
}

QString ComposerPage::TemplatesTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-templates");
}

ComposerPageTemplatesTab::ComposerPageTemplatesTab( QWidget * parent )
    : ConfigModuleTab ( parent )
{
    QVBoxLayout* vlay = new QVBoxLayout( this );
    vlay->setMargin( 0 );
    vlay->setSpacing( KDialog::spacingHint() );

    mWidget = new TemplateParser::TemplatesConfiguration( this );
    vlay->addWidget( mWidget );

    connect( mWidget, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
}

void ComposerPage::TemplatesTab::doLoadFromGlobalSettings()
{
    mWidget->loadFromGlobal();
}

void ComposerPage::TemplatesTab::save()
{
    mWidget->saveToGlobal();
}

void ComposerPage::TemplatesTab::doResetToDefaultsOther()
{
    mWidget->resetToDefault();
}

QString ComposerPage::CustomTemplatesTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-custom-templates");
}

ComposerPageCustomTemplatesTab::ComposerPageCustomTemplatesTab( QWidget * parent )
    : ConfigModuleTab ( parent )
{
    QVBoxLayout* vlay = new QVBoxLayout( this );
    vlay->setMargin( 0 );
    vlay->setSpacing( KDialog::spacingHint() );

    mWidget = new TemplateParser::CustomTemplates( kmkernel->getKMMainWidget() ? kmkernel->getKMMainWidget()->actionCollections() : QList<KActionCollection*>(), this );
    vlay->addWidget( mWidget );

    connect( mWidget, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
    if( KMKernel::self() )
        connect( mWidget, SIGNAL(templatesUpdated()), KMKernel::self(), SLOT(updatedTemplates()) );
}

void ComposerPage::CustomTemplatesTab::doLoadFromGlobalSettings()
{
    mWidget->load();
}

void ComposerPage::CustomTemplatesTab::save()
{
    mWidget->save();
}

QString ComposerPage::SubjectTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-subject");
}

ComposerPageSubjectTab::ComposerPageSubjectTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    QGroupBox   *group = new QGroupBox( i18n("Repl&y Subject Prefixes"), this );
    QLayout *layout = new QVBoxLayout( group );
    group->layout()->setSpacing( KDialog::spacingHint() );

    // row 0: help text:
    QLabel *label = new QLabel( i18n("Recognize any sequence of the following prefixes\n"
                             "(entries are case-insensitive regular expressions):"), group );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignLeft );

    // row 1, string list editor:
    PimCommon::SimpleStringListEditor::ButtonCode buttonCode =
            static_cast<PimCommon::SimpleStringListEditor::ButtonCode>( PimCommon::SimpleStringListEditor::Add | PimCommon::SimpleStringListEditor::Remove | PimCommon::SimpleStringListEditor::Modify );
    mReplyListEditor =
            new PimCommon::SimpleStringListEditor( group, buttonCode,
                                                   i18n("A&dd..."), i18n("Re&move"),
                                                   i18n("Mod&ify..."),
                                                   i18n("Enter new reply prefix:") );
    connect( mReplyListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    // row 2: "replace [...]" check box:
    mReplaceReplyPrefixCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->replaceReplyPrefixItem()->label(),
                group);
    connect( mReplaceReplyPrefixCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    layout->addWidget( label );
    layout->addWidget( mReplyListEditor );
    layout->addWidget( mReplaceReplyPrefixCheck );

    vlay->addWidget( group );


    group = new QGroupBox( i18n("For&ward Subject Prefixes"), this );
    layout = new QVBoxLayout( group );
    group->layout()->setSpacing( KDialog::marginHint() );

    // row 0: help text:
    label= new QLabel( i18n("Recognize any sequence of the following prefixes\n"
                            "(entries are case-insensitive regular expressions):"), group );
    label->setAlignment( Qt::AlignLeft );
    label->setWordWrap( true );

    // row 1: string list editor
    mForwardListEditor =
            new PimCommon::SimpleStringListEditor( group, buttonCode,
                                                   i18n("Add..."),
                                                   i18n("Remo&ve"),
                                                   i18n("Modify..."),
                                                   i18n("Enter new forward prefix:") );
    connect( mForwardListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    // row 3: "replace [...]" check box:
    mReplaceForwardPrefixCheck = new QCheckBox(
                MessageComposer::MessageComposerSettings::self()->replaceForwardPrefixItem()->label(),
                group);
    connect( mReplaceForwardPrefixCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    layout->addWidget( label );
    layout->addWidget( mForwardListEditor );
    layout->addWidget( mReplaceForwardPrefixCheck );
    vlay->addWidget( group );
}

void ComposerPage::SubjectTab::doLoadFromGlobalSettings()
{
    mReplyListEditor->setStringList( MessageComposer::MessageComposerSettings::self()->replyPrefixes() );
    mForwardListEditor->setStringList( MessageComposer::MessageComposerSettings::self()->forwardPrefixes() );
    loadWidget(mReplaceForwardPrefixCheck, MessageComposer::MessageComposerSettings::self()->replaceForwardPrefixItem());
    loadWidget(mReplaceReplyPrefixCheck, MessageComposer::MessageComposerSettings::self()->replaceReplyPrefixItem());
}

void ComposerPage::SubjectTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setReplyPrefixes( mReplyListEditor->stringList() );
    MessageComposer::MessageComposerSettings::self()->setForwardPrefixes( mForwardListEditor->stringList() );
    saveCheckBox(mReplaceForwardPrefixCheck, MessageComposer::MessageComposerSettings::self()->replaceForwardPrefixItem());
    saveCheckBox(mReplaceReplyPrefixCheck, MessageComposer::MessageComposerSettings::self()->replaceReplyPrefixItem());
}

void ComposerPage::SubjectTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    const QStringList messageReplyPrefixes = MessageComposer::MessageComposerSettings::replyPrefixes();

    const QStringList messageForwardPrefixes = MessageComposer::MessageComposerSettings::forwardPrefixes();

    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );
    mReplyListEditor->setStringList( messageReplyPrefixes );
    mReplaceReplyPrefixCheck->setChecked( MessageComposer::MessageComposerSettings::replaceReplyPrefix() );
    mForwardListEditor->setStringList( messageForwardPrefixes );
    mReplaceForwardPrefixCheck->setChecked( MessageComposer::MessageComposerSettings::replaceForwardPrefix() );
}


QString ComposerPage::CharsetTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-charset");
}

ComposerPageCharsetTab::ComposerPageCharsetTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    QLabel *label = new QLabel( i18n("This list is checked for every outgoing message "
                             "from the top to the bottom for a charset that "
                             "contains all required characters."), this );
    label->setWordWrap(true);
    vlay->addWidget( label );

    mCharsetListEditor =
            new PimCommon::SimpleStringListEditor( this, PimCommon::SimpleStringListEditor::All,
                                                   i18n("A&dd..."), i18n("Remo&ve"),
                                                   i18n("&Modify..."), i18n("Enter charset:") );
    mCharsetListEditor->setUpDownAutoRepeat(true);
    connect( mCharsetListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );

    vlay->addWidget( mCharsetListEditor, 1 );

    mKeepReplyCharsetCheck = new QCheckBox( i18n("&Keep original charset when "
                                                 "replying or forwarding (if "
                                                 "possible)"), this );
    connect( mKeepReplyCharsetCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mKeepReplyCharsetCheck );

    connect( mCharsetListEditor, SIGNAL(aboutToAdd(QString&)),
             this, SLOT(slotVerifyCharset(QString&)) );
}

void ComposerPage::CharsetTab::slotVerifyCharset( QString & charset )
{
    if ( charset.isEmpty() ) return;

    // KCharsets::codecForName("us-ascii") returns "iso-8859-1" (cf. Bug #49812)
    // therefore we have to treat this case specially
    if ( charset.toLower() == QString::fromLatin1("us-ascii") ) {
        charset = QString::fromLatin1("us-ascii");
        return;
    }

    if ( charset.toLower() == QString::fromLatin1("locale") ) {
        charset =  QString::fromLatin1("%1 (locale)")
                .arg( QString::fromLatin1(kmkernel->networkCodec()->name() ).toLower() );
        return;
    }

    bool ok = false;
    QTextCodec *codec = KGlobal::charsets()->codecForName( charset, ok );
    if ( ok && codec ) {
        charset = QString::fromLatin1( codec->name() ).toLower();
        return;
    }

    KMessageBox::sorry( this, i18n("This charset is not supported.") );
    charset.clear();
}

void ComposerPage::CharsetTab::doLoadOther()
{
    QStringList charsets = MessageComposer::MessageComposerSettings::preferredCharsets();
    QStringList::Iterator end( charsets.end() );
    for ( QStringList::Iterator it = charsets.begin() ;
          it != end ; ++it )
        if ( (*it) == QString::fromLatin1("locale") ) {
            QByteArray cset = kmkernel->networkCodec()->name();
            kAsciiToLower( cset.data() );
            (*it) = QString::fromLatin1("%1 (locale)").arg( QString::fromLatin1( cset ) );
        }

    mCharsetListEditor->setStringList( charsets );
    mKeepReplyCharsetCheck->setChecked( MessageComposer::MessageComposerSettings::forceReplyCharset() );
}


void ComposerPage::CharsetTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    mCharsetListEditor->setStringList( MessageComposer::MessageComposerSettings::preferredCharsets());
    mKeepReplyCharsetCheck->setChecked( MessageComposer::MessageComposerSettings::forceReplyCharset() );
    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );
    slotEmitChanged();
}

void ComposerPage::CharsetTab::save()
{
    QStringList charsetList = mCharsetListEditor->stringList();
    QStringList::Iterator it = charsetList.begin();
    QStringList::Iterator end = charsetList.end();

    for ( ; it != end ; ++it )
        if ( (*it).endsWith( QLatin1String("(locale)") ) )
            (*it) = QLatin1String("locale");
    MessageComposer::MessageComposerSettings::setPreferredCharsets( charsetList );
    MessageComposer::MessageComposerSettings::setForceReplyCharset( mKeepReplyCharsetCheck->isChecked() );
}

QString ComposerPage::HeadersTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-headers");
}

ComposerPageHeadersTab::ComposerPageHeadersTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Use custom Message-Id suffix" checkbox:
    mCreateOwnMessageIdCheck =
            new QCheckBox( i18n("&Use custom message-id suffix"), this );
    connect( mCreateOwnMessageIdCheck, SIGNAL (stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mCreateOwnMessageIdCheck );

    // "Message-Id suffix" line edit and label:
    QHBoxLayout *hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mMessageIdSuffixEdit = new KLineEdit( this );
    mMessageIdSuffixEdit->setClearButtonShown( true );
    // only ASCII letters, digits, plus, minus and dots are allowed
    QRegExpValidator *messageIdSuffixValidator =
            new QRegExpValidator( QRegExp( QLatin1String("[a-zA-Z0-9+-]+(?:\\.[a-zA-Z0-9+-]+)*") ), this );
    mMessageIdSuffixEdit->setValidator( messageIdSuffixValidator );
    QLabel *label = new QLabel(i18n("Custom message-&id suffix:"), this );
    label->setBuddy( mMessageIdSuffixEdit );
    label->setEnabled( false ); // since !mCreateOwnMessageIdCheck->isChecked()
    mMessageIdSuffixEdit->setEnabled( false );
    hlay->addWidget( label );
    hlay->addWidget( mMessageIdSuffixEdit, 1 );
    connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mCreateOwnMessageIdCheck, SIGNAL(toggled(bool)),
             mMessageIdSuffixEdit, SLOT(setEnabled(bool)) );
    connect( mMessageIdSuffixEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotEmitChanged()) );

    // horizontal rule and "custom header fields" label:
    vlay->addWidget( new KSeparator( Qt::Horizontal, this ) );
    vlay->addWidget( new QLabel( i18n("Define custom mime header fields:"), this) );

    // "custom header fields" listbox:
    QGridLayout *glay = new QGridLayout(); // inherits spacing
    vlay->addLayout( glay );
    glay->setRowStretch( 2, 1 );
    glay->setColumnStretch( 1, 1 );
    mHeaderList = new ListView( this );
    mHeaderList->setHeaderLabels( QStringList() << i18nc("@title:column Name of the mime header.","Name")
                                  << i18nc("@title:column Value of the mimeheader.","Value") );
    mHeaderList->setSortingEnabled( false );
    connect( mHeaderList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
             this, SLOT(slotMimeHeaderSelectionChanged()) );
    connect( mHeaderList, SIGNAL(addHeader()), SLOT(slotNewMimeHeader()));
    connect( mHeaderList, SIGNAL(removeHeader()), SLOT(slotRemoveMimeHeader()));
    glay->addWidget( mHeaderList, 0, 0, 3, 2 );

    // "new" and "remove" buttons:
    QPushButton *button = new QPushButton( i18nc("@action:button Add new mime header field.","Ne&w"), this );
    connect( button, SIGNAL(clicked()), this, SLOT(slotNewMimeHeader()) );
    button->setAutoDefault( false );
    glay->addWidget( button, 0, 2 );
    mRemoveHeaderButton = new QPushButton( i18n("Re&move"), this );
    connect( mRemoveHeaderButton, SIGNAL(clicked()),
             this, SLOT(slotRemoveMimeHeader()) );
    button->setAutoDefault( false );
    glay->addWidget( mRemoveHeaderButton, 1, 2 );

    // "name" and "value" line edits and labels:
    mTagNameEdit = new KLineEdit( this );
    mTagNameEdit->setClearButtonShown(true);
    mTagNameEdit->setEnabled( false );
    mTagNameLabel = new QLabel( i18nc("@label:textbox Name of the mime header.","&Name:"), this );
    mTagNameLabel->setBuddy( mTagNameEdit );
    mTagNameLabel->setEnabled( false );
    glay->addWidget( mTagNameLabel, 3, 0 );
    glay->addWidget( mTagNameEdit, 3, 1 );
    connect( mTagNameEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotMimeHeaderNameChanged(QString)) );

    mTagValueEdit = new KLineEdit( this );
    mTagValueEdit->setClearButtonShown(true);
    mTagValueEdit->setEnabled( false );
    mTagValueLabel = new QLabel( i18n("&Value:"), this );
    mTagValueLabel->setBuddy( mTagValueEdit );
    mTagValueLabel->setEnabled( false );
    glay->addWidget( mTagValueLabel, 4, 0 );
    glay->addWidget( mTagValueEdit, 4, 1 );
    connect( mTagValueEdit, SIGNAL(textChanged(QString)),
             this, SLOT(slotMimeHeaderValueChanged(QString)) );
}

void ComposerPage::HeadersTab::slotMimeHeaderSelectionChanged()
{
    mEmitChanges = false;
    QTreeWidgetItem * item = mHeaderList->currentItem();

    if ( item ) {
        mTagNameEdit->setText( item->text( 0 ) );
        mTagValueEdit->setText( item->text( 1 ) );
    } else {
        mTagNameEdit->clear();
        mTagValueEdit->clear();
    }
    mRemoveHeaderButton->setEnabled( item );
    mTagNameEdit->setEnabled( item );
    mTagValueEdit->setEnabled( item );
    mTagNameLabel->setEnabled( item );
    mTagValueLabel->setEnabled( item );
    mEmitChanges = true;
}


void ComposerPage::HeadersTab::slotMimeHeaderNameChanged( const QString & text )
{
    // is called on ::setup(), when clearing the line edits. So be
    // prepared to not find a selection:
    QTreeWidgetItem * item = mHeaderList->currentItem();
    if ( item )
        item->setText( 0, text );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotMimeHeaderValueChanged( const QString & text )
{
    // is called on ::setup(), when clearing the line edits. So be
    // prepared to not find a selection:
    QTreeWidgetItem * item = mHeaderList->currentItem();
    if ( item )
        item->setText( 1, text );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotNewMimeHeader()
{
    QTreeWidgetItem *listItem = new QTreeWidgetItem( mHeaderList );
    mHeaderList->setCurrentItem( listItem );
    slotEmitChanged();
}


void ComposerPage::HeadersTab::slotRemoveMimeHeader()
{
    // calling this w/o selection is a programming error:
    QTreeWidgetItem *item = mHeaderList->currentItem();
    if ( !item ) {
        kDebug() << "=================================================="
                 << "Error: Remove button was pressed although no custom header was selected\n"
                 << "==================================================\n";
        return;
    }

    QTreeWidgetItem *below = mHeaderList->itemBelow( item );

    if ( below ) {
        kDebug() << "below";
        mHeaderList->setCurrentItem( below );
        delete item;
        item = 0;
    } else if ( mHeaderList->topLevelItemCount() > 0 ) {
        delete item;
        item = 0;
        mHeaderList->setCurrentItem(
                    mHeaderList->topLevelItem( mHeaderList->topLevelItemCount() - 1 )
                    );
    }

    slotEmitChanged();
}

void ComposerPage::HeadersTab::doLoadOther()
{
    mMessageIdSuffixEdit->setText( MessageComposer::MessageComposerSettings::customMsgIDSuffix() );
    const bool state = ( !MessageComposer::MessageComposerSettings::customMsgIDSuffix().isEmpty() &&
                         MessageComposer::MessageComposerSettings::useCustomMessageIdSuffix() );
    mCreateOwnMessageIdCheck->setChecked( state );

    mHeaderList->clear();
    mTagNameEdit->clear();
    mTagValueEdit->clear();

    QTreeWidgetItem * item = 0;

    const int count = GlobalSettings::self()->customMessageHeadersCount();
    for ( int i = 0 ; i < count ; ++i ) {
        KConfigGroup config( KMKernel::self()->config(),
                             QLatin1String("Mime #") + QString::number(i) );
        const QString name  = config.readEntry( "name" );
        const QString value = config.readEntry( "value" );
        if( !name.isEmpty() ) {
            item = new QTreeWidgetItem( mHeaderList, item );
            item->setText( 0, name );
            item->setText( 1, value );
        }
    }
    if ( mHeaderList->topLevelItemCount() > 0 ) {
        mHeaderList->setCurrentItem( mHeaderList->topLevelItem( 0 ) );
    }
    else {
        // disable the "Remove" button
        mRemoveHeaderButton->setEnabled( false );
    }
}

void ComposerPage::HeadersTab::save()
{
    MessageComposer::MessageComposerSettings::self()->setCustomMsgIDSuffix( mMessageIdSuffixEdit->text() );
    MessageComposer::MessageComposerSettings::self()->setUseCustomMessageIdSuffix( mCreateOwnMessageIdCheck->isChecked() );

    //Clean config
    const int oldHeadersCount = GlobalSettings::self()->customMessageHeadersCount();
    for ( int i = 0; i < oldHeadersCount; ++i ) {
        const QString groupMimeName = QString::fromLatin1( "Mime #%1" ).arg( i );
        if ( KMKernel::self()->config()->hasGroup( groupMimeName ) ) {
            KConfigGroup config( KMKernel::self()->config(), groupMimeName);
            config.deleteGroup();
        }
    }


    int numValidEntries = 0;
    QTreeWidgetItem *item = 0;
    const int numberOfEntry = mHeaderList->topLevelItemCount();
    for ( int i = 0; i < numberOfEntry; ++i ) {
        item = mHeaderList->topLevelItem( i );
        if( !item->text(0).isEmpty() ) {
            KConfigGroup config( KMKernel::self()->config(), QString::fromLatin1("Mime #%1").arg( numValidEntries ) );
            config.writeEntry( "name",  item->text( 0 ) );
            config.writeEntry( "value", item->text( 1 ) );
            numValidEntries++;
        }
    }
    GlobalSettings::self()->setCustomMessageHeadersCount( numValidEntries );
}

void ComposerPage::HeadersTab::doResetToDefaultsOther()
{
    const bool bUseDefaults = MessageComposer::MessageComposerSettings::self()->useDefaults( true );
    const QString messageIdSuffix = MessageComposer::MessageComposerSettings::customMsgIDSuffix();
    const bool useCustomMessageIdSuffix = MessageComposer::MessageComposerSettings::useCustomMessageIdSuffix();
    MessageComposer::MessageComposerSettings::self()->useDefaults( bUseDefaults );

    mMessageIdSuffixEdit->setText( messageIdSuffix );
    const bool state = ( !messageIdSuffix.isEmpty() && useCustomMessageIdSuffix );
    mCreateOwnMessageIdCheck->setChecked( state );

    mHeaderList->clear();
    mTagNameEdit->clear();
    mTagValueEdit->clear();
    // disable the "Remove" button
    mRemoveHeaderButton->setEnabled( false );
}

QString ComposerPage::AttachmentsTab::helpAnchor() const
{
    return QString::fromLatin1("configure-composer-attachments");
}

ComposerPageAttachmentsTab::ComposerPageAttachmentsTab( QWidget * parent )
    : ConfigModuleTab( parent )
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( KDialog::marginHint() );

    // "Outlook compatible attachment naming" check box
    mOutlookCompatibleCheck =
            new QCheckBox( i18n( "Outlook-compatible attachment naming" ), this );
    mOutlookCompatibleCheck->setChecked( false );
    mOutlookCompatibleCheck->setToolTip( i18n(
                                             "Turn this option on to make Outlook(tm) understand attachment names "
                                             "containing non-English characters" ) );
    connect( mOutlookCompatibleCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    connect( mOutlookCompatibleCheck, SIGNAL(clicked()),
             this, SLOT(slotOutlookCompatibleClicked()) );
    vlay->addWidget( mOutlookCompatibleCheck );
    vlay->addSpacing( 5 );

    // "Enable detection of missing attachments" check box
    mMissingAttachmentDetectionCheck =
            new QCheckBox( i18n("E&nable detection of missing attachments"), this );
    mMissingAttachmentDetectionCheck->setChecked( true );
    connect( mMissingAttachmentDetectionCheck, SIGNAL(stateChanged(int)),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mMissingAttachmentDetectionCheck );


    QHBoxLayout * layAttachment = new QHBoxLayout;
    QLabel *label = new QLabel( i18n("Warn when inserting attachments larger than:"), this );
    label->setAlignment( Qt::AlignLeft );
    layAttachment->addWidget(label);

    mMaximumAttachmentSize = new KIntNumInput( this );
    mMaximumAttachmentSize->setRange( -1, 99999 );
    mMaximumAttachmentSize->setSingleStep( 100 );
    mMaximumAttachmentSize->setSuffix(i18nc("spinbox suffix: unit for kilobyte", " kB"));
    connect( mMaximumAttachmentSize, SIGNAL(valueChanged(int)),
             this, SLOT(slotEmitChanged()) );
    mMaximumAttachmentSize->setSpecialValueText(i18n("No limit"));
    layAttachment->addWidget(mMaximumAttachmentSize);
    vlay->addLayout(layAttachment);


    // "Attachment key words" label and string list editor
    label = new QLabel( i18n("Recognize any of the following key words as "
                             "intention to attach a file:"), this );
    label->setAlignment( Qt::AlignLeft );
    label->setWordWrap( true );

    vlay->addWidget( label );

    PimCommon::SimpleStringListEditor::ButtonCode buttonCode =
            static_cast<PimCommon::SimpleStringListEditor::ButtonCode>( PimCommon::SimpleStringListEditor::Add | PimCommon::SimpleStringListEditor::Remove | PimCommon::SimpleStringListEditor::Modify );
    mAttachWordsListEditor =
            new PimCommon::SimpleStringListEditor( this, buttonCode,
                                                   i18n("A&dd..."), i18n("Re&move"),
                                                   i18n("Mod&ify..."),
                                                   i18n("Enter new key word:") );
    connect( mAttachWordsListEditor, SIGNAL(changed()),
             this, SLOT(slotEmitChanged()) );
    vlay->addWidget( mAttachWordsListEditor );

    connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    connect( mMissingAttachmentDetectionCheck, SIGNAL(toggled(bool)),
             mAttachWordsListEditor, SLOT(setEnabled(bool)) );
}

void ComposerPage::AttachmentsTab::doLoadFromGlobalSettings()
{
    loadWidget(mOutlookCompatibleCheck, MessageComposer::MessageComposerSettings::self()->outlookCompatibleAttachmentsItem());
    loadWidget(mMissingAttachmentDetectionCheck, GlobalSettings::self()->showForgottenAttachmentWarningItem());

    const QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();
    mAttachWordsListEditor->setStringList( attachWordsList );
    const int maximumAttachmentSize(MessageComposer::MessageComposerSettings::self()->maximumAttachmentSize());
    mMaximumAttachmentSize->setValue(maximumAttachmentSize == -1 ? -1 : MessageComposer::MessageComposerSettings::self()->maximumAttachmentSize()/1024);
}

void ComposerPage::AttachmentsTab::save()
{
    saveCheckBox(mOutlookCompatibleCheck, MessageComposer::MessageComposerSettings::self()->outlookCompatibleAttachmentsItem());
    saveCheckBox(mMissingAttachmentDetectionCheck, GlobalSettings::self()->showForgottenAttachmentWarningItem());
    GlobalSettings::self()->setAttachmentKeywords(
                mAttachWordsListEditor->stringList() );

    KMime::setUseOutlookAttachmentEncoding( mOutlookCompatibleCheck->isChecked() );
    const int maximumAttachmentSize(mMaximumAttachmentSize->value());
    MessageComposer::MessageComposerSettings::self()->setMaximumAttachmentSize(maximumAttachmentSize == -1 ? -1 : maximumAttachmentSize*1024);

}

void ComposerPageAttachmentsTab::slotOutlookCompatibleClicked()
{
    if (mOutlookCompatibleCheck->isChecked()) {
        KMessageBox::information(0,i18n("You have chosen to "
                                        "encode attachment names containing non-English characters in a way that "
                                        "is understood by Outlook(tm) and other mail clients that do not "
                                        "support standard-compliant encoded attachment names.\n"
                                        "Note that KMail may create non-standard compliant messages, "
                                        "and consequently it is possible that your messages will not be "
                                        "understood by standard-compliant mail clients; so, unless you have no "
                                        "other choice, you should not enable this option." ) );
    }
}

ComposerPageAutoCorrectionTab::ComposerPageAutoCorrectionTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( 0 );
    vlay->setMargin( 0 );
    autocorrectionWidget = new PimCommon::ComposerAutoCorrectionWidget(this);
    if(KMKernel::self())
        autocorrectionWidget->setAutoCorrection(KMKernel::self()->composerAutoCorrection());
    vlay->addWidget(autocorrectionWidget);
    setLayout(vlay);
    connect( autocorrectionWidget, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );

}

QString ComposerPageAutoCorrectionTab::helpAnchor() const
{
    return QString::fromLatin1("configure-autocorrection");
}

void ComposerPageAutoCorrectionTab::save()
{
    autocorrectionWidget->writeConfig();
}

void ComposerPageAutoCorrectionTab::doLoadFromGlobalSettings()
{
    autocorrectionWidget->loadConfig();
}

void ComposerPageAutoCorrectionTab::doResetToDefaultsOther()
{
    autocorrectionWidget->resetToDefault();
}


ComposerPageAutoImageResizeTab::ComposerPageAutoImageResizeTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QVBoxLayout *vlay = new QVBoxLayout( this );
    vlay->setSpacing( 0 );
    vlay->setMargin( 0 );
    autoResizeWidget = new MessageComposer::ImageScalingWidget(this);
    vlay->addWidget(autoResizeWidget);
    setLayout(vlay);
    connect( autoResizeWidget, SIGNAL(changed()), this, SLOT(slotEmitChanged()) );

}

QString ComposerPageAutoImageResizeTab::helpAnchor() const
{
    return QString::fromLatin1("configure-image-resize");
}

void ComposerPageAutoImageResizeTab::save()
{
    autoResizeWidget->writeConfig();
}

void ComposerPageAutoImageResizeTab::doLoadFromGlobalSettings()
{
    autoResizeWidget->loadConfig();
}

void ComposerPageAutoImageResizeTab::doResetToDefaultsOther()
{
    autoResizeWidget->resetToDefault();
}
