/*
   SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDebug>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <KToolInvocation>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    qDebug() << "Test kinvocation by desktop name.";

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    QString errmsg;
    if (KToolInvocation::startServiceByDesktopName(QStringLiteral("org.kde.kmail2"), QString(), &errmsg)) {
        qDebug() << " Can not start kmail" << errmsg;
    }

    const QString desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("org.kde.korganizer.desktop"));
    if (KToolInvocation::startServiceByDesktopPath(desktopFile) > 0) {
        qDebug() << " Can not start korganizer";
    }

    qDebug() << "kinvocation done.";

    return 0;
}
