/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2016-2024 Laurent Montel <montel@kde.org>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <KLocalizedString>
#include <QCommandLineParser>

static void kmail_options(QCommandLineParser *parser)
{
    QList<QCommandLineOption> options;

    options << QCommandLineOption(QStringList() << QStringLiteral("s") << QStringLiteral("subject"), i18n("Set subject of message"), QStringLiteral("subject"))
            << QCommandLineOption(QStringList() << QStringLiteral("c") << QStringLiteral("cc"),
                                  i18n("Send CC: to 'address'. This can be repeated"),
                                  QStringLiteral("address"))
            << QCommandLineOption(QStringList() << QStringLiteral("b") << QStringLiteral("bcc"),
                                  i18n("Send BCC: to 'address'. This can be repeated"),
                                  QStringLiteral("address"))
            << QCommandLineOption(QStringList() << QStringLiteral("r") << QStringLiteral("replyTo"),
                                  i18n("Set replyTo to 'address'"),
                                  QStringLiteral("address"))
            << QCommandLineOption(QStringLiteral("header"),
                                  i18nc("@info:shell", "Add 'header' to message. This can be repeated"),
                                  QStringLiteral("header_name:header_value"))
            << QCommandLineOption(QStringLiteral("msg"), i18nc("@info:shell", "Read message body from 'file'"), QStringLiteral("file"))
            << QCommandLineOption(QStringLiteral("body"), i18nc("@info:shell", "Set body of message"), QStringLiteral("text"))
            << QCommandLineOption(QStringLiteral("attach"), i18nc("@info:shell", "Add an attachment to the mail. This can be repeated"), QStringLiteral("url"))
            << QCommandLineOption(QStringLiteral("check"), i18nc("@info:shell", "Only check for new mail"))
            << QCommandLineOption(QStringLiteral("startintray"), i18nc("@info:shell", "Start minimized to tray"))
            << QCommandLineOption(QStringLiteral("composer"), i18nc("@info:shell", "Only open composer window"))
            << QCommandLineOption(QStringLiteral("identity"), i18nc("@info:shell", "Set identity name"), QStringLiteral("identity"))
            << QCommandLineOption(QStringLiteral("view"), i18nc("@info:shell", "View the given message file"), QStringLiteral("url"));
#ifdef WITH_KUSERFEEDBACK
    parser->addOption(QCommandLineOption(QStringLiteral("feedback"), i18nc("@info:shell", "Lists the available options for user feedback")));
#endif

    parser->addOptions(options);
    parser->addPositionalArgument(QStringLiteral("address"),
                                  i18nc("@info:shell", "Send message to 'address' or attach the file the 'URL' points to"),
                                  QStringLiteral("address|URL"));
}
