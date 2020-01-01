/*
  Copyright (c) 2013-2020 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "addemailtoexistingcontactdialog.h"
#include "kmkernel.h"

#include <Akonadi/Contact/EmailAddressSelectionWidget>
#include <AkonadiCore/Session>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/EntityDisplayAttribute>
#include <Akonadi/Contact/ContactsTreeModel>
#include <AkonadiCore/ChangeRecorder>

#include <KContacts/Addressee>

#include <KLocalizedString>

#include <QTreeView>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <KConfigGroup>

AddEmailToExistingContactDialog::AddEmailToExistingContactDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Select Contact"));
    setModal(true);

    Akonadi::Session *session = new Akonadi::Session("AddEmailToExistingContactDialog", this);

    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);
    scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

    Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder(this);
    changeRecorder->setSession(session);
    changeRecorder->fetchCollection(true);
    changeRecorder->setItemFetchScope(scope);
    changeRecorder->setCollectionMonitored(Akonadi::Collection::root());
    //Just select address no group
    changeRecorder->setMimeTypeMonitored(KContacts::Addressee::mimeType(), true);

    Akonadi::ContactsTreeModel *model = new Akonadi::ContactsTreeModel(changeRecorder, this);

    mEmailSelectionWidget = new Akonadi::EmailAddressSelectionWidget(false, model, this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mEmailSelectionWidget);
    mEmailSelectionWidget->view()->setSelectionMode(QAbstractItemView::SingleSelection);
    readConfig();
    connect(mEmailSelectionWidget->view()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AddEmailToExistingContactDialog::slotSelectionChanged);
    connect(mEmailSelectionWidget->view(), &QTreeView::doubleClicked, this, &AddEmailToExistingContactDialog::slotDoubleClicked);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
    KConfigGroup group(KMKernel::self()->config(), "AddEmailToExistingContactDialog");
    const QSize size = group.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void AddEmailToExistingContactDialog::writeConfig()
{
    KConfigGroup group(KMKernel::self()->config(), "AddEmailToExistingContactDialog");
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
