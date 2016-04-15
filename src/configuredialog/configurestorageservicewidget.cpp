/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

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
#include "settings/kmailsettings.h"
#include "PimCommon/StorageServiceSettingsWidget"
#include "PimCommon/ConfigureImmutableWidgetUtils"
using namespace PimCommon::ConfigureImmutableWidgetUtils;

#include <KLocalizedString>
#include <KMessageBox>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>

ConfigureStorageServiceWidget::ConfigureStorageServiceWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mStorageServiceWidget = new PimCommon::StorageServiceSettingsWidget;
    connect(mStorageServiceWidget, &PimCommon::StorageServiceSettingsWidget::changed, this, &ConfigureStorageServiceWidget::changed);
    lay->addWidget(mStorageServiceWidget);

    QHBoxLayout *hbox = new QHBoxLayout;
    mManageStorageService = new QPushButton(i18n("Manage Storage Service"));
    hbox->addWidget(mManageStorageService);
    hbox->addStretch();
    lay->addLayout(hbox);
    if (QStandardPaths::findExecutable(QStringLiteral("storageservicemanager")).isEmpty()) {
        mManageStorageService->setEnabled(false);
    } else {
        connect(mManageStorageService, &QPushButton::clicked, this, &ConfigureStorageServiceWidget::slotManageStorageService);
    }
    QList<PimCommon::StorageServiceAbstract::Capability> lstCapabilities;
    lstCapabilities << PimCommon::StorageServiceAbstract::ShareLinkCapability;
    if (KMKernel::self() && KMKernel::self()->storageServiceManager()) {
        mStorageServiceWidget->setListService(KMKernel::self()->storageServiceManager()->listService(), lstCapabilities);
    } else {
        mStorageServiceWidget->setEnabled(false);
    }
    setLayout(lay);
}

ConfigureStorageServiceWidget::~ConfigureStorageServiceWidget()
{
}

void ConfigureStorageServiceWidget::slotManageStorageService()
{
    if (!QProcess::startDetached(QStringLiteral("storageservicemanager")))
        KMessageBox::error(this, i18n("Could not start storage service manager; "
                                      "please check your installation."),
                           i18n("KMail Error"));
}

void ConfigureStorageServiceWidget::save()
{
    if (KMKernel::self() && KMKernel::self()->storageServiceManager()) {
        KMKernel::self()->storageServiceManager()->setListService(mStorageServiceWidget->listService());
    }
}

void ConfigureStorageServiceWidget::doLoadFromGlobalSettings()
{
}
