/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

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

#include "sendlaterconfigurewidget.h"
#include "sendlaterinfo.h"
#include "sendlaterutil.h"
#include "sendlaterdialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KHelpMenu>
#include <QMenu>
#include <KMessageBox>
#include <QIcon>
#include <QPointer>

namespace
{
inline QString sendLaterItemPattern()
{
    return QStringLiteral("SendLaterItem \\d+");
}
}

//#define DEBUG_MESSAGE_ID

SendLaterItem::SendLaterItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent),
      mInfo(nullptr)
{
}

SendLaterItem::~SendLaterItem()
{
    delete mInfo;
}

void SendLaterItem::setInfo(SendLater::SendLaterInfo *info)
{
    mInfo = info;
}

SendLater::SendLaterInfo *SendLaterItem::info() const
{
    return mInfo;
}

SendLaterWidget::SendLaterWidget(QWidget *parent)
    : QWidget(parent),
      mChanged(false)
{
    mWidget = new Ui::SendLaterConfigureWidget;
    mWidget->setupUi(this);
    QStringList headers;
    headers << i18n("To")
            << i18n("Subject")
            << i18n("Send around")
            << i18n("Recurrent")
#ifdef DEBUG_MESSAGE_ID
            << i18n("Message Id");
#else
            ;
#endif

    mWidget->treeWidget->setObjectName(QStringLiteral("treewidget"));
    mWidget->treeWidget->setHeaderLabels(headers);
    mWidget->treeWidget->setSortingEnabled(true);
    mWidget->treeWidget->setRootIsDecorated(false);
    mWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mWidget->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mWidget->treeWidget->setDefaultText(i18n("No messages waiting..."));

    connect(mWidget->treeWidget, &QTreeWidget::customContextMenuRequested, this, &SendLaterWidget::customContextMenuRequested);

    connect(mWidget->removeItem, &QPushButton::clicked, this, &SendLaterWidget::slotRemoveItem);
    connect(mWidget->modifyItem, &QPushButton::clicked, this, &SendLaterWidget::slotModifyItem);
    connect(mWidget->treeWidget, &QTreeWidget::itemSelectionChanged, this, &SendLaterWidget::updateButtons);
    connect(mWidget->treeWidget, &QTreeWidget::itemDoubleClicked, this, &SendLaterWidget::slotModifyItem);
    load();
    updateButtons();
}

SendLaterWidget::~SendLaterWidget()
{
    delete mWidget;
}

void SendLaterWidget::customContextMenuRequested(const QPoint &)
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (!listItems.isEmpty()) {
        QMenu menu;
        if (listItems.count() == 1) {
            menu.addAction(i18n("Send now"), this, &SendLaterWidget::slotSendNow);
        }
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"), this, &SendLaterWidget::slotRemoveItem);
        menu.exec(QCursor::pos());
    }
}

void SendLaterWidget::slotSendNow()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.count() == 1) {
        SendLaterItem *mailItem = static_cast<SendLaterItem *>(listItems.first());
        Q_EMIT sendNow(mailItem->info()->itemId());
    }
}

void SendLaterWidget::restoreTreeWidgetHeader(const QByteArray &data)
{
    mWidget->treeWidget->header()->restoreState(data);
}

void SendLaterWidget::saveTreeWidgetHeader(KConfigGroup &group)
{
    group.writeEntry("HeaderState", mWidget->treeWidget->header()->saveState());
}

void SendLaterWidget::updateButtons()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.isEmpty()) {
        mWidget->removeItem->setEnabled(false);
        mWidget->modifyItem->setEnabled(false);
    } else if (listItems.count() == 1) {
        mWidget->removeItem->setEnabled(true);
        mWidget->modifyItem->setEnabled(true);
    } else {
        mWidget->removeItem->setEnabled(true);
        mWidget->modifyItem->setEnabled(false);
    }
}

void SendLaterWidget::load()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(sendLaterItemPattern()));
    const int numberOfItem = filterGroups.count();
    for (int i = 0; i < numberOfItem; ++i) {
        KConfigGroup group = config->group(filterGroups.at(i));
        SendLater::SendLaterInfo *info = new SendLater::SendLaterInfo(group);
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
    mWidget->treeWidget->setShowDefaultText(numberOfItem == 0);
}

void SendLaterWidget::createOrUpdateItem(SendLater::SendLaterInfo *info, SendLaterItem *item)
{
    if (!item) {
        item = new SendLaterItem(mWidget->treeWidget);
    }
    item->setText(Recursive, info->isRecurrence() ? i18n("Yes") : i18n("No"));
#ifdef DEBUG_MESSAGE_ID
    item->setText(MessageId, QString::number(info->itemId()));
#endif
    item->setText(SendAround, info->dateTime().toString());
    item->setText(Subject, info->subject());
    item->setText(To, info->to());
    item->setInfo(info);
    mWidget->treeWidget->setShowDefaultText(false);
}

void SendLaterWidget::save()
{
    if (!mChanged) {
        return;
    }
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // first, delete all filter groups:
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(sendLaterItemPattern()));

    for (const QString &group : filterGroups) {
        config->deleteGroup(group);
    }

    const int numberOfItem(mWidget->treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        SendLaterItem *mailItem = static_cast<SendLaterItem *>(mWidget->treeWidget->topLevelItem(i));
        if (mailItem->info()) {
            KConfigGroup group = config->group(SendLater::SendLaterUtil::sendLaterPattern().arg(mailItem->info()->itemId()));
            mailItem->info()->writeConfig(group);
        }
    }
    config->sync();
    config->reparseConfiguration();
}

void SendLaterWidget::slotRemoveItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();

    if (listItems.isEmpty()) {
        return;
    }
    const int numberOfItems(listItems.count());
    if (KMessageBox::warningYesNo(this, i18np("Do you want to delete the selected item?", "Do you want to delete the selected items?", numberOfItems), i18n("Remove items")) == KMessageBox::No) {
        return;
    }

    bool removeMessage = false;
    if (KMessageBox::warningYesNo(this, i18n("Do you want to remove the messages as well?"), i18n("Remove messages")) == KMessageBox::Yes) {
        removeMessage = true;
    }

    for (QTreeWidgetItem *item : listItems) {
        if (removeMessage) {
            SendLaterItem *mailItem = static_cast<SendLaterItem *>(item);
            if (mailItem->info()) {
                Akonadi::Item::Id id = mailItem->info()->itemId();
                if (id != -1) {
                    mListMessagesToRemove << id;
                }
            }
        }
        delete item;
    }
    mChanged = true;
    mWidget->treeWidget->setShowDefaultText(mWidget->treeWidget->topLevelItemCount() == 0);
    updateButtons();
}

void SendLaterWidget::slotModifyItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.at(0);
        if (!item) {
            return;
        }
        SendLaterItem *mailItem = static_cast<SendLaterItem *>(item);

        QPointer<SendLater::SendLaterDialog> dialog = new SendLater::SendLaterDialog(mailItem->info(), this);
        if (dialog->exec()) {
            SendLater::SendLaterInfo *info = dialog->info();
            createOrUpdateItem(info, mailItem);
            mChanged = true;
        }
        delete dialog;
    }
}

void SendLaterWidget::needToReload()
{
    mWidget->treeWidget->clear();
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration();
    load();
}

QList<Akonadi::Item::Id> SendLaterWidget::messagesToRemove() const
{
    return mListMessagesToRemove;
}
