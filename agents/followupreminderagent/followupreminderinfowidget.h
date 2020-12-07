/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FOLLOWUPREMINDERINFOWIDGET_H
#define FOLLOWUPREMINDERINFOWIDGET_H

#include <QWidget>
#include <KConfigGroup>
#include <QTreeWidgetItem>
#include <AkonadiCore/Item>
class QTreeWidget;
namespace FollowUpReminder {
class FollowUpReminderInfo;
}

class FollowUpReminderInfoItem : public QTreeWidgetItem
{
public:
    explicit FollowUpReminderInfoItem(QTreeWidget *parent = nullptr);
    ~FollowUpReminderInfoItem() override;

    void setInfo(FollowUpReminder::FollowUpReminderInfo *info);
    FollowUpReminder::FollowUpReminderInfo *info() const;

private:
    FollowUpReminder::FollowUpReminderInfo *mInfo = nullptr;
};

class FollowUpReminderInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FollowUpReminderInfoWidget(QWidget *parent = nullptr);
    ~FollowUpReminderInfoWidget() override;

    void restoreTreeWidgetHeader(const QByteArray &data);
    void saveTreeWidgetHeader(KConfigGroup &group);

    void setInfo(const QList<FollowUpReminder::FollowUpReminderInfo *> &infoList);

    bool save() const;
    void load();

    Q_REQUIRED_RESULT QList<qint32> listRemoveId() const;

private:
    void slotCustomContextMenuRequested(const QPoint &pos);
    void createOrUpdateItem(FollowUpReminder::FollowUpReminderInfo *info, FollowUpReminderInfoItem *item = nullptr);
    void removeItem(const QList<QTreeWidgetItem *> &mailItem);
    void openShowMessage(Akonadi::Item::Id id);
    enum ItemData {
        AnswerItemId = Qt::UserRole + 1,
        AnswerItemFound = Qt::UserRole + 2
    };

    enum FollowUpReminderColumn {
        To = 0,
        Subject,
        DeadLine,
        AnswerWasReceived,
        MessageId,
        AnswerMessageId
    };
    QList<qint32> mListRemoveId;
    QTreeWidget *mTreeWidget = nullptr;
    bool mChanged = false;
};
#endif // FOLLOWUPREMINDERINFOWIDGET_H
