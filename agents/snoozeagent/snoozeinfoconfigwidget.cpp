/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeinfoconfigwidget.h"
#include "snoozeconfigurewidget.h"
#include "snoozeutil.h"

#include <QApplication>
#include <QDBusConnection>
#include <QIcon>
#include <QLayout>

using namespace Qt::Literals::StringLiterals;

SnoozeInfoConfigWidget::SnoozeInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mWidget(new SnoozeWidget(parent))
{
    parent->layout()->addWidget(mWidget);
    QApplication::setWindowIcon(QIcon::fromTheme(u"kmail"_s));

    connect(mWidget, &SnoozeWidget::wakeUpNow, this, &Snooze::SnoozeUtil::wakeUpNow);
    // The agent runs in another process: follow its changes over D-Bus.
    QDBusConnection::sessionBus().connect(QString(),
                                          Snooze::SnoozeUtil::dbusPath(),
                                          Snooze::SnoozeUtil::dbusInterface(),
                                          u"needUpdateConfigDialogBox"_s,
                                          this,
                                          SLOT(slotNeedToReloadConfig()));
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
    // The agent drops the attribute, the link and its config entry itself.
    Snooze::SnoozeUtil::cancelSnooze(mWidget->messagesToUnsnooze());
    return false;
}

void SnoozeInfoConfigWidget::slotNeedToReloadConfig()
{
    mWidget->needToReload();
}

#include "moc_snoozeinfoconfigwidget.cpp"
