/*
    This file is part of KMail.

    SPDX-FileCopyrightText: 2005 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "globalsettings_kmail.h"

class QTimer;

class KMailSettings : public GlobalSettingsBase
{
    Q_OBJECT
public:
    static KMailSettings *self();

    /** Call this slot instead of directly KConfig::sync() to
        minimize the overall config writes. Calling this slot will
        schedule a sync of the application config file using a timer, so
        that many consecutive calls can be condensed into a single
        sync, which is more efficient. */
    void requestSync();

private Q_SLOTS:
    void slotSyncNow();

private:
    KMailSettings();
    ~KMailSettings() override;
    static KMailSettings *mSelf;

    QTimer *mConfigSyncTimer = nullptr;
};

