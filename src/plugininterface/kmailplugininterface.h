/*
   Copyright (C) 2015-2018 Montel Laurent <montel@kde.org>

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

#ifndef KMAILPLUGININTERFACE_H
#define KMAILPLUGININTERFACE_H

#include <QObject>
#include <PimCommonAkonadi/PluginInterface>
class KMMainWidget;
class KMailPluginInterface : public PimCommon::PluginInterface
{
    Q_OBJECT
public:
    explicit KMailPluginInterface(QObject *parent = nullptr);
    ~KMailPluginInterface();

    void setMainWidget(KMMainWidget *mainwindow);
    void initializeInterfaceRequires(PimCommon::AbstractGenericPluginInterface *interface) override;
    static KMailPluginInterface *self();
private:
    KMMainWidget *mMainWindow = nullptr;
};

#endif // KMAILPLUGININTERFACE_H
