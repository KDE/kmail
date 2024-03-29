/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ktnefview.h"
#include "attachpropertydialog.h"

#include <KTNEF/KTNEFAttach>

#include <KLocalizedString>

#include <QIcon>

#include <QMimeDatabase>
#include <QMimeType>
#include <QPixmap>
#include <QTimer>

class Attachment : public QTreeWidgetItem
{
public:
    Attachment(QTreeWidget *parent, KTNEFAttach *attach);
    ~Attachment() override;

    [[nodiscard]] KTNEFAttach *getAttachment() const
    {
        return mAttach;
    }

private:
    KTNEFAttach *const mAttach;
};

Attachment::Attachment(QTreeWidget *parent, KTNEFAttach *attach)
    : QTreeWidgetItem(parent, QStringList(attach->name()))
    , mAttach(attach)
{
    setText(2, QString::number(mAttach->size()));
    if (!mAttach->fileName().isEmpty()) {
        setText(0, mAttach->fileName());
    }

    QMimeDatabase db;
    const QMimeType mimeType = db.mimeTypeForName(mAttach->mimeTag());
    setText(1, mimeType.comment());

    QPixmap pix = AttachPropertyDialog::loadRenderingPixmap(attach, qApp->palette().color(QPalette::Window));
    if (!pix.isNull()) {
        setIcon(0, pix);
    } else {
        setIcon(0, QIcon::fromTheme(mimeType.iconName()));
    }
}

Attachment::~Attachment() = default;

//----------------------------------------------------------------------------//

KTNEFView::KTNEFView(QWidget *parent)
    : QTreeWidget(parent)
{
    const QStringList headerLabels = (QStringList(i18nc("@title:column file name", "File Name"))
                                      << i18nc("@title:column file type", "File Type") << i18nc("@title:column file size", "Size"));
    setHeaderLabels(headerLabels);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setSortingEnabled(true);
    QTimer::singleShot(0, this, &KTNEFView::adjustColumnWidth);
}

KTNEFView::~KTNEFView() = default;

void KTNEFView::setAttachments(const QList<KTNEFAttach *> &list)
{
    clear();
    for (const auto &s : list) {
        new Attachment(this, s);
    }
}

void KTNEFView::resizeEvent(QResizeEvent *e)
{
    adjustColumnWidth();
    resize(width(), height());
    if (e) {
        QTreeWidget::resizeEvent(e);
    }
}

QList<KTNEFAttach *> KTNEFView::getSelection()
{
    mAttachments.clear();

    const QList<QTreeWidgetItem *> list = selectedItems();
    if (list.isEmpty() || !list.first()) {
        return mAttachments;
    }

    QList<QTreeWidgetItem *>::const_iterator it;
    QList<QTreeWidgetItem *>::const_iterator end(list.constEnd());
    mAttachments.reserve(list.count());
    for (it = list.constBegin(); it != end; ++it) {
        auto a = static_cast<Attachment *>(*it);
        mAttachments.append(a->getAttachment());
    }
    return mAttachments;
}

void KTNEFView::startDrag(Qt::DropActions dropAction)
{
    Q_UNUSED(dropAction)

    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::Selected);
    QList<KTNEFAttach *> list;
    while (*it) {
        auto a = static_cast<Attachment *>(*it);
        list << a->getAttachment();
        ++it;
    }
    if (!list.isEmpty()) {
        Q_EMIT dragRequested(list);
    }
}

void KTNEFView::adjustColumnWidth()
{
    const int w = width() / 2;
    setColumnWidth(0, w);
    setColumnWidth(1, w / 2);
    setColumnWidth(2, w / 2);
}

#include "moc_ktnefview.cpp"
