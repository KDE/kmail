/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
class QUndoStack;
class HistorySwitchFolderManager : public QObject
{
    Q_OBJECT
public:
    explicit HistorySwitchFolderManager(QObject *parent = nullptr);
    ~HistorySwitchFolderManager() override;
    // Add static method
private:
    QUndoStack *const mUndoStack;
};
