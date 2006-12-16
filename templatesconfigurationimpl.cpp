/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#include <klocale.h>
#include <kglobal.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qtoolbox.h>
#include <kdebug.h>
#include <qfont.h>

#include "templatesconfiguration.h"
#include "templatesconfiguration_kfg.h"
#include "globalsettings.h"

#include "templatesconfigurationimpl.h"

TemplatesConfiguration::TemplatesConfiguration(QWidget *parent, const char *name)
    :TemplatesConfigurationStub(parent, name)
{
    QFont f("Courier New");
    textEdit_new->setFont(f);
    textEdit_reply->setFont(f);
    textEdit_reply_all->setFont(f);
    textEdit_forward->setFont(f);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizeHint();

    connect( textEdit_new, SIGNAL( textChanged() ),
	    this, SLOT( slotTextChanged( void ) ) );
    connect( textEdit_reply, SIGNAL( textChanged() ),
	    this, SLOT( slotTextChanged( void ) ) );
    connect( textEdit_reply_all, SIGNAL( textChanged() ),
	    this, SLOT( slotTextChanged( void ) ) );
    connect( textEdit_forward, SIGNAL( textChanged() ),
	    this, SLOT( slotTextChanged( void ) ) );
    connect( lineEdit_quote, SIGNAL( textChanged( const QString & ) ),
	    this, SLOT( slotTextChanged( void ) ) );
};

void TemplatesConfiguration::slotTextChanged() {
    emit changed();
};

void TemplatesConfiguration::loadFromGlobal() {
    QString str;
    str = GlobalSettings::self()->templateNewMessage();
    if(str.isEmpty()) {
        textEdit_new->setText(defaultNewMessage());
    } else {
        textEdit_new->setText(str);
    };
    str = GlobalSettings::self()->templateReply();
    if(str.isEmpty()) {
        textEdit_reply->setText(defaultReply());
    } else {
        textEdit_reply->setText(str);
    };
    str = GlobalSettings::self()->templateReplyAll();
    if(str.isEmpty()) {
        textEdit_reply_all->setText(defaultReplyAll());
    } else {
        textEdit_reply_all->setText(str);
    };
    str = GlobalSettings::self()->templateForward();
    if(str.isEmpty()) {
        textEdit_forward->setText(defaultForward());
    } else {
        textEdit_forward->setText(str);
    };
    str = GlobalSettings::self()->quoteString();
    if(str.isEmpty()) {
        lineEdit_quote->setText(defaultQuoteString());
    } else {
        lineEdit_quote->setText(str);
    };
};

void TemplatesConfiguration::saveToGlobal() {
    GlobalSettings::self()->setTemplateNewMessage(textEdit_new->text());
    GlobalSettings::self()->setTemplateReply(textEdit_reply->text());
    GlobalSettings::self()->setTemplateReplyAll(textEdit_reply_all->text());
    GlobalSettings::self()->setTemplateForward(textEdit_forward->text());
    GlobalSettings::self()->setQuoteString(lineEdit_quote->text());
    GlobalSettings::self()->writeConfig();
};

void TemplatesConfiguration::loadFromFolder(QString id) {
    Templates t(id);

    QString str;

    str = t.templateNewMessage();
    if(str.isEmpty()) {
        str = GlobalSettings::self()->templateNewMessage();
    };
    if(str.isEmpty()) {
        str = defaultNewMessage();
    };
    textEdit_new->setText(str);

    str = t.templateReply();
    if(str.isEmpty()) {
        str = GlobalSettings::self()->templateReply();
    };
    if(str.isEmpty()) {
        str = defaultReply();
    };
    textEdit_reply->setText(str);

    str = t.templateReplyAll();
    if(str.isEmpty()) {
        str = GlobalSettings::self()->templateReplyAll();
    };
    if(str.isEmpty()) {
        str = defaultReplyAll();
    };
    textEdit_reply_all->setText(str);

    str = t.templateForward();
    if(str.isEmpty()) {
        str = GlobalSettings::self()->templateForward();
    };
    if(str.isEmpty()) {
        str = defaultForward();
    };
    textEdit_forward->setText(str);

    str = t.quoteString();
    if(str.isEmpty()) {
        str = GlobalSettings::self()->quoteString();
    };
    if(str.isEmpty()) {
        str = defaultQuoteString();
    };
    lineEdit_quote->setText(str);
};

