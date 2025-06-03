/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
class KMComposerWin;
class KMComposerGlobalAction : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerGlobalAction(KMComposerWin *composerWin, QObject *parent = nullptr);
    ~KMComposerGlobalAction() override;

public Q_SLOTS:
    void slotUndo();
    void slotRedo();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotMarkAll();
    void slotInsertEmoticon(const QString &str);
    void slotInsertText(const QString &str);

private:
    KMComposerWin *const mComposerWin;
};
