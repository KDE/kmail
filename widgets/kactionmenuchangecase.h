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

#ifndef KACTIONMENUCHANGECASE_H
#define KACTIONMENUCHANGECASE_H

#include <KActionMenu>
class QAction;
class KActionCollection;
class KActionMenuChangeCase : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuChangeCase(QObject *parent = 0);
    ~KActionMenuChangeCase();

    QAction *upperCaseAction() const;

    QAction *sentenceCaseAction() const;

    QAction *lowerCaseAction() const;

    void appendInActionCollection(KActionCollection *ac);

Q_SIGNALS:
    void upperCase();
    void sentenceCase();
    void lowerCase();

private:
    QAction *mUpperCase;
    QAction *mSentenceCase;
    QAction *mLowerCase;
};

#endif // KACTIONMENUCHANGECASE_H