void TemplatesConfiguration::saveToFolder(QString id) {
    Templates t(id);

    t.setTemplateNewMessage(textEdit_new->text());
    t.setTemplateReply(textEdit_reply->text());
    t.setTemplateReplyAll(textEdit_reply_all->text());
    t.setTemplateForward(textEdit_forward->text());
    t.setQuoteString(lineEdit_quote->text());
    t.writeConfig();
};

void TemplatesConfiguration::button_insert_clicked() {
    QPopupMenu popup(button_insert);

    QPopupMenu quote_popup(&popup);
    quote_popup.insertItem(i18n("Quoted message"), CQuote);
    quote_popup.insertItem(i18n("Message text as is"), CText);
    quote_popup.insertItem(i18n("Message id"), COMsgId);
    quote_popup.insertItem(i18n("Date"), CODate);
    quote_popup.insertItem(i18n("Date in short format"), CODateShort);
    quote_popup.insertItem(i18n("Date in C locale"), CODateEn);
    quote_popup.insertItem(i18n("Day of week"), CODow);
    quote_popup.insertItem(i18n("Time"), COTime);
    quote_popup.insertItem(i18n("Time in long format"), COTimeLong);
    quote_popup.insertItem(i18n("Time in C locale"), COTimeLongEn);
    quote_popup.insertItem(i18n("To field address"), COToAddr);
    quote_popup.insertItem(i18n("To field name"), COToName);
    quote_popup.insertItem(i18n("To field first name"), COToFName);
    quote_popup.insertItem(i18n("To field last name"), COToLName);
    quote_popup.insertItem(i18n("From field address"), COFromAddr);
    quote_popup.insertItem(i18n("From field name"), COFromName);
    quote_popup.insertItem(i18n("From field first name"), COFromFName);
    quote_popup.insertItem(i18n("From field last name"), COFromLName);
    quote_popup.insertItem(i18n("Subject"), COFullSubject);
    quote_popup.insertItem(i18n("Quoted headers"), CQHeaders);
    quote_popup.insertItem(i18n("Headers as is"), CHeaders);
    quote_popup.insertItem(i18n("Header content"), COHeader);
    popup.insertItem(i18n("Original message"), &quote_popup);

    QPopupMenu msg_popup(&popup);
    msg_popup.insertItem(i18n("Message id"), CMsgId);
    msg_popup.insertItem(i18n("Date"), CDate);
    msg_popup.insertItem(i18n("Date in short format"), CDateShort);
    msg_popup.insertItem(i18n("Date in C locale"), CDateEn);
    msg_popup.insertItem(i18n("Day of week"), CDow);
    msg_popup.insertItem(i18n("Time"), CTime);
    msg_popup.insertItem(i18n("Time in long format"), CTimeLong);
    msg_popup.insertItem(i18n("Time in C locale"), CTimeLongEn);
    msg_popup.insertItem(i18n("To field address"), CToAddr);
    msg_popup.insertItem(i18n("To field name"), CToName);
    msg_popup.insertItem(i18n("To field first name"), CToFName);
    msg_popup.insertItem(i18n("To field last name"), CToLName);
    msg_popup.insertItem(i18n("From field address"), CFromAddr);
    msg_popup.insertItem(i18n("From field name"), CFromName);
    msg_popup.insertItem(i18n("From field first name"), CFromFName);
    msg_popup.insertItem(i18n("From field last name"), CFromLName);
    msg_popup.insertItem(i18n("Subject"), CFullSubject);
    msg_popup.insertItem(i18n("Header content"), CHeader);
    popup.insertItem(i18n("Current message"), &msg_popup);

    QPopupMenu pipe_popup(&popup);
    pipe_popup.insertItem(i18n("Insert result of command"), CSystem);
    pipe_popup.insertItem(i18n("Pipe original message body and insert result as quoted text"), CQuotePipe);
    pipe_popup.insertItem(i18n("Pipe original message body and insert result as is"), CTextPipe);
    pipe_popup.insertItem(i18n("Pipe original message with headers and insert result as is"), CMsgPipe);
    pipe_popup.insertItem(i18n("Pipe current message body and insert result as is"), CBodyPipe);
    pipe_popup.insertItem(i18n("Pipe current message body and replace with result"), CClearPipe);
    popup.insertItem(i18n("Process with external programs"), &pipe_popup);

    QPopupMenu misc_popup(&popup);
    misc_popup.insertItem(i18n("Set cursor position"), CCursor);
    misc_popup.insertItem(i18n("Insert file content"), CInsert);
    misc_popup.insertItem(i18n("DNL"), CDnl);
    misc_popup.insertItem(i18n("Template comment"), CRem);
    misc_popup.insertItem(i18n("No operation"), CNop);
    misc_popup.insertItem(i18n("Clear generated message"), CClear);
    misc_popup.insertItem(i18n("Turn debug on"), CDebug);
    misc_popup.insertItem(i18n("Turn debug off"), CDebugOff);
    popup.insertItem(i18n("Miscellaneous"), &misc_popup);

    QSize ps = popup.sizeHint();
    int index = popup.exec(button_insert->mapToGlobal(QPoint(0,-(ps.height()))));

    // kdDebug() << "Template command index: " << index << endl;

    if(index > 0) {
        switch(index) {
            case CQuote: insertCommand("%QUOTE"); break;
            case CText: insertCommand("%TEXT"); break;
            case COMsgId: insertCommand("%OMSGID"); break;
            case CODate: insertCommand("%ODATE"); break;
            case CODateShort: insertCommand("%ODATESHORT"); break;
            case CODateEn: insertCommand("%ODATEEN"); break;
            case CODow: insertCommand("%ODOW"); break;
            case COTime: insertCommand("%OTIME"); break;
            case COTimeLong: insertCommand("%OTIMELONG"); break;
            case COTimeLongEn: insertCommand("%OTIMELONGEN"); break;
            case COToAddr: insertCommand("%OTOADDR"); break;
            case COToName: insertCommand("%OTONAME"); break;
            case COToFName: insertCommand("%OTOFNAME"); break;
            case COToLName: insertCommand("%OTOLNAME"); break;
            case COFromAddr: insertCommand("%OFROMADDR"); break;
            case COFromName: insertCommand("%OFROMNAME"); break;
            case COFromFName: insertCommand("%OFROMFNAME"); break;
            case COFromLName: insertCommand("%OFROMLNAME"); break;
            case COFullSubject: insertCommand("%OFULLSUBJECT"); break;
            case CQHeaders: insertCommand("%QHEADERS"); break;
            case CHeaders: insertCommand("%HEADERS"); break;
            case COHeader: insertCommand("%OHEADER=\"\"", -1); break;
            case CMsgId: insertCommand("%MSGID"); break;
            case CDate: insertCommand("%DATE"); break;
            case CDateShort: insertCommand("%DATESHORT"); break;
            case CDateEn: insertCommand("%DATEEN"); break;
            case CDow: insertCommand("%DOW"); break;
            case CTime: insertCommand("%TIME"); break;
            case CTimeLong: insertCommand("%TIMELONG"); break;
            case CTimeLongEn: insertCommand("%TIMELONGEN"); break;
            case CToAddr: insertCommand("%TOADDR"); break;
            case CToName: insertCommand("%TONAME"); break;
            case CToFName: insertCommand("%TOFNAME"); break;
            case CToLName: insertCommand("%TOLNAME"); break;
            case CFromAddr: insertCommand("%FROMADDR"); break;
            case CFromName: insertCommand("%FROMNAME"); break;
            case CFromFName: insertCommand("%FROMFNAME"); break;
            case CFromLName: insertCommand("%FROMLNAME"); break;
            case CFullSubject: insertCommand("%FULLSUBJECT"); break;
            case CHeader: insertCommand("%HEADER=\"\"", -1); break;
            case CSystem: insertCommand("%SYSTEM=\"\"", -1); break;
            case CQuotePipe: insertCommand("%QUOTEPIPE=\"\"", -1); break;
            case CTextPipe: insertCommand("%TEXTPIPE=\"\"", -1); break;
            case CMsgPipe: insertCommand("%MSGPIPE=\"\"", -1); break;
            case CBodyPipe: insertCommand("%BODYPIPE=\"\"", -1); break;
            case CClearPipe: insertCommand("%CLEARPIPE=\"\"", -1); break;
            case CCursor: insertCommand("%CURSOR"); break;
            case CInsert: insertCommand("%INSERT=\"\"", -1); break;
            case CDnl: insertCommand("%-"); break;
            case CRem: insertCommand("%REM=\"\"", -1); break;
            case CNop: insertCommand("%NOP"); break;
            case CClear: insertCommand("%CLEAR"); break;
            case CDebug: insertCommand("%DEBUG"); break;
            case CDebugOff: insertCommand("%DEBUGOFF"); break;
            default:
                kdDebug() << "Unknown template command index: " << index << endl;
                break;
        };
    };
};

