/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef UNIFIEDMAILBOXEDITOR_H
#define UNIFIEDMAILBOXEDITOR_H

#include <QDialog>

#include "unifiedmailboxmanager.h"

class UnifiedMailbox;
class UnifiedMailboxEditor : public QDialog
{
    Q_OBJECT
public:
    explicit UnifiedMailboxEditor(const KSharedConfigPtr &config, QWidget *parent = nullptr);
    explicit UnifiedMailboxEditor(UnifiedMailbox *mailbox, const KSharedConfigPtr &config,
		                  QWidget *parent = nullptr);

    ~UnifiedMailboxEditor() override;
private:
    UnifiedMailbox *mMailbox = nullptr;
    KSharedConfigPtr mConfig;
};

#endif
