/*
   Copyright (C) 2014-2019 Montel Laurent <montel@kde.org>

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

#ifndef FOLLOWUPREMINDERINFOWIDGET_H
#define FOLLOWUPREMINDERINFOWIDGET_H

#include <QWidget>
#include <KConfigGroup>
#include <QTreeWidgetItem>
#include <QVariantList>
#include <AkonadiCore/Item>
class QTreeWidget;
namespace FollowUpReminder {
class FollowUpReminderInfo;
}

class FollowUpReminderInfoItem : public QTreeWidgetItem
{
public:
    explicit FollowUpReminderInfoItem(QTreeWidget *parent = nullptr);
    ~FollowUpReminderInfoItem();

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
