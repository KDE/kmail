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

#include "plugineditor.h"
#include "plugineditormanager.h"

#include <QFileInfo>
#include <kpluginmetadata.h>

class PluginEditorManagerInstancePrivate
{
public:
    PluginEditorManagerInstancePrivate()
        : pluginManager(new PluginEditorManager)
    {
    }

    ~PluginEditorManagerInstancePrivate()
    {
        delete pluginManager;
    }
    PluginEditorManager *pluginManager;
};

class PluginEditorInfo
{
public:
    PluginEditorInfo()
        : plugin(Q_NULLPTR)
    {

    }
    QString saveName() const;

    KPluginMetaData metaData;
    PluginEditor *plugin;
};


Q_GLOBAL_STATIC(PluginEditorManagerInstancePrivate, sInstance)

class PluginEditorManagerPrivate
{
public:
    PluginEditorManagerPrivate(PluginEditorManager *qq)
        : q(qq)
    {
    }
    PluginEditorManager *q;
};

PluginEditorManager::PluginEditorManager(QObject *parent)
    : QObject(parent),
      d(new PluginEditorManagerPrivate(this))
{

}

PluginEditorManager::~PluginEditorManager()
{
    delete d;
}

PluginEditorManager *PluginEditorManager::self()
{
    return sInstance->pluginManager;
}

QString PluginEditorInfo::saveName() const
{
    return QFileInfo(metaData.fileName()).baseName();
}
