/*
  SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityexpirespamfolderdialog.h"
#include <KLocalizedString>
#include <MailCommon/CollectionExpiryWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;
IdentityExpireSpamFolderDialog::IdentityExpireSpamFolderDialog(QWidget *parent)
    : QDialog(parent)
    , mCollectionExpiryWidget(new MailCommon::CollectionExpiryWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Expire Spam Folder"));
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName("mainLayout"_L1);

    mCollectionExpiryWidget->setObjectName("mCollectionExpiryWidget"_L1);
    mainLayout->addWidget(mCollectionExpiryWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setObjectName("buttonBox"_L1);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityExpireSpamFolderDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityExpireSpamFolderDialog::reject);
}

IdentityExpireSpamFolderDialog::~IdentityExpireSpamFolderDialog() = default;

#include "moc_identityexpirespamfolderdialog.cpp"
