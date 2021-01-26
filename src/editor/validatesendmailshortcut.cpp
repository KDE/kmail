/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "validatesendmailshortcut.h"
#include "kmail_debug.h"
#include "pimmessagebox.h"
#include "settings/kmailsettings.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <QAction>

ValidateSendMailShortcut::ValidateSendMailShortcut(KActionCollection *actionCollection, QWidget *parent)
    : mParent(parent)
    , mActionCollection(actionCollection)
{
}

ValidateSendMailShortcut::~ValidateSendMailShortcut() = default;

bool ValidateSendMailShortcut::validate()
{
    bool sendNow = false;
    const int result = PIMMessageBox::fourBtnMsgBox(mParent,
                                                    QMessageBox::Question,
                                                    i18n("This shortcut allows to send mail directly. Mail can be send accidentally. What do you want to do?"),
                                                    i18n("Configure shortcut"),
                                                    i18n("Remove Shortcut"),
                                                    i18n("Ask Before Sending"),
                                                    i18n("Sending Without Confirmation"));
    if (result == QDialogButtonBox::Yes) {
        QAction *act = mActionCollection->action(QStringLiteral("send_mail"));
        if (act) {
            act->setShortcut(QKeySequence());
            mActionCollection->writeSettings();
        } else {
            qCDebug(KMAIL_LOG) << "Unable to find action named \"send_mail\"";
        }
        sendNow = false;
    } else if (result == QDialogButtonBox::No) {
        KMailSettings::self()->setConfirmBeforeSendWhenUseShortcut(true);
        sendNow = true;
    } else if (result == QDialogButtonBox::Ok) {
        KMailSettings::self()->setConfirmBeforeSendWhenUseShortcut(false);
        sendNow = true;
    } else if (result == QDialogButtonBox::Cancel) {
        return false;
    }
    KMailSettings::self()->setCheckSendDefaultActionShortcut(true);
    KMailSettings::self()->save();
    return sendNow;
}
