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
#include <QCheckBox>
#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"

using namespace Akonadi;

CollectionTemplatesPage::CollectionTemplatesPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18n( "Templates" ) );
  init();
}

bool CollectionTemplatesPage::canHandle( const Collection &collection ) const
{
  //TODO it
  return true;
}

void CollectionTemplatesPage::init()
{
#if 0
  mIsLocalSystemFolder = mDlg->folder()->isSystemFolder();
#endif
  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( 0 );
  topLayout->setSpacing( KDialog::spacingHint() );

  QHBoxLayout *topItems = new QHBoxLayout( this );
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
#if 0 //TODO port it
  initializeWithValuesFromFolder( mDlg->folder() );

#endif
}


void CollectionTemplatesPage::load(const Collection &)
{
  //Nothing
}

void CollectionTemplatesPage::save(Collection &)
{
  //Nothing
}

#if 0 //TODO port it.
void FolderDialogTemplatesTab::initializeWithValuesFromFolder( KMFolder* folder ) {
  if ( !folder )
    return;

  mFolder = folder;

  QString fid = folder->idString();

  Templates t( fid );

  mCustom->setChecked(t.useCustomTemplates());

  mIdentity = folder->identity();

  mWidget->loadFromFolder( fid, mIdentity );
}

//-----------------------------------------------------------------------------
bool FolderDialogTemplatesTab::save()
{
  KMFolder* folder = mDlg->folder();

  QString fid = folder->idString();
  Templates t(fid);

  kDebug() << "use custom templates for folder" << fid <<":" << mCustom->isChecked();
  t.setUseCustomTemplates(mCustom->isChecked());
  t.writeConfig();

  mWidget->saveToFolder(fid);

  return true;
}

#endif
void CollectionTemplatesPage::slotCopyGlobal() {
  if ( mIdentity ) {
    mWidget->loadFromIdentity( mIdentity );
  }
  else {
    mWidget->loadFromGlobal();
  }
}

#include "collectiontemplatespage.moc"
