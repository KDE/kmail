/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterinfoconfigwidget.h"
#include "sendlaterremovemessagejob.h"

#include "sendlaterconfigurewidget.h"

using namespace Qt::Literals::StringLiterals;

SendLaterInfoConfigWidget::SendLaterInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mWidget(new SendLaterWidget(parent))
{
    parent->layout()->addWidget(mWidget);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
}

SendLaterInfoConfigWidget::~SendLaterInfoConfigWidget() = default;

QList<Akonadi::Item::Id> SendLaterInfoConfigWidget::messagesToRemove() const
{
    return mWidget->messagesToRemove();
}

void SendLaterInfoConfigWidget::load()
{
    mWidget->load();
}

bool SendLaterInfoConfigWidget::save() const
{
    const QList<Akonadi::Item::Id> listMessage = mWidget->messagesToRemove();
    if (!listMessage.isEmpty()) {
        // Will delete in specific job when done.
        auto sendlaterremovejob = new SendLaterRemoveMessageJob(listMessage);
        sendlaterremovejob->start();
    }
    return mWidget->save();
}

void SendLaterInfoConfigWidget::slotNeedToReloadConfig()
{
    mWidget->needToReload();
}

#include "moc_sendlaterinfoconfigwidget.cpp"
