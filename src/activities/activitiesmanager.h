/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QObject>
namespace KActivities
{
class Consumer;
}
class TransportActivities;
class IdentityActivities;
class ActivitiesManager : public QObject
{
    Q_OBJECT
public:
    static ActivitiesManager *self();

    explicit ActivitiesManager(QObject *parent = nullptr);
    ~ActivitiesManager() override;

    [[nodiscard]] bool enabled() const;
    void setEnabled(bool newEnabled);

    [[nodiscard]] TransportActivities *transportActivities() const;

    [[nodiscard]] IdentityActivities *identityActivities() const;

    [[nodiscard]] bool isInCurrentActivity(const QStringList &lst) const;
Q_SIGNALS:
    void activitiesChanged();

private:
    TransportActivities *const mTransportActivities;
    IdentityActivities *const mIdentityActivities;
    KActivities::Consumer *const mActivitiesConsumer;
    bool mEnabled = false;
};
