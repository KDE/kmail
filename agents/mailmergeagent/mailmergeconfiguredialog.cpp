/*
   SPDX-FileCopyrightText: 2021-2026 Laurent Montel <montel@kde.org>

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
#include <KWindowConfig>
#include <QApplication>
#include <QDialogButtonBox>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>
#include <TextAddonsWidgets/LoadDialogSizeUtils>

using namespace Qt::Literals::StringLiterals;
namespace
{
const char myConfigureMailMergeConfigureDialogGroupName[] = "MailMergeConfigureDialog";
}

MailMergeConfigureDialog::MailMergeConfigureDialog(QWidget *parent)
    : QDialog(parent)
    , mWidget(new MailMergeConfigureWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Configure"));
    setWindowIcon(QIcon::fromTheme(u"kmail"_s));
    auto mainLayout = new QVBoxLayout(this);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MailMergeConfigureDialog::reject);

    mWidget->setObjectName("mailmergewidget"_L1);
    mainLayout->addWidget(mWidget);
    mainLayout->addWidget(buttonBox);
    connect(okButton, &QPushButton::clicked, this, &MailMergeConfigureDialog::slotSave);

    readConfig();

    KAboutData aboutData = KAboutData(u"mailmergeagent"_s,
                                      i18n("Mail Merge Agent"),
                                      QStringLiteral(KDEPIM_VERSION),
                                      i18n("Merge email addresses agent."),
                                      KAboutLicense::GPL_V2,
                                      i18n("© 2021–%1 Laurent Montel", u"2025"_s));

    aboutData.addAuthor(i18nc("@info:credit", "Laurent Montel"), i18n("Maintainer"), u"montel@kde.org"_s);
    aboutData.setProductName("Akonadi/MailMergeAgent"_ba);
    QApplication::setWindowIcon(QIcon::fromTheme(u"kmail"_s));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    auto helpMenu = new KHelpMenu(this, aboutData);
    helpMenu->setShowWhatsThis(true);
    // Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(u"kmail"_s));
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
    create(); // ensure a window is created
    TextAddonsWidgets::LoadDialogSizeUtils::loadDialogSizeScaled(this, QLatin1StringView(myConfigureMailMergeConfigureDialogGroupName), 800, 600);
}

void MailMergeConfigureDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myConfigureMailMergeConfigureDialogGroupName));
    KWindowConfig::saveWindowSize(windowHandle(), group);
    group.sync();
}

#include "moc_mailmergeconfiguredialog.cpp"
