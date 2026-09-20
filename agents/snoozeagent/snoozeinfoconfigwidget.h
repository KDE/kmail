/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentConfigurationBase>
#include <Akonadi/Item>

class SnoozeWidget;

class SnoozeInfoConfigWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit SnoozeInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~SnoozeInfoConfigWidget() override;

    [[nodiscard]] QList<Akonadi::Item::Id> messagesToUnsnooze() const;

    [[nodiscard]] bool save() const override;
    void load() override;

public Q_SLOTS:
    void slotNeedToReloadConfig();

private:
    SnoozeWidget *const mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(SnoozeInfoConfigFactory, "snoozeagentconfig.json", SnoozeInfoConfigWidget)
