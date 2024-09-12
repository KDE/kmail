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

#include "identityngpage.h"
#include "config-kmail.h"
using namespace Qt::Literals::StringLiterals;

#include "identitydialog.h"
#include "identitytreengwidget.h"
#include "newidentitydialog.h"
#include "settings/kmailsettings.h"

#include <MailCommon/MailKernel>

#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>

#include <KLocalizedString>
#include <KMessageBox>
#include <QMenu>

#include <QTreeWidgetItem>

#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include "activities/activitiesmanager.h"
#include "activities/identityactivities.h"
#endif

using namespace KMail;

QString IdentityNgPage::helpAnchor() const
{
    return QStringLiteral("configure-identity");
}

IdentityNgPage::IdentityNgPage(QWidget *parent)
    : ConfigModuleTab(parent)
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mIdentityManager = KernelIf->identityManager();
    connect(mIdentityManager, &KIdentityManagementCore::IdentityManager::needToReloadIdentitySettings, this, &IdentityNgPage::load);
    mIPage.setupUi(this);
    mIPage.mIdentityList->setIdentityManager(mIdentityManager);
#if 0
    connect(mIPage.mIdentityList, &QTreeWidget::itemSelectionChanged, this, &IdentityNgPage::slotIdentitySelectionChanged);
    connect(this, qOverload<bool>(&IdentityNgPage::changed), this, &IdentityNgPage::slotIdentitySelectionChanged);
    connect(mIPage.mIdentityList,
            qOverload<KMail::IdentityTreeWidgetItem *, const QString &>(&IdentityTreeWidget::rename),
            this,
            &IdentityNgPage::slotRenameIdentityFromItem);
    connect(mIPage.mIdentityList, &QTreeWidget::itemDoubleClicked, this, &IdentityNgPage::slotModifyIdentity);

#endif
    connect(mIPage.mIdentityList, &IdentityTreeNgWidget::contextMenuRequested, this, &IdentityNgPage::slotContextMenu);
    connect(mIPage.mButtonAdd, &QPushButton::clicked, this, &IdentityNgPage::slotNewIdentity);
    connect(mIPage.mModifyButton, &QPushButton::clicked, this, &IdentityNgPage::slotModifyIdentity);
    connect(mIPage.mRenameButton, &QPushButton::clicked, this, &IdentityNgPage::slotRenameIdentity);
    connect(mIPage.mRemoveButton, &QPushButton::clicked, this, &IdentityNgPage::slotRemoveIdentity);
    connect(mIPage.mSetAsDefaultButton, &QPushButton::clicked, this, &IdentityNgPage::slotSetAsDefault);
    // Identity
    mIPage.identitiesOnCurrentActivity->setVisible(false);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    mIPage.identitiesOnCurrentActivity->setVisible(KMailSettings::self()->plasmaActivitySupport());
    // TODO add identifyactivities support
    // mIPage.mIdentityList->setIdentityManager(ActivitiesManager::self()->identityActivities());

#endif
    load();
}

IdentityNgPage::~IdentityNgPage() = default;

void IdentityNgPage::setEnablePlasmaActivities(bool enable)
{
    mIPage.identitiesOnCurrentActivity->setVisible(enable);
}

void IdentityNgPage::load()
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mOldNumberOfIdentities = mIdentityManager->shadowIdentities().count();
    // Fill the list:
#if 0
    mIPage.mIdentityList->clear();
    QTreeWidgetItem *item = nullptr;
    KIdentityManagementCore::IdentityManager::Iterator end(mIdentityManager->modifyEnd());

    for (KIdentityManagementCore::IdentityManager::Iterator it = mIdentityManager->modifyBegin(); it != end; ++it) {
        item = new IdentityTreeWidgetItem(mIPage.mIdentityList, item, *it);
    }
    if (auto currentItem = mIPage.mIdentityList->currentItem()) {
        currentItem->setSelected(true);
    }
#endif
}

void IdentityNgPage::save()
{
    if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
        return;
    }
    mIdentityManager->sort();
    mIdentityManager->commit();
    if (mOldNumberOfIdentities < 2 && mIPage.mIdentityList->model()->rowCount() > 1) {
        // have more than one identity, so better show the combo in the
        // composer now:
        int showHeaders = KMailSettings::self()->headers();
        showHeaders |= KMail::Composer::HDR_IDENTITY;
        KMailSettings::self()->setHeaders(showHeaders);
    }
    // and now the reverse
    if (mOldNumberOfIdentities > 1 && mIPage.mIdentityList->model()->rowCount() < 2) {
        // have only one identity, so remove the combo in the composer:
        int showHeaders = KMailSettings::self()->headers();
        showHeaders &= ~KMail::Composer::HDR_IDENTITY;
        KMailSettings::self()->setHeaders(showHeaders);
    }
}

void IdentityNgPage::slotNewIdentity()
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
#if 0
        QTreeWidgetItem *item = nullptr;
        if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
            item = mIPage.mIdentityList->selectedItems().at(0);
        }

        QTreeWidgetItem *newItem = nullptr;
        if (item) {
            newItem = new IdentityTreeWidgetItem(mIPage.mIdentityList, mIPage.mIdentityList->itemAbove(item), newIdent);
        } else {
            newItem = new IdentityTreeWidgetItem(mIPage.mIdentityList, newIdent);
        }

        mIPage.mIdentityList->selectionModel()->clearSelection();
        if (newItem) {
            newItem->setSelected(true);
        }
