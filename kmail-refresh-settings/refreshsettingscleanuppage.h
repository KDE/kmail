/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <QWidget>

class RefreshSettingsCleanupPage : public QWidget
{
    Q_OBJECT
public:
    explicit RefreshSettingsCleanupPage(QWidget *parent = nullptr);
    ~RefreshSettingsCleanupPage() override;

Q_SIGNALS:
    void cleanDoneInfo(const QString &msg);
    void cleanUpDone();

private:
    void cleanSettings();
    void cleanupFolderSettings(KConfigGroup &oldGroup);
    void initCleanupFolderSettings(const QString &configName);
    void initCleanupFiltersSettings(const QString &configName);
    void initCleanDialogSettings(const QString &configName);
    void removeTipOfDay(const QString &configName);
    void initCleanupDialogSettings(const QString &configName);
};

