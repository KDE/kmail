/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2003 Marc Mutz <mutz@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"

#include "kmkernel.h"
#include "settings/kmailsettings.h"

#include <QPushButton>

ConfigureDialog::ConfigureDialog(QWidget *parent, bool modal)
    : KCMultiDialog(parent)
{
    setFaceType(List);
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Help | QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Cancel | QDialogButtonBox::Apply
                       | QDialogButtonBox::Reset);
    setModal(modal);
    addModule(QStringLiteral("kmail_config_accounts"));
    addModule(QStringLiteral("kmail_config_appearance"));
    addModule(QStringLiteral("kmail_config_composer"));
    addModule(QStringLiteral("kmail_config_security"));
    addModule(QStringLiteral("kmail_config_misc"));
    addModule(QStringLiteral("kmail_config_plugins"));

    connect(button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ConfigureDialog::slotOk);
    connect(button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigureDialog::slotApply);
    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &ConfigureDialog::slotHelpClicked);
    connect(button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ConfigureDialog::slotUser1Clicked);
    connect(button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ConfigureDialog::slotDefaultClicked);
    connect(button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &ConfigureDialog::slotUser1Clicked);
}

QSize ConfigureDialog::sizeHint() const
{
    const int width = KMailSettings::self()->configureDialogWidth();
    const int height = KMailSettings::self()->configureDialogHeight();
    return {width, height};
}

void ConfigureDialog::hideEvent(QHideEvent *ev)
{
    KMailSettings::self()->setConfigureDialogWidth(width());
    KMailSettings::self()->setConfigureDialogHeight(height());
    KPageDialog::hideEvent(ev);
}

ConfigureDialog::~ConfigureDialog() = default;

void ConfigureDialog::slotApply()
{
    slotApplyClicked();
    if (KMKernel::self()) {
        KMKernel::self()->slotRequestConfigSync();
    }
    Q_EMIT configChanged();
}

void ConfigureDialog::slotOk()
{
    slotOkClicked();
    if (KMKernel::self()) {
        KMKernel::self()->slotRequestConfigSync();
    }
    Q_EMIT configChanged();
}
