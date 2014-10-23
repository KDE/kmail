/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "validatesendmailshortcut.h"

#include "libkdepim/widgets/pimmessagebox.h"
#include "settings/globalsettings.h"

#include <KMessageBox>
#include <KActionCollection>
#include <KLocalizedString>
#include <kaction.h>

ValidateSendMailShortcut::ValidateSendMailShortcut(KActionCollection *actionCollection, QWidget *parent)
    : mParent(parent),
      mActionCollection(actionCollection)
{
}

ValidateSendMailShortcut::~ValidateSendMailShortcut()
{

}

bool ValidateSendMailShortcut::validate()
{
    bool sendNow = false;
    const int result = PIMMessageBox::fourBtnMsgBox(mParent,
                                                    QMessageBox::Question,
                                                    i18n("This shortcut allows to send mail directly. Mail can be send accidentally. What do you want to do?"),
                                                    i18n("Configure shortcut"),
                                                    KGuiItem(i18n("Remove Shortcut")),
                                                    KGuiItem(i18n("Ask Before Sending")),
                                                    KGuiItem(i18n("Sending Without Confirmation")));
    if (result == KMessageBox::Yes) {
        QAction *act = mActionCollection->action( QLatin1String("send_mail") );
        if (act) {
            act->setShortcut(QKeySequence());
            mActionCollection->writeSettings();
        } else {
            qDebug()<<"Unable to find action named \"send_mail\"";
        }
        GlobalSettings::self()->setCheckSendDefaultActionShortcut(true);
        sendNow = false;
    } else if (result == KMessageBox::No) {
        GlobalSettings::self()->setConfirmBeforeSendWhenUseShortcut(true);
        GlobalSettings::self()->setCheckSendDefaultActionShortcut(true);
        sendNow = true;
    } else if (result == KMessageBox::Ok) {
        GlobalSettings::self()->setConfirmBeforeSendWhenUseShortcut(false);
        GlobalSettings::self()->setCheckSendDefaultActionShortcut(true);
        sendNow = true;
    } else if (result == KMessageBox::Cancel) {
        return false;
    }
    GlobalSettings::self()->writeConfig();
    return sendNow;
}
