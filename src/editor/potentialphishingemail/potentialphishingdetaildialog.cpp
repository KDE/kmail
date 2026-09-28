/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetaildialog.h"

#include "potentialphishingdetailwidget.h"
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <TextAddonsWidgets/LoadDialogSizeUtils>
using namespace Qt::Literals::StringLiterals;
namespace
{
const char myPotentialPhishingDetailDialogGroupName[] = "PotentialPhishingDetailDialog";
}
PotentialPhishingDetailDialog::PotentialPhishingDetailDialog(QWidget *parent)
    : QDialog(parent)
    , mPotentialPhishingDetailWidget(new PotentialPhishingDetailWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Details"));
    auto topLayout = new QVBoxLayout(this);

    mPotentialPhishingDetailWidget->setObjectName("potentialphising_widget"_L1);

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

PotentialPhishingDetailDialog::~PotentialPhishingDetailDialog() = default;

void PotentialPhishingDetailDialog::fillList(const QStringList &lst)
{
    mPotentialPhishingDetailWidget->fillList(lst);
}

void PotentialPhishingDetailDialog::readConfig()
{
    TextAddonsWidgets::LoadDialogSizeUtils::manageDialogSize(this, QLatin1StringView(myPotentialPhishingDetailDialogGroupName), QSize(800, 600));
}

void PotentialPhishingDetailDialog::slotSave()
{
    mPotentialPhishingDetailWidget->save();
    accept();
}

#include "moc_potentialphishingdetaildialog.cpp"
