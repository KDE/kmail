/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#include "statusbarlabeltoggledstate.h"
#include <KLocalizedString>
#include <QDebug>

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
        qWarning() <<" State string is empty. Need to fix it";
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
