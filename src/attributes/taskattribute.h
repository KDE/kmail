/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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

    [[nodiscard]] QByteArray serialized() const override;

    void deserialize(const QByteArray &data) override;

    void setTaskId(Akonadi::Item::Id id);
    [[nodiscard]] Akonadi::Item::Id taskId() const;

private:
    Akonadi::Item::Id mId = {-1};
};
