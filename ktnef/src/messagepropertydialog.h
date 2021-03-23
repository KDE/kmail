/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2003 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#pragma once

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
    ~MessagePropertyDialog() override;

private:
    void slotSaveProperty();
    void readConfig();
    void writeConfig();
    KTNEFMessage *mMessage = nullptr;
    QTreeWidget *mListView = nullptr;
};

