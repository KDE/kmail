/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "spellcheckerconfigdialog.h"
#include "kmkernel.h"
#include "kmail_debug.h"
#include <QCheckBox>
#include <QLabel>

#include <Sonnet/DictionaryComboBox>

#include <KConfigGroup>

SpellCheckerConfigDialog::SpellCheckerConfigDialog(QWidget *parent)
    : Sonnet::ConfigDialog(parent)
{
    // Hackish way to hide the "Enable spell check by default" checkbox
    // Our highlighter ignores this setting, so we should not expose its UI
    QCheckBox *enabledByDefaultCB = findChild<QCheckBox *>(QStringLiteral("m_checkerEnabledByDefaultCB"));
    if (enabledByDefaultCB) {
        enabledByDefaultCB->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any checkbox named 'm_checkerEnabledByDefaultCB'. Sonnet::ConfigDialog must have changed!";
    }
    QLabel *textLabel = findChild<QLabel *>(QStringLiteral("textLabel1"));
    if (textLabel) {
        textLabel->hide();
    } else {
        qCWarning(KMAIL_LOG) << "Could not find any label named 'textLabel'. Sonnet::ConfigDialog must have changed!";
    }
    Sonnet::DictionaryComboBox *dictionaryComboBox = findChild<Sonnet::DictionaryComboBox *>(QStringLiteral("m_langCombo"));
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
