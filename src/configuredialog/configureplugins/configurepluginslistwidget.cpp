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

#include "configurepluginslistwidget.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>

class PluginItem : public QTreeWidgetItem
{
public:
    PluginItem(QTreeWidget *parent)
        : QTreeWidgetItem(parent)
    {

    }
    QString mIdentifier;
    QString mDescription;
};

ConfigurePluginsListWidget::ConfigurePluginsListWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);

    mListWidget = new QTreeWidget(this);
    mListWidget->setObjectName(QStringLiteral("listwidget"));
    mainLayout->addWidget(mListWidget);

}

ConfigurePluginsListWidget::~ConfigurePluginsListWidget()
{

}

void ConfigurePluginsListWidget::save()
{
    //TODO
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    //TODO
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    //TODO
}

void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();
    //Load plugin editor
    //Load messageviewer plugin
    //Load webengineplugin
    //TODO
}
