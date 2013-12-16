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
#include "ksieveui/vacation/multiimapvacationmanager.h"
#include "ksieveui/vacation/multiimapvacationdialog.h"
#include "ksieveui/vacation/vacationcreatescriptjob.h"

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

    mCheckVacation = new KSieveUi::MultiImapVacationManager( this );
    connect( mCheckVacation, SIGNAL(scriptActive(bool,QString)), SIGNAL(updateVacationScriptStatus(bool,QString)) );
    connect( mCheckVacation, SIGNAL(requestEditVacation()), SIGNAL(editVacation()) );
    mCheckVacation->checkVacation();
}

void VacationManager::slotEditVacation()
{
    if ( mMultiImapVacationDialog ) {
        mMultiImapVacationDialog->show();
        mMultiImapVacationDialog->raise();
        mMultiImapVacationDialog->activateWindow();
        return;
    }

    mMultiImapVacationDialog = new KSieveUi::MultiImapVacationDialog(mWidget);
    connect( mMultiImapVacationDialog, SIGNAL(okClicked()), SLOT(slotDialogOk()) );
    connect( mMultiImapVacationDialog, SIGNAL(cancelClicked()), SLOT(slotDialogCanceled()) );
    mMultiImapVacationDialog->show();
}

void VacationManager::slotDialogCanceled()
{
    mMultiImapVacationDialog->delayedDestruct();
    mMultiImapVacationDialog = 0;
}

void VacationManager::slotDialogOk()
{
    QList<KSieveUi::VacationCreateScriptJob *> listJob = mMultiImapVacationDialog->listCreateJob();
    Q_FOREACH (KSieveUi::VacationCreateScriptJob *job, listJob) {
        connect(job, SIGNAL(scriptActive(bool,QString)), SIGNAL(updateVacationScriptStatus(bool,QString)));
        job->start();
    }
    mMultiImapVacationDialog->delayedDestruct();
    mMultiImapVacationDialog = 0;
}
