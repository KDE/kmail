/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterconfigurewidget.h"
#include "sendlaterdialog.h"
#include "sendlaterutil.h"

#include <MessageComposer/SendLaterDialog>
#include <MessageComposer/SendLaterInfo>

#include <KLocalizedString>
#include <KMessageBox>
#include <QIcon>
#include <QMenu>
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
    : QTreeWidgetItem(parent)
{
}

SendLaterItem::~SendLaterItem()
{
    delete mInfo;
}

void SendLaterItem::setInfo(MessageComposer::SendLaterInfo *info)
{
    mInfo = info;
}

MessageComposer::SendLaterInfo *SendLaterItem::info() const
{
    return mInfo;
}

SendLaterWidget::SendLaterWidget(QWidget *parent)
    : QWidget(parent)
{
    mWidget = new Ui::SendLaterConfigureWidget;
    mWidget->setupUi(this);
    QStringList headers;
    headers << i18n("To") << i18n("Subject") << i18n("Send around") << i18n("Recurrent")
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

    connect(mWidget->treeWidget, &QTreeWidget::customContextMenuRequested, this, &SendLaterWidget::slotCustomContextMenuRequested);

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

void SendLaterWidget::slotCustomContextMenuRequested(const QPoint &)
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (!listItems.isEmpty()) {
        QMenu menu(this);
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
        auto mailItem = static_cast<SendLaterItem *>(listItems.first());
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
    KSharedConfig::Ptr config = SendLaterUtil::defaultConfig();
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(sendLaterItemPattern()));
    const int numberOfItem = filterGroups.count();
    for (int i = 0; i < numberOfItem; ++i) {
        KConfigGroup group = config->group(filterGroups.at(i));
        auto info = SendLaterUtil::readSendLaterInfo(group);
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
    mWidget->treeWidget->setShowDefaultText(numberOfItem == 0);
}

void SendLaterWidget::createOrUpdateItem(MessageComposer::SendLaterInfo *info, SendLaterItem *item)
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
    KSharedConfig::Ptr config = SendLaterUtil::defaultConfig();

    // first, delete all filter groups:
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(sendLaterItemPattern()));

    for (const QString &group : filterGroups) {
        config->deleteGroup(group);
    }

    const int numberOfItem(mWidget->treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        auto mailItem = static_cast<SendLaterItem *>(mWidget->treeWidget->topLevelItem(i));
        if (mailItem->info()) {
            SendLaterUtil::writeSendLaterInfo(config, mailItem->info());
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
    if (KMessageBox::warningYesNo(this,
                                  i18np("Do you want to delete the selected item?", "Do you want to delete the selected items?", numberOfItems),
                                  i18n("Remove items"))
        == KMessageBox::No) {
        return;
    }

    bool removeMessage = false;
    if (KMessageBox::warningYesNo(this, i18n("Do you want to remove the messages as well?"), i18n("Remove messages")) == KMessageBox::Yes) {
        removeMessage = true;
    }

    for (QTreeWidgetItem *item : listItems) {
        if (removeMessage) {
            auto mailItem = static_cast<SendLaterItem *>(item);
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
        auto mailItem = static_cast<SendLaterItem *>(item);

        QPointer<MessageComposer::SendLaterDialog> dialog = new MessageComposer::SendLaterDialog(mailItem->info(), this);
        if (dialog->exec()) {
            auto info = dialog->info();
            createOrUpdateItem(info, mailItem);
            mChanged = true;
        }
        delete dialog;
    }
}

void SendLaterWidget::needToReload()
{
    mWidget->treeWidget->clear();
    KSharedConfig::Ptr config = SendLaterUtil::defaultConfig();
    config->reparseConfiguration();
    load();
}

QVector<Akonadi::Item::Id> SendLaterWidget::messagesToRemove() const
{
    return mListMessagesToRemove;
}
