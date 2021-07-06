/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>
   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class KMailPluginCheckBeforeDeletingManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginCheckBeforeDeletingManagerInterface(QObject *parent = nullptr);
    ~KMailPluginCheckBeforeDeletingManagerInterface() override;
    void initializePlugins();

    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

private:
    QWidget *mParentWidget = nullptr;
    bool mWasInitialized = false;
};
