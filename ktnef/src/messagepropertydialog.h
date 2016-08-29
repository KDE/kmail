/*
  This file is part of KTnef.

  Copyright (C) 2003 Michael Goffioul <kdeprint@swing.be>
  Copyright (c) 2012 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#ifndef MESSAGEPROPERTYDIALOG_H
#define MESSAGEPROPERTYDIALOG_H

#include <QDialog>

namespace KTnef
{
class KTNEFMessage;
}
using namespace KTnef;

class QTreeWidget;

class MessagePropertyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MessagePropertyDialog(QWidget *parent, KTNEFMessage *msg);
    ~MessagePropertyDialog();
private Q_SLOTS:
    void slotSaveProperty();

private:
    void readConfig();
    void writeConfig();
    KTNEFMessage *mMessage;
    QTreeWidget *mListView;
};

#endif
