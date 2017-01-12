/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "followupreminderinfodialog.h"
#include "followupreminderinfowidget.h"

#include "kmail-version.h"

#include <KLocalizedString>
#include <KAboutData>
#include <KHelpMenu>
#include <QMenu>
#include <QIcon>

#include <KSharedConfig>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

FollowUpReminderInfoDialog::FollowUpReminderInfoDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Configure"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FollowUpReminderInfoDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &FollowUpReminderInfoDialog::reject);

    mWidget = new FollowUpReminderInfoWidget;
    mWidget->setObjectName(QStringLiteral("FollowUpReminderInfoWidget"));
    mainLayout->addWidget(mWidget);
    mainLayout->addWidget(buttonBox);

    readConfig();
    KAboutData aboutData = KAboutData(
                               QStringLiteral("followupreminderagent"),
                               i18n("Follow Up Reminder Agent"),
                               QStringLiteral(KDEPIM_VERSION),
                               i18n("Follow Up Mail."),
                               KAboutLicense::GPL_V2,
                               i18n("Copyright (C) 2014-2017 Laurent Montel"));

    aboutData.addAuthor(i18n("Laurent Montel"),
                        i18n("Maintainer"), QStringLiteral("montel@kde.org"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KHelpMenu *helpMenu = new KHelpMenu(this, aboutData, true);
    //Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    buttonBox->button(QDialogButtonBox::Help)->setMenu(menu);
}

FollowUpReminderInfoDialog::~FollowUpReminderInfoDialog()
{
    writeConfig();
}

void FollowUpReminderInfoDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "FollowUpReminderInfoDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(800, 600));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
    mWidget->restoreTreeWidgetHeader(group.readEntry("HeaderState", QByteArray()));
}

void FollowUpReminderInfoDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "FollowUpReminderInfoDialog");
    group.writeEntry("Size", size());
    mWidget->saveTreeWidgetHeader(group);
}

void FollowUpReminderInfoDialog::load()
{
    mWidget->load();
}

void FollowUpReminderInfoDialog::setInfo(const QList<FollowUpReminder::FollowUpReminderInfo *> &info)
{
    mWidget->setInfo(info);
}

QList<qint32> FollowUpReminderInfoDialog::listRemoveId() const
{
    return mWidget->listRemoveId();
}

