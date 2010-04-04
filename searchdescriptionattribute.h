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

#ifndef AKONADI_SEARCHDESCRIPTIONATTRIBUTE_H
#define AKONADI_SEARCHDESCRIPTIONATTRIBUTE_H

#include <Akonadi/Attribute>
#include <Akonadi/Collection>

#include <QByteArray>

namespace Akonadi {

class SearchDescriptionAttribute : public Akonadi::Attribute
{
  public:
    SearchDescriptionAttribute();
    QByteArray description() const ;
    void setDescription( const QByteArray& desc );
    Akonadi::Collection baseCollection() const;
    void setBaseCollection( const Akonadi::Collection & );
    bool recursive() const;
    void setRecursive( bool );

    virtual QByteArray type() const;
    virtual Attribute *clone() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

  private:
    QByteArray mDescription;
    Akonadi::Collection mBaseCollection;
    bool mRecursive;
};

}

#endif
