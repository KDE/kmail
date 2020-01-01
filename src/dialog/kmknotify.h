/*
   Copyright (C) 2011-2020 Laurent Montel <montel@kde.org>

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
