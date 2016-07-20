/*
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "settings/kmailsettings.h"
#include "kmkernel.h"
// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"

#include <KWindowSystem>
#include <KIconLoader>
#include <QPushButton>
#include <KHelpClient>

ConfigureDialog::ConfigureDialog(QWidget *parent, bool modal)
    : KCMultiDialog(parent)
{
    setFaceType(List);
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Help |
                       QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Cancel |
                       QDialogButtonBox::Apply | QDialogButtonBox::Reset);
    setModal(modal);
    KWindowSystem::setIcons(winId(), qApp->windowIcon().pixmap(IconSize(KIconLoader::Desktop), IconSize(KIconLoader::Desktop)), qApp->windowIcon().pixmap(IconSize(KIconLoader::Small), IconSize(KIconLoader::Small)));
    addModule(QStringLiteral("kmail_config_identity"));
    addModule(QStringLiteral("kmail_config_accounts"));
    addModule(QStringLiteral("kmail_config_appearance"));
    addModule(QStringLiteral("kmail_config_composer"));
    addModule(QStringLiteral("kmail_config_security"));
    addModule(QStringLiteral("kmail_config_misc"));

    connect(button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ConfigureDialog::slotOk);
    connect(button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigureDialog::slotApply);
    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &ConfigureDialog::slotHelpClicked);
    connect(button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ConfigureDialog::slotUser1Clicked);
    connect(button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ConfigureDialog::slotDefaultClicked);
    connect(button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &ConfigureDialog::slotUser1Clicked);
    // We store the size of the dialog on hide, because otherwise
    // the KCMultiDialog starts with the size of the first kcm, not
    // the largest one. This way at least after the first showing of
    // the largest kcm the size is kept.
    const int width = KMailSettings::self()->configureDialogWidth();
    const int height = KMailSettings::self()->configureDialogHeight();
    if (width != 0 && height != 0) {
        resize(width, height);
    }

}

void ConfigureDialog::hideEvent(QHideEvent *ev)
{
    KMailSettings::self()->setConfigureDialogWidth(width());
    KMailSettings::self()->setConfigureDialogHeight(height());
    KPageDialog::hideEvent(ev);
}

ConfigureDialog::~ConfigureDialog()
{
}

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
