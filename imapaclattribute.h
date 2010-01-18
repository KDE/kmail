/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_IMAPACLATTRIBUTE_H
#define AKONADI_IMAPACLATTRIBUTE_H

#include <akonadi/attribute.h>

#include <QtCore/QMap>

#include <kimap/acl.h>

namespace Akonadi {

class ImapAclAttribute : public Akonadi::Attribute
{
  public:
    ImapAclAttribute();
    ImapAclAttribute( const QMap<QByteArray, KIMAP::Acl::Rights> &rights );
    void setRights( const QMap<QByteArray, KIMAP::Acl::Rights> &rights );
    QMap<QByteArray, KIMAP::Acl::Rights> rights() const;
    virtual QByteArray type() const;
    virtual Attribute *clone() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

  private:
    QMap<QByteArray, KIMAP::Acl::Rights> mRights;
};

}

#endif
