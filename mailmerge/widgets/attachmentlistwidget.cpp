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

#include "attachmentlistwidget.h"
#include "selectattachmentdialog.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>

using namespace MailMerge;
AttachmentListWidget::AttachmentListWidget(QWidget *parent,
        ButtonCode buttons,
        const QString &addLabel,
        const QString &removeLabel,
        const QString &modifyLabel)
    : PimCommon::SimpleStringListEditor(parent, buttons, addLabel, removeLabel, modifyLabel, QString())
{
}

AttachmentListWidget::~AttachmentListWidget()
{

}

void AttachmentListWidget::addNewEntry()
{
    QPointer<SelectAttachmentDialog> dlg = new SelectAttachmentDialog(this);
    if (dlg->exec()) {
        insertNewEntry(dlg->attachmentPath());
    }
    delete dlg;
}

QString AttachmentListWidget::modifyEntry(const QString &text)
{
    QString attachmentPath;
    QPointer<SelectAttachmentDialog> dlg = new SelectAttachmentDialog(this);
    dlg->setAttachmentPath(text);
    if (dlg->exec()) {
        attachmentPath = dlg->attachmentPath();
    }
    delete dlg;
    return attachmentPath;
}

