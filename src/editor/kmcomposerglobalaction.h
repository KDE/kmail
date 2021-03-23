/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
    Q_DISABLE_COPY(KMComposerGlobalAction)
    KMComposerWin *const mComposerWin;
};

