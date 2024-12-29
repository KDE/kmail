/*
   SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/AgentConfigurationBase>
#include <Akonadi/Item>
#include <QDialog>
class SendLaterWidget;
class SendLaterInfoConfigWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit SendLaterInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~SendLaterInfoConfigWidget() override;

    [[nodiscard]] QList<Akonadi::Item::Id> messagesToRemove() const;

    [[nodiscard]] QSize restoreDialogSize() const override;
    void saveDialogSize(const QSize &size) override;

    [[nodiscard]] bool save() const override;
    void load() override;

public Q_SLOTS:
    void slotNeedToReloadConfig();

private:
    SendLaterWidget *const mWidget;
};
AKONADI_AGENTCONFIG_FACTORY(SendLaterInfoConfigFactory, "sendlateragentconfig.json", SendLaterInfoConfigWidget)
