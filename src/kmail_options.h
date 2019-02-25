/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifndef KMAIL_OPTIONS_H
#define KMAIL_OPTIONS_H

#include <QCommandLineParser>
#include <KLocalizedString>

static void kmail_options(QCommandLineParser *parser)
{
    QList<QCommandLineOption> options;

    options << QCommandLineOption(
        QStringList() << QStringLiteral("s") << QStringLiteral("subject"),
        i18n("Set subject of message"),
        QStringLiteral("subject"))
            << QCommandLineOption(
        QStringList() << QStringLiteral("c") << QStringLiteral("cc"),
        i18n("Send CC: to 'address'"),
        QStringLiteral("address"))
            << QCommandLineOption(
        QStringList() << QStringLiteral("b") << QStringLiteral("bcc"),
        i18n("Send BCC: to 'address'"),
        QStringLiteral("address"))
            << QCommandLineOption(
        QStringList() << QStringLiteral("h") << QStringLiteral("replyTo"),
        i18n("Set replyTo to 'address'"),
        QStringLiteral("address"))
            << QCommandLineOption(
        QStringLiteral("header"),
        i18n("Add 'header' to message. This can be repeated"),
        QStringLiteral("header_name:header_value"))
            << QCommandLineOption(
        QStringLiteral("msg"),
        i18n("Read message body from 'file'"),
        QStringLiteral("file"))
            << QCommandLineOption(
        QStringLiteral("body"),
        i18n("Set body of message"),
        QStringLiteral("text"))
            << QCommandLineOption(
        QStringLiteral("attach"),
        i18n("Add an attachment to the mail. This can be repeated"),
        QStringLiteral("url"))
            << QCommandLineOption(
        QStringLiteral("check"),
        i18n("Only check for new mail"))
            << QCommandLineOption(
        QStringLiteral("startintray"),
        i18n("Start minimized to tray"))
            << QCommandLineOption(
        QStringLiteral("composer"),
        i18n("Only open composer window"))
            << QCommandLineOption(
        QStringLiteral("identity"),
        i18n("Set identity"),
        QStringLiteral("identity"))
            << QCommandLineOption(
        QStringLiteral("view"),
        i18n("View the given message file"),
        QStringLiteral("url"));

    parser->addOptions(options);
    parser->addPositionalArgument(
        QStringLiteral("address"),
        i18n("Send message to 'address' or attach the file the 'URL' points to"),
        QStringLiteral("address|URL"));
}

#endif
