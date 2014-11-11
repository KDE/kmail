/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifndef KMAIL_OPTIONS_H
#define KMAIL_OPTIONS_H

#include <kcmdlineargs.h>
#include <KLocalizedString>

static KCmdLineOptions kmail_options()
{
    KCmdLineOptions options;
    options.add("s");
    options.add("subject <subject>",        ki18n("Set subject of message"));
    options.add("c");
    options.add("cc <address>",                ki18n("Send CC: to 'address'"));
    options.add("b");
    options.add("bcc <address>",                ki18n("Send BCC: to 'address'"));
    options.add("h");
    options.add("replyTo <address>",                ki18n("Set replyTo to 'address'"));
    options.add("header <header_name:header_value>",        ki18n("Add 'header' to message. This can be repeated"));
    options.add("msg <file>",                ki18n("Read message body from 'file'"));
    options.add("body <text>",                ki18n("Set body of message"));
    options.add("attach <url>",                ki18n("Add an attachment to the mail. This can be repeated"));
    options.add("check",                        ki18n("Only check for new mail"));
    options.add("composer",                ki18n("Only open composer window"));
    options.add("view <url>",                ki18n("View the given message file"));
    options.add("+[address|URL]",                ki18n("Send message to 'address' or "
                "attach the file the 'URL' points "
                "to"));
    return options;
}

#endif
