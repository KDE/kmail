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


#ifndef CONFIGUREAGENTLISTMODEL_H
#define CONFIGUREAGENTLISTMODEL_H

#include <QAbstractItemModel>



class ConfigureAgentListModel: public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        DescriptionRole = Qt::UserRole + 1
    };
    ConfigureAgentListModel(QObject *parent = 0);

    virtual ~ConfigureAgentListModel();

    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

private:
    struct AgentItem
    {
        AgentItem()
            : checked(false)
        {

        }
        bool checked;
        QString text;
        QString description;
    };

    QList<AgentItem> mAgentItems;
};

#endif // CONFIGUREAGENTLISTMODEL_H
