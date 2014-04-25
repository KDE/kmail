/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COLLECTIONTEMPLATESPAGE_H
#define COLLECTIONTEMPLATESPAGE_H
#include <AkonadiWidgets/collectionpropertiespage.h>

class QCheckBox;
namespace TemplateParser {
class TemplatesConfiguration;
}

template <typename T> class QSharedPointer;

class CollectionTemplatesPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionTemplatesPage( QWidget *parent = 0 );
    ~CollectionTemplatesPage();

    void load( const Akonadi::Collection & col );
    void save( Akonadi::Collection & col );
    bool canHandle( const Akonadi::Collection &collection ) const;

public slots:
    void slotCopyGlobal();

private Q_SLOTS:
    void slotChanged();

private:
    void init();
    QCheckBox* mCustom;
    TemplateParser::TemplatesConfiguration* mWidget;
    uint mIdentity;
    QString mCollectionId;
    bool mIsLocalSystemFolder;
    bool mChanged;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( CollectionTemplatesPageFactory, CollectionTemplatesPage )

#endif /* COLLECTIONTEMPLATESPAGE_H */

