/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeselectdatetimedialog.h"
#include "snoozemenu.h"
#include <KLocalizedString>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;

SnoozeSelectDateTimeDialog::SnoozeSelectDateTimeDialog(QWidget *parent)
    : QDialog(parent)
    , mDateTimeEdit(new QDateTimeEdit(this))
{
    setWindowTitle(i18nc("@title:window", "Snooze Until"));
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName(u"mainLayout"_s);

    auto formLayout = new QFormLayout;
    formLayout->setObjectName(u"formLayout"_s);
    formLayout->setContentsMargins({});
    mainLayout->addLayout(formLayout);

    const QDateTime now = QDateTime::currentDateTime();
    mDateTimeEdit->setObjectName(u"mDateTimeEdit"_s);
    mDateTimeEdit->setCalendarPopup(true);
    mDateTimeEdit->setMinimumDateTime(now);
    mDateTimeEdit->setDateTime(SnoozeMenu::wakeUpDateTime(SnoozeMenu::SnoozeType::Tomorrow, now));
    formLayout->addRow(i18nc("@label:textbox", "Wake up on:"), mDateTimeEdit);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setObjectName(u"buttonBox"_s);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SnoozeSelectDateTimeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SnoozeSelectDateTimeDialog::reject);
    mainLayout->addWidget(buttonBox);
}

SnoozeSelectDateTimeDialog::~SnoozeSelectDateTimeDialog() = default;

QDateTime SnoozeSelectDateTimeDialog::wakeUpDateTime() const
{
    return mDateTimeEdit->dateTime();
}

#include "moc_snoozeselectdatetimedialog.cpp"
