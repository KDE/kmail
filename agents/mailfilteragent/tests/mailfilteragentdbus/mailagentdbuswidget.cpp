/*
   SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailagentdbuswidget.h"

#include <Akonadi/Monitor>
#include <Akonadi/ServerManager>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
using namespace Qt::Literals::StringLiterals;

MailAgentDbusWidget::MailAgentDbusWidget(QWidget *parent)
    : QWidget{parent}
{
    const auto service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent, u"akonadi_mailfilter_agent"_s);
    mMailFilterAgentInterface = new org::freedesktop::Akonadi::MailFilterAgent(service, u"/MailFilterAgent"_s, QDBusConnection::sessionBus(), this);

    auto mainLayout = new QVBoxLayout(this);

    auto openfilterLogViewer = new QPushButton(u"Open Filter Log Viewer"_s, this);
    connect(openfilterLogViewer, &QPushButton::clicked, this, [this]() {
        qDebug() << " open filter log viewer";
        mMailFilterAgentInterface->showFilterLogDialog(0);
    });
    mainLayout->addWidget(openfilterLogViewer);

    auto printCollectionMonitored = new QPushButton(u"Print Collection Monitored"_s, this);
    connect(printCollectionMonitored, &QPushButton::clicked, this, [this]() {
        qDebug() << " print collection monitored";
        const QString str = mMailFilterAgentInterface->printCollectionMonitored();
        qDebug() << " result " << str;
    });
    mainLayout->addWidget(printCollectionMonitored);
    auto testFilterItems = new QPushButton(u"Test Filter Items"_s, this);
    connect(testFilterItems, &QPushButton::clicked, this, [this]() {
        qDebug() << " Test Filter Items";
        QList<qint64> itemIds;
        int set = 0;
        mMailFilterAgentInterface->filterItems(itemIds, static_cast<int>(set));
    });
    mainLayout->addWidget(testFilterItems);

    auto testFilterItem = new QPushButton(u"Test Filter Item"_s, this);
    connect(testFilterItem, &QPushButton::clicked, this, [this]() {
        qDebug() << " Test Filter Item";
        int set = 0;
        qlonglong item = 3;
        mMailFilterAgentInterface->filterItem(item, set, u"foo"_s);
    });
    mainLayout->addWidget(testFilterItem);
}

MailAgentDbusWidget::~MailAgentDbusWidget() = default;

#include "moc_mailagentdbuswidget.cpp"
