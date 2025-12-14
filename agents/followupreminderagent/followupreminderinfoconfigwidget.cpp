/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "followupreminderinfoconfigwidget.h"
#include "followupreminderinfowidget.h"
#include "kmail-version.h"
#include <KAboutData>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QLayout>
namespace
{
const char myConfigGroupName[] = "FollowUpReminderInfoDialog";
}

FollowUpReminderInfoConfigWidget::FollowUpReminderInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mWidget(new FollowUpReminderInfoWidget(parent))
{
    parent->layout()->addWidget(mWidget);
}

FollowUpReminderInfoConfigWidget::~FollowUpReminderInfoConfigWidget() = default;

void FollowUpReminderInfoConfigWidget::load()
{
    mWidget->load();
}

bool FollowUpReminderInfoConfigWidget::save() const
{
    return mWidget->save();
}

#include "moc_followupreminderinfoconfigwidget.cpp"
