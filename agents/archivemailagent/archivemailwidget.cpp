/*
   Copyright (C) 2015-2017 Montel Laurent <montel@kde.org>

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

#include "archivemailwidget.h"
#include "addarchivemaildialog.h"
#include "archivemailagentutil.h"

#include "kmail-version.h"

#include <MailCommon/MailUtil>

#include <QLocale>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KMessageBox>
#include <QMenu>
#include <KRun>

#include <QHBoxLayout>

namespace
{
inline QString archiveMailCollectionPattern()
{
    return  QStringLiteral("ArchiveMailCollection \\d+");
}
}

ArchiveMailItem::ArchiveMailItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent), mInfo(0)
{
}

ArchiveMailItem::~ArchiveMailItem()
{
    delete mInfo;
}

void ArchiveMailItem::setInfo(ArchiveMailInfo *info)
{
    mInfo = info;
}

ArchiveMailInfo *ArchiveMailItem::info() const
{
    return mInfo;
}

ArchiveMailWidget::ArchiveMailWidget(QWidget *parent)
    : QWidget(parent),
      mChanged(false)
{
    mWidget = new Ui::ArchiveMailWidget;
    mWidget->setupUi(this);
    QStringList headers;
    headers << i18n("Name") << i18n("Last archive") << i18n("Next archive in") << i18n("Storage directory");
    mWidget->treeWidget->setHeaderLabels(headers);
    mWidget->treeWidget->setObjectName(QStringLiteral("treewidget"));
    mWidget->treeWidget->setSortingEnabled(true);
    mWidget->treeWidget->setRootIsDecorated(false);
    mWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mWidget->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mWidget->treeWidget, &QWidget::customContextMenuRequested,
            this, &ArchiveMailWidget::customContextMenuRequested);

    load();
    connect(mWidget->removeItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotRemoveItem);
    connect(mWidget->modifyItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotModifyItem);
    connect(mWidget->addItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotAddItem);
    connect(mWidget->treeWidget, &QTreeWidget::itemChanged, this, &ArchiveMailWidget::slotItemChanged);
    connect(mWidget->treeWidget, &QTreeWidget::itemSelectionChanged, this, &ArchiveMailWidget::updateButtons);
    connect(mWidget->treeWidget, &QTreeWidget::itemDoubleClicked, this, &ArchiveMailWidget::slotModifyItem);
    updateButtons();
}

ArchiveMailWidget::~ArchiveMailWidget()
{
    delete mWidget;
}

void ArchiveMailWidget::customContextMenuRequested(const QPoint &)
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    QMenu menu;
    menu.addAction(i18n("Add..."), this, &ArchiveMailWidget::slotAddItem);
    if (!listItems.isEmpty()) {
        if (listItems.count() == 1) {
            menu.addAction(i18n("Open Containing Folder..."), this, &ArchiveMailWidget::slotOpenFolder);
            menu.addSeparator();
            menu.addAction(i18n("Archive now"), this, &ArchiveMailWidget::slotArchiveNow);
        }
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"), this, &ArchiveMailWidget::slotRemoveItem);
    }
    menu.exec(QCursor::pos());
}

void ArchiveMailWidget::restoreTreeWidgetHeader(const QByteArray &data)
{
    mWidget->treeWidget->header()->restoreState(data);
}

void ArchiveMailWidget::saveTreeWidgetHeader(KConfigGroup &group)
{
    group.writeEntry("HeaderState", mWidget->treeWidget->header()->saveState());
}

void ArchiveMailWidget::updateButtons()
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

void ArchiveMailWidget::needReloadConfig()
{
    //TODO add messagebox which informs that we save settings here.
    mWidget->treeWidget->clear();
    load();
}

void ArchiveMailWidget::load()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const QStringList collectionList = config->groupList().filter(QRegularExpression(archiveMailCollectionPattern()));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup group = config->group(collectionList.at(i));
        ArchiveMailInfo *info = new ArchiveMailInfo(group);
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            delete info;
        }
    }
}

void ArchiveMailWidget::createOrUpdateItem(ArchiveMailInfo *info, ArchiveMailItem *item)
{
    if (!item) {
        item = new ArchiveMailItem(mWidget->treeWidget);
    }
    item->setText(ArchiveMailWidget::Name, i18n("Folder: %1", MailCommon::Util::fullCollectionPath(Akonadi::Collection(info->saveCollectionId()))));
    item->setCheckState(ArchiveMailWidget::Name, info->isEnabled() ? Qt::Checked : Qt::Unchecked);
    item->setText(ArchiveMailWidget::StorageDirectory, info->url().toLocalFile());
    if (info->lastDateSaved().isValid()) {
        item->setText(ArchiveMailWidget::LastArchiveDate, QLocale().toString(info->lastDateSaved(), QLocale::ShortFormat));
        updateDiffDate(item, info);
    } else {
        item->setBackgroundColor(ArchiveMailWidget::NextArchive, Qt::green);
    }
    item->setInfo(info);
}

void ArchiveMailWidget::updateDiffDate(ArchiveMailItem *item, ArchiveMailInfo *info)
{
    const QDate diffDate = ArchiveMailAgentUtil::diffDate(info);
    const int diff = QDate::currentDate().daysTo(diffDate);
    item->setText(ArchiveMailWidget::NextArchive, i18np("Tomorrow", "%1 days", diff));
    if (diff < 0) {
        if (info->isEnabled()) {
            item->setBackgroundColor(ArchiveMailWidget::NextArchive, Qt::red);
        } else {
            item->setBackgroundColor(ArchiveMailWidget::NextArchive, Qt::lightGray);
        }
    } else {
        item->setToolTip(ArchiveMailWidget::NextArchive, i18n("Archive will be done %1", QLocale().toString(diffDate, QLocale::ShortFormat)));
    }
}

void ArchiveMailWidget::save()
{
    if (!mChanged) {
        return;
    }
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // first, delete all filter groups:
    const QStringList filterGroups = config->groupList().filter(QRegularExpression(archiveMailCollectionPattern()));

    for (const QString &group : filterGroups) {
        config->deleteGroup(group);
    }

    const int numberOfItem(mWidget->treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        ArchiveMailItem *mailItem = static_cast<ArchiveMailItem *>(mWidget->treeWidget->topLevelItem(i));
        if (mailItem->info()) {
            KConfigGroup group = config->group(ArchiveMailAgentUtil::archivePattern.arg(mailItem->info()->saveCollectionId()));
            mailItem->info()->writeConfig(group);
        }
    }
    config->sync();
    config->reparseConfiguration();
}

void ArchiveMailWidget::slotRemoveItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (KMessageBox::warningYesNo(this, i18n("Do you want to delete selected items? Do you want to continue?"), i18n("Remove items")) == KMessageBox::No) {
        return;
    }

    for (QTreeWidgetItem *item : listItems) {
        delete item;
    }
    mChanged = true;
    updateButtons();
}

void ArchiveMailWidget::slotModifyItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.at(0);
        if (!item) {
            return;
        }
        ArchiveMailItem *archiveItem = static_cast<ArchiveMailItem *>(item);
        QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(archiveItem->info(), this);
        if (dialog->exec()) {
            ArchiveMailInfo *info = dialog->info();
            createOrUpdateItem(info, archiveItem);
            mChanged = true;
        }
        delete dialog;
    }
}

void ArchiveMailWidget::slotAddItem()
{
    QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(0, this);
    if (dialog->exec()) {
        ArchiveMailInfo *info = dialog->info();
        if (verifyExistingArchive(info)) {
            KMessageBox::error(this, i18n("Cannot add a second archive for this folder. Modify the existing one instead."), i18n("Add Archive Mail"));
            delete info;
        } else {
            createOrUpdateItem(info);
            updateButtons();
            mChanged = true;
        }
    }
    delete dialog;
}

bool ArchiveMailWidget::verifyExistingArchive(ArchiveMailInfo *info) const
{
    const int numberOfItem(mWidget->treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        ArchiveMailItem *mailItem = static_cast<ArchiveMailItem *>(mWidget->treeWidget->topLevelItem(i));
        ArchiveMailInfo *archiveItemInfo = mailItem->info();
        if (archiveItemInfo) {
            if (info->saveCollectionId() == archiveItemInfo->saveCollectionId()) {
                return true;
            }
        }
    }
    return false;
}

void ArchiveMailWidget::slotOpenFolder()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.first();
        if (!item) {
            return;
        }
        ArchiveMailItem *archiveItem = static_cast<ArchiveMailItem *>(item);
        ArchiveMailInfo *archiveItemInfo = archiveItem->info();
        if (archiveItemInfo) {
            const QUrl url = archiveItemInfo->url();
            KRun *runner = new KRun(url, this);   // will delete itself
            runner->setRunExecutables(false);
        }
    }
}

void ArchiveMailWidget::slotArchiveNow()
{
    const QList<QTreeWidgetItem *> listItems = mWidget->treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.first();
        if (!item) {
            return;
        }
        ArchiveMailItem *archiveItem = static_cast<ArchiveMailItem *>(item);
        ArchiveMailInfo *archiveItemInfo = archiveItem->info();
        save();
        if (archiveItemInfo) {
            Q_EMIT archiveNow(archiveItemInfo);
        }
    }
}

void ArchiveMailWidget::slotItemChanged(QTreeWidgetItem *item, int col)
{
    if (item) {
        ArchiveMailItem *archiveItem = static_cast<ArchiveMailItem *>(item);
        if (archiveItem->info()) {
            if (col == ArchiveMailWidget::Name) {
                archiveItem->info()->setEnabled(archiveItem->checkState(ArchiveMailWidget::Name) == Qt::Checked);
                mChanged = true;
            } else if (col == ArchiveMailWidget::NextArchive) {
                updateDiffDate(archiveItem, archiveItem->info());
            }
        }
    }
}

