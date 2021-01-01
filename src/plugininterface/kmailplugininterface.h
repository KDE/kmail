/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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
    ~KMailPluginInterface() override;

    void setMainWidget(KMMainWidget *mainwindow);
    void initializeInterfaceRequires(PimCommon::AbstractGenericPluginInterface *interface) override;
    static KMailPluginInterface *self();
private:
    KMMainWidget *mMainWindow = nullptr;
};

#endif // KMAILPLUGININTERFACE_H
