/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "searchdbustest.h"

#include <QApplication>
#include <QDBusInterface>
#include <QPushButton>
#include <QVBoxLayout>
#include <PimCommon/PimUtil>
#include <QDebug>

searchdbustest::searchdbustest(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    QPushButton *button = new QPushButton(QStringLiteral("reindex collections"), this);
    mainlayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, &searchdbustest::slotReindexCollections);
}

void searchdbustest::slotReindexCollections()
{
    QDBusInterface interfaceBalooIndexer(PimCommon::Util::indexerServiceName(), QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Indexer"));
    if (interfaceBalooIndexer.isValid()) {
        const QList<qlonglong> lst = {100,300};
        qDebug() << "reindex "<< lst;
        //qCDebug(KMAIL_LOG) << "Reindex collections :" << mCollectionsIndexed;
        interfaceBalooIndexer.call(QStringLiteral("reindexCollections"), QVariant::fromValue(lst));
    } else {
        qDebug()<<" interface is not valid";
    }

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    searchdbustest *w = new searchdbustest;
    w->show();
    app.exec();
    delete w;
    return 0;
}
