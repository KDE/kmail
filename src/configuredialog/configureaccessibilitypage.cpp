/*
  SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configureaccessibilitypage.h"
#include <QHBoxLayout>
#include <QShowEvent>
#include <TextEditTextToSpeech/TextToSpeech>
#include <TextEditTextToSpeech/TextToSpeechConfigWidget>

ConfigureAccessibilityPage::ConfigureAccessibilityPage(QObject *parent, const KPluginMetaData &data)
    : ConfigModule(parent, data)
    , mTextToSpeechWidget(new TextEditTextToSpeech::TextToSpeechConfigWidget(widget()))
{
    auto l = new QHBoxLayout(widget());
    l->setContentsMargins({});
    l->addWidget(mTextToSpeechWidget);

    mTextToSpeechWidget->initializeSettings();
    connect(mTextToSpeechWidget, &TextEditTextToSpeech::TextToSpeechConfigWidget::configChanged, this, [this](bool state) {
        setNeedsSave(state);
    });
}

ConfigureAccessibilityPage::~ConfigureAccessibilityPage() = default;

void ConfigureAccessibilityPage::save()
{
    mTextToSpeechWidget->writeConfig();
    TextEditTextToSpeech::TextToSpeech::self()->reloadSettings();
}

void ConfigureAccessibilityPage::defaults()
{
    mTextToSpeechWidget->restoreDefaults();
}

QString ConfigureAccessibilityPage::helpAnchor() const
{
    return {};
}

void ConfigureAccessibilityPage::load()
{
    mTextToSpeechWidget->initializeSettings();
}

#include "moc_configureaccessibilitypage.cpp"
