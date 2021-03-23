/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <KActionMenu>
#include <MessageViewer/Viewer>

class KMAILTESTS_TESTS_EXPORT DisplayMessageFormatActionMenu : public KActionMenu
{
    Q_OBJECT
public:
    explicit DisplayMessageFormatActionMenu(QObject *parent = nullptr);
    ~DisplayMessageFormatActionMenu() override;

    Q_REQUIRED_RESULT MessageViewer::Viewer::DisplayFormatMessage displayMessageFormat() const;
    void setDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage displayMessageFormat);

Q_SIGNALS:
    void changeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage format);

private:
    void slotChangeDisplayMessageFormat(QAction *act);
    void updateMenu();
    MessageViewer::Viewer::DisplayFormatMessage mDisplayMessageFormat = MessageViewer::Viewer::UseGlobalSetting;
};

