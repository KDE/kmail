// -*- mode: C++; c-file-style: "gnu" -*-
/**
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 * Copyright (c) 2009-2015 Laurent Montel <montel@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef COLLECTIONQUOTAPAGE_H
#define COLLECTIONQUOTAPAGE_H
#include <akonadi/collectionpropertiespage.h>

class QuotaWidget;

/**
 * "Quota" tab in the folder dialog
 */
class CollectionQuotaPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionQuotaPage( QWidget* parent = 0 );
    void load( const Akonadi::Collection & col );
    void save( Akonadi::Collection & col );
    bool canHandle( const Akonadi::Collection &collection ) const;

protected:
    void init();

private:
    QuotaWidget* mQuotaWidget;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( CollectionQuotaPageFactory, CollectionQuotaPage )

#endif /* COLLECTIONQUOTAPAGE_H */

