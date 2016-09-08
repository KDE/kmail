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

#ifndef CONFIGUREPLUGINSLISTWIDGET_H
#define CONFIGUREPLUGINSLISTWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
class QTreeWidget;
class ConfigurePluginsListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigurePluginsListWidget(QWidget *parent = Q_NULLPTR);
    ~ConfigurePluginsListWidget();

    void save();
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();
    void initialize();
Q_SIGNALS:
    void descriptionChanged(const QString &description);
    void changed();

private:
    void slotItemSelectionChanged();
    void slotItemChanged(QTreeWidgetItem *item, int column);
    class PluginItem : public QTreeWidgetItem
    {
    public:
        PluginItem(QTreeWidgetItem *parent)
            : QTreeWidgetItem(parent),
              mEnableByDefault(false)
        {

        }
        QString mIdentifier;
        QString mDescription;
        bool mEnableByDefault;
    };
    QList<PluginItem *> mPluginEditorItems;
    QList<PluginItem *> mPluginMessageViewerItems;
    QTreeWidget *mListWidget;
};

#endif // CONFIGUREPLUGINSLISTWIDGET_H
