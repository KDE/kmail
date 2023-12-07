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

#include "kmkernel.h"
#include "settings/kmailsettings.h"

#include <KPluginMetaData>
#include <QPushButton>

ConfigureDialog::ConfigureDialog(QWidget *parent, bool modal)
    : KCMultiDialog(parent)
{
    setFaceType(List);
    setModal(modal);
    const QList<KPluginMetaData> availablePlugins = KPluginMetaData::findPlugins(QStringLiteral("pim6/kcms/kmail"));
    for (const KPluginMetaData &metaData : availablePlugins) {
        addModule(metaData);
    }
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

#include "moc_configuredialog.cpp"
