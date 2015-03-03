/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include <akonadi/collectioncombobox.h>

#include <KCalCore/Todo>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <kdatecombobox.h>


FollowUpReminderSelectDateDialog::FollowUpReminderSelectDateDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Select Date" ) );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    setModal( true );

    QWidget *mainWidget = new QWidget( this );
    QVBoxLayout *mainLayout = new QVBoxLayout( mainWidget );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    QFormLayout *formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);

    mDateComboBox = new KDateComboBox;
    mDateComboBox->setMinimumDate(QDate::currentDate());
    mDateComboBox->setObjectName(QLatin1String("datecombobox"));

    QDate currentDate = QDate::currentDate();
    currentDate = currentDate.addDays(1);
    mDateComboBox->setDate(currentDate);

    formLayout->addRow(i18n("Date:"), mDateComboBox);

    mCollectionCombobox = new Akonadi::CollectionComboBox;
    mCollectionCombobox->setMinimumWidth(250);
    mCollectionCombobox->setAccessRightsFilter(Akonadi::Collection::CanCreateItem);
    mCollectionCombobox->setMimeTypeFilter( QStringList() << KCalCore::Todo::todoMimeType() );
    mCollectionCombobox->setObjectName(QLatin1String("collectioncombobox"));

    formLayout->addRow(i18n("Store ToDo in:"), mCollectionCombobox);

    connect(mDateComboBox->lineEdit(), SIGNAL(textChanged(QString)), SLOT(slotDateChanged(QString)));
    setMainWidget( mainWidget );
}

FollowUpReminderSelectDateDialog::~FollowUpReminderSelectDateDialog()
{
}

void FollowUpReminderSelectDateDialog::slotDateChanged(const QString &date)
{
    enableButtonOk(!date.isEmpty() && mDateComboBox->date().isValid());
}

QDate FollowUpReminderSelectDateDialog::selectedDate() const
{
    return mDateComboBox->date();
}

Akonadi::Collection FollowUpReminderSelectDateDialog::collection() const
{
    return mCollectionCombobox->currentCollection();
}

void FollowUpReminderSelectDateDialog::accept()
{
    const QDate date = selectedDate();
    if (date <= QDate::currentDate()) {
        KMessageBox::error(this, i18n("The selected date must be greater than the current date."), i18n("Invalid date"));
        return;
    }
    KDialog::accept();
}
