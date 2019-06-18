/*
  Copyright (c) 2015-2019 Montel Laurent <montel@kde.org>

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
#include "potentialphishingdetailwidget.h"

#include <KConfigGroup>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <KSharedConfig>
#include <KLocalizedString>

PotentialPhishingDetailWidget::PotentialPhishingDetailWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *lab = new QLabel(i18n("Select email to put in whitelist:"), this);
    lab->setObjectName(QStringLiteral("label"));
    mainLayout->addWidget(lab);

    mListWidget = new QListWidget(this);
    mListWidget->setObjectName(QStringLiteral("list_widget"));
    mainLayout->addWidget(mListWidget);
}

PotentialPhishingDetailWidget::~PotentialPhishingDetailWidget()
{
}

void PotentialPhishingDetailWidget::fillList(const QStringList &lst)
{
    mListWidget->clear();
    QStringList emailsAdded;
    for (const QString &mail : lst) {
        if (!emailsAdded.contains(mail)) {
            QListWidgetItem *item = new QListWidgetItem(mListWidget);
            item->setCheckState(Qt::Unchecked);
            item->setText(mail);
            emailsAdded << mail;
        }
    }
}

void PotentialPhishingDetailWidget::save()
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
}
