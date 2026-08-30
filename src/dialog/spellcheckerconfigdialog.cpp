/*
   SPDX-FileCopyrightText: 2019-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "spellcheckerconfigdialog.h"
#include "kmail_debug.h"
#include "kmkernel.h"
#include <QCheckBox>
#include <QLabel>

#include <Sonnet/DictionaryComboBox>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QWindow>
#include <TextAddonsWidgets/LoadDialogSizeUtils>
using namespace Qt::Literals::StringLiterals;

SpellCheckerConfigDialog::SpellCheckerConfigDialog(QWidget *parent)
    : Sonnet::ConfigDialog(parent)
{
    // Hackish way to hide the "Enable spell check by default" checkbox
    // Our highlighter ignores this setting, so we should not expose its UI
    if (auto enabledByDefaultCB = findChild<QCheckBox *>(u"kcfg_autodetectLanguage"_s)) {
        enabledByDefaultCB->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any checkbox named 'm_checkerEnabledByDefaultCB'. Sonnet::ConfigDialog must have changed!";
    }
    if (auto textLabel = findChild<QLabel *>(u"textLabel1"_s)) {
        textLabel->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any label named 'textLabel'. Sonnet::ConfigDialog must have changed!";
    }
    if (auto dictionaryComboBox = findChild<Sonnet::DictionaryComboBox *>(u"m_langCombo"_s)) {
        dictionaryComboBox->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any Sonnet::DictionaryComboBox named 'dictionaryComboBox'. Sonnet::ConfigDialog must have changed!";
    }
    readConfig();
}

SpellCheckerConfigDialog::~SpellCheckerConfigDialog()
{
    writeConfig();
}

void SpellCheckerConfigDialog::readConfig()
{
    create(); // ensure a window is created
    TextAddonsWidgets::LoadDialogSizeUtils::loadDialogSizeScaled(this, u"SpellCheckerConfigDialog"_s, 600, 400);
}

void SpellCheckerConfigDialog::writeConfig()
{
    KConfigGroup notifyDialog(KSharedConfig::openStateConfig(), u"SpellCheckerConfigDialog"_s);
    KWindowConfig::saveWindowSize(windowHandle(), notifyDialog);
    notifyDialog.sync();
}

#include "moc_spellcheckerconfigdialog.cpp"
