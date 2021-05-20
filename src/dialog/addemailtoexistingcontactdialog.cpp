/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "addemailtoexistingcontactdialog.h"
#include "kmkernel.h"

#include <Akonadi/Contact/ContactsTreeModel>
#include <Akonadi/Contact/EmailAddressSelectionWidget>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/Session>

#include <KContacts/Addressee>

#include <KLocalizedString>

#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
namespace
{
static const char myAddEmailToExistingContactDialogGroupName[] = "AddEmailToExistingContactDialog";
}
AddEmailToExistingContactDialog::AddEmailToExistingContactDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Select Contact"));
    setModal(true);

    auto session = new Akonadi::Session("AddEmailToExistingContactDialog", this);

    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);
    scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

    auto changeRecorder = new Akonadi::ChangeRecorder(this);
    changeRecorder->setSession(session);
    changeRecorder->fetchCollection(true);
    changeRecorder->setItemFetchScope(scope);
    changeRecorder->setCollectionMonitored(Akonadi::Collection::root());
    // Just select address no group
    changeRecorder->setMimeTypeMonitored(KContacts::Addressee::mimeType(), true);

    auto model = new Akonadi::ContactsTreeModel(changeRecorder, this);

    mEmailSelectionWidget = new Akonadi::EmailAddressSelectionWidget(false, model, this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mEmailSelectionWidget);
    mEmailSelectionWidget->view()->setSelectionMode(QAbstractItemView::SingleSelection);
    readConfig();
    connect(mEmailSelectionWidget->view()->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &AddEmailToExistingContactDialog::slotSelectionChanged);
    connect(mEmailSelectionWidget->view(), &QTreeView::doubleClicked, this, &AddEmailToExistingContactDialog::slotDoubleClicked);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddEmailToExistingContactDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AddEmailToExistingContactDialog::reject);
    mainLayout->addWidget(buttonBox);
    mOkButton->setText(i18n("Select"));
    mOkButton->setEnabled(false);
}

AddEmailToExistingContactDialog::~AddEmailToExistingContactDialog()
{
    writeConfig();
}

void AddEmailToExistingContactDialog::slotDoubleClicked()
{
    if (!mEmailSelectionWidget->selectedAddresses().isEmpty()) {
        accept();
    }
}

void AddEmailToExistingContactDialog::slotSelectionChanged()
{
    mOkButton->setEnabled(!mEmailSelectionWidget->selectedAddresses().isEmpty());
}

void AddEmailToExistingContactDialog::readConfig()
{
    KConfigGroup group(KMKernel::self()->config(), myAddEmailToExistingContactDialogGroupName);
    const QSize size = group.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void AddEmailToExistingContactDialog::writeConfig()
{
    KConfigGroup group(KMKernel::self()->config(), myAddEmailToExistingContactDialogGroupName);
    group.writeEntry("Size", size());
    group.sync();
}

Akonadi::Item AddEmailToExistingContactDialog::selectedContact() const
{
    Akonadi::Item item;
    Akonadi::EmailAddressSelection::List lst = mEmailSelectionWidget->selectedAddresses();
    if (!lst.isEmpty()) {
        item = lst.at(0).item();
    }
    return item;
}
