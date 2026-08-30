/*
   SPDX-FileCopyrightText: 2017-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KMessageBox>
#include <MailCommon/MailUtil>
#include <QApplication>
using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (QStringList listOfMailerFound = MailCommon::Util::foundMailer(); !listOfMailerFound.isEmpty()) {
        if (KMessageBox::questionTwoActionsList(nullptr,
                                                u"Another mailer was found on system. Do you want to import data from it?"_s,
                                                listOfMailerFound,
                                                QString(),
                                                KGuiItem(u"Import"_s, u"document-import"_s),
                                                KGuiItem(u"Do Not Import"_s, u"dialog-cancel"_s))
            == KMessageBox::ButtonCode::PrimaryAction) {
            qDebug() << " launch importwizard";
        } else {
            qDebug() << " no importing";
        }
    } else {
        qDebug() << "no mailer found";
    }

    return 0;
}
