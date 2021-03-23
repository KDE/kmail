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

#include "ui_attachpropertywidgetbase.h"

#include <QDialog>

#include <QMap>
#include <QPixmap>

namespace KTnef
{
class KTNEFAttach;
class KTNEFProperty;
class KTNEFPropertySet;
}
using namespace KTnef;

class QTreeWidget;
class QTreeWidgetItem;

class AttachPropertyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AttachPropertyDialog(QWidget *parent = nullptr);
    ~AttachPropertyDialog() override;

    void setAttachment(KTNEFAttach *attach);

    static QPixmap loadRenderingPixmap(KTNEFPropertySet *, const QColor &);
    static void formatProperties(const QMap<int, KTNEFProperty *> &, QTreeWidget *, QTreeWidgetItem *, const QString & = QStringLiteral("prop"));
    static void formatPropertySet(KTNEFPropertySet *, QTreeWidget *);
    static bool saveProperty(QTreeWidget *, KTNEFPropertySet *, QWidget *);

protected:
    Ui::AttachPropertyWidgetBase mUI;

private:
    void slotSave();
    void readConfig();
    void writeConfig();
    KTNEFAttach *mAttach = nullptr;
};

