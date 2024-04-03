/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailrangewidget.h"
using namespace Qt::Literals::StringLiterals;

#include "archivemailagent_debug.h"
#include "widgets/hourcombobox.h"
#include <KLocalizedString>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>

ArchiveMailRangeWidget::ArchiveMailRangeWidget(QWidget *parent)
    : QWidget{parent}
    , mStartRange(new HourComboBox(this))
    , mEndRange(new HourComboBox(this))
    , mRangeEnabled(new QCheckBox(i18n("Use Range"), this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName("mainLayout"_L1);
    mainLayout->setContentsMargins({});

    mRangeEnabled->setObjectName("mRangeEnabled"_L1);
    mainLayout->addWidget(mRangeEnabled);

    mStartRange->setObjectName("mStartRange"_L1);

    mEndRange->setObjectName("mEndRange"_L1);

    mainLayout->addWidget(mStartRange);
    mainLayout->addWidget(mEndRange);

    connect(mRangeEnabled, &QCheckBox::toggled, this, &ArchiveMailRangeWidget::changeRangeState);
    changeRangeState(false);
    mEndRange->setCurrentIndex(1); // Make sure that we have 1 hour between start/end
    connect(mStartRange, &HourComboBox::activated, this, [this](int index) {
        const int startHours = mStartRange->itemData(index).toInt();
        int endHours = mEndRange->currentData().toInt();
        if (startHours == endHours) {
            if (endHours == 23) {
                endHours = 0;
            } else {
                endHours = startHours + 1;
            }
            mEndRange->blockSignals(true);
            mEndRange->setHour(endHours);
            mEndRange->blockSignals(false);
        }
    });
    connect(mEndRange, &HourComboBox::activated, this, [this](int index) {
        int startHours = mStartRange->currentData().toInt();
        const int endHours = mEndRange->itemData(index).toInt();
        if (startHours == endHours) {
            if (endHours == 0) {
                startHours = 23;
            } else {
                startHours = startHours - 1;
            }
            mStartRange->blockSignals(true);
            mStartRange->setHour(startHours);
            mStartRange->blockSignals(false);
        }
    });
}

ArchiveMailRangeWidget::~ArchiveMailRangeWidget() = default;

void ArchiveMailRangeWidget::changeRangeState(bool enabled)
{
    mStartRange->setEnabled(enabled);
    mEndRange->setEnabled(enabled);
}

bool ArchiveMailRangeWidget::isRangeEnabled() const
{
    return mRangeEnabled->isChecked();
}

void ArchiveMailRangeWidget::setRangeEnabled(bool isEnabled)
{
    mRangeEnabled->setChecked(isEnabled);
}

QList<int> ArchiveMailRangeWidget::range() const
{
    const QList<int> timeRange{mStartRange->hour(), mEndRange->hour()};
    return timeRange;
}

void ArchiveMailRangeWidget::setRange(const QList<int> &hours)
{
    if (hours.count() != 2) {
        qCWarning(ARCHIVEMAILAGENT_LOG) << "Ranges is invalid " << hours;
    } else {
        mStartRange->setHour(hours.at(0));
        mEndRange->setHour(hours.at(1));
    }
}

#include "moc_archivemailrangewidget.cpp"
