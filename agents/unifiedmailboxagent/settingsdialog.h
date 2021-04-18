/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include <KSharedConfig>

#include "unifiedmailboxmanager.h"

class QStandardItemModel;
class MailKernel;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const KSharedConfigPtr &config, UnifiedMailboxManager &manager, WId windowId, QWidget *parent = nullptr);
    ~SettingsDialog() override;

private:
    void loadBoxes();
    void addBox(UnifiedMailbox *box);

private:
    void readConfig();
    void writeConfig();
    QStandardItemModel *const mBoxModel;
    UnifiedMailboxManager &mBoxManager;
    MailKernel *const mKernel;
    const KSharedConfigPtr mConfig;
};

