/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef KMCOMPOSERGLOBALACTION_H
#define KMCOMPOSERGLOBALACTION_H

#include <QObject>
class KMComposerWin;
class KMComposerGlobalAction : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerGlobalAction(KMComposerWin *composerWin, QObject *parent = nullptr);
    ~KMComposerGlobalAction();

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

#endif // KMCOMPOSERGLOBALACTION_H
