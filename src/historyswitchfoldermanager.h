/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
class QUndoStack;
namespace Akonadi
{
class Collection;
}
class HistorySwitchFolderManager : public QObject
{
    Q_OBJECT
public:
    explicit HistorySwitchFolderManager(QObject *parent = nullptr);
    ~HistorySwitchFolderManager() override;
    // Add static method
    void clear();
    void addHistory();

    void addCollection(const Akonadi::Collection &col);
Q_SIGNALS:
    void switchToFolder(const Akonadi::Collection &col);

private:
    QUndoStack *const mUndoStack;
};
