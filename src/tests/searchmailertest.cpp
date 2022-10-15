/*
   SPDX-FileCopyrightText: 2017-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KMessageBox>
#include <MailCommon/MailUtil>
#include <QApplication>
#include <kwidgetsaddons_version.h>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStringList listOfMailerFound = MailCommon::Util::foundMailer();
    if (!listOfMailerFound.isEmpty()) {
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        if (KMessageBox::questionTwoActionsList(nullptr,
#else
        if (KMessageBox::questionYesNoList(nullptr,

#endif
                                                QStringLiteral("Another mailer was found on system. Do you want to import data from it?"),
                                                listOfMailerFound,
                                                QString(),
                                                KGuiItem(QStringLiteral("Import"), QStringLiteral("document-import")),
                                                KGuiItem(QStringLiteral("Do Not Import"), QStringLiteral("dialog-cancel")))
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            == KMessageBox::ButtonCode::PrimaryAction) {
#else
            == KMessageBox::Yes) {
#endif
            qDebug() << " launch importwizard";
        } else {
            qDebug() << " no importing";
        }
    } else {
        qDebug() << "no mailer found";
    }

    return 0;
}
