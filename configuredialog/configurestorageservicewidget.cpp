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
#include "settings/globalsettings.h"
#include "pimcommon/storageservice/settings/storageservicesettingswidget.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include <KLocalizedString>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>

ConfigureStorageServiceWidget::ConfigureStorageServiceWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mActivateStorageService = new QCheckBox(i18n("Use Storage Service for Attachments"));
    lay->addWidget(mActivateStorageService);

    mStorageServiceWidget = new PimCommon::StorageServiceSettingsWidget;
    lay->addWidget(mStorageServiceWidget);

    setLayout(lay);
}

ConfigureStorageServiceWidget::~ConfigureStorageServiceWidget()
{

}

void ConfigureStorageServiceWidget::save()
{
    saveCheckBox(mActivateStorageService, GlobalSettings::self()->useStorageServiceItem());
    //TODO
}

void ConfigureStorageServiceWidget::doLoadFromGlobalSettings()
{
    loadWidget(mActivateStorageService, GlobalSettings::self()->useStorageServiceItem());
    //TODO
}
