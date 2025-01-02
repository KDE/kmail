/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeconfiguredialog.h"
using namespace Qt::Literals::StringLiterals;

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

    mWidget->setObjectName("mailmergewidget"_L1);
    mainLayout->addWidget(mWidget);
    mainLayout->addWidget(buttonBox);
    connect(okButton, &QPushButton::clicked, this, &MailMergeConfigureDialog::slotSave);

    readConfig();

    KAboutData aboutData = KAboutData(QStringLiteral("mailmergeagent"),
                                      i18n("Mail Merge Agent"),
                                      QStringLiteral(KDEPIM_VERSION),
                                      i18n("Merge email addresses agent."),
                                      KAboutLicense::GPL_V2,
                                      i18n("Copyright (C) 2021-%1 Laurent Montel", QStringLiteral("2025")));

    aboutData.addAuthor(i18nc("@info:credit", "Laurent Montel"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));
    aboutData.setProductName(QByteArrayLiteral("Akonadi/MailMergeAgent"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

#if KXMLGUI_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    auto helpMenu = new KHelpMenu(this, aboutData);
    helpMenu->setShowWhatsThis(true);
#else
    auto helpMenu = new KHelpMenu(this, aboutData, true);
#endif
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
    create(); // ensure a window is created
    windowHandle()->resize(QSize(800, 600));
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myConfigureMailMergeConfigureDialogGroupName));
    KWindowConfig::restoreWindowSize(windowHandle(), group);
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

void MailMergeConfigureDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myConfigureMailMergeConfigureDialogGroupName));
    KWindowConfig::saveWindowSize(windowHandle(), group);
    group.sync();
}

#include "moc_mailmergeconfiguredialog.cpp"
