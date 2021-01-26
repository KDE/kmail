/*
    SPDX-FileCopyrightText: 2010 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchdescriptionattribute.h"

#include <QDataStream>

using namespace Akonadi;

SearchDescriptionAttribute::SearchDescriptionAttribute() = default;

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
