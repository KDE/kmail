/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#ifndef SELECTATTACHMENTDIALOG_H
#define SELECTATTACHMENTDIALOG_H

#include <QDialog>
class KUrlRequester;

namespace MailMerge
{
class SelectAttachmentDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SelectAttachmentDialog(QWidget *parent = Q_NULLPTR);
    ~SelectAttachmentDialog();

    void setAttachmentPath(const QString &path);
    QString attachmentPath() const;

private:
    KUrlRequester *mUrlRequester;
};
}

#endif // SELECTATTACHMENTDIALOG_H
