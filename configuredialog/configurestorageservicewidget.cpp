/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "configurestorageservicewidget.h"
#include "kmkernel.h"
#include "settings/globalsettings.h"
#include "pimcommon/storageservice/settings/storageservicesettingswidget.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;

#include <KLocalizedString>
#include <KStandardDirs>
#include <KMessageBox>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QProcess>
#include <QPushButton>

ConfigureStorageServiceWidget::ConfigureStorageServiceWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;
    mActivateStorageService = new QCheckBox(i18n("Offer to share for files larger than"));
    hbox->addWidget(mActivateStorageService);
    mLimitAttachment = new QSpinBox;
    mLimitAttachment->setMinimum(0);
    mLimitAttachment->setMaximum(9999);
    mLimitAttachment->setSuffix(i18nc("mega bytes"," MB"));
    hbox->addWidget(mLimitAttachment);
    lay->addLayout(hbox);
    connect(mActivateStorageService, SIGNAL(toggled(bool)), mLimitAttachment, SLOT(setEnabled(bool)));
    connect(mActivateStorageService, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(mLimitAttachment, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    mStorageServiceWidget = new PimCommon::StorageServiceSettingsWidget;
    connect(mStorageServiceWidget, SIGNAL(changed()), this, SIGNAL(changed()));
    lay->addWidget(mStorageServiceWidget);

    hbox = new QHBoxLayout;
    mManageStorageService = new QPushButton(i18n("Manage Storage Service"));
    hbox->addWidget(mManageStorageService);
    hbox->addStretch();
    lay->addLayout(hbox);
    if (KStandardDirs::findExe(QLatin1String("storageservicemanager")).isEmpty()) {
        mManageStorageService->setEnabled(false);
    } else {
        connect(mManageStorageService, SIGNAL(clicked(bool)), this, SLOT(slotManageStorageService()));
    }
    setLayout(lay);
}

ConfigureStorageServiceWidget::~ConfigureStorageServiceWidget()
{
}

void ConfigureStorageServiceWidget::slotManageStorageService()
{
    if ( !QProcess::startDetached(QLatin1String("storageservicemanager") ) )
        KMessageBox::error( this, i18n( "Could not start storage service manager; "
                                        "please check your installation." ),
                            i18n( "KMail Error" ) );
}

void ConfigureStorageServiceWidget::save()
{
    saveCheckBox(mActivateStorageService, GlobalSettings::self()->useStorageServiceItem());
    saveSpinBox(mLimitAttachment, GlobalSettings::self()->storageServiceLimitItem());
    KMKernel::self()->storageServiceManager()->setListService(mStorageServiceWidget->listService());
}

void ConfigureStorageServiceWidget::doLoadFromGlobalSettings()
{
    loadWidget(mActivateStorageService, GlobalSettings::self()->useStorageServiceItem());
    loadWidget(mLimitAttachment, GlobalSettings::self()->storageServiceLimitItem());
    mStorageServiceWidget->setListService(KMKernel::self()->storageServiceManager()->listService(), PimCommon::StorageServiceAbstract::ShareLinkCapability);
    mLimitAttachment->setEnabled(mActivateStorageService->isChecked());
}

#include "moc_configurestorageservicewidget.cpp"
