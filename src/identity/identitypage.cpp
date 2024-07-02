/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2003 Marc Mutz <mutz@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "identitypage.h"
using namespace Qt::Literals::StringLiterals;

#include "identitydialog.h"
#include "newidentitydialog.h"
#include "settings/kmailsettings.h"

#include <MailCommon/MailKernel>

#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>

#include <KLocalizedString>
#include <KMessageBox>
#include <QMenu>

#include <QTreeWidgetItem>

using namespace KMail;

QString IdentityPage::helpAnchor() const
{
    return QStringLiteral("configure-identity");
}

IdentityPage::IdentityPage(QWidget *parent)
    : ConfigModuleTab(parent)
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mIdentityManager = KernelIf->identityManager();
    connect(mIdentityManager, &KIdentityManagementCore::IdentityManager::needToReloadIdentitySettings, this, &IdentityPage::load);

    mIPage.setupUi(this);
    mIPage.mIdentityList->setIdentityManager(mIdentityManager);

    connect(mIPage.mIdentityList, &QTreeWidget::itemSelectionChanged, this, &IdentityPage::slotIdentitySelectionChanged);
    connect(this, qOverload<bool>(&IdentityPage::changed), this, &IdentityPage::slotIdentitySelectionChanged);
    connect(mIPage.mIdentityList,
            qOverload<KMail::IdentityListViewItem *, const QString &>(&IdentityListView::rename),
            this,
            &IdentityPage::slotRenameIdentityFromItem);
    connect(mIPage.mIdentityList, &QTreeWidget::itemDoubleClicked, this, &IdentityPage::slotModifyIdentity);
    connect(mIPage.mIdentityList, &IdentityListView::contextMenu, this, &IdentityPage::slotContextMenu);

    connect(mIPage.mButtonAdd, &QPushButton::clicked, this, &IdentityPage::slotNewIdentity);
    connect(mIPage.mModifyButton, &QPushButton::clicked, this, &IdentityPage::slotModifyIdentity);
    connect(mIPage.mRenameButton, &QPushButton::clicked, this, &IdentityPage::slotRenameIdentity);
    connect(mIPage.mRemoveButton, &QPushButton::clicked, this, &IdentityPage::slotRemoveIdentity);
    connect(mIPage.mSetAsDefaultButton, &QPushButton::clicked, this, &IdentityPage::slotSetAsDefault);
    load();
}

IdentityPage::~IdentityPage() = default;

void IdentityPage::load()
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mOldNumberOfIdentities = mIdentityManager->shadowIdentities().count();
    // Fill the list:
    mIPage.mIdentityList->clear();
    QTreeWidgetItem *item = nullptr;
    KIdentityManagementCore::IdentityManager::Iterator end(mIdentityManager->modifyEnd());

    for (KIdentityManagementCore::IdentityManager::Iterator it = mIdentityManager->modifyBegin(); it != end; ++it) {
        item = new IdentityListViewItem(mIPage.mIdentityList, item, *it);
    }
    if (auto currentItem = mIPage.mIdentityList->currentItem()) {
        currentItem->setSelected(true);
    }
}

void IdentityPage::save()
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mIdentityManager->sort();
    mIdentityManager->commit();

    if (mOldNumberOfIdentities < 2 && mIPage.mIdentityList->topLevelItemCount() > 1) {
        // have more than one identity, so better show the combo in the
        // composer now:
        int showHeaders = KMailSettings::self()->headers();
        showHeaders |= KMail::Composer::HDR_IDENTITY;
        KMailSettings::self()->setHeaders(showHeaders);
    }
    // and now the reverse
    if (mOldNumberOfIdentities > 1 && mIPage.mIdentityList->topLevelItemCount() < 2) {
        // have only one identity, so remove the combo in the composer:
        int showHeaders = KMailSettings::self()->headers();
        showHeaders &= ~KMail::Composer::HDR_IDENTITY;
        KMailSettings::self()->setHeaders(showHeaders);
    }
}

