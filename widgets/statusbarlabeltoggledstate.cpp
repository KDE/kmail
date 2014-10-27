/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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
      mOverwriteMode(false)
{
    updateLabel();
}

StatusBarLabelToggledState::~StatusBarLabelToggledState()
{

}


void StatusBarLabelToggledState::setOverwriteMode(bool state)
{
    if (mOverwriteMode != state) {
        mOverwriteMode = state;
        Q_EMIT overwriteModeChanged(mOverwriteMode);
        updateLabel();
    }
}

bool StatusBarLabelToggledState::overwriteMode() const
{
    return mOverwriteMode;
}

void StatusBarLabelToggledState::updateLabel()
{
    if (mOverwriteMode) {
        setText(i18n("OVR"));
    } else {
        setText(i18n("INS"));
    }
}

void StatusBarLabelToggledState::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    setOverwriteMode(!mOverwriteMode);
}
