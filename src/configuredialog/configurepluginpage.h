/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "configuredialog_p.h"

#include <QWidget>
namespace PimCommon
{
class ConfigurePluginsWidget;
}
class KMAIL_EXPORT ConfigurePluginPage : public ConfigModule
{
    Q_OBJECT
public:
    explicit ConfigurePluginPage(QObject *parent, const KPluginMetaData &data);
    ~ConfigurePluginPage() override;

    [[nodiscard]] QString helpAnchor() const override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    PimCommon::ConfigurePluginsWidget *const mConfigurePlugins;
};
