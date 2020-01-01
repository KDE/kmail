/*
  Copyright (c) 2014-2020 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "taskattribute.h"

#include <QByteArray>
#include <QDataStream>

TaskAttribute::TaskAttribute()
    : Akonadi::Attribute()
    , mId(-1)
{
}

TaskAttribute::TaskAttribute(Akonadi::Item::Id id)
    : Akonadi::Attribute()
    , mId(id)
{
}

TaskAttribute::~TaskAttribute()
{
}

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
