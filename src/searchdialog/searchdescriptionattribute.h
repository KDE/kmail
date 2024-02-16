/*
    SPDX-FileCopyrightText: 2010 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>
#include <Akonadi/Collection>

#include <QByteArray>

namespace Akonadi
{
class SearchDescriptionAttribute : public Akonadi::Attribute
{
public:
    SearchDescriptionAttribute();
    [[nodiscard]] QByteArray description() const;
    void setDescription(const QByteArray &desc);
    [[nodiscard]] Akonadi::Collection baseCollection() const;
    void setBaseCollection(const Akonadi::Collection &);
    [[nodiscard]] bool recursive() const;
    void setRecursive(bool);

    void setListCollection(const QList<Akonadi::Collection::Id> &col);
    [[nodiscard]] QList<Akonadi::Collection::Id> listCollection() const;

    [[nodiscard]] QByteArray type() const override;
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
