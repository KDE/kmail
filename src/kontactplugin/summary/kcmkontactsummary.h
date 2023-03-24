/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2008 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once
#include "kcmutils_version.h"
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
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    explicit KCMKontactSummary(QWidget *parent, const QVariantList &args);
#else
    explicit KCMKontactSummary(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
#endif

    void load() override;
    void save() override;

private:
    PluginView *const mPluginView;
};
