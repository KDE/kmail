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

#ifndef ATTACHMENTLISTWIDGET_H
#define ATTACHMENTLISTWIDGET_H

#include "pimcommon/widgets/simplestringlisteditor.h"
#include <QDialog>

namespace MailMerge
{
class AttachmentListWidget : public PimCommon::SimpleStringListEditor
{
    Q_OBJECT
public:
    explicit AttachmentListWidget(QWidget *parent = Q_NULLPTR,
                                  ButtonCode buttons = Unsorted,
                                  const QString &addLabel = QString(),
                                  const QString &removeLabel = QString(),
                                  const QString &modifyLabel = QString());
    ~AttachmentListWidget();

    void addNewEntry() Q_DECL_OVERRIDE;
    QString modifyEntry(const QString &text) Q_DECL_OVERRIDE;
};
}
#endif // ATTACHMENTLISTWIDGET_H
