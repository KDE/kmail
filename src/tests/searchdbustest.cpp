/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchdbustest.h"

#include <PimCommonAkonadi/MailUtil>
#include <QApplication>
#include <QDBusInterface>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>

searchdbustest::searchdbustest(QWidget *parent)
    : QWidget(parent)
{
    auto mainlayout = new QVBoxLayout(this);
    auto button = new QPushButton(QStringLiteral("reindex collections"), this);
    mainlayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &searchdbustest::slotReindexCollections);
}

void searchdbustest::slotReindexCollections()
{
    QDBusInterface interfaceAkonadiIndexer(PimCommon::MailUtil::indexerServiceName(), QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Indexer"));
    if (interfaceAkonadiIndexer.isValid()) {
        const QList<qlonglong> lst = {100, 300};
        qDebug() << "reindex " << lst;
        // qCDebug(KMAIL_LOG) << "Reindex collections :" << mCollectionsIndexed;
        interfaceAkonadiIndexer.asyncCall(QStringLiteral("reindexCollections"), QVariant::fromValue(lst));
    } else {
        qDebug() << " interface is not valid";
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    auto w = new searchdbustest;
    w->show();
    app.exec();
    delete w;
    return 0;
}
