/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

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
                                           listOfMailerFound)
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
