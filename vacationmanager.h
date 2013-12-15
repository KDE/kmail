/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#ifndef VACATIONMANAGER_H
#define VACATIONMANAGER_H

#include <QObject>
#include <QPointer>

namespace KSieveUi {
class Vacation;
class MultiImapVacationDialog;
}
class QWidget;
namespace KMail {
class VacationManager : public QObject
{
    Q_OBJECT
public:
    explicit VacationManager(QWidget *parent);
    ~VacationManager();

    void checkVacation();

public Q_SLOTS:
    void slotEditVacation();

Q_SIGNALS:
    void updateVacationScriptStatus(bool, const QString&);
    void editVacation();

private:
    QPointer<KSieveUi::MultiImapVacationDialog> mMultiImapVacationDialog;
    QPointer<KSieveUi::Vacation> mCheckVacation;
    QWidget *mWidget;
};
}

#endif // VACATIONMANAGER_H
