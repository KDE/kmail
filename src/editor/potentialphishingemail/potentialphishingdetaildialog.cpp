/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetaildialog.h"
#include "potentialphishingdetailwidget.h"
#include <KSharedConfig>
#include <KLocalizedString>
#include <KConfigGroup>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

PotentialPhishingDetailDialog::PotentialPhishingDetailDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Details"));
    auto topLayout = new QVBoxLayout(this);
    setModal(true);

    mPotentialPhishingDetailWidget = new PotentialPhishingDetailWidget(this);
    mPotentialPhishingDetailWidget->setObjectName(QStringLiteral("potentialphising_widget"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(okButton, &QAbstractButton::clicked, this, &PotentialPhishingDetailDialog::slotSave);

    topLayout->addWidget(mPotentialPhishingDetailWidget);
    topLayout->addWidget(buttonBox);

    readConfig();
}

PotentialPhishingDetailDialog::~PotentialPhishingDetailDialog()
{
    writeConfig();
}

void PotentialPhishingDetailDialog::fillList(const QStringList &lst)
{
    mPotentialPhishingDetailWidget->fillList(lst);
}

void PotentialPhishingDetailDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "PotentialPhishingDetailDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(800, 600));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}

void PotentialPhishingDetailDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "PotentialPhishingDetailDialog");
    group.writeEntry("Size", size());
}

void PotentialPhishingDetailDialog::slotSave()
{
    mPotentialPhishingDetailWidget->save();
    accept();
}
