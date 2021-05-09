/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2008 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <KCModule>
#include <QTreeWidget>

class PluginView : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PluginView(QWidget *parent);
    ~PluginView() override;
};

class KCMKontactSummary : public KCModule
{
    Q_OBJECT

public:
    explicit KCMKontactSummary(QWidget *parent = nullptr, const QVariantList &args = {});

    void load() override;
    void save() override;

private:
    PluginView *mPluginView = nullptr;
};

