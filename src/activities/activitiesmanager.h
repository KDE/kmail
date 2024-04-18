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
class ActivitiesManager : public QObject
{
    Q_OBJECT
public:
    explicit ActivitiesManager(QObject *parent = nullptr);
    ~ActivitiesManager() override;

    [[nodiscard]] bool enabled() const;
    void setEnabled(bool newEnabled);

    [[nodiscard]] TransportActivities *transportActivities() const;

Q_SIGNALS:
    void activitiesChanged();

private:
    TransportActivities *const mTransportActivities;
    KActivities::Consumer *const mActivitiesConsumer;
    bool mEnabled = false;
};
