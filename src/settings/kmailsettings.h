/*
    This file is part of KMail.

    Copyright (c) 2005 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2,
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef KMAIL_GLOBALSETTINGS_H
#define KMAIL_GLOBALSETTINGS_H

#include "globalsettings_kmail.h"

class QTimer;

class KMailSettings : public GlobalSettingsBase
{
    Q_OBJECT
public:
    static KMailSettings *self();

    /** Call this slot instead of directly @ref KConfig::sync() to
        minimize the overall config writes. Calling this slot will
        schedule a sync of the application config file using a timer, so
        that many consecutive calls can be condensed into a single
        sync, which is more efficient. */
    void requestSync();

private Q_SLOTS:
    void slotSyncNow();

private:
    KMailSettings();
    virtual ~KMailSettings();
    static KMailSettings *mSelf;

    QTimer *mConfigSyncTimer = nullptr;
};

#endif /* KMAIL_GLOBALSETTINGS_H */
