/*
  SPDX-FileCopyrightText: 2016-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    explicit ConfigurePluginPage(QWidget *parent, const QVariantList &args = {});
#else
    explicit ConfigurePluginPage(QObject *parent, const KPluginMetaData &data, const QVariantList &args = {});
#endif
    ~ConfigurePluginPage() override;

    Q_REQUIRED_RESULT QString helpAnchor() const override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    void slotConfigureChanged();
    PimCommon::ConfigurePluginsWidget *const mConfigurePlugins;
};
