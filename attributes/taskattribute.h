/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#ifndef KMAIL_TASK_ATTRIBUTE_H
#define KMAIL_TASK_ATTRIBUTE_H

#include <Akonadi/Attribute>
#include <Akonadi/Item>

#include <KDateTime>
class TaskAttribute : public Akonadi::Attribute
{
public:
    TaskAttribute();
    ~TaskAttribute();

    QByteArray type() const;

    TaskAttribute* clone() const;

    QByteArray serialized() const;

    void deserialize( const QByteArray &data );

    void setTaskId(Akonadi::Item::Id id);
    Akonadi::Item::Id taskId() const;

private:
    Akonadi::Item::Id mId;
};

#endif
