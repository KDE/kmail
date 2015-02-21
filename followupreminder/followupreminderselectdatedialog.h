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

#ifndef FOLLOWUPREMINDERSELECTDATEDIALOG_H
#define FOLLOWUPREMINDERSELECTDATEDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <AkonadiCore/Collection>
namespace Akonadi
{
class CollectionComboBox;
}
class KDateComboBox;
class FollowUpReminderSelectDateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FollowUpReminderSelectDateDialog(QWidget *parent = Q_NULLPTR);
    ~FollowUpReminderSelectDateDialog();

    QDate selectedDate() const;

    void accept() Q_DECL_OVERRIDE;

    Akonadi::Collection collection() const;
private slots:
    void slotDateChanged(const QString &date);
private:
    KDateComboBox *mDateComboBox;
    Akonadi::CollectionComboBox *mCollectionCombobox;
    QPushButton *mOkButton;
};

#endif // FOLLOWUPREMINDERSELECTDATEDIALOG_H
