/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "spellcheckerconfigdialog.h"
#include "kmail_debug.h"
#include "kmkernel.h"
#include <QCheckBox>
#include <QLabel>

#include <Sonnet/DictionaryComboBox>

#include <KConfigGroup>

SpellCheckerConfigDialog::SpellCheckerConfigDialog(QWidget *parent)
    : Sonnet::ConfigDialog(parent)
{
    // Hackish way to hide the "Enable spell check by default" checkbox
    // Our highlighter ignores this setting, so we should not expose its UI
    auto enabledByDefaultCB = findChild<QCheckBox *>(QStringLiteral("kcfg_autodetectLanguage"));
    if (enabledByDefaultCB) {
        enabledByDefaultCB->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any checkbox named 'm_checkerEnabledByDefaultCB'. Sonnet::ConfigDialog must have changed!";
    }
    auto textLabel = findChild<QLabel *>(QStringLiteral("textLabel1"));
    if (textLabel) {
        textLabel->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any label named 'textLabel'. Sonnet::ConfigDialog must have changed!";
    }
    auto dictionaryComboBox = findChild<Sonnet::DictionaryComboBox *>(QStringLiteral("m_langCombo"));
    if (dictionaryComboBox) {
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
    KConfigGroup notifyDialog(KMKernel::self()->config(), "KMKnotifyDialog");
    const QSize size = notifyDialog.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void SpellCheckerConfigDialog::writeConfig()
{
    KConfigGroup notifyDialog(KMKernel::self()->config(), "KMKnotifyDialog");
    notifyDialog.writeEntry("Size", size());
    notifyDialog.sync();
}