void IdentityPage::slotNewIdentity()
{
    Q_ASSERT(!mIdentityDialog);

    QScopedPointer<NewIdentityDialog> dialog(new NewIdentityDialog(mIdentityManager, this));
    dialog->setObjectName("new"_L1);

    if (dialog->exec() == QDialog::Accepted && dialog) {
        QString identityName = dialog->identityName().trimmed();
        Q_ASSERT(!identityName.isEmpty());

        //
        // Construct a new Identity:
        //
        switch (dialog->duplicateMode()) {
        case NewIdentityDialog::ExistingEntry: {
            KIdentityManagementCore::Identity &dupThis = mIdentityManager->modifyIdentityForName(dialog->duplicateIdentity());
            mIdentityManager->newFromExisting(dupThis, identityName);
            break;
        }
        case NewIdentityDialog::ControlCenter:
            mIdentityManager->newFromControlCenter(identityName);
            break;
        case NewIdentityDialog::Empty:
            mIdentityManager->newFromScratch(identityName);
        default:;
        }

        //
        // Insert into listview:
        //
        KIdentityManagementCore::Identity &newIdent = mIdentityManager->modifyIdentityForName(identityName);
        QTreeWidgetItem *item = nullptr;
        if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
            item = mIPage.mIdentityList->selectedItems().at(0);
        }

        QTreeWidgetItem *newItem = nullptr;
        if (item) {
            newItem = new IdentityListViewItem(mIPage.mIdentityList, mIPage.mIdentityList->itemAbove(item), newIdent);
        } else {
            newItem = new IdentityListViewItem(mIPage.mIdentityList, newIdent);
        }

        mIPage.mIdentityList->selectionModel()->clearSelection();
        if (newItem) {
            newItem->setSelected(true);
        }

        slotModifyIdentity();
        updateButtons();
    }
}

void IdentityPage::slotModifyIdentity()
{
    Q_ASSERT(!mIdentityDialog);

    IdentityListViewItem *item = nullptr;
    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = dynamic_cast<IdentityListViewItem *>(mIPage.mIdentityList->selectedItems().first());
    }
    if (!item) {
        return;
    }

    mIdentityDialog = new IdentityDialog(this);
    mIdentityDialog->setIdentity(item->identity());

    // Hmm, an unmodal dialog would be nicer, but a modal one is easier ;-)
    if (mIdentityDialog->exec() == QDialog::Accepted) {
        mIdentityDialog->updateIdentity(item->identity());
        item->redisplay();
        save();
    }

    delete mIdentityDialog;
    mIdentityDialog = nullptr;
}

void IdentityPage::slotRemoveIdentity()
{
    Q_ASSERT(!mIdentityDialog);

    if (mIdentityManager->shadowIdentities().count() < 2) {
        qCritical() << "Attempted to remove the last identity!";
    }

    const int numberOfIdentity = mIPage.mIdentityList->selectedItems().count();
    QString identityName;
    IdentityListViewItem *item = nullptr;
    const QList<QTreeWidgetItem *> selectedItems = mIPage.mIdentityList->selectedItems();
    if (numberOfIdentity == 1) {
        if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
            item = dynamic_cast<IdentityListViewItem *>(mIPage.mIdentityList->selectedItems().at(0));
        }
        if (!item) {
            return;
        }
        identityName = item->identity().identityName();
    }
    const QString msg = numberOfIdentity == 1
        ? i18n(
            "<qt>Do you really want to remove the identity named "
            "<b>%1</b>?</qt>",
            identityName)
        : i18np("Do you really want to remove this %1 identity?", "Do you really want to remove these %1 identities?", numberOfIdentity);
    if (KMessageBox::warningContinueCancel(this,
                                           msg,
                                           i18np("Remove Identity", "Remove Identities", numberOfIdentity),
                                           KGuiItem(i18nc("@action:button", "&Remove"), QStringLiteral("edit-delete")))
        == KMessageBox::Continue) {
        for (QTreeWidgetItem *selecteditem : selectedItems) {
            auto identityItem = dynamic_cast<IdentityListViewItem *>(selecteditem);
            identityName = identityItem->identity().identityName();
            if (mIdentityManager->removeIdentity(identityName)) {
                delete selecteditem;
            }
            if (mIPage.mIdentityList->currentItem()) {
                mIPage.mIdentityList->currentItem()->setSelected(true);
            }
            refreshList();
            updateButtons();
        }
    }
}

