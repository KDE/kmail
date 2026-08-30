/*
   SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clientdbuswidget.h"

#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
using namespace Qt::Literals::StringLiterals;

ClientDbusWidget::ClientDbusWidget(QWidget *parent)
    : QWidget{parent}
{
    const QString path = u"/ServerDbusTest"_s;
    const QString service = u"org.kde.server_dbus_test"_s;

    mDbusInterface = new OrgFreedesktopTestDbusInterface(service, path, QDBusConnection::sessionBus(), this);

    auto mainLayout = new QVBoxLayout(this);

    auto debugPushButton = new QPushButton(u"Debug"_s, this);
    connect(debugPushButton, &QPushButton::clicked, this, [this]() {
        qDebug() << " debug";
        const QString str = mDbusInterface->debug();
        qDebug() << " result " << str;
    });
    mainLayout->addWidget(debugPushButton);
    auto testSendElements = new QPushButton(u"Send Elements"_s, this);
    connect(testSendElements, &QPushButton::clicked, this, [this]() {
        qDebug() << " Test Send Elements";
        QList<qint64> itemIds{10, 20, 50};
        int set = 5;
        mDbusInterface->sendElements(itemIds, static_cast<int>(set));
    });
    mainLayout->addWidget(testSendElements);

    auto testShowDialog = new QPushButton(u"Test Show Dialog"_s, this);
    connect(testShowDialog, &QPushButton::clicked, this, [this]() {
        qDebug() << " Show Dialog";
        mDbusInterface->showDialog(0);
    });
    mainLayout->addWidget(testShowDialog);
}

ClientDbusWidget::~ClientDbusWidget() = default;

#include "moc_clientdbuswidget.cpp"
