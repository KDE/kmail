/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2016-2026 Laurent Montel <montel@kde.org>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "config-kmail.h"
#include <KLocalizedString>
#include <QCommandLineParser>
using namespace Qt::Literals::StringLiterals;

static void kmail_options(QCommandLineParser *parser)
{
    QList<QCommandLineOption> options;

    options << QCommandLineOption(QStringList() << u"s"_s << u"subject"_s, i18nc("@info:shell", "Set subject of message"), i18n("Subject"))
            << QCommandLineOption(QStringList() << u"c"_s << u"cc"_s, i18n("Send CC: to 'address'. This can be repeated"), i18n("Address"))
            << QCommandLineOption(QStringList() << u"b"_s << u"bcc"_s, i18n("Send BCC: to 'address'. This can be repeated"), i18n("Address"))
            << QCommandLineOption(QStringList() << u"r"_s << u"replyTo"_s, i18n("Set replyTo to 'address'"), i18n("Address"))
            << QCommandLineOption(u"header"_s, i18nc("@info:shell", "Add 'header' to message. This can be repeated"), u"header_name:header_value"_s)
            << QCommandLineOption(u"msg"_s, i18nc("@info:shell", "Read message body from 'file'"), i18n("File"))
            << QCommandLineOption(u"body"_s, i18nc("@info:shell", "Set body of message"), i18n("Text"))
            << QCommandLineOption(u"attach"_s, i18nc("@info:shell", "Add an attachment to the mail. This can be repeated"), i18n("Attachment Url"))
            << QCommandLineOption(u"check"_s, i18nc("@info:shell", "Only check for new mail"))
            << QCommandLineOption(u"startintray"_s, i18nc("@info:shell", "Start minimized to tray"))
            << QCommandLineOption(u"composer"_s, i18nc("@info:shell", "Only open composer window"))
            << QCommandLineOption(u"identity"_s, i18nc("@info:shell", "Set identity name"), i18n("Identity"))
            << QCommandLineOption(u"view"_s, i18nc("@info:shell", "View the given message file"), i18n("Message Url"))
            << QCommandLineOption(u"html"_s, i18nc("@info:shell", "Set body of message as html"), i18n("Body Message"));
#if KMAIL_WITH_KUSERFEEDBACK
    parser->addOption(QCommandLineOption(u"feedback"_s, i18nc("@info:shell", "Lists the available options for user feedback")));
#endif
    parser->addOption(QCommandLineOption(u"debug"_s, i18nc("@info:shell", "Activate Debug Mode")));

    parser->addOptions(options);
    parser->addPositionalArgument(u"address"_s, i18nc("@info:shell", "Send message to 'address' or attach the file the 'URL' points to"), u"address|URL"_s);
}
