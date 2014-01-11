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

#ifndef CONFIGURESTORAGESERVICEWIDGET_H
#define CONFIGURESTORAGESERVICEWIDGET_H

#include <QWidget>
class QCheckBox;
namespace PimCommon {
class StorageServiceSettingsWidget;
}
class ConfigureStorageServiceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigureStorageServiceWidget(QWidget *parent=0);
    ~ConfigureStorageServiceWidget();

    void saveSettings();
    void loadSettings();

private:
    QCheckBox *mActivateStorageService;
    PimCommon::StorageServiceSettingsWidget *mStorageServiceWidget;

};

#endif // CONFIGURESTORAGESERVICEWIDGET_H
