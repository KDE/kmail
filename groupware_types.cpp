/*
    Copyright (c) 2007 Montel Laurent <montel@kde.org>
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "groupware_types.h"

#include <QDBusArgument>
#include <QDBusMetaType>

const QDBusArgument &operator<<(QDBusArgument &arg, const KMail::SubResource &subResource)
{
  arg.beginStructure();
  arg << subResource.location << subResource.label << subResource.writable << subResource.alarmRelevant;
  arg.endStructure();
  return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, KMail::SubResource &subResource)
{
  arg.beginStructure();
  arg >> subResource.location >> subResource.label >> subResource.writable >> subResource.alarmRelevant;
  arg.endStructure();
  return arg;
}

const QDBusArgument &operator<<(QDBusArgument &arg, const KMail::StorageFormat &format)
{
  arg.beginStructure();
  qint32 i = format;
  arg << i;
  arg.endStructure();
  return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, KMail::StorageFormat &format)
{
  arg.beginStructure();
  arg >> format;
  arg.endStructure();
  return arg;
}

const QDBusArgument &operator<<( QDBusArgument &arg, const KMail::CustomHeader &header )
{
  arg.beginStructure();
  arg << header.name << header.value;
  arg.endStructure();
  return arg;
}

const QDBusArgument &operator>>( const QDBusArgument &arg, KMail::CustomHeader &header )
{
  arg.beginStructure();
  arg >> header.name >> header.value;
  arg.endStructure();
  return arg;
}

void KMail::registerGroupwareTypes()
{
  qDBusRegisterMetaType< KMail::SubResource >();
  qDBusRegisterMetaType< KMail::SubResource::List >();
  qDBusRegisterMetaType< QMap<quint32,QString> >();
  qDBusRegisterMetaType< KMail::CustomHeader >();
  qDBusRegisterMetaType< KMail::CustomHeader::List >();
}
