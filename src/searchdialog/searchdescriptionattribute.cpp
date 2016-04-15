/*
    Copyright (C) 2010 Till Adam <adam@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "searchdescriptionattribute.h"

#include <QByteArray>
#include <QDataStream>

#include <AkonadiCore/Collection>

using namespace Akonadi;

SearchDescriptionAttribute::SearchDescriptionAttribute()
    : mRecursive(false)
{
}

QByteArray SearchDescriptionAttribute::type() const
{
    static const QByteArray sType("kmailsearchdescription");
    return sType;
}

Akonadi::Attribute *SearchDescriptionAttribute::clone() const
{
    return new SearchDescriptionAttribute(*this);
}

QByteArray SearchDescriptionAttribute::serialized() const
{
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s << mBaseCollection.id();
    s << mRecursive;
    s << mDescription;
    s << mListCollection;
    return ba;
}

void SearchDescriptionAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    Akonadi::Collection::Id id;
    s >> id;
    mBaseCollection = Akonadi::Collection(id);
    s >> mRecursive;
    s >> mDescription;
    s >> mListCollection;
}

QByteArray SearchDescriptionAttribute::description() const
{
    return mDescription;
}

void SearchDescriptionAttribute::setDescription(const QByteArray &desc)
{
    mDescription = desc;
}

Akonadi::Collection SearchDescriptionAttribute::baseCollection() const
{
    return mBaseCollection;
}

void SearchDescriptionAttribute::setBaseCollection(const Akonadi::Collection &col)
{
    mBaseCollection = col;
}

bool SearchDescriptionAttribute::recursive() const
{
    return mRecursive;
}

void SearchDescriptionAttribute::setRecursive(bool r)
{
    mRecursive = r;
}

void SearchDescriptionAttribute::setListCollection(const QList<Akonadi::Collection::Id> &col)
{
    mListCollection = col;
}

QList<Akonadi::Collection::Id> SearchDescriptionAttribute::listCollection() const
{
    return mListCollection;
}
