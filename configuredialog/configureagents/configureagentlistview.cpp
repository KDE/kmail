/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "configureagentlistview.h"
#include "configureagentlistdelegate.h"
#include "configureagentlistmodel.h"

#include <QDBusInterface>
#include <KDebug>

ConfigureAgentListView::ConfigureAgentListView(QWidget *parent)
    : QListView(parent)
{
    ConfigureAgentListDelegate *configureListDelegate = new ConfigureAgentListDelegate(this, this);
    connect(configureListDelegate, SIGNAL(requestConfiguration(QModelIndex)), this, SLOT(slotConfigureAgent(QModelIndex)));
    connect(configureListDelegate, SIGNAL(requestChangeAgentState(QModelIndex,bool)), this, SLOT(slotChangeAgentState(QModelIndex,bool)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(slotAgentClicked(QModelIndex)));
    ConfigureAgentListModel *configureAgentListModel  = new ConfigureAgentListModel(this);

    setModel(configureAgentListModel);
    setItemDelegate(configureListDelegate);
}

ConfigureAgentListView::~ConfigureAgentListView()
{

}

void ConfigureAgentListView::setAgentItems(const QVector<ConfigureAgentItem> &lst)
{
    Q_FOREACH(const ConfigureAgentItem &agentItem, lst) {
        model()->insertRow(0);
        const QModelIndex index = model()->index(0, 0);
        model()->setData(index, agentItem.agentName(), Qt::DisplayRole);
        model()->setData(index, agentItem.checked(), Qt::CheckStateRole);
        model()->setData(index, agentItem.description(), ConfigureAgentListModel::DescriptionRole);
        model()->setData(index, agentItem.failed(), ConfigureAgentListModel::FailedRole);
        model()->setData(index, agentItem.interfaceName(), ConfigureAgentListModel::InterfaceNameRole);
        model()->setData(index, agentItem.path(), ConfigureAgentListModel::PathRole);
    }
    model()->sort(Qt::DisplayRole);
}

void ConfigureAgentListView::slotConfigureAgent(const QModelIndex &index)
{
    const QAbstractItemModel* model = index.model();
    const QString interfaceName = model->data(index, ConfigureAgentListModel::InterfaceNameRole).toString();
    const QString path = model->data(index, ConfigureAgentListModel::PathRole).toString();
    if (!interfaceName.isEmpty() && !path.isEmpty()) {
        QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, path );
        if (interface.isValid()) {
            interface.call(QLatin1String("showConfigureDialog"), (qlonglong)winId());
        } else {
            kDebug()<<" interface does not exist ";
        }

    }
}

void ConfigureAgentListView::slotChangeAgentState(const QModelIndex &index, bool enable)
{
    const QAbstractItemModel* model = index.model();
    const QString interfaceName = model->data(index, ConfigureAgentListModel::InterfaceNameRole).toString();
    const QString path = model->data(index, ConfigureAgentListModel::PathRole).toString();
    if (!interfaceName.isEmpty() && !path.isEmpty()) {
        QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, path );
        if (interface.isValid()) {
            interface.call(QLatin1String("setEnableAgent"), enable);
        } else {
            kDebug()<<interfaceName << "does not exist ";
        }
    }
}

void ConfigureAgentListView::slotAgentClicked(const QModelIndex &index)
{
    const QAbstractItemModel* model = index.model();
    const QString description = model->data(index, ConfigureAgentListModel::DescriptionRole).toString();
    Q_EMIT descriptionChanged(description);
}
