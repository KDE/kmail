/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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


#include "followupreminderselectdatedialog.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KDatePicker>
#include <KMessageBox>

#include <QVBoxLayout>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>

FollowUpReminderSelectDateDialog::FollowUpReminderSelectDateDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle( i18n( "Select Date" ) );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *topLayout = new QVBoxLayout;
    setLayout(topLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    okButton->setDefault(true);
    setModal( true );

    QWidget *mainWidget = new QWidget( this );
    topLayout->addWidget(mainWidget);
    topLayout->addWidget(buttonBox);
    QVBoxLayout *mainLayout = new QVBoxLayout( mainWidget );
//TODO PORT QT5     mainLayout->setSpacing( QDialog::spacingHint() );
//TODO PORT QT5     mainLayout->setMargin( QDialog::marginHint() );
    mDatePicker = new KDatePicker;
    QDate currentDate = QDate::currentDate();
    currentDate = currentDate.addDays(1);
    mDatePicker->setDate(currentDate);
    mainLayout->addWidget(mDatePicker);

    readConfig();
}

FollowUpReminderSelectDateDialog::~FollowUpReminderSelectDateDialog()
{
    writeConfig();
}

void FollowUpReminderSelectDateDialog::readConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), "FollowUpReminderSelectDateDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void FollowUpReminderSelectDateDialog::writeConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), "FollowUpReminderSelectDateDialog" );
    group.writeEntry( "Size", size() );
}

QDate FollowUpReminderSelectDateDialog::selectedDate() const
{
    return mDatePicker->date();
}

void FollowUpReminderSelectDateDialog::accept()
{
    const QDate date = selectedDate();
    if (date < QDate::currentDate()) {
        KMessageBox::error(this, i18n("The selected date must be greater than the current date."), i18n("Invalid date"));
        return;
    }
    QDialog::accept();
}
