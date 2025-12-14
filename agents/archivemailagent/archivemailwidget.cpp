/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailwidget.h"

#include "addarchivemaildialog.h"
#include "archivemailagent_debug.h"
#include "archivemailagentutil.h"
#include "archivemailkernel.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>

#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <QLocale>
#include <QMenu>

using namespace Qt::Literals::StringLiterals;
namespace
{
inline QString archiveMailCollectionPattern()
{
    return QStringLiteral("ArchiveMailCollection \\d+");
}

const char myConfigGroupName[] = "ArchiveMailDialog";
}

ArchiveMailItem::ArchiveMailItem(QTreeWidget *parent)
    : QTreeWidgetItem(parent)
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

ArchiveMailWidget::ArchiveMailWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    ArchiveMailKernel *archiveMailKernel = ArchiveMailKernel::self();
    CommonKernel->registerKernelIf(archiveMailKernel); // register KernelIf early, it is used by the Filter classes
    CommonKernel->registerSettingsIf(archiveMailKernel); // SettingsIf is used in FolderTreeWidget

    auto w = new QWidget(parent);
    mWidget.setupUi(w);
    parent->layout()->addWidget(w);

    const QStringList headers = QStringList{i18n("Name"), i18n("Last archive"), i18n("Next archive in"), i18n("Storage directory")};
    mWidget.treeWidget->setHeaderLabels(headers);
    mWidget.treeWidget->setObjectName("treewidget"_L1);
    mWidget.treeWidget->setSortingEnabled(true);
    mWidget.treeWidget->setRootIsDecorated(false);
    mWidget.treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mWidget.treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mWidget.treeWidget, &QWidget::customContextMenuRequested, this, &ArchiveMailWidget::slotCustomContextMenuRequested);

    connect(mWidget.deleteItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotDeleteItem);
    connect(mWidget.modifyItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotModifyItem);
    connect(mWidget.addItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotAddItem);
    connect(mWidget.treeWidget, &QTreeWidget::itemChanged, this, &ArchiveMailWidget::slotItemChanged);
    connect(mWidget.treeWidget, &QTreeWidget::itemSelectionChanged, this, &ArchiveMailWidget::updateButtons);
    connect(mWidget.treeWidget, &QTreeWidget::itemDoubleClicked, this, &ArchiveMailWidget::slotModifyItem);
    updateButtons();
}

ArchiveMailWidget::~ArchiveMailWidget() = default;

void ArchiveMailWidget::slotCustomContextMenuRequested(const QPoint &)
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    QMenu menu(mWidget.treeWidget);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action", "Add…"), this, &ArchiveMailWidget::slotAddItem);
    if (!listItems.isEmpty()) {
        if (listItems.count() == 1) {
            menu.addSeparator();
            menu.addAction(mWidget.modifyItem->text(), this, &ArchiveMailWidget::slotModifyItem);
            menu.addSeparator();
            menu.addAction(i18nc("@action", "Open Containing Folder…"), this, &ArchiveMailWidget::slotOpenFolder);
        }
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action", "Delete"), this, &ArchiveMailWidget::slotDeleteItem);
    }
    menu.exec(QCursor::pos());
}

void ArchiveMailWidget::updateButtons()
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    if (listItems.isEmpty()) {
        mWidget.deleteItem->setEnabled(false);
        mWidget.modifyItem->setEnabled(false);
    } else if (listItems.count() == 1) {
        mWidget.deleteItem->setEnabled(true);
        mWidget.modifyItem->setEnabled(true);
    } else {
        mWidget.deleteItem->setEnabled(true);
        mWidget.modifyItem->setEnabled(false);
    }
}

void ArchiveMailWidget::needReloadConfig()
{
    // TODO add messagebox which informs that we save settings here.
    mWidget.treeWidget->clear();
    load();
}

void ArchiveMailWidget::load()
{
    const auto group = config()->group(QLatin1StringView(myConfigGroupName));
    mWidget.treeWidget->header()->restoreState(group.readEntry("HeaderState", QByteArray()));

    const QStringList collectionList = config()->groupList().filter(QRegularExpression(archiveMailCollectionPattern()));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup collectionGroup = config()->group(collectionList.at(i));
        auto info = new ArchiveMailInfo(collectionGroup);
        if (info->isValid()) {
            createOrUpdateItem(info);
        } else {
            qCWarning(ARCHIVEMAILAGENT_LOG) << " Invalid info " << info << "collectionGroup" << collectionGroup.name();
            delete info;
        }
    }
}

