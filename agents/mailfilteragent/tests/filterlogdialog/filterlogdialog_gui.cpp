/*
   SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "filterlogdialog.h"
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

    auto log = MailCommon::FilterLog::instance();
    log->setLogging(true);
    for (int i = 0; i < 50; ++i) {
        log->add(QStringLiteral("Test %1").arg(i), MailCommon::FilterLog::AppliedAction);
    }

    auto dialog = new FilterLogDialog(nullptr);
    dialog->exec();
    delete dialog;
    return 0;
}
