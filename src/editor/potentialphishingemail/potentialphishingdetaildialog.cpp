/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetaildialog.h"
#include "potentialphishingdetailwidget.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
static const char myPotentialPhishingDetailDialogGroupName[] = "PotentialPhishingDetailDialog";
}
PotentialPhishingDetailDialog::PotentialPhishingDetailDialog(QWidget *parent)
    : QDialog(parent)
    , mPotentialPhishingDetailWidget(new PotentialPhishingDetailWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Details"));
    auto topLayout = new QVBoxLayout(this);
    setModal(true);

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
    KConfigGroup group(KSharedConfig::openStateConfig(), myPotentialPhishingDetailDialogGroupName);
    const QSize sizeDialog = group.readEntry("Size", QSize(800, 600));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}

void PotentialPhishingDetailDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), myPotentialPhishingDetailDialogGroupName);
    group.writeEntry("Size", size());
}

void PotentialPhishingDetailDialog::slotSave()
{
    mPotentialPhishingDetailWidget->save();
    accept();
}
