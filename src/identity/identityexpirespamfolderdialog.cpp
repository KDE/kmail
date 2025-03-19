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

    connect(mCollectionExpiryWidget, &MailCommon::CollectionExpiryWidget::saveAndExpireRequested, this, &IdentityExpireSpamFolderDialog::slotSaveAndExpire);
    connect(mCollectionExpiryWidget, &MailCommon::CollectionExpiryWidget::configChanged, this, &IdentityExpireSpamFolderDialog::slotConfigChanged);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setObjectName("buttonBox"_L1);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityExpireSpamFolderDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityExpireSpamFolderDialog::reject);
}

IdentityExpireSpamFolderDialog::~IdentityExpireSpamFolderDialog()
{
    if (mChanged) {
        saveAndExpire(mCollection, true, false);
    }
}

void IdentityExpireSpamFolderDialog::load(const Akonadi::Collection &collection)
{
    mCollection = collection;
    const auto *attr = collection.attribute<MailCommon::ExpireCollectionAttribute>();
    if (attr) {
        MailCommon::CollectionExpirySettings settings;
        settings.convertFromExpireCollectionAttribute(attr);
        mCollectionExpiryWidget->load(settings);
    } else {
        mCollectionExpiryWidget->load({});
    }
    mChanged = false;
}

void IdentityExpireSpamFolderDialog::slotSaveAndExpire()
{
    saveAndExpire(mCollection, true, true); // save and start expire job
}

void IdentityExpireSpamFolderDialog::slotChanged()
{
    mChanged = true;
}

void IdentityExpireSpamFolderDialog::saveAndExpire(Akonadi::Collection &collection, bool saveSettings, bool expireNow)
{
    mCollectionExpiryWidget->save(collection, saveSettings, expireNow);
    mChanged = false;
}

void IdentityExpireSpamFolderDialog::slotConfigChanged(bool changed)
{
    mChanged = changed;
}

#include "moc_identityexpirespamfolderdialog.cpp"
