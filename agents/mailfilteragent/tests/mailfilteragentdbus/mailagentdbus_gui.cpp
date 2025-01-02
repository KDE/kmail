/*
   SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailagentdbuswidget.h"
#include <MailCommon/FilterLog>
#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    auto mailagentdbuswidget = new MailAgentDbusWidget(nullptr);
    mailagentdbuswidget->show();
    return app.exec();
}
