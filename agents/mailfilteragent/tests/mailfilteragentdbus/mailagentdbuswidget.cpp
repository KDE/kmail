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

    auto openfilterLogViewer = new QPushButton(QStringLiteral("Open Filter Log Viewer"), this);
    connect(openfilterLogViewer, &QPushButton::clicked, this, [this]() {
        qDebug() << " open filter log viewer";
        mMailFilterAgentInterface->showFilterLogDialog(0);
    });
    mainLayout->addWidget(openfilterLogViewer);

    auto printCollectionMonitored = new QPushButton(QStringLiteral("Print Collection Monitored"), this);
    connect(printCollectionMonitored, &QPushButton::clicked, this, [this]() {
        qDebug() << " print collection monitored";
        const QString str = mMailFilterAgentInterface->printCollectionMonitored();
        qDebug() << " result " << str;
    });
    mainLayout->addWidget(printCollectionMonitored);
    auto testFilterItems = new QPushButton(QStringLiteral("Test Filter Items"), this);
    connect(testFilterItems, &QPushButton::clicked, this, [this]() {
        qDebug() << " Test Filter Items";
        QList<qint64> itemIds;
        int set = 0;
        mMailFilterAgentInterface->filterItems(itemIds, static_cast<int>(set));
    });
    mainLayout->addWidget(testFilterItems);

    auto testFilterItem = new QPushButton(QStringLiteral("Test Filter Item"), this);
    connect(testFilterItem, &QPushButton::clicked, this, [this]() {
        qDebug() << " Test Filter Item";
        int set = 0;
        qlonglong item = 3;
        mMailFilterAgentInterface->filterItem(item, set, QStringLiteral("foo"));
    });
    mainLayout->addWidget(testFilterItem);
}

MailAgentDbusWidget::~MailAgentDbusWidget() = default;
