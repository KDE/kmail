/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2003 Marc Mutz <mutz@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include "kcmutils_version.h"
#include "kmail_export.h"
#include <QObject>
#if KCMUTILS_VERSION >= QT_VERSION_CHECK(5, 240, 0)
#include <KCMUtils/KCModule>
#include <KPluginMetaData>
#else
#include <KCModule>
#endif
class ConfigModule : public KCModule
{
public:
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    explicit ConfigModule(QWidget *parent, const QVariantList &args)
        : KCModule(parent, args)
    {
    }
#else
    explicit ConfigModule(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
        : KCModule(parent, data, args)
    {
    }
#endif

    ~ConfigModule() override = default;

    void defaults() override
    {
    }

    /** Should return the help anchor for this page or tab */
    virtual QString helpAnchor() const = 0;
};
