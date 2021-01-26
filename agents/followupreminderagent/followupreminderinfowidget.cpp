/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "followupreminderinfowidget.h"
#include "followupreminderagent_debug.h"
#include "followupreminderinfo.h"
#include "followupreminderutil.h"
#include "jobs/followupremindershowmessagejob.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLocale>
#include <QMenu>
#include <QTreeWidget>

// #define DEBUG_MESSAGE_ID
namespace
{
inline QString followUpItemPattern()
{
    return QStringLiteral("FollowupReminderItem \\d+");
}
}

FollowUpReminderInfoItem::FollowUpReminderInfoItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent)
{
}

FollowUpReminderInfoItem::~FollowUpReminderInfoItem()
{
    delete mInfo;
}

void FollowUpReminderInfoItem::setInfo(FollowUpReminder::FollowUpReminderInfo *info)
{
    mInfo = info;
}

FollowUpReminder::FollowUpReminderInfo *FollowUpReminderInfoItem::info() const
{
    return mInfo;
}

FollowUpReminderInfoWidget::FollowUpReminderInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("FollowUpReminderInfoWidget"));
    auto hbox = new QHBoxLayout(this);
    hbox->setContentsMargins({});
    mTreeWidget = new QTreeWidget(this);
    mTreeWidget->setObjectName(QStringLiteral("treewidget"));
    QStringList headers;
    headers << i18n("To") << i18n("Subject") << i18n("Dead Line") << i18n("Answer")
#ifdef DEBUG_MESSAGE_ID
            << QStringLiteral("Message Id") << QStringLiteral("Answer Message Id")
#endif
        ;

    mTreeWidget->setHeaderLabels(headers);
    mTreeWidget->setSortingEnabled(true);
    mTreeWidget->setRootIsDecorated(false);
    mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mTreeWidget, &QTreeWidget::customContextMenuRequested, this, &FollowUpReminderInfoWidget::slotCustomContextMenuRequested);

    hbox->addWidget(mTreeWidget);
}

FollowUpReminderInfoWidget::~FollowUpReminderInfoWidget() = default;

void FollowUpReminderInfoWidget::setInfo(const QList<FollowUpReminder::FollowUpReminderInfo *> &infoList)
{
    mTreeWidget->clear();
    for (FollowUpReminder::FollowUpReminderInfo *info : infoList) {
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
}

void FollowUpReminderInfoWidget::load()
{
    auto config = FollowUpReminder::FollowUpReminderUtil::defaultConfig();
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(followUpItemPattern()));
    const int numberOfItem = filterGroups.count();
    for (int i = 0; i < numberOfItem; ++i) {
        KConfigGroup group = config->group(filterGroups.at(i));

        auto info = new FollowUpReminder::FollowUpReminderInfo(group);
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
}

QList<qint32> FollowUpReminderInfoWidget::listRemoveId() const
{
    return mListRemoveId;
}

void FollowUpReminderInfoWidget::createOrUpdateItem(FollowUpReminder::FollowUpReminderInfo *info, FollowUpReminderInfoItem *item)
{
    if (!item) {
        item = new FollowUpReminderInfoItem(mTreeWidget);
    }
    item->setInfo(info);
    item->setText(To, info->to());
    item->setText(Subject, info->subject());
    const QString date = QLocale().toString(info->followUpReminderDate());
    item->setText(DeadLine, date);
    const bool answerWasReceived = info->answerWasReceived();
    item->setText(AnswerWasReceived, answerWasReceived ? i18n("Received") : i18n("On hold"));
    item->setData(0, AnswerItemFound, answerWasReceived);
    if (answerWasReceived) {
        item->setBackground(DeadLine, Qt::green);
    } else {
        if (info->followUpReminderDate() < QDate::currentDate()) {
            item->setBackground(DeadLine, Qt::red);
        }
    }
#ifdef DEBUG_MESSAGE_ID
    item->setText(MessageId, QString::number(info->originalMessageItemId()));
    item->setText(AnswerMessageId, QString::number(info->answerMessageItemId()));
#endif
}

bool FollowUpReminderInfoWidget::save() const
{
    if (!mChanged) {
        return false;
    }
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // first, delete all filter groups:
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(followUpItemPattern()));

    for (const QString &group : filterGroups) {
        config->deleteGroup(group);
    }

    const int numberOfItem(mTreeWidget->topLevelItemCount());
    int i = 0;
    for (; i < numberOfItem; ++i) {
        auto mailItem = static_cast<FollowUpReminderInfoItem *>(mTreeWidget->topLevelItem(i));
        if (mailItem->info()) {
            KConfigGroup group = config->group(FollowUpReminder::FollowUpReminderUtil::followUpReminderPattern().arg(i));
            mailItem->info()->writeConfig(group, i);
        }
    }
    ++i;
    KConfigGroup general = config->group(QStringLiteral("General"));
    general.writeEntry("Number", i);
    config->sync();
    return true;
}

void FollowUpReminderInfoWidget::slotCustomContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems();
    const int nbElementSelected = listItems.count();
    if (nbElementSelected > 0) {
        QMenu menu(this);
        QAction *showMessage = nullptr;
        FollowUpReminderInfoItem *mailItem = nullptr;
        if ((nbElementSelected == 1)) {
            mailItem = static_cast<FollowUpReminderInfoItem *>(listItems.at(0));
            if (mailItem->data(0, AnswerItemFound).toBool()) {
                showMessage = menu.addAction(i18n("Show Message"));
                menu.addSeparator();
            }
        }
        QAction *deleteItem = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"));
        QAction *result = menu.exec(QCursor::pos());
        if (result) {
            if (result == showMessage) {
                openShowMessage(mailItem->info()->answerMessageItemId());
            } else if (result == deleteItem) {
                removeItem(listItems);
            }
        }
    }
}

void FollowUpReminderInfoWidget::openShowMessage(Akonadi::Item::Id id)
{
    auto job = new FollowUpReminderShowMessageJob(id);
    job->start();
}

void FollowUpReminderInfoWidget::removeItem(const QList<QTreeWidgetItem *> &mailItemLst)
{
    if (mailItemLst.isEmpty()) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << "Not item selected";
    } else {
        if (KMessageBox::Yes
            == KMessageBox::warningYesNo(
                this,
                i18np("Do you want to remove this selected item?", "Do you want to remove these %1 selected items?", mailItemLst.count()),
                i18n("Delete"))) {
            for (QTreeWidgetItem *item : mailItemLst) {
                auto mailItem = static_cast<FollowUpReminderInfoItem *>(item);
                mListRemoveId << mailItem->info()->uniqueIdentifier();
                delete mailItem;
            }
            mChanged = true;
        }
    }
}

void FollowUpReminderInfoWidget::restoreTreeWidgetHeader(const QByteArray &data)
{
    mTreeWidget->header()->restoreState(data);
}

void FollowUpReminderInfoWidget::saveTreeWidgetHeader(KConfigGroup &group)
{
    group.writeEntry("HeaderState", mTreeWidget->header()->saveState());
}
