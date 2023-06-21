/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "attachpropertydialog.h"
#include "qwmf.h"

#include <KTNEF/KTNEFAttach>
#include <KTNEF/KTNEFDefs>
#include <KTNEF/KTNEFProperty>
#include <KTNEF/KTNEFPropertySet>

#include "ktnef_debug.h"
#include <KLocalizedString>
#include <KMessageBox>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QBuffer>
#include <QDataStream>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWindow>

namespace
{
static const char myAttachPropertyDialogGroupName[] = "AttachPropertyDialog";
}

AttachPropertyDialog::AttachPropertyDialog(QWidget *parent)
    : QDialog(parent)
{
    setModal(true);

    auto mainLayout = new QVBoxLayout(this);

    auto mainWidget = new QWidget(this);
    mUI.setupUi(mainWidget);
    mUI.mProperties->setHeaderHidden(true);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto user1Button = new QPushButton;
    buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AttachPropertyDialog::reject);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    user1Button->setText(i18n("Save..."));
    connect(user1Button, &QPushButton::clicked, this, &AttachPropertyDialog::slotSave);

    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);
    readConfig();
}

AttachPropertyDialog::~AttachPropertyDialog()
{
    writeConfig();
}

void AttachPropertyDialog::readConfig()
{
    create(); // ensure a window is created
    windowHandle()->resize(QSize(600, 700));
    KConfigGroup group(KSharedConfig::openStateConfig(), myAttachPropertyDialogGroupName);
    KWindowConfig::restoreWindowSize(windowHandle(), group);
    resize(windowHandle()->size()); // workaround for QTBUG-40584
}

void AttachPropertyDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), myAttachPropertyDialogGroupName);
    KWindowConfig::saveWindowSize(windowHandle(), group);
    group.sync();
}

void AttachPropertyDialog::setAttachment(KTNEFAttach *attach)
{
    QString s = attach->fileName().isEmpty() ? attach->name() : attach->fileName();
    mUI.mFilename->setText(QLatin1String("<b>") + s + QLatin1String("</b>"));
    setWindowTitle(i18nc("@title:window", "Properties for Attachment %1", s));
    mUI.mDisplay->setText(attach->displayName());
    mUI.mMime->setText(attach->mimeTag());
    s.setNum(attach->size());
    s.append(i18n(" bytes"));
    mUI.mSize->setText(s);
    QMimeDatabase db;
    const QMimeType mimetype = db.mimeTypeForName(attach->mimeTag());
    const QPixmap pix = loadRenderingPixmap(attach, qApp->palette().color(QPalette::Window));
    if (!pix.isNull()) {
        mUI.mIcon->setPixmap(pix);
    } else {
        mUI.mIcon->setPixmap(mimetype.iconName());
    }
    mUI.mDescription->setText(mimetype.comment());
    s.setNum(attach->index());
    mUI.mIndex->setText(s);

    formatPropertySet(attach, mUI.mProperties);
    mAttach = attach;
}

void AttachPropertyDialog::slotSave()
{
    if (saveProperty(mUI.mProperties, mAttach, this)) {
        accept();
    }
}

void AttachPropertyDialog::formatProperties(const QMap<int, KTNEFProperty *> &props, QTreeWidget *lv, QTreeWidgetItem *item, const QString &prefix)
{
    QMap<int, KTNEFProperty *>::ConstIterator end(props.constEnd());
    for (QMap<int, KTNEFProperty *>::ConstIterator it = props.begin(); it != end; ++it) {
        QTreeWidgetItem *newItem = nullptr;
        if (lv) {
            newItem = new QTreeWidgetItem(lv, QStringList((*it)->keyString()));
        } else if (item) {
            newItem = new QTreeWidgetItem(item, QStringList((*it)->keyString()));
        } else {
            qCWarning(KTNEFAPPS_LOG) << "formatProperties() called with no listview and no item";
            return;
        }

        QVariant value = (*it)->value();
        if (value.userType() == QMetaType::QVariantList) {
            newItem->setExpanded(true);
            newItem->setText(0, newItem->text(0) + QLatin1String(" [") + QString::number(value.toList().count()) + QLatin1Char(']'));
            int i = 0;
            QList<QVariant>::ConstIterator litEnd = value.toList().constEnd();
            for (QList<QVariant>::ConstIterator lit = value.toList().constBegin(); lit != litEnd; ++lit, ++i) {
                new QTreeWidgetItem(newItem,
                                    QStringList() << QLatin1Char('[') + QString::number(i) + QLatin1Char(']') << QString(KTNEFProperty::formatValue(*lit)));
            }
        } else if (value.userType() == QMetaType::QDateTime) {
            newItem->setText(1, value.toDateTime().toString());
        } else {
            newItem->setText(1, (*it)->valueString());
            newItem->setText(2, prefix + QLatin1Char('_') + QString::number(it.key()));
        }
    }
}

