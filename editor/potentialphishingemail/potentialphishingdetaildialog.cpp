/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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
#include <KSharedConfig>
#include <KLocalizedString>
#include <qboxlayout.h>
#include <QLabel>
#include <QListWidget>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

PotentialPhishingDetailDialog::PotentialPhishingDetailDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Details"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    okButton->setDefault(true);

    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);

    setModal(true);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    QLabel *lab = new QLabel(i18n("Select email to put in whitelist:"));
    lab->setObjectName(QStringLiteral("label"));
    mainLayout->addWidget(lab);

    mListWidget = new QListWidget;
    mListWidget->setObjectName(QStringLiteral("list_widget"));
    mainLayout->addWidget(mListWidget);

    mainLayout->addWidget(buttonBox);

    connect(okButton, &QAbstractButton::clicked, this, &PotentialPhishingDetailDialog::slotSave);
    topLayout->addWidget(mainWidget);
    readConfig();
}

PotentialPhishingDetailDialog::~PotentialPhishingDetailDialog()
{
    writeConfig();
}

void PotentialPhishingDetailDialog::fillList(const QStringList &lst)
{
    mListWidget->clear();
    QStringList emailsAdded;
    Q_FOREACH (const QString &mail, lst) {
        if (!emailsAdded.contains(mail)) {
            QListWidgetItem *item = new QListWidgetItem(mListWidget);
            item->setCheckState(Qt::Unchecked);
            item->setText(mail);
            emailsAdded << mail;
        }
    }
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
    KConfigGroup group(KSharedConfig::openConfig(), "PotentialPhishing");
    QStringList potentialPhishing = group.readEntry("whiteList", QStringList());
    bool emailsAdded = false;
    const int numberOfItem(mListWidget->count());
    for (int i = 0; i < numberOfItem; ++i) {
        QListWidgetItem *item = mListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            const QString email = item->text();
            if (!potentialPhishing.contains(email)) {
                potentialPhishing << email;
                emailsAdded = true;
            }
        }
    }
    if (emailsAdded) {
        group.writeEntry("whiteList", potentialPhishing);
    }
    accept();
}

