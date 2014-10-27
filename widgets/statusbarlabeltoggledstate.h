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

#ifndef STATUSBARLABELTOGGLEDSTATE_H
#define STATUSBARLABELTOGGLEDSTATE_H

#include <QLabel>


class StatusBarLabelToggledState : public QLabel
{
    Q_OBJECT
public:
    explicit StatusBarLabelToggledState(QWidget *parent = 0);
    ~StatusBarLabelToggledState();

    void setOverwriteMode(bool state);

    bool overwriteMode() const;

Q_SIGNALS:
    void overwriteModeChanged(bool state);

protected:
    void mousePressEvent(QMouseEvent *ev);

private:
    void updateLabel();
    bool mOverwriteMode;
};



#endif // STATUSBARLABELTOGGLEDSTATE_H

