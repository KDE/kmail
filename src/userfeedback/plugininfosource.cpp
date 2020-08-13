/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "plugininfosource.h"
#include <QVariant>
#include <KUserFeedback/Provider>
#include <KLocalizedString>

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
    //TODO add list of plugins.
    return {};
}
