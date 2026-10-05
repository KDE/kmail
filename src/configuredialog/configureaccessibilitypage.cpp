/*
  SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configureaccessibilitypage.h"
#include "settings/kmailsettings.h"

#include <KLocalizedString>
#include <QCheckBox>
#include <QVBoxLayout>
#if HAVE_TEXT_TO_SPEECH_SUPPORT
#include <TextEditTextToSpeech/TextToSpeech>
#include <TextEditTextToSpeech/TextToSpeechConfigWidget>
#endif
#if HAVE_SPEECH_TO_TEXT
#include <TextSpeechToText/SpeechToTextConfigureWidget>
#endif

using namespace Qt::Literals::StringLiterals;

ConfigureAccessibilityPage::ConfigureAccessibilityPage(QObject *parent, const KPluginMetaData &data)
    : ConfigModuleWithTabs(parent, data)
{
#if HAVE_TEXT_TO_SPEECH_SUPPORT
    auto textToSpeechTab = new AccessibilityPageTextToSpeechTab();
    addTab(textToSpeechTab, i18n("Text to Speech"));
#endif
#if HAVE_SPEECH_TO_TEXT
    auto speechToTextTab = new AccessibilityPageSpeechToTextTab();
    addTab(speechToTextTab, i18n("Speech to Text"));
#endif
}

ConfigureAccessibilityPage::~ConfigureAccessibilityPage() = default;

QString ConfigureAccessibilityPage::helpAnchor() const
{
    return {};
}

#if HAVE_TEXT_TO_SPEECH_SUPPORT
AccessibilityPageTextToSpeechTab::AccessibilityPageTextToSpeechTab(QWidget *parent)
    : ConfigModuleTab(parent)
    , mTextToSpeechWidget(new TextEditTextToSpeech::TextToSpeechConfigWidget(this))
{
    auto l = new QVBoxLayout(this);
    l->addWidget(mTextToSpeechWidget);

    mTextToSpeechWidget->initializeSettings();
    connect(mTextToSpeechWidget, &TextEditTextToSpeech::TextToSpeechConfigWidget::configChanged, this, [this](bool state) {
        if (state) {
            slotEmitChanged();
        }
    });
}

AccessibilityPageTextToSpeechTab::~AccessibilityPageTextToSpeechTab() = default;

void AccessibilityPageTextToSpeechTab::save()
{
    mTextToSpeechWidget->writeConfig();
    TextEditTextToSpeech::TextToSpeech::self()->reloadSettings();
}

void AccessibilityPageTextToSpeechTab::doLoadOther()
{
    mTextToSpeechWidget->initializeSettings();
}

void AccessibilityPageTextToSpeechTab::doResetToDefaultsOther()
{
    mTextToSpeechWidget->restoreDefaults();
}
#endif

#if HAVE_SPEECH_TO_TEXT
AccessibilityPageSpeechToTextTab::AccessibilityPageSpeechToTextTab(QWidget *parent)
    : ConfigModuleTab(parent)
    , mSpeechToTextWidget(new TextSpeechToText::SpeechToTextConfigureWidget(this))
    , mEnableSpeechToText(new QCheckBox(i18nc("@option:check", "Enable Speech To Text"), this))
{
    auto l = new QVBoxLayout(this);

    // Managed by KConfigDialogManager (load/save/defaults/changed)
    mEnableSpeechToText->setObjectName(u"kcfg_EnableSpeechToText"_s);
    l->addWidget(mEnableSpeechToText);

    mSpeechToTextWidget->setObjectName(u"mSpeechToTextWidget"_s);
    l->addWidget(mSpeechToTextWidget);

    connect(mEnableSpeechToText, &QCheckBox::toggled, mSpeechToTextWidget, &TextSpeechToText::SpeechToTextConfigureWidget::setEnabled);
}

AccessibilityPageSpeechToTextTab::~AccessibilityPageSpeechToTextTab() = default;

void AccessibilityPageSpeechToTextTab::save()
{
    mSpeechToTextWidget->saveSettings();
}

void AccessibilityPageSpeechToTextTab::doLoadFromGlobalSettings()
{
    mSpeechToTextWidget->setEnabled(KMailSettings::self()->enableSpeechToText());
}

void AccessibilityPageSpeechToTextTab::doLoadOther()
{
    mSpeechToTextWidget->loadSettings();
}
#endif

#include "moc_configureaccessibilitypage.cpp"
