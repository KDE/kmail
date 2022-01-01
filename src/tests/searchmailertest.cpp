/*
   SPDX-FileCopyrightText: 2017-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KMessageBox>
#include <MailCommon/MailUtil>
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStringList listOfMailerFound = MailCommon::Util::foundMailer();
    if (!listOfMailerFound.isEmpty()) {
        if (KMessageBox::questionYesNoList(nullptr,
                                           QStringLiteral("Another mailer was found on system. Do you want to import data from it?"),
                                           listOfMailerFound,
                                           QString(),
                                           KGuiItem(QStringLiteral("Import"), QStringLiteral("document-import")),
                                           KGuiItem(QStringLiteral("Do Not Import"), QStringLiteral("dialog-cancel")))
            == KMessageBox::Yes) {
            qDebug() << " launch importwizard";
        } else {
            qDebug() << " no importing";
        }
    } else {
        qDebug() << "no mailer found";
    }

    return 0;
}
