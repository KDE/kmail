/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TAGMONITORMANAGER_H
#define TAGMONITORMANAGER_H

#include <QObject>
#include <AkonadiCore/Tag>
namespace Akonadi {
class Monitor;
}
class TagMonitorManager : public QObject
{
    Q_OBJECT
public:
    explicit TagMonitorManager(QObject *parent = nullptr);
    ~TagMonitorManager();

    static TagMonitorManager *self();

Q_SIGNALS:
    void tagAdded(const Akonadi::Tag &tag);
    void tagChanged(const Akonadi::Tag &tag);
    void tagRemoved(const Akonadi::Tag &tag);

private:
    Akonadi::Monitor *const mMonitor;
};

#endif // TAGMONITORMANAGER_H
