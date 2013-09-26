/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "selectmulticollectiondialog.h"
#include "selectmulticollectionwidget.h"

#include <KLocale>
#include <KConfigGroup>

SelectMultiCollectionDialog::SelectMultiCollectionDialog(const QList<Akonadi::Collection::Id> &selectedCollection, QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Select Multiple Folders" ) );
    setButtons( Close | Ok );

    mSelectMultiCollection = new SelectMultiCollectionWidget(selectedCollection);
    setMainWidget( mSelectMultiCollection );
    readConfig();
}

SelectMultiCollectionDialog::~SelectMultiCollectionDialog()
{
    writeConfig();
}

void SelectMultiCollectionDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "SelectMultiCollectionDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void SelectMultiCollectionDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "SelectMultiCollectionDialog" );
    group.writeEntry( "Size", size() );
}

QList<Akonadi::Collection> SelectMultiCollectionDialog::selectedCollection() const
{
    return mSelectMultiCollection->selectedCollection();
}


#include "selectmulticollectiondialog.moc"
