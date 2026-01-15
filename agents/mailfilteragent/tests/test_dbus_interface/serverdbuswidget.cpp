/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "serverdbuswidget.h"
using namespace Qt::Literals::StringLiterals;

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
    mEdit->setReadOnly(true);

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
    QString str = QStringLiteral("index: %1").arg(QString::number(index));
    str += QStringLiteral("items: ");
    for (qint64 i : items) {
        str += QString::number(i) + u' ';
    }
    str += u'\n';
    mEdit->append(str);
}

void ServerDbusWidget::showDialog([[maybe_unused]] qlonglong windowId)
{
    qDebug() << " show dialog";
    QMessageBox::warning(this, QStringLiteral("TITLE"), QStringLiteral("MESSAGE"));
}

#include "moc_serverdbuswidget.cpp"
