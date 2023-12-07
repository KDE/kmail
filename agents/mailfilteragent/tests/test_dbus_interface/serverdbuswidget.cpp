/*
   SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "serverdbuswidget.h"
#include "dbusadaptor.h"

#include <QDBusConnection>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ServerDbusWidget::ServerDbusWidget(QWidget *parent)
    : QWidget{parent}
    , mEdit(new QTextEdit(this))
{
    const QString path = QStringLiteral("/ServerDbusTest");
    auto mainLayout = new QHBoxLayout(this);

    mainLayout->addWidget(mEdit);

    new DbusAdaptor(this);

    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);

    const QString service = QStringLiteral("org.kde.server_dbus_test");

    QDBusConnection::sessionBus().registerService(service);
}

ServerDbusWidget::~ServerDbusWidget() = default;

QString ServerDbusWidget::debug()
{
    qDebug() << " DEBUG ***********";
    return QStringLiteral("DEBUGGING");
}

void ServerDbusWidget::sendElements(const QList<qint64> &items, int index)
{
    qDebug() << " sendElements " << items << " index " << index;
    // mEdit->append(QStringLiteral("items: %1    index: %2").arg(QString::items).arg(index));
}

void ServerDbusWidget::showDialog(qlonglong windowId)
{
    Q_UNUSED(windowId);
    qDebug() << " show dialog";
    QMessageBox::warning(this, QStringLiteral("TITLE"), QStringLiteral("MESSAGE"));
}
