/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#ifndef KACTIONMENUACCOUNT_H
#define KACTIONMENUACCOUNT_H

#include <KActionMenu>

class AgentIdentifier
{
public:
    AgentIdentifier()
        : mIndex(-1)
    {

    }

    AgentIdentifier(const QString &identifier, const QString &name, int index = -1)
        : mIdentifier(identifier),
          mName(name),
          mIndex(index)
    {

    }
    QString mIdentifier;
    QString mName;
    int mIndex;
};

class KActionMenuAccount : public KActionMenu
{
    Q_OBJECT
public:
    explicit KActionMenuAccount(QObject *parent = Q_NULLPTR);
    ~KActionMenuAccount();

    void setAccountOrder(const QStringList &identifier);

private Q_SLOTS:
    void updateAccountMenu();
    void slotCheckTransportMenu();
    void slotSelectAccount(QAction *act);
private:
    QStringList mOrderIdentifier;
    bool mInitialized;
};

#endif // KACTIONMENUACCOUNT_H
