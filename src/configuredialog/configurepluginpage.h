/*
  SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef CONFIGUREPLUGINPAGE_H
#define CONFIGUREPLUGINPAGE_H

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
    explicit ConfigurePluginPage(QWidget *parent);
    ~ConfigurePluginPage() override;

    Q_REQUIRED_RESULT QString helpAnchor() const override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    void slotConfigureChanged();
    PimCommon::ConfigurePluginsWidget *mConfigurePlugins = nullptr;
};

#endif // CONFIGUREPLUGINPAGE_H
