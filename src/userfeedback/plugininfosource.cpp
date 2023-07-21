/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "plugininfosource.h"
#include <KLocalizedString>
#ifdef USE_KUSERFEEDBACK_QT6
#include <KUserFeedbackQt6/Provider>
#else
#include <KUserFeedback/Provider>
#endif
#include <QVariant>

PluginInfoSource::PluginInfoSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("plugins"), KUserFeedback::Provider::DetailedSystemInformation)
{
}

QString PluginInfoSource::name() const
{
    return i18n("Plugins Information");
}

QString PluginInfoSource::description() const
{
    return i18n("Plugins used in KMail.");
}

QVariant PluginInfoSource::data()
{
    // TODO add list of plugins.
    return {};
}
