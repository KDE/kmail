/*  -*- mode: C++; c-file-style: "gnu" -*- */
#ifndef KMAIL_OPTIONS_H
#define KMAIL_OPTIONS_H

#include <kcmdlineargs.h>
#include <klocale.h>

KCmdLineOptions kmail_options[] =
{
  { "s", 0 , 0 },
  { "subject <subject>",	I18N_NOOP("Set subject of message"), 0 },
  { "c", 0 , 0 },
  { "cc <address>",		I18N_NOOP("Send CC: to 'address'"), 0 },
  { "b", 0 , 0 },
  { "bcc <address>",		I18N_NOOP("Send BCC: to 'address'"), 0 },
  { "h", 0 , 0 },
  { "header <header>",		I18N_NOOP("Add 'header' to message"), 0 },
  { "msg <file>",		I18N_NOOP("Read message body from 'file'"), 0 },
  { "body <text>",              I18N_NOOP("Set body of message"), 0 },
  { "attach <url>",             I18N_NOOP("Add an attachment to the mail. This can be repeated"), 0 },
  { "check",			I18N_NOOP("Only check for new mail"), 0 },
  { "composer",			I18N_NOOP("Only open composer window"), 0 },
  { "view <url>",               I18N_NOOP("View the given message file" ), 0 },
  { "+[address|URL]",		I18N_NOOP("Send message to 'address' resp. "
                                          "attach the file the 'URL' points "
                                          "to"), 0 },
//  { "+[file]",                  I18N_NOOP("Show message from 'file'"), 0 },
  KCmdLineLastOption
};

#endif
