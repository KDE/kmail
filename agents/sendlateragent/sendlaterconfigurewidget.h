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

#ifndef SENDLATERCONFIGUREWIDGET_H
#define SENDLATERCONFIGUREWIDGET_H
#include "ui_sendlaterconfigurewidget.h"

#include <AkonadiCore/Item>

#include <QTreeWidgetItem>
#include <KConfigGroup>

namespace SendLater {
class SendLaterInfo;
}

class SendLaterItem : public QTreeWidgetItem
{
public:
    explicit SendLaterItem(QTreeWidget *parent = nullptr);
    ~SendLaterItem();

    void setInfo(SendLater::SendLaterInfo *info);
    SendLater::SendLaterInfo *info() const;

private:
    SendLater::SendLaterInfo *mInfo = nullptr;
};

class SendLaterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SendLaterWidget(QWidget *parent = nullptr);
    ~SendLaterWidget();

    enum SendLaterColumn {
        To = 0,
        Subject,
        SendAround,
        Recursive,
        MessageId
    };

    void save();
    void saveTreeWidgetHeader(KConfigGroup &group);
    void restoreTreeWidgetHeader(const QByteArray &group);
    void needToReload();
    QVector<Akonadi::Item::Id> messagesToRemove() const;

Q_SIGNALS:
    void sendNow(Akonadi::Item::Id);

private:
    void slotRemoveItem();
    void slotModifyItem();
    void updateButtons();
    void slotCustomContextMenuRequested(const QPoint &);
    void slotSendNow();
    void createOrUpdateItem(SendLater::SendLaterInfo *info, SendLaterItem *item = nullptr);
    void load();
    QVector<Akonadi::Item::Id> mListMessagesToRemove;
    bool mChanged;
    Ui::SendLaterConfigureWidget *mWidget = nullptr;
};

#endif // SENDLATERCONFIGUREWIDGET_H
