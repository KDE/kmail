/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifndef KMAIL_OPTIONS_H
#define KMAIL_OPTIONS_H

#include <QCommandLineParser>
#include <KLocalizedString>

static void kmail_options(QCommandLineParser *parser)
{
    QList<QCommandLineOption> options;

    options << QCommandLineOption(
          QStringList() << QLatin1String("s") << QLatin1String("subject"),
          i18n("Set subject of message"), QLatin1String("subject"))
    << QCommandLineOption(
          QStringList() << QLatin1String("c") << QLatin1String("cc"),
          i18n("Send CC: to 'address'"), QLatin1String("address"))
    << QCommandLineOption(
          QStringList() << QLatin1String("b") << QLatin1String("bcc"),
          i18n("Send BCC: to 'address'"), QLatin1String("address"))
    << QCommandLineOption(
          QStringList() << QLatin1String("h") << QLatin1String("replyTo"),
          i18n("Set replyTo to 'address'"), QLatin1String("address"))
    << QCommandLineOption(
          QLatin1String("header"),
          i18n("Add 'header' to message. This can be repeated"),
          QLatin1String("header_name:header_value"))
    << QCommandLineOption(
          QLatin1String("msg"),
          i18n("Read message body from 'file'"),
          QLatin1String("file"))
    << QCommandLineOption(
          QLatin1String("body"),
          i18n("Set body of message"),
          QLatin1String("text"))
    << QCommandLineOption(
          QLatin1String("attach"),
          i18n("Add an attachment to the mail. This can be repeated"),
          QLatin1String("url"))
    << QCommandLineOption(
          QLatin1String("check"),
          i18n("Only check for new mail"))
    << QCommandLineOption(
          QLatin1String("composer"),
          i18n("Only open composer window"))
    << QCommandLineOption(
          QLatin1String("view"),
          i18n("View the given message file"),
          QLatin1String("url"));

    parser->addOptions(options);
    parser->addPositionalArgument(
          QLatin1String("address"),
          i18n("Send message to 'address' or attach the file the 'URL' points to"),
          QLatin1String("address|URL"));
}

#endif
