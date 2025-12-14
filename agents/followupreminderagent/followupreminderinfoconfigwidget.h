/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentConfigurationBase>
#include <Akonadi/Item>
#include <QVariantList>
class QWidget;
class FollowUpReminderInfoWidget;
class FollowUpReminderInfoConfigWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit FollowUpReminderInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args);
    ~FollowUpReminderInfoConfigWidget() override;

    [[nodiscard]] bool save() const override;
    void load() override;

private:
    FollowUpReminderInfoWidget *const mWidget;
};
AKONADI_AGENTCONFIG_FACTORY(FollowUpReminderInfoAgentConfigFactory, "followupreminderagentconfig.json", FollowUpReminderInfoConfigWidget)
