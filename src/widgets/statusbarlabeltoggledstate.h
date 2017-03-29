/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef STATUSBARLABELTOGGLEDSTATE_H
#define STATUSBARLABELTOGGLEDSTATE_H

#include <QLabel>

class StatusBarLabelToggledState : public QLabel
{
    Q_OBJECT
public:
    explicit StatusBarLabelToggledState(QWidget *parent = nullptr);
    ~StatusBarLabelToggledState();

    void setToggleMode(bool state);

    bool toggleMode() const;

    void setStateString(const QString &toggled, const QString &untoggled);
Q_SIGNALS:
    void toggleModeChanged(bool state);

protected:
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

private:
    void updateLabel();
    QString mToggled;
    QString mUnToggled;
    bool mToggleMode;
};

#endif // STATUSBARLABELTOGGLEDSTATE_H

