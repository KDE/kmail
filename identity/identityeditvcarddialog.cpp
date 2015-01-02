/*
  Copyright (c) 2012-2015 Montel Laurent <montel@kde.org>

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

#include "identityeditvcarddialog.h"

#include <KContacts/VCardConverter>
#include <KLocalizedString>
#include <Akonadi/Contact/ContactEditor>
#include "kmail_debug.h"
#include <KMessageBox>
#include <QStandardPaths>

#include <QHBoxLayout>
#include <QFile>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

IdentityEditVcardDialog::IdentityEditVcardDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityEditVcardDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityEditVcardDialog::reject);
    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);

    if (QFile(fileName).exists()) {
        setWindowTitle(i18n("Edit own vCard"));
        QPushButton *user1Button = new QPushButton;
        buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
        user1Button->setText(i18n("Delete current vCard"));
        connect(user1Button, &QPushButton::clicked, this, &IdentityEditVcardDialog::slotDeleteCurrentVCard);
    } else {
        setWindowTitle(i18n("Create own vCard"));
    }

    okButton->setDefault(true);
    setModal(true);
    QWidget *mainWidget = new QWidget(this);
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);
    QHBoxLayout *mainLayout = new QHBoxLayout(mainWidget);

    mContactEditor = new Akonadi::ContactEditor(Akonadi::ContactEditor::CreateMode, Akonadi::ContactEditor::VCardMode, this);
    mainLayout->addWidget(mContactEditor);
    loadVcard(fileName);
}

IdentityEditVcardDialog::~IdentityEditVcardDialog()
{
}

void IdentityEditVcardDialog::slotDeleteCurrentVCard()
{
    if (mVcardFileName.isEmpty()) {
        return;
    }
    if (KMessageBox::Yes == KMessageBox::questionYesNo(this, i18n("Are you sure to want to delete this vCard?"), i18n("Delete vCard"))) {
        if (mVcardFileName.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))) {
            deleteCurrentVcard(true);
        } else {
            deleteCurrentVcard(false);
        }
        reject();
    }
}

void IdentityEditVcardDialog::deleteCurrentVcard(bool deleteOnDisk)
{
    if (!mVcardFileName.isEmpty()) {
        if (deleteOnDisk) {
            QFile file(mVcardFileName);
            if (file.exists()) {
                if (!file.remove()) {
                    KMessageBox::error(this, i18n("We cannot delete vCard file."), i18n("Delete vCard"));
                }
            }
        }
        Q_EMIT vcardRemoved();
    }
}

void IdentityEditVcardDialog::loadVcard(const QString &vcardFileName)
{
    if (vcardFileName.isEmpty()) {
        return;
    }
    mVcardFileName = vcardFileName;
    QFile file(vcardFileName);

    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        file.close();
        if (!data.isEmpty()) {
            KContacts::VCardConverter converter;
            KContacts::Addressee addr = converter.parseVCard(data);
            mContactEditor->setContactTemplate(addr);
        }
    }
}

QString IdentityEditVcardDialog::saveVcard() const
{
    const KContacts::Addressee addr = mContactEditor->contact();
    KContacts::VCardConverter converter;
    QFile file(mVcardFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QByteArray data = converter.exportVCard(addr, KContacts::VCardConverter::v3_0);
        file.write(data);
        file.flush();
        file.close();
    } else {
        qCDebug(KMAIL_LOG) << "We cannot open file: " << mVcardFileName;
    }
    return mVcardFileName;
}

