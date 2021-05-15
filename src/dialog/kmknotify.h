/*
   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDialog>
class QComboBox;
class KNotifyConfigWidget;

namespace KMail
{
class KMKnotify : public QDialog
{
    Q_OBJECT
public:
    explicit KMKnotify(QWidget *parent = nullptr);
    ~KMKnotify() override;

    void setCurrentNotification(const QString &name);

private:
    void slotComboChanged(int);
    void slotOk();
    void slotConfigChanged(bool changed);

    void initCombobox();
    void writeConfig();
    void readConfig();
    QComboBox *const m_comboNotify;
    KNotifyConfigWidget *const m_notifyWidget;
    bool m_changed = false;
};
}

