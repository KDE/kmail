/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeconfigurewidget.h"
#include "snoozeinfo.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QPushButton>
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
    // TODO fill the tree from the "SnoozeItem <n>" config groups.
}

bool SnoozeWidget::save()
{
    // TODO rewrite the config groups from the tree.
    return mChanged;
}

void SnoozeWidget::needToReload()
{
    // TODO clear and load() again.
}

void SnoozeWidget::saveTreeWidgetHeader(KConfigGroup &group)
{
    // TODO persist the header state.
    Q_UNUSED(group)
}

void SnoozeWidget::restoreTreeWidgetHeader(const QByteArray &group)
{
    // TODO restore the header state.
    Q_UNUSED(group)
}

QList<Akonadi::Item::Id> SnoozeWidget::messagesToUnsnooze() const
{
    return mListMessagesToUnsnooze;
}

void SnoozeWidget::slotRemoveItem()
{
    // TODO confirm, then queue the ids into mListMessagesToUnsnooze.
}

void SnoozeWidget::slotWakeUpNow()
{
    // TODO Q_EMIT wakeUpNow() for the current item.
}

void SnoozeWidget::slotCustomContextMenuRequested(QPoint pos)
{
    // TODO context menu mirroring the buttons.
    Q_UNUSED(pos)
}

void SnoozeWidget::updateButtons()
{
    const bool hasSelection = !mTreeWidget->selectedItems().isEmpty();
    mRemoveButton->setEnabled(hasSelection);
    mWakeUpNowButton->setEnabled(hasSelection);
}

void SnoozeWidget::createOrUpdateItem(Snooze::SnoozeInfo *info, SnoozeItem *item)
{
    // TODO fill the columns from info.
    Q_UNUSED(info)
    Q_UNUSED(item)
}

#include "moc_snoozeconfigurewidget.cpp"