void IdentityPage::slotRenameIdentity()
{
    Q_ASSERT(!mIdentityDialog);

    QTreeWidgetItem *item = nullptr;

    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = mIPage.mIdentityList->selectedItems().first();
    }
    if (!item) {
        return;
    }

    mIPage.mIdentityList->editItem(item);
}

void IdentityPage::slotRenameIdentityFromItem(KMail::IdentityListViewItem *item, const QString &text)
{
    if (!item) {
        return;
    }

    const QString newName = text.trimmed();
    if (!newName.isEmpty() && !mIdentityManager->shadowIdentities().contains(newName)) {
        KIdentityManagementCore::Identity &ident = item->identity();
        ident.setIdentityName(newName);
        save();
    }
    item->redisplay();
}

void IdentityPage::slotContextMenu(IdentityListViewItem *item, const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action", "Add…"), this, &IdentityPage::slotNewIdentity);
    if (item) {
        menu.addAction(QIcon::fromTheme(QStringLiteral("document-edit")), i18nc("@action", "Modify…"), this, &IdentityPage::slotModifyIdentity);
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action", "Rename"), this, &IdentityPage::slotRenameIdentity);
        if (mIPage.mIdentityList->topLevelItemCount() > 1) {
            menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18nc("@action", "Remove"), this, &IdentityPage::slotRemoveIdentity);
        }
        if (!item->identity().isDefault()) {
            menu.addSeparator();
            menu.addAction(i18nc("@action", "Set as Default"), this, &IdentityPage::slotSetAsDefault);
        }
    }
    menu.exec(pos);
}

void IdentityPage::slotSetAsDefault()
{
    Q_ASSERT(!mIdentityDialog);

    IdentityListViewItem *item = nullptr;
    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = dynamic_cast<IdentityListViewItem *>(mIPage.mIdentityList->selectedItems().first());
    }
    if (!item) {
        return;
    }

    mIdentityManager->setAsDefault(item->identity().uoid());
    refreshList();
    mIPage.mSetAsDefaultButton->setEnabled(false);
}

void IdentityPage::refreshList()
{
    const int numberOfTopLevel(mIPage.mIdentityList->topLevelItemCount());
    for (int i = 0; i < numberOfTopLevel; ++i) {
        auto item = dynamic_cast<IdentityListViewItem *>(mIPage.mIdentityList->topLevelItem(i));
        if (item) {
            item->redisplay();
        }
    }
    save();
}

void IdentityPage::slotIdentitySelectionChanged()
{
    updateButtons();
}

void IdentityPage::updateButtons()
{
    const int numSelectedItems = mIPage.mIdentityList->selectedItems().count();
    mIPage.mRemoveButton->setEnabled(numSelectedItems >= 1);
    mIPage.mModifyButton->setEnabled(numSelectedItems == 1);
    mIPage.mRenameButton->setEnabled(numSelectedItems == 1);
    IdentityListViewItem *item = nullptr;
    if (numSelectedItems > 0) {
        item = dynamic_cast<IdentityListViewItem *>(mIPage.mIdentityList->selectedItems().first());
    }
    const bool enableDefaultButton = (numSelectedItems == 1) && item && !item->identity().isDefault();
    mIPage.mSetAsDefaultButton->setEnabled(enableDefaultButton);
}

#include "moc_identitypage.cpp"
