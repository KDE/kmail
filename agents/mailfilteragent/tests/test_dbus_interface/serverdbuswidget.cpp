/*
   SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

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
    const QString path = u"/ServerDbusTest"_s;
    auto mainLayout = new QHBoxLayout(this);

    mainLayout->addWidget(mEdit);
    mEdit->setReadOnly(true);

    new DbusAdaptor(this);

    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);

    const QString service = u"org.kde.server_dbus_test"_s;

    QDBusConnection::sessionBus().registerService(service);
}

ServerDbusWidget::~ServerDbusWidget() = default;

QString ServerDbusWidget::debug()
{
    qDebug() << " DEBUG ***********";
    return u"DEBUGGING"_s;
}

void ServerDbusWidget::sendElements(const QList<qint64> &items, int index)
{
    qDebug() << " sendElements " << items << " index " << index;
    QString str = u"index: %1"_s.arg(QString::number(index));
    str += u"items: "_s;
    for (qint64 i : items) {
        str += QString::number(i) + u' ';
    }
    str += u'\n';
    mEdit->append(str);
}

void ServerDbusWidget::showDialog([[maybe_unused]] qlonglong windowId)
{
    qDebug() << " show dialog";
    QMessageBox::warning(this, u"TITLE"_s, u"MESSAGE"_s);
}

#include "moc_serverdbuswidget.cpp"
