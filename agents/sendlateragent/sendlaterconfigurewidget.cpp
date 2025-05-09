/*
   SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterconfigurewidget.h"

#include "sendlaterutil.h"

#include <MessageComposer/SendLaterDialog>
#include <MessageComposer/SendLaterInfo>

#include <KLocalizedString>
#include <KMessageBox>
#include <QIcon>
#include <QMenu>
#include <QPointer>

using namespace Qt::Literals::StringLiterals;
namespace
{
inline QString sendLaterItemPattern()
{
    return QStringLiteral("SendLaterItem \\d+");
}
}

// #define DEBUG_MESSAGE_ID

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
    const QStringList headers = QStringList() << i18n("To") << i18n("Subject") << i18n("Send around") << i18n("Recurrent")
#ifdef DEBUG_MESSAGE_ID
                                              << i18n("Message Id");
#else
        ;
#endif

    mWidget->treeWidget->setObjectName("treewidget"_L1);
    mWidget->treeWidget->setHeaderLabels(headers);
    mWidget->treeWidget->setSortingEnabled(true);
    mWidget->treeWidget->setRootIsDecorated(false);
    mWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mWidget->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mWidget->treeWidget->setDefaultText(i18n("No messages waiting…"));

    connect(mWidget->treeWidget, &QTreeWidget::customContextMenuRequested, this, &SendLaterWidget::slotCustomContextMenuRequested);

    connect(mWidget->deleteItem, &QPushButton::clicked, this, &SendLaterWidget::slotDeleteItem);
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

void SendLaterWidget::slotCustomContextMenuRequested(QPoint)
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (!listItems.isEmpty()) {
        QMenu menu(this);
        if (listItems.count() == 1) {
            menu.addAction(mWidget->modifyItem->text(), this, &SendLaterWidget::slotModifyItem);
            menu.addSeparator();
            menu.addAction(i18nc("@action", "Send now"), this, &SendLaterWidget::slotSendNow);
        }
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action", "Delete"), this, &SendLaterWidget::slotDeleteItem);
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
        mWidget->deleteItem->setEnabled(false);
        mWidget->modifyItem->setEnabled(false);
    } else if (listItems.count() == 1) {
        mWidget->deleteItem->setEnabled(true);
        mWidget->modifyItem->setEnabled(true);
    } else {
        mWidget->deleteItem->setEnabled(true);
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
    const QString date{info->dateTime().toString()};
    item->setText(SendAround, date);
    item->setToolTip(SendAround, date);
    const QString subject{info->subject()};
    item->setText(Subject, subject);
    item->setToolTip(Subject, subject);
    item->setText(To, info->to());
    item->setToolTip(To, info->to());
    item->setInfo(info);
    mWidget->treeWidget->setShowDefaultText(false);
}

bool SendLaterWidget::save()
{
    if (!mChanged) {
        return false;
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
    return true;
}

void SendLaterWidget::slotDeleteItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();

    if (listItems.isEmpty()) {
        return;
    }
    const int numberOfItems(listItems.count());
    int answer = KMessageBox::warningTwoActions(this,
                                                i18np("Do you want to delete the selected item?", "Do you want to delete the selected items?", numberOfItems),
                                                i18nc("@title:window", "Delete Items"),
                                                KStandardGuiItem::del(),
                                                KStandardGuiItem::cancel());
    if (answer == KMessageBox::ButtonCode::SecondaryAction) {
        return;
    }

    answer = KMessageBox::warningTwoActions(this,
                                            i18np("Do you want to delete the message as well?", "Do you want to delete the messages as well?", numberOfItems),
                                            i18nc("@title:window", "Delete Messages"),
                                            KStandardGuiItem::del(),
                                            KGuiItem(i18nc("@action:button", "Do Not Delete"), QStringLiteral("dialog-cancel")));
    const bool deleteMessage = (answer == KMessageBox::ButtonCode::PrimaryAction);

    for (QTreeWidgetItem *item : listItems) {
        if (deleteMessage) {
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

QList<Akonadi::Item::Id> SendLaterWidget::messagesToRemove() const
{
    return mListMessagesToRemove;
}

#include "moc_sendlaterconfigurewidget.cpp"
