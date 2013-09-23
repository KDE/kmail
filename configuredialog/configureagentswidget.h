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

#ifndef CONFIGUREAGENTSWIDGET_H
#define CONFIGUREAGENTSWIDGET_H

#include <QWidget>

class QTreeWidget;
class QSplitter;
class KTextEdit;
class QTreeWidgetItem;
class ConfigureAgentsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigureAgentsWidget(QWidget *parent = 0);
    ~ConfigureAgentsWidget();

    void save();
    QString helpAnchor() const;
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();

private Q_SLOTS:
    void slotItemClicked(QTreeWidgetItem *item);

Q_SIGNALS:
    void changed();

private:
    void writeConfig();
    void readConfig();

    enum ConfigureAgentsColumn {
        AgentState = 0,
        AgentName
    };

    enum ItemData {
        InterfaceName = Qt::UserRole + 1,
        PathName = Qt::UserRole + 2,
        Description = Qt::UserRole + 3
    };

    void changeAgentActiveState(bool enable, const QString &interfaceName, const QString &pathName);
    bool agentActivateState(const QString &interfaceName, const QString &pathName, bool &failed);
    void initialize();
    void addInfos(QTreeWidgetItem *item, const QString &desktopFile);
    void createItem(const QString &interfaceName, const QString &path, const QString &name);
    QStringList mAgentPathList;
    QTreeWidget *mTreeWidget;
    QSplitter *mSplitter;
    KTextEdit *mDescription;
};

#endif // CONFIGUREAGENTSWIDGET_H
