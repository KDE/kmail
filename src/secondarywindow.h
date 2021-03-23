/*
    secondarywindow.h

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2003 Ingo Kloecker <kloecker@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "kmail_private_export.h"
#include <kxmlguiwindow.h>
class QCloseEvent;

namespace KMail
{
/**
 *  Window class for secondary KMail window like the composer window and
 *  the separate message window.
 */
class KMAILTESTS_TESTS_EXPORT SecondaryWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit SecondaryWindow(const QString &name = QString());
    ~SecondaryWindow() override;
    using KMainWindow::setCaption;
public Q_SLOTS:
    /**
     * Reimplement because we have <a href="https://bugs.kde.org/show_bug.cgi?id=163978">this bug</a>
     * @brief setCaption
     * @param caption
     */
    void setCaption(const QString &caption) override;

protected:
    /**
     *  Reimplemented because we don't want the application to quit when the
     *  last _visible_ secondary window is closed in case a system tray applet
     *  exists.
     */
    void closeEvent(QCloseEvent *) override;
};
} // namespace KMail

