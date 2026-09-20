/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeinfoconfigwidget.h"
#include "snoozeconfigurewidget.h"

#include <QApplication>
#include <QIcon>
#include <QLayout>

using namespace Qt::Literals::StringLiterals;

SnoozeInfoConfigWidget::SnoozeInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mWidget(new SnoozeWidget(parent))
{
    parent->layout()->addWidget(mWidget);
    QApplication::setWindowIcon(QIcon::fromTheme(u"kmail"_s));
}

SnoozeInfoConfigWidget::~SnoozeInfoConfigWidget() = default;

QList<Akonadi::Item::Id> SnoozeInfoConfigWidget::messagesToUnsnooze() const
{
    return mWidget->messagesToUnsnooze();
}

void SnoozeInfoConfigWidget::load()
{
    mWidget->load();
}

bool SnoozeInfoConfigWidget::save() const
{
    // TODO tell the agent over D-Bus about the messages the user unsnoozed.
    return mWidget->save();
}

void SnoozeInfoConfigWidget::slotNeedToReloadConfig()
{
    mWidget->needToReload();
}

#include "moc_snoozeinfoconfigwidget.cpp"
