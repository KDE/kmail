/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNDOSENDMANAGER_H
#define UNDOSENDMANAGER_H

#include <QObject>

class UndoSendManager : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendManager(QObject *parent = nullptr);
    ~UndoSendManager() override;
    static UndoSendManager *self();

    void addItem(qint64 index, const QString &subject, int delay);
};

#endif // UNDOSENDMANAGER_H
