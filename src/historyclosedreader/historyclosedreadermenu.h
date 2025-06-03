/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KActionMenu>

class HistoryClosedReaderMenu : public KActionMenu
{
    Q_OBJECT
public:
    explicit HistoryClosedReaderMenu(QObject *parent = nullptr);
    ~HistoryClosedReaderMenu() override;

    [[nodiscard]] QAction *reopenAction() const;

    void createReOpenClosedAction();
Q_SIGNALS:
    void openMessage(Akonadi::Item::Id id);

private:
    void slotClear();
    void updateMenu();
    void addReOpenClosedAction();
    void slotReopenLastClosedViewer();
    QAction *mReopenAction = nullptr;
    QAction *mSeparatorAction = nullptr;
};
