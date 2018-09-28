/*
   Copyright (C) 2018 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MAILFILTERPURPOSEMENUWIDGET_H
#define MAILFILTERPURPOSEMENUWIDGET_H

#include <PimCommon/PurposeMenuWidget>
namespace KPIMTextEdit {
class PlainTextEditor;
}
class MailfilterPurposeMenuWidget : public PimCommon::PurposeMenuWidget
{
    Q_OBJECT
public:
    explicit MailfilterPurposeMenuWidget(QWidget *parentWidget, QObject *parent = nullptr);
    ~MailfilterPurposeMenuWidget() override;

    QByteArray text() override;
    void setEditorWidget(KPIMTextEdit::PlainTextEditor *editor);
private:
    KPIMTextEdit::PlainTextEditor *mEditor = nullptr;
};

#endif // MAILFILTERPURPOSEMENUWIDGET_H
