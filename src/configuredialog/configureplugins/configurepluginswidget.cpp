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
#include "configurepluginswidget.h"

#include <QVBoxLayout>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QSplitter>

ConfigurePluginsWidget::ConfigurePluginsWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    mSplitter = new QSplitter(this);
    mSplitter->setObjectName(QStringLiteral("splitter"));
    mSplitter->setChildrenCollapsible(false);
    layout->addWidget(mSplitter);

    mConfigureListWidget = new ConfigurePluginsListWidget(this);
    mConfigureListWidget->setObjectName(QStringLiteral("configureListWidget"));
    mSplitter->addWidget(mConfigureListWidget);


    readConfig();
}

ConfigurePluginsWidget::~ConfigurePluginsWidget()
{
    writeConfig();
}

void ConfigurePluginsWidget::save()
{
    //TODO
}

QString ConfigurePluginsWidget::helpAnchor() const
{
    //TODO
    return {};
}

void ConfigurePluginsWidget::doLoadFromGlobalSettings()
{
    //TODO
}

void ConfigurePluginsWidget::doResetToDefaultsOther()
{
    //TODO
}

void ConfigurePluginsWidget::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigurePluginsWidget");
    QList<int> size;
    size << 400 << 100;
    mSplitter->setSizes(group.readEntry("splitter", size));
}

void ConfigurePluginsWidget::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigurePluginsWidget");
    group.writeEntry("splitter", mSplitter->sizes());
}


void ConfigurePluginsWidget::initialize()
{

}
