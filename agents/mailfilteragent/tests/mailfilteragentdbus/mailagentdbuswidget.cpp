/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailagentdbuswidget.h"
#include <QPushButton>
#include <QVBoxLayout>

MailAgentDbusWidget::MailAgentDbusWidget(QWidget *parent)
    : QWidget{parent}
{
    auto mainLayout = new QVBoxLayout(this);
    auto openFilterDialog = new QPushButton(QStringLiteral("Open Filter Dialog"), this);
    connect(openFilterDialog, &QPushButton::clicked, this, [this]() {
        // TODO
    });
    mainLayout->addWidget(openFilterDialog);

    auto openfilterLogViewer = new QPushButton(QStringLiteral("Open Filter Log Viewer"), this);
    connect(openfilterLogViewer, &QPushButton::clicked, this, [this]() {
        // TODO
    });
    mainLayout->addWidget(openfilterLogViewer);

    auto printCollectionMonitored = new QPushButton(QStringLiteral("Print Collection Monitored"), this);
    connect(printCollectionMonitored, &QPushButton::clicked, this, [this]() {
        // TODO
    });
    mainLayout->addWidget(printCollectionMonitored);
}

MailAgentDbusWidget::~MailAgentDbusWidget() = default;
