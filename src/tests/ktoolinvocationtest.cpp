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

#include <qdebug.h>

#include <QCoreApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <KToolInvocation>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    qDebug() << "Test kinvocation by desktop name.";

    KAboutData aboutData(QStringLiteral("testKInvocation"), i18n("Test for KMail invocation"), QStringLiteral("0.0"));
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QString errmsg;
    if (KToolInvocation::startServiceByDesktopName(QStringLiteral("org.kde.kmail"), QString(), &errmsg)) {
        qDebug() << " Can not start kmail" << errmsg;
    }

    const QString desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("org.kde.korganizer.desktop"));
    if (KToolInvocation::startServiceByDesktopPath(desktopFile) > 0) {
        qDebug() << " Can not start korganizer";
    }

    qDebug() << "kinvocation done.";

    return 0;
}
