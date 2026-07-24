/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
namespace KMail
{
class KMUndoRedoManager : public QObject
{
    Q_OBJECT
public:
    explicit KMUndoRedoManager(QObject *parent = nullptr);
    ~KMUndoRedoManager() override;
};
}
