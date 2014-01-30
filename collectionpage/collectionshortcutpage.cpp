/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2004 Till Adam <adam@kde.org>
  Copyright (c) 2013 Jonathan Marten <jjm@keelhaul.me.uk>

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

#include "collectionshortcutpage.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "foldershortcutactionmanager.h"

#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <KDialog>
#include <KHBox>
#include <KLocalizedString>
#include <KKeySequenceWidget>

using namespace MailCommon;


CollectionShortcutPage::CollectionShortcutPage(QWidget * parent) :
    CollectionPropertiesPage( parent ),
    mShortcutChanged( false )
{
    setObjectName( QLatin1String( "KMail::CollectionShortcutPage" ) );
    setPageTitle( i18nc( "@title:tab Shortcut settings for a folder.", "Shortcut" ) );
}

CollectionShortcutPage::~CollectionShortcutPage()
{
}

void CollectionShortcutPage::init(const Akonadi::Collection & col)
{
    mFolder = FolderCollection::forCollection( col, false );

    QVBoxLayout *topLayout = new QVBoxLayout( this );
    topLayout->setSpacing( KDialog::spacingHint() );

    QLabel *label = new QLabel( i18n( "<qt>To choose a key or a combination "
                                      "of keys which select the current folder, "
                                      "click the button below and then press the key(s) "
                                      "you wish to associate with this folder.</qt>" ), this );
    label->setWordWrap(true);
    topLayout->addWidget(label);

    KHBox *hb = new KHBox( this );

    new QWidget(hb);
    mKeySeqWidget = new KKeySequenceWidget( hb );
    mKeySeqWidget->setObjectName( QLatin1String("FolderShortcutSelector") );
    connect( mKeySeqWidget, SIGNAL(keySequenceChanged(QKeySequence)),
             SLOT(slotShortcutChanged()) );
    new QWidget(hb);

    topLayout->addItem( new QSpacerItem( 0, 10 ));
    topLayout->addWidget( hb );
    topLayout->addStretch( 1 );

    mKeySeqWidget->setCheckActionCollections( KMKernel::self()->getKMMainWidget()->actionCollections() );
}

void CollectionShortcutPage::load( const Akonadi::Collection & col )
{
    init( col );
    if ( mFolder ) {
        mKeySeqWidget->setKeySequence( mFolder->shortcut().primary(),
                                       KKeySequenceWidget::NoValidate );
        mShortcutChanged = false;
    }
}

void CollectionShortcutPage::save( Akonadi::Collection & col )
{
    if ( mFolder ) {
        if ( mShortcutChanged ) {
            mKeySeqWidget->applyStealShortcut();
            mFolder->setShortcut( KShortcut( mKeySeqWidget->keySequence(), QKeySequence() ) );
            KMKernel::self()->getKMMainWidget()->folderShortcutActionManager()->shortcutChanged( mFolder->collection() );
        }
    }
}

void CollectionShortcutPage::slotShortcutChanged()
{
    mShortcutChanged = true;
}



