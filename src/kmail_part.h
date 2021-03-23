/*
    This file is part of KMail.
    SPDX-FileCopyrightText: 2002-2003 Don Sanders <sanders@kde.org>,
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include <kparts/part.h>
#include <kparts/readonlypart.h>

#include <QWidget>

class KMMainWidget;

class KMailPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.kmailpart")

public:
    KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &);
    ~KMailPart() override;

    QWidget *parentWidget() const;

public Q_SLOTS:
    Q_SCRIPTABLE void save();
    Q_SCRIPTABLE void exit();
    void updateQuickSearchText();

protected:
    bool openFile() override;
    void guiActivateEvent(KParts::GUIActivateEvent *e) override;

private:
    KMMainWidget *mainWidget = nullptr;
    QWidget *const mParentWidget;
};

