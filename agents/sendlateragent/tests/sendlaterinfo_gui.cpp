/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>

#include "sendlaterdialog.h"
#include <MessageComposer/SendLaterDialog>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);
    auto dialog = new MessageComposer::SendLaterDialog(nullptr);
    dialog->exec();
    delete dialog;
    return 0;
}
