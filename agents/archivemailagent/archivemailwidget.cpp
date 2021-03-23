/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailwidget.h"
#include "addarchivemaildialog.h"
#include "archivemailagentutil.h"
#include "archivemailkernel.h"

#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>

#include "kmail-version.h"

#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <QLocale>
#include <QMenu>

namespace
{
inline QString archiveMailCollectionPattern()
{
    return QStringLiteral("ArchiveMailCollection \\d+");
}

static const char myConfigGroupName[] = "ArchiveMailDialog";
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

    QStringList headers;
    headers << i18n("Name") << i18n("Last archive") << i18n("Next archive in") << i18n("Storage directory");
    mWidget.treeWidget->setHeaderLabels(headers);
    mWidget.treeWidget->setObjectName(QStringLiteral("treewidget"));
    mWidget.treeWidget->setSortingEnabled(true);
    mWidget.treeWidget->setRootIsDecorated(false);
    mWidget.treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mWidget.treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mWidget.treeWidget, &QWidget::customContextMenuRequested, this, &ArchiveMailWidget::slotCustomContextMenuRequested);

    connect(mWidget.removeItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotRemoveItem);
    connect(mWidget.modifyItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotModifyItem);
    connect(mWidget.addItem, &QAbstractButton::clicked, this, &ArchiveMailWidget::slotAddItem);
    connect(mWidget.treeWidget, &QTreeWidget::itemChanged, this, &ArchiveMailWidget::slotItemChanged);
    connect(mWidget.treeWidget, &QTreeWidget::itemSelectionChanged, this, &ArchiveMailWidget::updateButtons);
    connect(mWidget.treeWidget, &QTreeWidget::itemDoubleClicked, this, &ArchiveMailWidget::slotModifyItem);
    updateButtons();

    KAboutData aboutData(QStringLiteral("archivemailagent"),
                         i18n("Archive Mail Agent"),
                         QStringLiteral(KDEPIM_VERSION),
                         i18n("Archive emails automatically."),
                         KAboutLicense::GPL_V2,
                         i18n("Copyright (C) 2014-2021 Laurent Montel"));
    aboutData.addAuthor(i18n("Laurent Montel"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    aboutData.setProductName(QByteArrayLiteral("Akonadi/Archive Mail Agent"));
    setKAboutData(aboutData);
}

ArchiveMailWidget::~ArchiveMailWidget() = default;

void ArchiveMailWidget::slotCustomContextMenuRequested(const QPoint &)
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    QMenu menu(parentWidget());
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add..."), this, &ArchiveMailWidget::slotAddItem);
    if (!listItems.isEmpty()) {
        if (listItems.count() == 1) {
            menu.addAction(i18n("Open Containing Folder..."), this, &ArchiveMailWidget::slotOpenFolder);
        }
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"), this, &ArchiveMailWidget::slotRemoveItem);
    }
    menu.exec(QCursor::pos());
}

void ArchiveMailWidget::updateButtons()
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    if (listItems.isEmpty()) {
        mWidget.removeItem->setEnabled(false);
        mWidget.modifyItem->setEnabled(false);
    } else if (listItems.count() == 1) {
        mWidget.removeItem->setEnabled(true);
        mWidget.modifyItem->setEnabled(true);
    } else {
        mWidget.removeItem->setEnabled(true);
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
    const auto group = config()->group(myConfigGroupName);
    mWidget.treeWidget->header()->restoreState(group.readEntry("HeaderState", QByteArray()));

    const QStringList collectionList = config()->groupList().filter(QRegularExpression(archiveMailCollectionPattern()));
    const int numberOfCollection = collectionList.count();
    for (int i = 0; i < numberOfCollection; ++i) {
        KConfigGroup collectionGroup = config()->group(collectionList.at(i));
        auto info = new ArchiveMailInfo(collectionGroup);
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
        item = new ArchiveMailItem(mWidget.treeWidget);
    }
    item->setText(ArchiveMailWidget::Name, i18n("Folder: %1", MailCommon::Util::fullCollectionPath(Akonadi::Collection(info->saveCollectionId()))));
    item->setCheckState(ArchiveMailWidget::Name, info->isEnabled() ? Qt::Checked : Qt::Unchecked);
    item->setText(ArchiveMailWidget::StorageDirectory, info->url().toLocalFile());
    if (info->lastDateSaved().isValid()) {
        item->setText(ArchiveMailWidget::LastArchiveDate, QLocale().toString(info->lastDateSaved(), QLocale::ShortFormat));
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
    item->setText(ArchiveMailWidget::NextArchive, i18np("Tomorrow", "%1 days", diff));
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

    auto group = config()->group(myConfigGroupName);
    group.writeEntry("HeaderState", mWidget.treeWidget->header()->saveState());

    return true;
}

void ArchiveMailWidget::slotRemoveItem()
{
    const QList<QTreeWidgetItem *> listItems = mWidget.treeWidget->selectedItems();
    if (KMessageBox::warningYesNo(parentWidget(), i18n("Do you want to delete the selected items?"), i18n("Remove items")) == KMessageBox::No) {
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
        QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(archiveItem->info(), parentWidget());
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
    QPointer<AddArchiveMailDialog> dialog = new AddArchiveMailDialog(nullptr, parentWidget());
    if (dialog->exec()) {
        ArchiveMailInfo *info = dialog->info();
        if (verifyExistingArchive(info)) {
            KMessageBox::error(parentWidget(), i18n("Cannot add a second archive for this folder. Modify the existing one instead."), i18n("Add Archive Mail"));
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
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
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

QSize ArchiveMailWidget::restoreDialogSize() const
{
    auto group = config()->group(myConfigGroupName);
    const QSize size = group.readEntry("Size", QSize(500, 300));
    return size;
}

void ArchiveMailWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(myConfigGroupName);
    group.writeEntry("Size", size);
}
