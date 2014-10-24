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

#include <akonadi/collectioncombobox.h>

#include <KCalCore/Todo>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>


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

    mDatePicker = new KDatePicker;
    mDatePicker->setObjectName(QLatin1String("datepicker"));

    QDate currentDate = QDate::currentDate();
    currentDate = currentDate.addDays(1);
    mDatePicker->setDate(currentDate);

    formLayout->addRow(i18n("Date:"), mDatePicker);

    mCollectionCombobox = new Akonadi::CollectionComboBox;
    mCollectionCombobox->setAccessRightsFilter(Akonadi::Collection::CanCreateItem);
    mCollectionCombobox->setMimeTypeFilter( QStringList() << KCalCore::Todo::todoMimeType() );
    mCollectionCombobox->setObjectName(QLatin1String("collectioncombobox"));

    formLayout->addRow(i18n("Store ToDo in:"), mCollectionCombobox);

    setMainWidget( mainWidget );
    readConfig();
}

FollowUpReminderSelectDateDialog::~FollowUpReminderSelectDateDialog()
{
    writeConfig();
}

void FollowUpReminderSelectDateDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "FollowUpReminderSelectDateDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void FollowUpReminderSelectDateDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "FollowUpReminderSelectDateDialog" );
    group.writeEntry( "Size", size() );
}

QDate FollowUpReminderSelectDateDialog::selectedDate() const
{
    return mDatePicker->date();
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
