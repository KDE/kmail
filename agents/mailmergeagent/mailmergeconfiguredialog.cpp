/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeconfiguredialog.h"
#include "kmail-version.h"
#include "mailmergeconfigurewidget.h"
#include <KAboutData>
#include <KConfigGroup>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QApplication>
#include <QDialogButtonBox>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
static const char myConfigureMailMergeConfigureDialogGroupName[] = "MailMergeConfigureDialog";
}

MailMergeConfigureDialog::MailMergeConfigureDialog(QWidget *parent)
    : QDialog(parent)
    , mWidget(new MailMergeConfigureWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Configure"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    auto mainLayout = new QVBoxLayout(this);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MailMergeConfigureDialog::reject);

    mWidget->setObjectName(QStringLiteral("mailmergewidget"));
    mainLayout->addWidget(mWidget);
    mainLayout->addWidget(buttonBox);
    connect(okButton, &QPushButton::clicked, this, &MailMergeConfigureDialog::slotSave);

    readConfig();

    KAboutData aboutData = KAboutData(QStringLiteral("mailmergeagent"),
                                      i18n("Mail Merge Agent"),
                                      QStringLiteral(KDEPIM_VERSION),
                                      i18n("Merge email addresses agent."),
                                      KAboutLicense::GPL_V2,
                                      i18n("Copyright (C) 2021 Laurent Montel"));

    aboutData.addAuthor(i18n("Laurent Montel"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));
    aboutData.setProductName(QByteArrayLiteral("Akonadi/MailMergeAgent"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    auto helpMenu = new KHelpMenu(this, aboutData, true);
    // Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    buttonBox->button(QDialogButtonBox::Help)->setMenu(menu);
}

MailMergeConfigureDialog::~MailMergeConfigureDialog()
{
    writeConfig();
}

void MailMergeConfigureDialog::slotSave()
{
    // TODO
}

void MailMergeConfigureDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), myConfigureMailMergeConfigureDialogGroupName);
    const QSize sizeDialog = group.readEntry("Size", QSize(800, 600));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}

void MailMergeConfigureDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), myConfigureMailMergeConfigureDialogGroupName);
    group.writeEntry("Size", size());
}
