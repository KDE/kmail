/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef KMAIL_TASK_ATTRIBUTE_H
#define KMAIL_TASK_ATTRIBUTE_H

#include <AkonadiCore/Attribute>
#include <AkonadiCore/Item>

class TaskAttribute : public Akonadi::Attribute
{
public:
    TaskAttribute();
    TaskAttribute(Akonadi::Item::Id id);
    ~TaskAttribute() override;

    Q_REQUIRED_RESULT QByteArray type() const override;

    TaskAttribute *clone() const override;

    QByteArray serialized() const override;

    void deserialize(const QByteArray &data) override;

    void setTaskId(Akonadi::Item::Id id);
    Akonadi::Item::Id taskId() const;

private:
    Akonadi::Item::Id mId;
};

#endif
