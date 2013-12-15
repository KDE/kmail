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

#include "vacationmanager.h"
#include "ksieveui/vacation/vacation.h"
#include "ksieveui/vacation/multiimapvacationdialog.h"

#include <KMessageBox>
#include <KLocale>

#include <QWidget>

using namespace KMail;

VacationManager::VacationManager(QWidget *parent)
    : QObject(parent),
      mWidget(parent)
{
}

VacationManager::~VacationManager()
{
}

void VacationManager::checkVacation()
{
    delete mCheckVacation;

    mCheckVacation = new KSieveUi::Vacation( this, true /* check only */ );
    connect( mCheckVacation, SIGNAL(scriptActive(bool,QString)), SIGNAL(updateVacationScriptStatus(bool,QString)) );
    connect( mCheckVacation, SIGNAL(requestEditVacation()), SIGNAL(editVacation()) );
}

void VacationManager::slotEditVacation()
{
    if ( mMultiImapVacationDialog ) {
        mMultiImapVacationDialog->show();
        mMultiImapVacationDialog->raise();
        mMultiImapVacationDialog->activateWindow();
        return;
    }

    mMultiImapVacationDialog = new KSieveUi::MultiImapVacationDialog(i18n("Configure Vacation"), mWidget);
    mMultiImapVacationDialog->setAttribute(Qt::WA_DeleteOnClose);
    mMultiImapVacationDialog->show();
}


