/*
   SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchdbustest.h"

#include <PimCommonAkonadi/MailUtil>
#include <QApplication>
#include <QDBusInterface>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
using namespace Qt::Literals::StringLiterals;

searchdbustest::searchdbustest(QWidget *parent)
    : QWidget(parent)
{
    auto mainlayout = new QVBoxLayout(this);
    auto button = new QPushButton(u"reindex collections"_s, this);
    mainlayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &searchdbustest::slotReindexCollections);
}

void searchdbustest::slotReindexCollections()
{
    if (QDBusInterface interfaceAkonadiIndexer(PimCommon::MailUtil::indexerServiceName(), u"/"_s, u"org.freedesktop.Akonadi.Indexer"_s);
        interfaceAkonadiIndexer.isValid()) {
        const QList<qlonglong> lst = {100, 300};
        qDebug() << "reindex " << lst;
        // qCDebug(KMAIL_LOG) << "Reindex collections :" << mCollectionsIndexed;
        interfaceAkonadiIndexer.asyncCall(u"reindexCollections"_s, QVariant::fromValue(lst));
    } else {
        qDebug() << " interface is not valid";
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    searchdbustest w;
    w.show();
    app.exec();
    return 0;
}

#include "moc_searchdbustest.cpp"
