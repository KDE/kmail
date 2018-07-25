/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

#include <QDialog>

#include <KSharedConfig>

#include "unifiedmailboxmanager.h"

class QStandardItemModel;
class MailKernel;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(KSharedConfigPtr config, UnifiedMailboxManager &manager,
                            WId windowId, QWidget *parent = nullptr);
    ~SettingsDialog() override;

public Q_SLOTS:
    void accept() override;

private:
    void loadBoxes();
    void addBox(const UnifiedMailbox &box);

private:
    QStandardItemModel *mBoxModel = nullptr;
    UnifiedMailboxManager &mBoxManager;
    MailKernel *mKernel = nullptr;
};



#endif
