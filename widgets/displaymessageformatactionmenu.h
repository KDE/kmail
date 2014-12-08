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

#ifndef DisplayMessageFormatActionMenu_H
#define DisplayMessageFormatActionMenu_H

#include <KActionMenu>
#include "messageviewer/viewer/viewer.h"

class DisplayMessageFormatActionMenu : public KActionMenu
{
    Q_OBJECT
public:
    explicit DisplayMessageFormatActionMenu(QObject *parent = Q_NULLPTR);
    ~DisplayMessageFormatActionMenu();

    MessageViewer::Viewer::DisplayFormatMessage displayMessageFormat() const;
    void setDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage displayMessageFormat);

Q_SIGNALS:
    void changeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format);

private Q_SLOTS:
    void slotChangeDisplayMessageFormat();

private:
    void updateMenu();
    MessageViewer::Viewer::DisplayFormatMessage mDisplayMessageFormat;
};

#endif // DisplayMessageFormatActionMenu_H
