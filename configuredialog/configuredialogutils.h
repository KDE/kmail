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


#ifndef CONFIGUREDIALOGUTILS_H
#define CONFIGUREDIALOGUTILS_H

#include <KConfigSkeletonItem>
#include <KConfigSkeleton>

class QWidget;
class QGroupBox;
class QCheckBox;
class QButtonGroup;
class QLineEdit;
class KConfigSkeletonItem;
class KUrlRequester;

namespace ConfigureDialogUtils {
void checkLockDown( QWidget * w, const KConfigSkeletonItem *item );
void populateButtonGroup( QGroupBox * box, QButtonGroup * group, int orientation, const KCoreConfigSkeleton::ItemEnum *e );
void populateCheckBox( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e );
void loadWidget( QCheckBox * b, const KCoreConfigSkeleton::ItemBool *e );
void loadWidget( QGroupBox * box, QButtonGroup * group, const KCoreConfigSkeleton::ItemEnum *e );
void loadWidget( QLineEdit * b, const KCoreConfigSkeleton::ItemString *e );
void loadWidget( KUrlRequester * b, const KCoreConfigSkeleton::ItemString *e );

void saveCheckBox( QCheckBox * b, KCoreConfigSkeleton::ItemBool *e );
void saveLineEdit( QLineEdit * b, KCoreConfigSkeleton::ItemString *e );
void saveUrlRequester( KUrlRequester * b, KCoreConfigSkeleton::ItemString *e );

void saveButtonGroup( QButtonGroup * group, KCoreConfigSkeleton::ItemEnum *e );
}

#endif // CONFIGUREDIALOGUTILS_H
