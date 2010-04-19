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

#include "collectiontemplatespage.h"

#include <akonadi/collection.h>
#include <KLocale>
#include <KPushButton>
#include "kmkernel.h"
#include <QCheckBox>
#include "foldercollection.h"
#include "templateparser/templatesconfiguration.h"
#include "templateparser/templatesconfiguration_kfg.h"

using namespace Akonadi;

CollectionTemplatesPage::CollectionTemplatesPage(QWidget * parent) :
    CollectionPropertiesPage( parent ), mFolderCollection( 0 )
{
  setPageTitle( i18n( "Templates" ) );
  init();
}

CollectionTemplatesPage::~CollectionTemplatesPage()
{
}

bool CollectionTemplatesPage::canHandle( const Collection &collection ) const
{

  return !KMKernel::self()->isSystemFolderCollection( collection );
}

void CollectionTemplatesPage::init()
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( KDialog::marginHint() );
  topLayout->setSpacing( KDialog::spacingHint() );

  QHBoxLayout *topItems = new QHBoxLayout;
  topLayout->addLayout( topItems );

  mCustom = new QCheckBox( i18n("&Use custom message templates in this folder"), this );
  topItems->addWidget( mCustom, Qt::AlignLeft );

  mWidget = new TemplatesConfiguration( this, "folder-templates" );
  mWidget->setEnabled( false );

  // Move the help label outside of the templates configuration widget,
  // so that the help can be read even if the widget is not enabled.
  topItems->addStretch( 9 );
  topItems->addWidget( mWidget->helpLabel(), Qt::AlignRight );

  topLayout->addWidget( mWidget );

  QHBoxLayout *btns = new QHBoxLayout();
  btns->setSpacing( KDialog::spacingHint() );
  mCopyGlobal = new KPushButton( i18n("&Copy Global Templates"), this );
  mCopyGlobal->setEnabled( false );
  btns->addWidget( mCopyGlobal );
  topLayout->addLayout( btns );

  connect( mCustom, SIGNAL(toggled( bool )),
           mWidget, SLOT(setEnabled( bool )) );
  connect( mCustom, SIGNAL(toggled( bool )),
           mCopyGlobal, SLOT(setEnabled( bool )) );

  connect( mCopyGlobal, SIGNAL(clicked()),
           this, SLOT(slotCopyGlobal()) );
}


void CollectionTemplatesPage::load(const Collection & col)
{
  mFolderCollection = FolderCollection::forCollection( col );
  QString fid = mFolderCollection->idString();

  Templates t( fid );

  mCustom->setChecked(t.useCustomTemplates());

  mIdentity = mFolderCollection->identity();

  mWidget->loadFromFolder( fid, mIdentity );
}

void CollectionTemplatesPage::save(Collection &)
{
  if ( mFolderCollection ) {
    QString fid = mFolderCollection->idString();
    Templates t(fid);

    kDebug() << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
    t.setUseCustomTemplates(mCustom->isChecked());
    t.writeConfig();

    mWidget->saveToFolder(fid);
  }
}

void CollectionTemplatesPage::slotCopyGlobal() {
  if ( mIdentity ) {
    mWidget->loadFromIdentity( mIdentity );
  }
  else {
    mWidget->loadFromGlobal();
  }
}

#include "collectiontemplatespage.moc"
