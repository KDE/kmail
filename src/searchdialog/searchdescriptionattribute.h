/*
    SPDX-FileCopyrightText: 2010 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Attribute>
#include <AkonadiCore/Collection>

#include <QByteArray>

namespace Akonadi
{
class SearchDescriptionAttribute : public Akonadi::Attribute
{
public:
    SearchDescriptionAttribute();
    Q_REQUIRED_RESULT QByteArray description() const;
    void setDescription(const QByteArray &desc);
    Q_REQUIRED_RESULT Akonadi::Collection baseCollection() const;
    void setBaseCollection(const Akonadi::Collection &);
    Q_REQUIRED_RESULT bool recursive() const;
    void setRecursive(bool);

    void setListCollection(const QList<Akonadi::Collection::Id> &col);
    QList<Akonadi::Collection::Id> listCollection() const;

    Q_REQUIRED_RESULT QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QList<Akonadi::Collection::Id> mListCollection;
    QByteArray mDescription;
    Akonadi::Collection mBaseCollection;
    bool mRecursive = false;
};
}

