/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#include "statusbarlabeltoggledstate.h"
#include <KLocalizedString>
#include "kmail_debug.h"

StatusBarLabelToggledState::StatusBarLabelToggledState(QWidget *parent)
    : QLabel(parent),
      mToggleMode(false)
{
}

StatusBarLabelToggledState::~StatusBarLabelToggledState()
{

}

void StatusBarLabelToggledState::setStateString(const QString &toggled, const QString &untoggled)
{
    if (toggled.isEmpty() || untoggled.isEmpty()) {
        qCWarning(KMAIL_LOG) << " State string is empty. Need to fix it";
    }
    mToggled = toggled;
    mUnToggled = untoggled;
    updateLabel();
}

void StatusBarLabelToggledState::setToggleMode(bool state)
{
    if (mToggleMode != state) {
        mToggleMode = state;
        Q_EMIT toggleModeChanged(mToggleMode);
        updateLabel();
    }
}

bool StatusBarLabelToggledState::toggleMode() const
{
    return mToggleMode;
}

void StatusBarLabelToggledState::updateLabel()
{
    if (mToggleMode) {
        setText(mToggled);
    } else {
        setText(mUnToggled);
    }
}

void StatusBarLabelToggledState::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    setToggleMode(!mToggleMode);
}
