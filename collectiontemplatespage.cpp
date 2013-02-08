/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2011 Montel Laurent <montel@kde.org>

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

#include "kmkernel.h"
#include "mailkernel.h"
#include "foldercollection.h"
#include "templateparser/templatesconfiguration.h"
#include "templateparser/templatesconfiguration_kfg.h"

#include <akonadi/collection.h>

#include <KLocale>
#include <KPushButton>
#include <QCheckBox>

using namespace Akonadi;
using namespace MailCommon;

CollectionTemplatesPage::CollectionTemplatesPage(QWidget * parent) :
    CollectionPropertiesPage( parent ), mChanged(false)
{
  setObjectName( QLatin1String( "KMail::CollectionTemplatesPage" ) );
  setPageTitle( i18n( "Templates" ) );
  init();
}

CollectionTemplatesPage::~CollectionTemplatesPage()
{
}

bool CollectionTemplatesPage::canHandle( const Collection &collection ) const
{

  return ( !CommonKernel->isSystemFolderCollection( collection ) || CommonKernel->isMainFolderCollection( collection ) );
}

void CollectionTemplatesPage::init()
{
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( KDialog::marginHint() );
  topLayout->setSpacing( KDialog::spacingHint() );

  QHBoxLayout *topItems = new QHBoxLayout;
  topLayout->addLayout( topItems );

  mCustom = new QCheckBox( i18n("&Use custom message templates in this folder"), this );
  connect(mCustom, SIGNAL(clicked(bool)), this, SLOT(slotChanged()));
  topItems->addWidget( mCustom, Qt::AlignLeft );

  mWidget = new TemplateParser::TemplatesConfiguration( this, "folder-templates" );
  connect(mWidget, SIGNAL(changed()), this, SLOT(slotChanged()));
  mWidget->setEnabled( false );

  // Move the help label outside of the templates configuration widget,
  // so that the help can be read even if the widget is not enabled.
  topItems->addStretch( 9 );
  topItems->addWidget( mWidget->helpLabel(), Qt::AlignRight );

  topLayout->addWidget( mWidget );

  QHBoxLayout *btns = new QHBoxLayout();
  btns->setSpacing( KDialog::spacingHint() );
  KPushButton *copyGlobal = new KPushButton( i18n("&Copy Global Templates"), this );
  copyGlobal->setEnabled( false );
  btns->addWidget( copyGlobal );
  topLayout->addLayout( btns );

  connect( mCustom, SIGNAL(toggled(bool)),
           mWidget, SLOT(setEnabled(bool)) );
  connect( mCustom, SIGNAL(toggled(bool)),
           copyGlobal, SLOT(setEnabled(bool)) );

  connect( copyGlobal, SIGNAL(clicked()),
           this, SLOT(slotCopyGlobal()) );
}


void CollectionTemplatesPage::load(const Collection & col)
{
  const QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( col, false );
  if ( !fd )
    return;

  mCollectionId = QString::number( col.id() );

  TemplateParser::Templates t( mCollectionId );

  mCustom->setChecked(t.useCustomTemplates());

  mIdentity = fd->identity();

  mWidget->loadFromFolder( mCollectionId, mIdentity );
  mChanged = false;
}

void CollectionTemplatesPage::save(Collection &)
{
  if ( mChanged && !mCollectionId.isEmpty() ) {
    TemplateParser::Templates t(mCollectionId);
    //kDebug() << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
    t.setUseCustomTemplates(mCustom->isChecked());
    t.writeConfig();

    mWidget->saveToFolder(mCollectionId);
  }
}

void CollectionTemplatesPage::slotCopyGlobal() {
  if ( mIdentity ) {
    mWidget->loadFromIdentity( mIdentity );
  } else {
    mWidget->loadFromGlobal();
  }
}

void CollectionTemplatesPage::slotChanged()
{
  mChanged = true;
}

#include "collectiontemplatespage.moc"
