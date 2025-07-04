/*
  SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityeditvcarddialog.h"

#include "kmail_debug.h"
#include <Akonadi/ContactEditor>
#include <KContacts/VCardConverter>
#include <KLocalizedString>
#include <KMessageBox>
#include <QStandardPaths>

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPushButton>
#include <QVBoxLayout>

IdentityEditVcardDialog::IdentityEditVcardDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , mContactEditor(new Akonadi::AkonadiContactEditor(Akonadi::AkonadiContactEditor::CreateMode, Akonadi::AkonadiContactEditor::VCardMode, this))
{
    auto topLayout = new QVBoxLayout(this);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityEditVcardDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityEditVcardDialog::reject);

    if (QFileInfo::exists(fileName)) {
        setWindowTitle(i18nc("@title:window", "Edit own vCard"));
        auto user1Button = new QPushButton(this);
        buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
        user1Button->setText(i18n("Delete current vCard"));
        connect(user1Button, &QPushButton::clicked, this, &IdentityEditVcardDialog::slotDeleteCurrentVCard);
    } else {
        setWindowTitle(i18nc("@title:window", "Create own vCard"));
    }

    topLayout->addWidget(mContactEditor);
    topLayout->addWidget(buttonBox);
    loadVcard(fileName);
}

IdentityEditVcardDialog::~IdentityEditVcardDialog() = default;

void IdentityEditVcardDialog::slotDeleteCurrentVCard()
{
    if (mVcardFileName.isEmpty()) {
        return;
    }
    const int answer = KMessageBox::questionTwoActions(this,
                                                       i18n("Are you sure you want to delete this vCard?"),
                                                       i18nc("@title:window", "Delete vCard"),
                                                       KStandardGuiItem::del(),
                                                       KStandardGuiItem::cancel());
    if (answer == KMessageBox::ButtonCode::PrimaryAction) {
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
                    KMessageBox::error(this, i18n("We cannot delete vCard file."), i18nc("@title:window", "Delete vCard"));
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

void IdentityEditVcardDialog::reject()
{
    const int answer = KMessageBox::questionTwoActions(this,
                                                       i18nc("@info", "Do you really want to cancel?"),
                                                       i18nc("@title:window", "Confirmation"),
                                                       KGuiItem(i18nc("@action:button", "Cancel Editing"), QStringLiteral("dialog-ok")),
                                                       KGuiItem(i18nc("@action:button", "Do Not Cancel"), QStringLiteral("dialog-cancel")));
    if (answer == KMessageBox::ButtonCode::PrimaryAction) {
        QDialog::reject(); // Discard current changes
    }
}

#include "moc_identityeditvcarddialog.cpp"