void ArchiveMailWidget::createOrUpdateItem(ArchiveMailInfo *info, ArchiveMailItem *item)
{
    if (!item) {
        item = new ArchiveMailItem(mWidget.treeWidget);
    }
    const QString folderName = i18n("Folder: %1", MailCommon::Util::fullCollectionPath(Akonadi::Collection(info->saveCollectionId())));
    item->setText(ArchiveMailWidget::Name, folderName);
    item->setToolTip(ArchiveMailWidget::Name, folderName);
    item->setCheckState(ArchiveMailWidget::Name, info->isEnabled() ? Qt::Checked : Qt::Unchecked);
    const QString path{info->url().toLocalFile()};
    item->setText(ArchiveMailWidget::StorageDirectory, path);
    item->setToolTip(ArchiveMailWidget::StorageDirectory, path);
    if (info->lastDateSaved().isValid()) {
        const QString date{QLocale().toString(info->lastDateSaved(), QLocale::ShortFormat)};
        item->setText(ArchiveMailWidget::LastArchiveDate, date);
        item->setToolTip(ArchiveMailWidget::LastArchiveDate, date);
        updateDiffDate(item, info);
    } else {
        item->setBackground(ArchiveMailWidget::NextArchive, Qt::green);
    }
    item->setInfo(info);
}

void ArchiveMailWidget::updateDiffDate(ArchiveMailItem *item, ArchiveMailInfo *info)
{
    const QDate diffDate = ArchiveMailAgentUtil::diffDate(info);
    const qint64 diff = QDate::currentDate().daysTo(diffDate);
    const QString dateStr{i18np("Tomorrow", "%1 days", diff)};
    item->setText(ArchiveMailWidget::NextArchive, dateStr);
    if (diff < 0) {
        if (info->isEnabled()) {
            item->setBackground(ArchiveMailWidget::NextArchive, Qt::red);
        } else {
            item->setBackground(ArchiveMailWidget::NextArchive, Qt::lightGray);
        }
    } else {
        item->setToolTip(ArchiveMailWidget::NextArchive, i18n("Archive will be done %1", QLocale().toString(diffDate, QLocale::ShortFormat)));
    }
}

bool ArchiveMailWidget::save() const
{
    if (!mChanged) {
        return false;
    }

    // first, delete all filter groups:
    const QStringList filterGroups = config()->groupList().filter(QRegularExpression(archiveMailCollectionPattern()));

    for (const QString &group : filterGroups) {
        config()->deleteGroup(group);
    }

    const int numberOfItem(mWidget.treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        auto mailItem = static_cast<ArchiveMailItem *>(mWidget.treeWidget->topLevelItem(i));
        if (mailItem->info()) {
            KConfigGroup group = config()->group(ArchiveMailAgentUtil::archivePattern.arg(mailItem->info()->saveCollectionId()));
            mailItem->info()->writeConfig(group);
        }
    }

    auto group = config()->group(QLatin1StringView(myConfigGroupName));
    group.writeEntry("HeaderState", mWidget.treeWidget->header()->saveState());

    return true;
}

void ArchiveMailWidget::slotDeleteItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    const int answer = KMessageBox::warningTwoActions(mWidget.treeWidget,
                                                      i18n("Do you want to delete the selected items?"),
                                                      i18nc("@title:window", "Delete Items"),
                                                      KStandardGuiItem::del(),
                                                      KStandardGuiItem::cancel());
    if (answer == KMessageBox::ButtonCode::SecondaryAction) {
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
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.at(0);
        if (!item) {
            return;
        }
        auto archiveItem = static_cast<ArchiveMailItem *>(item);
        QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(archiveItem->info(), mWidget.treeWidget);
        qCDebug(ARCHIVEMAILAGENT_LOG) << " archiveItem->info() " << *archiveItem->info();
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
    QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(nullptr, mWidget.addItem);
    if (dialog->exec()) {
        ArchiveMailInfo *info = dialog->info();
        if (verifyExistingArchive(info)) {
            KMessageBox::error(mWidget.addItem,
                               i18n("Cannot add a second archive for this folder. Modify the existing one instead."),
                               i18nc("@title:window", "Add Archive Mail"));
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
    const int numberOfItem(mWidget.treeWidget->topLevelItemCount());
    for (int i = 0; i < numberOfItem; ++i) {
        auto mailItem = static_cast<ArchiveMailItem *>(mWidget.treeWidget->topLevelItem(i));
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
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    if (listItems.count() == 1) {
        QTreeWidgetItem *item = listItems.first();
        if (!item) {
            return;
        }
        auto archiveItem = static_cast<ArchiveMailItem *>(item);
        ArchiveMailInfo *archiveItemInfo = archiveItem->info();
        if (archiveItemInfo) {
            const QUrl url = archiveItemInfo->url();
            auto job = new KIO::OpenUrlJob(url);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mWidget.treeWidget));
            job->setRunExecutables(false);
            job->start();
        }
    }
}

void ArchiveMailWidget::slotItemChanged(QTreeWidgetItem *item, int col)
{
    if (item) {
        auto archiveItem = static_cast<ArchiveMailItem *>(item);
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

#include "moc_archivemailwidget.cpp"
