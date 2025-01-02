/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <PimCommonAkonadi/PluginInterface>
#include <QObject>
class KMMainWidget;
class KMailPluginInterface : public PimCommon::PluginInterface
{
    Q_OBJECT
public:
    ~KMailPluginInterface() override;

    void setMainWidget(KMMainWidget *mainwindow);
    bool initializeInterfaceRequires(PimCommon::AbstractGenericPluginInterface *interface) override;
    static KMailPluginInterface *self();

private:
    explicit KMailPluginInterface(QObject *parent = nullptr);
    KMMainWidget *mMainWindow = nullptr;
};
