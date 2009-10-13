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
#include <akonadi/collectionpropertiespage.h>

class KPushButton;
class QCheckBox;
class TemplatesConfiguration;

class CollectionTemplatesPage : public Akonadi::CollectionPropertiesPage
{
  Q_OBJECT
public:
  explicit CollectionTemplatesPage( QWidget *parent = 0 );

  void load( const Akonadi::Collection & col );
  void save( Akonadi::Collection & col );
  bool canHandle( const Akonadi::Collection &collection ) const;

public slots:
  void slotCopyGlobal();

private:
#if 0
  void initializeWithValuesFromFolder( KMFolder* folder );
#endif
protected:
  void init();
private:
  QCheckBox* mCustom;
  TemplatesConfiguration* mWidget;
  KPushButton* mCopyGlobal;
#if 0
  KMFolder* mFolder;
#endif
  uint mIdentity;

  bool mIsLocalSystemFolder;
};

#endif /* COLLECTIONTEMPLATESPAGE_H */

