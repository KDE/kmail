/*
  SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "configuredialog_p.h"

#include <QWidget>
namespace TextEditTextToSpeech
{
class TextToSpeechConfigWidget;
}
class KMAIL_EXPORT ConfigureAccessibilityPage : public ConfigModule
{
    Q_OBJECT
public:
    explicit ConfigureAccessibilityPage(QObject *parent, const KPluginMetaData &data);
    ~ConfigureAccessibilityPage() override;

    [[nodiscard]] QString helpAnchor() const override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    TextEditTextToSpeech::TextToSpeechConfigWidget *const mTextToSpeechWidget;
};
