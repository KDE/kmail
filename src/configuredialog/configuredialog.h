/*   -*- c++ -*-
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2002 Marc Mutz <mutz@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include <KCMultiDialog>

#include <QHideEvent>

class ConfigureDialog : public KCMultiDialog
{
    Q_OBJECT

public:
    explicit ConfigureDialog(QWidget *parent = nullptr, bool modal = true);
    ~ConfigureDialog() override;

Q_SIGNALS:
    void configChanged();

protected:
    void hideEvent(QHideEvent *i) override;

    QSize sizeHint() const override;
protected Q_SLOTS:
    /** @b reimplemented
     * Saves the GlobalSettings stuff before passing on to KCMultiDialog.
     */
    void slotApply();

    /** @b reimplemented
     * Saves the GlobalSettings stuff before passing on to KCMultiDialog.
     */
    void slotOk();
};

