// -*- mode: C++; c-file-style: "gnu" -*-
/**
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 * Copyright (c) 2009 Laurent Montel <montel@kde.org>
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

#include "collectionquotapage.h"
#include "collectionquotapage_p.h"
#include <akonadi/collectionquotaattribute.h>
#include <akonadi/collection.h>

#include <qstackedwidget.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <KLocale>
#include <assert.h>


CollectionQuotaPage::CollectionQuotaPage( QWidget* parent )
  : CollectionPropertiesPage( parent )
{
  setPageTitle( i18n("Quota") );
  init();
}

bool CollectionQuotaPage::canHandle( const Akonadi::Collection &collection ) const
{
  return collection.hasAttribute<Akonadi::CollectionQuotaAttribute>();
}

void CollectionQuotaPage::init()
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  mQuotaWidget = new QuotaWidget(this);
  topLayout->addWidget(mQuotaWidget);
}

void CollectionQuotaPage::load( const Akonadi::Collection & col )
{
  if ( col.hasAttribute<Akonadi::CollectionQuotaAttribute>() ) {
    qint64 currentValue = col.attribute<Akonadi::CollectionQuotaAttribute>()->currentValue();
    qint64 maximumValue = col.attribute<Akonadi::CollectionQuotaAttribute>()->maximumValue();
    mQuotaWidget->setQuotaInfo( currentValue, maximumValue );
  }
}

void CollectionQuotaPage::save( Akonadi::Collection & )
{
  // nothing to do, we are read-only
}

