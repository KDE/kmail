/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class QMimeData;
class KMComposerWin;
class DndFromArkJob : public QObject
{
    Q_OBJECT
public:
    explicit DndFromArkJob(QObject *parent = nullptr);
    [[nodiscard]] static bool dndFromArk(const QMimeData *source);
    [[nodiscard]] bool extract(const QMimeData *source);
    void setComposerWin(KMComposerWin *composerWin);

private:
    KMComposerWin *mComposerWin = nullptr;
};
