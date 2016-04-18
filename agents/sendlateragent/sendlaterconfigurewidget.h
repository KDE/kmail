/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SENDLATERCONFIGUREWIDGET_H
#define SENDLATERCONFIGUREWIDGET_H
#include "ui_sendlaterconfigurewidget.h"

#include <AkonadiCore/Item>

#include <QTreeWidgetItem>
#include <KConfigGroup>

namespace SendLater
{
class SendLaterInfo;
}

class SendLaterItem : public QTreeWidgetItem
{
public:
    explicit SendLaterItem(QTreeWidget *parent = Q_NULLPTR);
    ~SendLaterItem();

    void setInfo(SendLater::SendLaterInfo *info);
    SendLater::SendLaterInfo *info() const;

private:
    SendLater::SendLaterInfo *mInfo;
};

class SendLaterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SendLaterWidget(QWidget *parent = Q_NULLPTR);
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
    QList<Akonadi::Item::Id> messagesToRemove() const;

private Q_SLOTS:
    void slotRemoveItem();
    void slotModifyItem();
    void updateButtons();
    void customContextMenuRequested(const QPoint &);
    void slotSendNow();

Q_SIGNALS:
    void sendNow(Akonadi::Item::Id);

private:
    void createOrUpdateItem(SendLater::SendLaterInfo *info, SendLaterItem *item = Q_NULLPTR);
    void load();
    QList<Akonadi::Item::Id> mListMessagesToRemove;
    bool mChanged;
    Ui::SendLaterConfigureWidget *mWidget;
};

#endif // SENDLATERCONFIGUREWIDGET_H

