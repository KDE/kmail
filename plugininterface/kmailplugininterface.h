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

#ifndef KMAILPLUGININTERFACE_H
#define KMAILPLUGININTERFACE_H

#include <QObject>
#include <pimcommon/plugininterface.h>
class KActionCollection;
class KMMainWidget;
class KMailPluginInterface : public PimCommon::PluginInterface
{
    Q_OBJECT
public:
    explicit KMailPluginInterface(KActionCollection *ac, QObject *parent = Q_NULLPTR);
    ~KMailPluginInterface();

    void setMainWidget(KMMainWidget *mainwindow);
    void initializeInterfaceRequires(PimCommon::GenericPluginInterface *interface) Q_DECL_OVERRIDE;
private:
    KMMainWidget *mMainWindow;
};

#endif // KMAILPLUGININTERFACE_H
