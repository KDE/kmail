/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailagentdbuswidget.h"

#include <Akonadi/Monitor>
#include <Akonadi/ServerManager>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>

MailAgentDbusWidget::MailAgentDbusWidget(QWidget *parent)
    : QWidget{parent}
{
    const auto service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_mailfilter_agent"));
    mMailFilterAgentInterface =
        new org::freedesktop::Akonadi::MailFilterAgent(service, QStringLiteral("/MailFilterAgent"), QDBusConnection::sessionBus(), this);

    auto mainLayout = new QVBoxLayout(this);
    auto openFilterDialog = new QPushButton(QStringLiteral("Open Filter Dialog"), this);
    connect(openFilterDialog, &QPushButton::clicked, this, [this]() {
        qDebug() << " open filter dialog";
        // TODO
    });
    mainLayout->addWidget(openFilterDialog);

    auto openfilterLogViewer = new QPushButton(QStringLiteral("Open Filter Log Viewer"), this);
    connect(openfilterLogViewer, &QPushButton::clicked, this, [this]() {
        qDebug() << " open filter log viewer";
        // TODO
    });
    mainLayout->addWidget(openfilterLogViewer);

    auto printCollectionMonitored = new QPushButton(QStringLiteral("Print Collection Monitored"), this);
    connect(printCollectionMonitored, &QPushButton::clicked, this, [this]() {
        qDebug() << " print collection monitored";
        // TODO
    });
    mainLayout->addWidget(printCollectionMonitored);
}

MailAgentDbusWidget::~MailAgentDbusWidget() = default;
