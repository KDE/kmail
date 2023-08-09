/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidget.h"
#include "archivemailagent_debug.h"
#include <KLocalizedString>
#include <KTimeComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>

ArchiveMailRangeWidget::ArchiveMailRangeWidget(QWidget *parent)
    : QWidget{parent}
    , mStartRange(new KTimeComboBox(this))
    , mEndRange(new KTimeComboBox(this))
    , mEnabled(new QCheckBox(i18n("Use Range"), this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mEnabled->setObjectName(QStringLiteral("mEnabled"));
    mainLayout->addWidget(mEnabled);

    mStartRange->setObjectName(QStringLiteral("mStartRange"));
    mStartRange->setTimeListInterval(60);

    mEndRange->setObjectName(QStringLiteral("mEndRange"));
    mEndRange->setTimeListInterval(60);

    mainLayout->addWidget(mStartRange);
    mainLayout->addWidget(mEndRange);

    connect(mEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        mStartRange->setEnabled(enabled);
        mEndRange->setEnabled(enabled);
    });
}

ArchiveMailRangeWidget::~ArchiveMailRangeWidget() = default;

bool ArchiveMailRangeWidget::isEnabled() const
{
    return mEnabled->isChecked();
}

void ArchiveMailRangeWidget::setEnabled(bool isEnabled)
{
    mEnabled->setChecked(isEnabled);
}

QList<int> ArchiveMailRangeWidget::range() const
{
    QList<int> timeRange;
    timeRange.append(mStartRange->time().hour());
    timeRange.append(mEndRange->time().hour());
    return timeRange;
}

void ArchiveMailRangeWidget::setRange(const QList<int> &hours)
{
    if (hours.count() != 2) {
        qCWarning(ARCHIVEMAILAGENT_LOG) << "Ranges is invalid " << hours;
    } else {
        const QTime startTime{hours.at(0), 0, 0};
        mStartRange->setTime(startTime);
        const QTime endTime{hours.at(1), 0, 0};
        mStartRange->setTime(endTime);
    }
}

#include "moc_archivemailrangewidget.cpp"
