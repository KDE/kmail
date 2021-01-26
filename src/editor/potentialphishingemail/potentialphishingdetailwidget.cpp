/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/
#include "potentialphishingdetailwidget.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

PotentialPhishingDetailWidget::PotentialPhishingDetailWidget(QWidget *parent)
    : QWidget(parent)
    , mListWidget(new QListWidget(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});
    auto lab = new QLabel(i18n("Select email to put in whitelist:"), this);
    lab->setObjectName(QStringLiteral("label"));
    mainLayout->addWidget(lab);

    mListWidget->setObjectName(QStringLiteral("list_widget"));
    mainLayout->addWidget(mListWidget);
}

PotentialPhishingDetailWidget::~PotentialPhishingDetailWidget() = default;

void PotentialPhishingDetailWidget::fillList(const QStringList &lst)
{
    mListWidget->clear();
    QStringList emailsAdded;
    for (const QString &mail : lst) {
        if (!emailsAdded.contains(mail)) {
            auto item = new QListWidgetItem(mListWidget);
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
