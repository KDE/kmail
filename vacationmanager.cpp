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

#include <KMessageBox>
#include <KLocale>

#include <QWidget>


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
    connect( mCheckVacation, SIGNAL(requestEditVacation()), SLOT(slotEditVacation()) );
}

void VacationManager::slotEditVacation()
{
    if ( mVacation ) {
        mVacation->showVacationDialog();
        return;
    }

    mVacation = new KSieveUi::Vacation( this );
    connect( mVacation, SIGNAL(scriptActive(bool,QString)), SLOT(updateVacationScriptStatus(bool,QString)) );
    connect( mVacation, SIGNAL(requestEditVacation()), SLOT(slotEditVacation()) );
    if ( mVacation->isUsable() ) {
        connect( mVacation, SIGNAL(result(bool)), mVacation, SLOT(deleteLater()) );
    } else {
        const QString msg = i18n("KMail's Out of Office Reply functionality relies on "
                           "server-side filtering. You have not yet configured an "
                           "IMAP server for this.\n"
                           "You can do this on the \"Filtering\" tab of the IMAP "
                           "account configuration.");
        KMessageBox::sorry( mWidget, msg, i18n("No Server-Side Filtering Configured") );

        delete mVacation; // QGuardedPtr sets itself to 0!
    }
}


#include "vacationmanager.moc"
