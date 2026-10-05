/*
  SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "config-kmail.h"
#include "configuredialog_p.h"

#include <QWidget>
namespace TextEditTextToSpeech
{
class TextToSpeechConfigWidget;
}
namespace TextSpeechToText
{
class SpeechToTextConfigureWidget;
}
class QCheckBox;

#if HAVE_TEXT_TO_SPEECH_SUPPORT
class AccessibilityPageTextToSpeechTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccessibilityPageTextToSpeechTab(QWidget *parent = nullptr);
    ~AccessibilityPageTextToSpeechTab() override;

    void save() override;

private:
    void doLoadOther() override;
    void doResetToDefaultsOther() override;
    TextEditTextToSpeech::TextToSpeechConfigWidget *const mTextToSpeechWidget;
};
#endif

#if HAVE_SPEECH_TO_TEXT
class AccessibilityPageSpeechToTextTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit AccessibilityPageSpeechToTextTab(QWidget *parent = nullptr);
    ~AccessibilityPageSpeechToTextTab() override;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doLoadOther() override;
    TextSpeechToText::SpeechToTextConfigureWidget *const mSpeechToTextWidget;
    QCheckBox *const mEnableSpeechToText;
};
#endif

class KMAIL_EXPORT ConfigureAccessibilityPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit ConfigureAccessibilityPage(QObject *parent, const KPluginMetaData &data);
    ~ConfigureAccessibilityPage() override;

    [[nodiscard]] QString helpAnchor() const override;
};