void TemplatesConfiguration::insertCommand(QString cmd, int adjustCursor) {
    QTextEdit* edit;

    if(toolBox1->currentItem() == page_new) {
        edit = textEdit_new;
    } else if(toolBox1->currentItem() == page_reply) {
        edit = textEdit_reply;
    } else if(toolBox1->currentItem() == page_reply_all) {
        edit = textEdit_reply_all;
    } else if(toolBox1->currentItem() == page_forward) {
        edit = textEdit_forward;
    } else {
        kdDebug() << "Unknown current page in TemplatesConfiguration!" << endl;
        return;
    };

    // kdDebug() << "Insert command: " << cmd << endl;

    int para, index;
    edit->getCursorPosition(&para, &index);
    edit->insertAt(cmd, para, index);

    index += adjustCursor;

    edit->setCursorPosition(para, index + cmd.length());
};

QString TemplatesConfiguration::defaultNewMessage() {
    return i18n(
        "%REM=\"Default new message template\"%-\n"
        "%BLANK"
        );
};

QString TemplatesConfiguration::defaultReply() {
    return i18n(
        "%REM=\"Default reply template\"%-\n"
        "Hello, %OFROMFNAME!\n"
        "\n"
        "On %ODATEEN %OTIMELONGEN you wrote:\n"
        "%QUOTE\n"
        "%CURSOR\n"
        );
};

QString TemplatesConfiguration::defaultReplyAll() {
    return i18n(
        "%REM=\"Default reply all template\"%-\n"
        "Hello!\n"
        "\n"
        "On %ODATEEN %OTIMELONGEN %OFROMNAME wrote:\n"
        "%QUOTE\n"
        "%CURSOR\n"
        );
};

QString TemplatesConfiguration::defaultForward() {
    return i18n(
        "%REM=\"Default forward template\"%-\n"
        "This is a forwarded message:\n"
        "==========================================\n"
        "%TEXT\n"
        "=========================================="
        );
};

QString TemplatesConfiguration::defaultQuoteString() {
    return "> ";
};

#include "templatesconfigurationimpl.moc"
