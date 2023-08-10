/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidget.h"
#include "archivemailagent_debug.h"
#include "hourcombobox.h"
#include <KLocalizedString>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>

ArchiveMailRangeWidget::ArchiveMailRangeWidget(QWidget *parent)
    : QWidget{parent}
    , mStartRange(new HourComboBox(this))
    , mEndRange(new HourComboBox(this))
    , mEnabled(new QCheckBox(i18n("Use Range"), this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainLayout"));
    mainLayout->setContentsMargins({});

    mEnabled->setObjectName(QStringLiteral("mEnabled"));
    mainLayout->addWidget(mEnabled);

    mStartRange->setObjectName(QStringLiteral("mStartRange"));

    mEndRange->setObjectName(QStringLiteral("mEndRange"));

    mainLayout->addWidget(mStartRange);
    mainLayout->addWidget(mEndRange);

    connect(mEnabled, &QCheckBox::toggled, this, &ArchiveMailRangeWidget::changeRangeState);
    changeRangeState(false);
    //    connect(mStartRange, &KTimeComboBox::timeChanged, this, [this](const QTime &time) {
    //        // TODO
    //    });
    //    connect(mEndRange, &KTimeComboBox::timeChanged, this, [this](const QTime &time) {
    //        // TODO
    //    });
}

ArchiveMailRangeWidget::~ArchiveMailRangeWidget() = default;

void ArchiveMailRangeWidget::changeRangeState(bool enabled)
{
    mStartRange->setEnabled(enabled);
    mEndRange->setEnabled(enabled);
}

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
    timeRange.append(mStartRange->hour());
    timeRange.append(mEndRange->hour());
    return timeRange;
}

void ArchiveMailRangeWidget::setRange(const QList<int> &hours)
{
    if (hours.count() != 2) {
        qCWarning(ARCHIVEMAILAGENT_LOG) << "Ranges is invalid " << hours;
    } else {
        mStartRange->setHour(hours.at(0));
        mStartRange->setHour(hours.at(1));
    }
}

#include "moc_archivemailrangewidget.cpp"
