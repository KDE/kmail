/*
  Copyright (c) 2012-2017 Montel Laurent <montel@kde.org>

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

#ifndef IDENTITYADDVCARDDIALOG_H
#define IDENTITYADDVCARDDIALOG_H
#include <QDialog>
#include <QUrl>
class QButtonGroup;
class KComboBox;
class KUrlRequester;

class IdentityAddVcardDialog: public QDialog
{
    Q_OBJECT
public:
    enum DuplicateMode {
        Empty,
        ExistingEntry,
        FromExistingVCard
    };

    explicit IdentityAddVcardDialog(const QStringList &shadowIdentities, QWidget *parent = nullptr);
    ~IdentityAddVcardDialog();

    DuplicateMode duplicateMode() const;
    QString duplicateVcardFromIdentity() const;
    QUrl existingVCard() const;

private:
    QButtonGroup *mButtonGroup;
    KComboBox *mComboBox;
    KUrlRequester *mVCardPath;
};

#endif // IDENTITYADDVCARDDIALOG_H
