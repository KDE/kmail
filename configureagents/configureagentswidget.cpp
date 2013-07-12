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

#include "configureagentswidget.h"

#include <KLocale>

#include <QVBoxLayout>
#include <QListWidget>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>


ConfigureAgentsWidget::ConfigureAgentsWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mListWidget = new QListWidget;
    setLayout(lay);
    initialize();
}

ConfigureAgentsWidget::~ConfigureAgentsWidget()
{

}

void ConfigureAgentsWidget::initialize()
{
    //TODO find a generic method.
    createItem(QLatin1String("org...."), QLatin1String(""), i18n("Send Later Agent"));
    //TODO
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &path, const QString &name)
{
    QListWidgetItem *item = new QListWidgetItem(name, mListWidget);
    item->setData(InterfaceName, interfaceName);
    item->setData(PathName, path);
}

bool ConfigureAgentsWidget::agentActivateState(const QString &interfaceName, const QString &pathName)
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, pathName );
    if (interface.isValid()) {
        QDBusReply<bool> enabled = interface.call(QLatin1String("enabledAgent"));
        if (enabled.isValid()) {
            return enabled;
        } else {
            qDebug()<<interfaceName << "doesn't have enabledAgent function";
            return false;
        }
    } else {
        qDebug()<<interfaceName << "does not exist ";
    }
    return false;
}

void ConfigureAgentsWidget::changeAgentActiveState(bool enable, const QString &interfaceName, const QString &pathName)
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, pathName );
    if (interface.isValid()) {
        interface.call(QLatin1String("setEnableAgent"), enable);
    } else {
        qDebug()<<interfaceName << "does not exist ";
    }
}

void ConfigureAgentsWidget::save()
{
    for (int i=0; i <mListWidget->count(); ++i) {

    }
    //TODO
}

QString ConfigureAgentsWidget::helpAnchor() const
{
    return QString();
}

void ConfigureAgentsWidget::doLoadFromGlobalSettings()
{
    //TODO
    for (int i=0; i <mListWidget->count(); ++i) {
        //Reload
    }
}

void ConfigureAgentsWidget::doResetToDefaultsOther()
{
    //TODO
    for (int i=0; i <mListWidget->count(); ++i) {
        //Reset to default
    }
}


#include "configureagentswidget.moc"