void AttachPropertyDialog::formatPropertySet(KTNEFPropertySet *pSet, QTreeWidget *lv)
{
    formatProperties(pSet->properties(), lv, nullptr, QStringLiteral("prop"));
    auto item = new QTreeWidgetItem(lv, QStringList(i18nc("@label", "TNEF Attributes")));
    item->setExpanded(true);
    formatProperties(pSet->attributes(), nullptr, item, QStringLiteral("attr"));
}

bool AttachPropertyDialog::saveProperty(QTreeWidget *lv, KTNEFPropertySet *pSet, QWidget *parent)
{
    const QList<QTreeWidgetItem *> list = lv->selectedItems();
    if (list.isEmpty()) {
        KMessageBox::error(parent, i18nc("@info", "Must select an item first."));
        return false;
    }

    QTreeWidgetItem *item = list.constFirst();
    if (item->text(2).isEmpty()) {
        KMessageBox::error(parent, i18nc("@info", "The selected item cannot be saved because it has an empty tag."));
    } else {
        QString tag = item->text(2);
        const int key = QStringView(tag).mid(5).toInt();
        const QVariant prop = (tag.startsWith(QLatin1String("attr_")) ? pSet->attribute(key) : pSet->property(key));
        QString filename = QFileDialog::getSaveFileName(parent, QString(), tag, QString());
        if (!filename.isEmpty()) {
            QFile f(filename);
            if (f.open(QIODevice::WriteOnly)) {
                switch (prop.metaType().id()) {
                case QMetaType::QByteArray:
                    f.write(prop.toByteArray().data(), prop.toByteArray().size());
                    break;
                default: {
                    QTextStream t(&f);
                    t << prop.toString();
                    break;
                }
                }
                f.close();
            } else {
                KMessageBox::error(parent, i18nc("@info", "Unable to open file for writing, check file permissions."));
            }
        }
    }
    return true;
}

QPixmap AttachPropertyDialog::loadRenderingPixmap(KTNEFPropertySet *pSet, const QColor &bgColor)
{
    QPixmap pix;
    QVariant rendData = pSet->attribute(attATTACHRENDDATA);
    QVariant wmf = pSet->attribute(attATTACHMETAFILE);

    if (!rendData.isNull() && !wmf.isNull()) {
        // Get rendering size
        QByteArray qb = rendData.toByteArray();
        QBuffer rendBuffer(&qb);
        rendBuffer.open(QIODevice::ReadOnly);
        QDataStream rendStream(&rendBuffer);
        rendStream.setByteOrder(QDataStream::LittleEndian);
        quint16 type;
        quint16 w;
        quint16 h;
        rendStream >> type >> w >> w; // read type and skip 4 bytes
        rendStream >> w >> h;
        rendBuffer.close();

        if (type == 1 && w > 0 && h > 0) {
            // Load WMF data
            QWinMetaFile wmfLoader;
            QByteArray qb2 = wmf.toByteArray();
            QBuffer wmfBuffer(&qb2);
            wmfBuffer.open(QIODevice::ReadOnly);
            if (wmfLoader.load(wmfBuffer)) {
                pix.scaled(w, h, Qt::KeepAspectRatio);
                pix.fill(bgColor);
                wmfLoader.paint(&pix);
            }
            wmfBuffer.close();
        }
    }
    return pix;
}

#include "moc_attachpropertydialog.cpp"
