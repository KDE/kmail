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
#include <MessageViewer/ViewerPluginManager>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QTreeWidget>

class PluginItem : public QTreeWidgetItem
{
public:
    PluginItem(QTreeWidgetItem *parent)
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
    mainLayout->setObjectName(QStringLiteral("mainlayout"));
    mainLayout->setMargin(0);

    mListWidget = new QTreeWidget(this);
    mListWidget->setObjectName(QStringLiteral("listwidget"));
    mListWidget->setHeaderHidden(true);
    mListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(mListWidget, &QTreeWidget::itemSelectionChanged, this, &ConfigurePluginsListWidget::slotItemSelectionChanged);

    mainLayout->addWidget(mListWidget);

}

ConfigurePluginsListWidget::~ConfigurePluginsListWidget()
{

}

void ConfigurePluginsListWidget::slotItemSelectionChanged()
{
    //TODO emit description changes.
}

void ConfigurePluginsListWidget::save()
{
    //TODO
}

void ConfigurePluginsListWidget::doLoadFromGlobalSettings()
{
    initialize();
}

void ConfigurePluginsListWidget::doResetToDefaultsOther()
{
    //TODO disable all ?
}

void ConfigurePluginsListWidget::initialize()
{
    mListWidget->clear();
    //Load plugin editor
    //Load messageviewer plugin
    const QVector<MessageViewer::ViewerPluginManager::ViewerPluginData> lstMessageViewerPluginData = MessageViewer::ViewerPluginManager::self()->pluginsDataList();
    if (!lstMessageViewerPluginData.isEmpty()) {
        QTreeWidgetItem *topLevel = new QTreeWidgetItem(mListWidget, QStringList() << i18n("Message Viewer"));
        topLevel->setFlags(topLevel->flags() & ~Qt::ItemIsSelectable);
        Q_FOREACH(const MessageViewer::ViewerPluginManager::ViewerPluginData &data, lstMessageViewerPluginData) {
            PluginItem *subItem = new PluginItem(topLevel);
            subItem->setText(0, data.mName);
            subItem->mIdentifier = data.mIdentifier;
            subItem->mDescription = data.mDescription;
            //TODO
            subItem->setCheckState(0, Qt::Checked);
            //TODO
        }
    }

    //Load webengineplugin
    //TODO
    mListWidget->expandAll();
}
