/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "unifiedmailboxmanager.h"

class UnifiedMailbox;
class UnifiedMailboxEditor : public QDialog
{
    Q_OBJECT
public:
    explicit UnifiedMailboxEditor(const KSharedConfigPtr &config, QWidget *parent = nullptr);
    explicit UnifiedMailboxEditor(UnifiedMailbox *mailbox, const KSharedConfigPtr &config, QWidget *parent = nullptr);

    ~UnifiedMailboxEditor() override;

private:
    void writeConfig();
    void readConfig();
    UnifiedMailbox *const mMailbox;
    KSharedConfigPtr mConfig;
};

