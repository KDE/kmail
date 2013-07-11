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
    createItem(QLatin1String("org...."), i18n("Send Later Agent"));
    //TODO
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &name)
{
    QListWidgetItem *item = new QListWidgetItem(name, mListWidget);
    item->setData(InterfaceName, interfaceName);
}

void ConfigureAgentsWidget::save()
{
    //TODO
}

QString ConfigureAgentsWidget::helpAnchor() const
{
    return QString();
}

void ConfigureAgentsWidget::doLoadFromGlobalSettings()
{
    //TODO
}

void ConfigureAgentsWidget::doResetToDefaultsOther()
{
    //TODO
}


#include "configureagentswidget.moc"
