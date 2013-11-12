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

#include "configuredialogutils.h"

#include <KLocale>
#include <KDialog>

#include <QWidget>
#include <QGroupBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRadioButton>


static const char * lockedDownWarning =
        I18N_NOOP("<qt><p>This setting has been fixed by your administrator.</p>"
                  "<p>If you think this is an error, please contact him.</p></qt>");

void ConfigureDialogUtils::checkLockDown( QWidget * w, const KConfigSkeletonItem *item ) {
    if ( item->isImmutable() ) {
        w->setEnabled( false );
        w->setToolTip( i18n( lockedDownWarning ) );
    }
}

void ConfigureDialogUtils::populateButtonGroup( QGroupBox * box, QButtonGroup * group, int orientation,
                          const KCoreConfigSkeleton::ItemEnum *e ) {
    box->setTitle( e->label() );
    if (orientation == Qt::Horizontal) {
        box->setLayout( new QHBoxLayout() );
    } else {
        box->setLayout( new QVBoxLayout() );
    }
    box->layout()->setSpacing( KDialog::spacingHint() );
    const int numberChoices(e->choices().size());
    for (int i = 0; i < numberChoices; ++i) {
        QRadioButton *button = new QRadioButton( e->choices().at(i).label, box );
        group->addButton( button, i );
        box->layout()->addWidget( button );
    }
}

void ConfigureDialogUtils::populateCheckBox( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e ) {
    b->setText( e->label() );
}

void ConfigureDialogUtils::loadWidget( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e ) {
    checkLockDown( b, e );
    b->setChecked( e->value() );
}

void ConfigureDialogUtils::loadWidget( QGroupBox * box, QButtonGroup * group,
                 const KCoreConfigSkeleton::ItemEnum *e ) {
    Q_ASSERT( group->buttons().size() == e->choices().size() );
    checkLockDown( box, e );
    group->buttons()[e->value()]->setChecked( true );
}

void ConfigureDialogUtils::saveCheckBox( QCheckBox * b, KCoreConfigSkeleton::ItemBool *e ) {
    e->setValue( b->isChecked() );
}

void ConfigureDialogUtils::saveButtonGroup( QButtonGroup * group, KCoreConfigSkeleton::ItemEnum *e ) {
    Q_ASSERT( group->buttons().size() == e->choices().size() );
    if ( group->checkedId() != -1 ) {
        e->setValue( group->checkedId() );
    }
}
