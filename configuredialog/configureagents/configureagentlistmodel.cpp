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

#include "configureagentlistmodel.h"


ConfigureAgentListModel::ConfigureAgentListModel(QObject* parent) :
    QAbstractListModel(parent),
    mAgentItems()
{
}

ConfigureAgentListModel::~ConfigureAgentListModel()
{
}

bool ConfigureAgentListModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (row > rowCount()) {
        return false;
    }

    if (count <= 0) {
        count = 1;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        mAgentItems.insert(row, info);
    }
    endInsertRows();

    return true;
}

bool ConfigureAgentListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    const int row = index.row();
    if (row >= rowCount()) {
        return false;
    }

    switch (role) {
    case Qt::DisplayRole:
        mAgentItems[row].setName(value.toString());
        break;
    case InformationRole:
        mAgentItems[row].setInformation(value.toString());
        break;
    default:
        return false;
    }

    emit dataChanged(index, index);
    return true;
}

QVariant ConfigureAgentListModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case Qt::DisplayRole:
            return mAgentItems[row].name();
        case InformationRole:
            return mAgentItems[row].information();
        default:
            break;
        }
    }

    return QVariant();
}

int ConfigureAgentListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mAgentItems.count();
}

