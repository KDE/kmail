/*
   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMKNOTIFY_H
#define KMKNOTIFY_H

#include <QDialog>
class QComboBox;
class KNotifyConfigWidget;

namespace KMail {
class KMKnotify : public QDialog
{
    Q_OBJECT
public:
    explicit KMKnotify(QWidget *parent);
    ~KMKnotify();

    void setCurrentNotification(const QString &name);

private:
    void slotComboChanged(int);
    void slotOk();
    void slotConfigChanged(bool changed);

    void initCombobox();
    void writeConfig();
    void readConfig();
    QComboBox *m_comboNotify = nullptr;
    KNotifyConfigWidget *m_notifyWidget = nullptr;
    bool m_changed = false;
};
}

#endif /* KMKNOTIFY_H */
