/*
    This file is part of KMail.

    SPDX-FileCopyrightText: 2005 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "settings/kmailsettings.h"
#include <QTimer>

KMailSettings *KMailSettings::mSelf = nullptr;

KMailSettings *KMailSettings::self()
{
    if (!mSelf) {
        mSelf = new KMailSettings();
        mSelf->load();
    }

    return mSelf;
}

KMailSettings::KMailSettings()
{
    mConfigSyncTimer = new QTimer(this);
    mConfigSyncTimer->setSingleShot(true);
    connect(mConfigSyncTimer, &QTimer::timeout, this, &KMailSettings::slotSyncNow);
}

void KMailSettings::requestSync()
{
    if (!mConfigSyncTimer->isActive()) {
        mConfigSyncTimer->start(0);
    }
}

void KMailSettings::slotSyncNow()
{
    config()->sync();
}

KMailSettings::~KMailSettings() = default;
