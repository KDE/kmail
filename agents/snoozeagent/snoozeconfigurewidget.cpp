/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeconfigurewidget.h"
#include "job/showmessagejob.h"
#include "snoozeinfo.h"
#include "snoozeutil.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

#include <QCursor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;
using namespace Snooze;

SnoozeItem::SnoozeItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent)
{
}

SnoozeItem::~SnoozeItem()
{
    delete mInfo;
}

void SnoozeItem::setInfo(Snooze::SnoozeInfo *info)
{
    delete mInfo;
    mInfo = info;
}

SnoozeInfo *SnoozeItem::info() const
{
    return mInfo;
}

SnoozeWidget::SnoozeWidget(QWidget *parent)
    : QWidget(parent)
    , mTreeWidget(new QTreeWidget(this))
    , mRemoveButton(new QPushButton(i18nc("@action:button", "Remove"), this))
    , mWakeUpNowButton(new QPushButton(i18nc("@action:button", "Wake Up Now"), this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});

    mTreeWidget->setObjectName("treewidget"_L1);
    mTreeWidget->setRootIsDecorated(false);
    mTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mTreeWidget->setHeaderLabels({i18nc("@title:column send mail from", "From"),
                                  i18nc("@title:column mail subject", "Subject"),
                                  i18nc("@title:column when the message comes back", "Wake Up")});
    mainLayout->addWidget(mTreeWidget);

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins({});
    mWakeUpNowButton->setObjectName("wakeupnowbutton"_L1);
    buttonLayout->addWidget(mWakeUpNowButton);
    mRemoveButton->setObjectName("removebutton"_L1);
    buttonLayout->addWidget(mRemoveButton);
    buttonLayout->addStretch(1);
    mainLayout->addLayout(buttonLayout);

    connect(mRemoveButton, &QPushButton::clicked, this, &SnoozeWidget::slotRemoveItem);
    connect(mWakeUpNowButton, &QPushButton::clicked, this, &SnoozeWidget::slotWakeUpNow);
    connect(mTreeWidget, &QTreeWidget::customContextMenuRequested, this, &SnoozeWidget::slotCustomContextMenuRequested);
    connect(mTreeWidget, &QTreeWidget::itemSelectionChanged, this, &SnoozeWidget::updateButtons);

    updateButtons();
}

SnoozeWidget::~SnoozeWidget() = default;

void SnoozeWidget::load()
{
    auto config = SnoozeUtil::defaultConfig();
    static const QRegularExpression reg(SnoozeUtil::snoozePattern());
    const QStringList filterGroups = config->groupList().filter(reg);
    const int numberOfItem = filterGroups.count();
    for (int i = 0; i < numberOfItem; ++i) {
        KConfigGroup group = config->group(filterGroups.at(i));

        // Skip the messages the user already unsnoozed but did not apply yet.
        if (auto info = new SnoozeInfo(group); info->isValid() && !mListMessagesToUnsnooze.contains(info->itemId())) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
    updateButtons();
}

void SnoozeWidget::needToReload()
{
    mTreeWidget->clear();
    SnoozeUtil::forceReparseConfiguration();
    load();
}

void SnoozeWidget::saveTreeWidgetHeader(KConfigGroup &group)
{
    group.writeEntry("HeaderState", mTreeWidget->header()->saveState());
}

void SnoozeWidget::restoreTreeWidgetHeader(const QByteArray &data)
{
    mTreeWidget->header()->restoreState(data);
}

QList<Akonadi::Item::Id> SnoozeWidget::messagesToUnsnooze() const
{
    return mListMessagesToUnsnooze;
}

void SnoozeWidget::slotRemoveItem()
{
    const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems();
    if (listItems.isEmpty()) {
        return;
    }
    const int answer = KMessageBox::questionTwoActions(this,
                                                       i18np("Do you want to unsnooze the selected message?\nIt will come back without notification.",
                                                             "Do you want to unsnooze the %1 selected messages?\nThey will come back without notification.",
                                                             listItems.count()),
                                                       i18nc("@title:window", "Unsnooze Messages"),
                                                       KGuiItem(i18nc("@action:button", "Unsnooze"), u"edit-delete"_s),
                                                       KStandardGuiItem::cancel());
    if (answer != KMessageBox::ButtonCode::PrimaryAction) {
        return;
    }
    for (QTreeWidgetItem *item : listItems) {
        mListMessagesToUnsnooze << static_cast<SnoozeItem *>(item)->info()->itemId();
        delete item;
    }
    updateButtons();
}

void SnoozeWidget::slotWakeUpNow()
{
    // The agent removes them from the config, the list follows on needToReload().
    const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems();
    for (QTreeWidgetItem *item : listItems) {
        Q_EMIT wakeUpNow(static_cast<SnoozeItem *>(item)->info()->itemId());
    }
}

void SnoozeWidget::slotShowMessage()
{
    if (const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems(); listItems.count() == 1) {
        auto job = new ShowMessageJob(static_cast<SnoozeItem *>(listItems.at(0))->info()->itemId());
        job->start();
    }
}

void SnoozeWidget::slotCustomContextMenuRequested(QPoint pos)
{
    Q_UNUSED(pos)
    const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems();
    if (listItems.isEmpty()) {
        return;
    }
    QMenu menu(this);
    if (listItems.count() == 1) {
        menu.addAction(QIcon::fromTheme(u"mail-message"_s), i18nc("@action", "Show Message"), this, &SnoozeWidget::slotShowMessage);
        menu.addSeparator();
    }
    menu.addAction(i18nc("@action", "Wake Up Now"), this, &SnoozeWidget::slotWakeUpNow);
    menu.addAction(QIcon::fromTheme(u"edit-delete"_s), i18nc("@action", "Unsnooze"), this, &SnoozeWidget::slotRemoveItem);
    menu.exec(QCursor::pos());
}

void SnoozeWidget::updateButtons()
{
    const bool hasSelection = !mTreeWidget->selectedItems().isEmpty();
    mRemoveButton->setEnabled(hasSelection);
    mWakeUpNowButton->setEnabled(hasSelection);
}

void SnoozeWidget::createOrUpdateItem(Snooze::SnoozeInfo *info, SnoozeItem *item)
{
    if (!item) {
        item = new SnoozeItem(mTreeWidget);
    }
    item->setText(From, info->from());
    item->setToolTip(From, info->from());
    item->setText(Subject, info->subject());
    item->setToolTip(Subject, info->subject());
    item->setText(WakeUpDate, QLocale().toString(info->wakeUpDateTime().toLocalTime(), QLocale::ShortFormat));
    item->setInfo(info);
}

#include "moc_snoozeconfigurewidget.cpp"
