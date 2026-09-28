/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeconfigurewidget.h"
#include "snoozeinfo.h"
#include "snoozeutil.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
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
    auto config = SnoozeUtil::defaultConfig();
    static const QRegularExpression reg(SnoozeUtil::snoozePattern());
    const QStringList filterGroups = config->groupList().filter(reg);
    const int numberOfItem = filterGroups.count();
    for (int i = 0; i < numberOfItem; ++i) {
        KConfigGroup group = config->group(filterGroups.at(i));

        if (auto info = new SnoozeInfo(group); info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
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
    // TODO confirm, then queue the ids into mListMessagesToUnsnooze.
}

void SnoozeWidget::slotWakeUpNow()
{
    // TODO Q_EMIT wakeUpNow() for the current item.
}

void SnoozeWidget::slotCustomContextMenuRequested(QPoint pos)
{
    const QList<QTreeWidgetItem *> listItems = mTreeWidget->selectedItems();
    if (const int nbElementSelected = listItems.count(); nbElementSelected > 0) {
        QMenu menu(this);
        QAction *showMessage = nullptr;
        QAction *showOriginalMessage = nullptr;
        SnoozeItem *mailItem = nullptr;
        if ((nbElementSelected == 1)) {
            mailItem = static_cast<SnoozeItem *>(listItems.at(0));
            // if (mailItem->data(0, AnswerItemFound).toBool()) {
            //     showMessage = menu.addAction(i18nc("@action", "Show Message"));
            //     menu.addSeparator();
            // }
            showOriginalMessage = menu.addAction(QIcon::fromTheme(u"mail-message"_s), i18nc("@action", "Show Original Message"));
            menu.addSeparator();
        }
        const QAction *deleteItem = menu.addAction(QIcon::fromTheme(u"edit-delete"_s), i18nc("@action", "Delete"));
        const QAction *result = menu.exec(QCursor::pos());
        if (result) {
            /*
            if (result == showMessage) {
                openShowMessage(mailItem->info()->answerMessageItemId());
            } else if (result == deleteItem) {
                deleteItems(listItems);
            } else if (result == showOriginalMessage) {
                openShowMessage(mailItem->info()->originalMessageItemId());
            }
            */
        }
    }
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
    item->setInfo(info);
}

#include "moc_snoozeconfigurewidget.cpp"
