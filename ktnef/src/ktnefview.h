/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

#include <QTreeWidget>

namespace KTnef
{
class KTNEFAttach;
}
using namespace KTnef;

class KTNEFView : public QTreeWidget
{
    Q_OBJECT

public:
    explicit KTNEFView(QWidget *parent = nullptr);
    ~KTNEFView() override;

    void setAttachments(const QList<KTNEFAttach *> &list);
    QList<KTNEFAttach *> getSelection();

Q_SIGNALS:
    void dragRequested(const QList<KTnef::KTNEFAttach *> &list);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void startDrag(Qt::DropActions dropAction) override;

private:
    void adjustColumnWidth();
    QList<KTNEFAttach *> mAttachments;
};

