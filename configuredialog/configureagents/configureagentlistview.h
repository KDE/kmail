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

#ifndef CONFIGUREAGENTLISTVIEW_H
#define CONFIGUREAGENTLISTVIEW_H

#include <QListView>
#include "configureagentitem.h"
class ConfigureAgentListView : public QListView
{
    Q_OBJECT
public:
    explicit ConfigureAgentListView(QWidget *parent = Q_NULLPTR);
    ~ConfigureAgentListView();

    void setAgentItems(const QVector<ConfigureAgentItem> &lst);
    void save();
    void resetToDefault();

private Q_SLOTS:
    void slotConfigureAgent(const QModelIndex &index);
    void slotAgentClicked(const QModelIndex &index);

Q_SIGNALS:
    void descriptionChanged(const QString &desc);
    void agentChanged();

private:
    void changeAgentActiveState(const QString &interfaceName, const QString &path, bool enable);
};

#endif // CONFIGUREAGENTLISTVIEW_H
