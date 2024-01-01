/*
   SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>
class QTextEdit;
class ServerDbusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ServerDbusWidget(QWidget *parent = nullptr);
    ~ServerDbusWidget() override;

    QString debug();
    void sendElements(const QList<qint64> &items, int index);
    void showDialog(qlonglong windowId);

private:
    QTextEdit *const mEdit;
};
