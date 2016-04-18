/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "followupremindernoanswerdialog.h"
#include "FollowupReminder/FollowUpReminderInfo"
#include "followupreminderinfowidget.h"

#include <KLocalizedString>
#include <KSharedConfig>

#include <QLabel>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QVBoxLayout>

FollowUpReminderNoAnswerDialog::FollowUpReminderNoAnswerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Follow Up Mail"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FollowUpReminderNoAnswerDialog::slotSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &FollowUpReminderNoAnswerDialog::reject);
    setAttribute(Qt::WA_DeleteOnClose);
    QWidget *w = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(w);
    QLabel *lab = new QLabel(i18n("You still wait an answer about this mail:"));
    vbox->addWidget(lab);
    mWidget = new FollowUpReminderInfoWidget;
    mWidget->setObjectName(QStringLiteral("FollowUpReminderInfoWidget"));
    vbox->addWidget(mWidget);
    mainLayout->addWidget(w);
    mainLayout->addWidget(buttonBox);

    readConfig();
}

FollowUpReminderNoAnswerDialog::~FollowUpReminderNoAnswerDialog()
{
    writeConfig();
}

void FollowUpReminderNoAnswerDialog::setInfo(const QList<FollowUpReminder::FollowUpReminderInfo *> &info)
{
    mWidget->setInfo(info);
}

void FollowUpReminderNoAnswerDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "FollowUpReminderNoAnswerDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(800, 600));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
    mWidget->restoreTreeWidgetHeader(group.readEntry("HeaderState", QByteArray()));
}

void FollowUpReminderNoAnswerDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "FollowUpReminderNoAnswerDialog");
    group.writeEntry("Size", size());
    mWidget->saveTreeWidgetHeader(group);
}

void FollowUpReminderNoAnswerDialog::slotSave()
{
    if (mWidget->save()) {
        Q_EMIT needToReparseConfiguration();
    }
    accept();
}
