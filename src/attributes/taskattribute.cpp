/*
  SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "taskattribute.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

using namespace Qt::Literals::StringLiterals;

TaskAttribute::TaskAttribute() = default;

TaskAttribute::TaskAttribute(Akonadi::Item::Id id)
    : mId(id)
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
    return "TaskAttribute"_ba;
}
