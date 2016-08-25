/*
   Copyright (C) 2012-2016 Montel Laurent <montel@kde.org>

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

#include "archivemaildialog.h"

#include "archivemailwidget.h"
#include "kmail-version.h"

#include <MailCommon/MailUtil>

#include <QMenu>
#include <KHelpMenu>
#include <kaboutdata.h>
#include <QIcon>

#include <QHBoxLayout>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QLocale>

namespace
{
inline QString archiveMailCollectionPattern()
{
    return  QStringLiteral("ArchiveMailCollection \\d+");
}
}

ArchiveMailDialog::ArchiveMailDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Configure Archive Mail Agent"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    setModal(true);
    QVBoxLayout *vlay = new QVBoxLayout(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setMargin(0);
    vlay->addLayout(mainLayout);
    mWidget = new ArchiveMailWidget(this);
    connect(mWidget, &ArchiveMailWidget::archiveNow, this, &ArchiveMailDialog::archiveNow);
    mWidget->setObjectName(QStringLiteral("archivemailwidget"));
    mainLayout->addWidget(mWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ArchiveMailDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ArchiveMailDialog::slotSave);

    vlay->addWidget(buttonBox);

    readConfig();

    KAboutData aboutData = KAboutData(
                               QStringLiteral("archivemailagent"),
                               i18n("Archive Mail Agent"),
                               QStringLiteral(KDEPIM_VERSION),
                               i18n("Archive emails automatically."),
                               KAboutLicense::GPL_V2,
                               i18n("Copyright (C) 2012-2016 Laurent Montel"));

    aboutData.addAuthor(i18n("Laurent Montel"),
                        i18n("Maintainer"), QStringLiteral("montel@kde.org"));

    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KHelpMenu *helpMenu = new KHelpMenu(this, aboutData, true);
    //Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    buttonBox->button(QDialogButtonBox::Help)->setMenu(menu);
}

ArchiveMailDialog::~ArchiveMailDialog()
{
    writeConfig();
}

void ArchiveMailDialog::slotNeedReloadConfig()
{
    mWidget->needReloadConfig();
}

static const char myConfigGroupName[] = "ArchiveMailDialog";

void ArchiveMailDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), myConfigGroupName);

    const QSize size = group.readEntry("Size", QSize(500, 300));
    if (size.isValid()) {
        resize(size);
    }

    mWidget->restoreTreeWidgetHeader(group.readEntry("HeaderState", QByteArray()));
}

void ArchiveMailDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), myConfigGroupName);
    group.writeEntry("Size", size());
    mWidget->saveTreeWidgetHeader(group);
    group.sync();
}

void ArchiveMailDialog::slotSave()
{
    mWidget->save();
    accept();
}

