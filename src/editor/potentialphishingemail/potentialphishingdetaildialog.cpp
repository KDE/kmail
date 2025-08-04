/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetaildialog.h"

#include "potentialphishingdetailwidget.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>

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
    setModal(true);

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
    create(); // ensure a window is created
    windowHandle()->resize(QSize(800, 600));
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myPotentialPhishingDetailDialogGroupName));
    KWindowConfig::restoreWindowSize(windowHandle(), group);
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

void PotentialPhishingDetailDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myPotentialPhishingDetailDialogGroupName));
    KWindowConfig::saveWindowSize(windowHandle(), group);
}

void PotentialPhishingDetailDialog::slotSave()
{
    mPotentialPhishingDetailWidget->save();
    accept();
}

#include "moc_potentialphishingdetaildialog.cpp"
