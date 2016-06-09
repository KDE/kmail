/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "configureagentlistmodel.h"

#include <QColor>

ConfigureAgentListModel::ConfigureAgentListModel(QObject *parent) :
    QAbstractListModel(parent),
    mAgentItems()
{
}

ConfigureAgentListModel::~ConfigureAgentListModel()
{
}

bool ConfigureAgentListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > rowCount()) {
        return false;
    }

    if (count <= 0) {
        count = 1;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        ConfigureAgentItem info;
        mAgentItems.insert(row, info);
    }
    endInsertRows();

    return true;
}

bool ConfigureAgentListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (row >= rowCount()) {
        return false;
    }

    switch (role) {
    case Qt::DisplayRole:
        mAgentItems[row].setAgentName(value.toString());
        break;
    case DescriptionRole:
        mAgentItems[row].setDescription(value.toString());
        break;
    case PathRole:
        mAgentItems[row].setPath(value.toString());
        break;
    case InterfaceNameRole:
        mAgentItems[row].setInterfaceName(value.toString());
        break;
    case FailedRole:
        mAgentItems[row].setFailed(value.toBool());
        break;
    case Qt::CheckStateRole:
        mAgentItems[row].setChecked(value.toBool());
        break;
    default:
        return false;
    }

    Q_EMIT dataChanged(index, index);
    return true;
}

QVariant ConfigureAgentListModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case Qt::DisplayRole:
            return mAgentItems[row].agentName();
        case DescriptionRole:
            return mAgentItems[row].description();
        case PathRole:
            return mAgentItems[row].path();
        case InterfaceNameRole:
            return mAgentItems[row].interfaceName();
        case FailedRole:
            return mAgentItems[row].failed();
        case Qt::CheckStateRole:
            return mAgentItems[row].checked();
        case Qt::BackgroundColorRole:
            if (mAgentItems[row].failed()) {
                return QColor(Qt::red);
            } else {
                return QColor();
            }
        default:
            break;
        }
    }

    return QVariant();
}

int ConfigureAgentListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mAgentItems.count();
}

