/*
 *
 * SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
 * SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "collectionquotawidget.h"

#include <KFormat>
#include <KLocalizedString>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>

CollectionQuotaWidget::CollectionQuotaWidget(QWidget *parent)
    : QWidget(parent)
    , mProgressBar(new QProgressBar(this))
    , mUsage(new QLabel(this))
{
    auto layout = new QGridLayout(this);

    auto lab = new QLabel(i18n("Usage:"), this);
    layout->addWidget(lab, 0, 0);

    mUsage->setTextFormat(Qt::PlainText);
    layout->addWidget(mUsage, 0, 1);

    auto Status = new QLabel(i18n("Status:"), this);
    layout->addWidget(Status, 1, 0);
    // xgettext: no-c-format
    mProgressBar->setFormat(i18n("%p% full"));
    layout->addWidget(mProgressBar, 1, 1);
    layout->setRowStretch(2, 1);
}

void CollectionQuotaWidget::setQuotaInfo(qint64 current, qint64 maxValue)
{
    const int perc = qBound(0, qRound(100.0 * current / qMax(1LL, maxValue)), 100);
    mProgressBar->setValue(perc);
    mUsage->setText(i18n("%1 of %2 used", KFormat().formatByteSize(qMax(0LL, current)), KFormat().formatByteSize(qMax(0LL, maxValue))));
}
