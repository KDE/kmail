/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QTreeWidgetItem>
#include <QWidget>

class KConfigGroup;
class QTreeWidget;
class QPushButton;

namespace Snooze
{
class SnoozeInfo;
}

class SnoozeItem : public QTreeWidgetItem
{
public:
    explicit SnoozeItem(QTreeWidget *parent = nullptr);
    ~SnoozeItem() override;

    void setInfo(Snooze::SnoozeInfo *info);
    [[nodiscard]] Snooze::SnoozeInfo *info() const;

private:
    Snooze::SnoozeInfo *mInfo = nullptr;
};

class SnoozeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SnoozeWidget(QWidget *parent = nullptr);
    ~SnoozeWidget() override;

    enum SnoozeColumn {
        From = 0,
        Subject,
        WakeUpDate,
    };

    void load();
    [[nodiscard]] bool save();
    void needToReload();

    void saveTreeWidgetHeader(KConfigGroup &group);
    void restoreTreeWidgetHeader(const QByteArray &group);

    [[nodiscard]] QList<Akonadi::Item::Id> messagesToUnsnooze() const;

Q_SIGNALS:
    void wakeUpNow(Akonadi::Item::Id id);

private:
    void slotRemoveItem();
    void slotWakeUpNow();
    void slotCustomContextMenuRequested(QPoint pos);
    void updateButtons();
    void createOrUpdateItem(Snooze::SnoozeInfo *info, SnoozeItem *item = nullptr);

    QList<Akonadi::Item::Id> mListMessagesToUnsnooze;
    QTreeWidget *const mTreeWidget;
    QPushButton *const mRemoveButton;
    QPushButton *const mWakeUpNowButton;
    bool mChanged = false;
};
