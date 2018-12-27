/**
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 *
 * Copyright (C) 2012-2019 Laurent Montel <montel@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "collectionquotawidget.h"

#include <KLocalizedString>

#include <QLabel>
#include <qlayout.h>
#include <qprogressbar.h>
#include <KFormat>

CollectionQuotaWidget::CollectionQuotaWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);

    QLabel *lab = new QLabel(i18n("Usage:"), this);
    layout->addWidget(lab, 0, 0);

    mUsage = new QLabel(this);
    mUsage->setTextFormat(Qt::PlainText);
    layout->addWidget(mUsage, 0, 1);

    QLabel *Status = new QLabel(i18n("Status:"), this);
    layout->addWidget(Status, 1, 0);
    mProgressBar = new QProgressBar(this);
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
