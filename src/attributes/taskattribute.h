/*
  SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <Akonadi/Attribute>
#include <Akonadi/Item>

class TaskAttribute : public Akonadi::Attribute
{
public:
    TaskAttribute();
    explicit TaskAttribute(Akonadi::Item::Id id);
    ~TaskAttribute() override;

    [[nodiscard]] QByteArray type() const override;

    TaskAttribute *clone() const override;

    QByteArray serialized() const override;

    void deserialize(const QByteArray &data) override;

    void setTaskId(Akonadi::Item::Id id);
    [[nodiscard]] Akonadi::Item::Id taskId() const;

private:
    Akonadi::Item::Id mId = {-1};
};
