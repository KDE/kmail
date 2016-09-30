/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

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

#ifndef CONFIGUREPLUGINPAGE_H
#define CONFIGUREPLUGINPAGE_H

#include "configuredialog_p.h"

#include <QWidget>
namespace PimCommon
{
class ConfigurePluginsWidget;
}
class KMAIL_EXPORT ConfigurePluginPage : public ConfigModule
{
    Q_OBJECT
public:
    explicit ConfigurePluginPage(QWidget *parent);
    ~ConfigurePluginPage();

    QString helpAnchor() const Q_DECL_OVERRIDE;
    void load() Q_DECL_OVERRIDE;
    void save() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;
private:
    void slotConfigureChanged();
    PimCommon::ConfigurePluginsWidget *mConfigurePlugins;
};

#endif // CONFIGUREPLUGINPAGE_H
