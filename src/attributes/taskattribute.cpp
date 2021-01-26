/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "taskattribute.h"

#include <QByteArray>
#include <QDataStream>

TaskAttribute::TaskAttribute()
    : Akonadi::Attribute()
{
}

TaskAttribute::TaskAttribute(Akonadi::Item::Id id)
    : Akonadi::Attribute()
    , mId(id)
{
}

TaskAttribute::~TaskAttribute() = default;

TaskAttribute *TaskAttribute::clone() const
{
    return new TaskAttribute(taskId());
}

void TaskAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    s >> mId;
}

void TaskAttribute::setTaskId(Akonadi::Item::Id id)
{
    mId = id;
}

Akonadi::Item::Id TaskAttribute::taskId() const
{
    return mId;
}

QByteArray TaskAttribute::serialized() const
{
    QByteArray result;
    QDataStream s(&result, QIODevice::WriteOnly);
    s << mId;
    return result;
}

QByteArray TaskAttribute::type() const
{
    static const QByteArray sType("TaskAttribute");
    return sType;
}
