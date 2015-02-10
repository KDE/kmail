/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#ifndef KMCOMPOSERGLOBALACTION_H
#define KMCOMPOSERGLOBALACTION_H

#include <QObject>
class KMComposeWin;
class KMComposerGlobalAction : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerGlobalAction(KMComposeWin *composerWin, QObject *parent = 0);
    ~KMComposerGlobalAction();

public Q_SLOTS:
    void slotUndo();
    void slotRedo();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotMarkAll();
private:
    KMComposeWin *mComposerWin;
};

#endif // KMCOMPOSERGLOBALACTION_H
