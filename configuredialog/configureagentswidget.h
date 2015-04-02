/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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
#include "configureagents/configureagentitem.h"

class QSplitter;
class KTextEdit;
class QPushButton;
class ConfigureAgentListView;
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

Q_SIGNALS:
    void changed();

private:
    void writeConfig();
    void readConfig();

    bool agentActivateState(const QString &interfaceName, const QString &pathName, bool &failed);
    void initialize();
    void addInfos(const QString &desktopFile, ConfigureAgentItem &item);
    void createItem(const QString &interfaceName, const QString &path, const QString &desktopFileName, QVector<ConfigureAgentItem> &listItem);
    QStringList mAgentPathList;
    ConfigureAgentListView *mConfigureAgentListView;
    QSplitter *mSplitter;
    KTextEdit *mDescription;
};

#endif // CONFIGUREAGENTSWIDGET_H
