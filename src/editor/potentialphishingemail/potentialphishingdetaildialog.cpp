/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

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
    setWindowTitle(i18n("Details"));
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    setModal(true);

    mPotentialPhishingDetailWidget = new PotentialPhishingDetailWidget(this);
    mPotentialPhishingDetailWidget->setObjectName(QStringLiteral("potentialphising_widget"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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

