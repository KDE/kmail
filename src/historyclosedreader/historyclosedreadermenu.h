/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: GPL-2.0-only
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