#endif
        slotModifyIdentity();
        updateButtons();
    }
}

void IdentityNgPage::slotModifyIdentity()
{
    Q_ASSERT(!mIdentityDialog);
#if 0

    IdentityTreeWidgetItem *item = nullptr;
    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = dynamic_cast<IdentityTreeWidgetItem *>(mIPage.mIdentityList->selectedItems().first());
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
#endif
    delete mIdentityDialog;
    mIdentityDialog = nullptr;
}

void IdentityNgPage::slotRemoveIdentity()
{
    Q_ASSERT(!mIdentityDialog);

    if (mIdentityManager->shadowIdentities().count() < 2) {
        qCritical() << "Attempted to remove the last identity!";
    }
#if 0
    const int numberOfIdentity = mIPage.mIdentityList->selectedItems().count();
    QString identityName;
    IdentityTreeWidgetItem *item = nullptr;
    const QList<QTreeWidgetItem *> selectedItems = mIPage.mIdentityList->selectedItems();
    if (numberOfIdentity == 1) {
        if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
            item = dynamic_cast<IdentityTreeWidgetItem *>(mIPage.mIdentityList->selectedItems().at(0));
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
            auto identityItem = dynamic_cast<IdentityTreeWidgetItem *>(selecteditem);
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
#endif
}

void IdentityNgPage::slotRenameIdentity()
{
    Q_ASSERT(!mIdentityDialog);
#if 0
    QTreeWidgetItem *item = nullptr;

    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = mIPage.mIdentityList->selectedItems().first();
    }
    if (!item) {
        return;
    }

    mIPage.mIdentityList->editItem(item);
#endif
}

void IdentityNgPage::slotRenameIdentityFromItem(KMail::IdentityTreeWidgetItem *item, const QString &text)
{
    if (!item) {
        return;
    }
#if 0
    const QString newName = text.trimmed();
    if (!newName.isEmpty() && !mIdentityManager->shadowIdentities().contains(newName)) {
        KIdentityManagementCore::Identity &ident = item->identity();
        ident.setIdentityName(newName);
        save();
    }
    item->redisplay();
#endif
}

void IdentityNgPage::slotContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action", "Add…"), this, &IdentityNgPage::slotNewIdentity);
#if 0
    if (item) {
        menu.addAction(QIcon::fromTheme(QStringLiteral("document-edit")), i18nc("@action", "Modify…"), this, &IdentityNgPage::slotModifyIdentity);
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action", "Rename"), this, &IdentityNgPage::slotRenameIdentity);
        if (mIPage.mIdentityList->model()->rowCount() > 1) {
            menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18nc("@action", "Remove"), this, &IdentityNgPage::slotRemoveIdentity);
        }
        if (!item->identity().isDefault()) {
            menu.addSeparator();
            menu.addAction(i18nc("@action", "Set as Default"), this, &IdentityNgPage::slotSetAsDefault);
        }
    }
#endif
    menu.exec(pos);
}

void IdentityNgPage::slotSetAsDefault()
{
#if 0
    Q_ASSERT(!mIdentityDialog);

    IdentityTreeWidgetItem *item = nullptr;
    if (!mIPage.mIdentityList->selectedItems().isEmpty()) {
        item = dynamic_cast<IdentityTreeWidgetItem *>(mIPage.mIdentityList->selectedItems().first());
    }
    if (!item) {
        return;
    }

    mIdentityManager->setAsDefault(item->identity().uoid());
    refreshList();
    mIPage.mSetAsDefaultButton->setEnabled(false);
#endif
}

void IdentityNgPage::refreshList()
{
#if 0
    const int numberOfTopLevel(mIPage.mIdentityList->model()->rowCount());
    for (int i = 0; i < numberOfTopLevel; ++i) {
        auto item = dynamic_cast<IdentityTreeWidgetItem *>(mIPage.mIdentityList->topLevelItem(i));
        if (item) {
            item->redisplay();
        }
    }
    save();
#endif
}

void IdentityNgPage::slotIdentitySelectionChanged()
{
    updateButtons();
}

void IdentityNgPage::updateButtons()
{
    const int numSelectedItems = mIPage.mIdentityList->selectionModel()->selectedRows().count();
    mIPage.mRemoveButton->setEnabled(numSelectedItems >= 1);
    mIPage.mModifyButton->setEnabled(numSelectedItems == 1);
    mIPage.mRenameButton->setEnabled(numSelectedItems == 1);
#if 0
    IdentityTreeWidgetItem *item = nullptr;
    if (numSelectedItems > 0) {
        item = dynamic_cast<IdentityTreeWidgetItem *>(mIPage.mIdentityList->selectedItems().first());
    }
    const bool enableDefaultButton = (numSelectedItems == 1) && item && !item->identity().isDefault();
    mIPage.mSetAsDefaultButton->setEnabled(enableDefaultButton);
#endif
}

#include "moc_identityngpage.cpp"