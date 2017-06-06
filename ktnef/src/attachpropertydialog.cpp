/*
  This file is part of KTnef.

  Copyright (C) 2002 Michael Goffioul <kdeprint@swing.be>
  Copyright (c) 2012 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "attachpropertydialog.h"
#include "qwmf.h"

#include <KTNEF/KTNEFAttach>
#include <KTNEF/KTNEFProperty>
#include <KTNEF/KTNEFPropertySet>
#include <KTNEF/KTNEFDefs>

#include "ktnef_debug.h"
#include <KLocalizedString>
#include <KMessageBox>

#include <QBuffer>
#include <QDataStream>
#include <QTreeWidget>
#include <KSharedConfig>
#include <QMimeDatabase>
#include <QMimeType>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>

AttachPropertyDialog::AttachPropertyDialog(QWidget *parent)
    : QDialog(parent)
    , mAttach(0)
{
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *mainWidget = new QWidget(this);
    mUI.setupUi(mainWidget);
    mUI.mProperties->setHeaderHidden(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *user1Button = new QPushButton;
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
    KConfigGroup group(KSharedConfig::openConfig(), "AttachPropertyDialog");
    const QSize size = group.readEntry("Size", QSize(500, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void AttachPropertyDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "AttachPropertyDialog");
    group.writeEntry("Size", size());
    group.sync();
}

void AttachPropertyDialog::setAttachment(KTNEFAttach *attach)
{
    QString s = attach->fileName().isEmpty()
                ? attach->name()
                : attach->fileName();
    mUI.mFilename->setText(QLatin1String("<b>") + s + QLatin1String("</b>"));
    setWindowTitle(i18nc("@title:window", "Properties for Attachment %1", s));
    mUI.mDisplay->setText(attach->displayName());
    mUI.mMime->setText(attach->mimeTag());
    s.setNum(attach->size());
    s.append(i18n(" bytes"));
    mUI.mSize->setText(s);
    QMimeDatabase db;
    QMimeType mimetype = db.mimeTypeForName(attach->mimeTag());
    QPixmap pix = loadRenderingPixmap(attach, qApp->palette().color(QPalette::Background));
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
        if (value.type() == QVariant::List) {
            newItem->setExpanded(true);
            newItem->setText(0,
                             newItem->text(0)
                             +QLatin1String(" [") + QString::number(value.toList().count()) + QLatin1Char(']'));
            int i = 0;
            QList<QVariant>::ConstIterator litEnd = value.toList().constEnd();
            for (QList<QVariant>::ConstIterator lit = value.toList().constBegin();
                 lit != litEnd; ++lit, ++i) {
                new QTreeWidgetItem(newItem,
                                    QStringList()
                                    << QLatin1Char('[')  + QString::number(i) + QLatin1Char(']')
                                    << QString(KTNEFProperty::formatValue(*lit)));
            }
        } else if (value.type() == QVariant::DateTime) {
            newItem->setText(1, value.toDateTime().toString());
        } else {
            newItem->setText(1, (*it)->valueString());
            newItem->setText(2, prefix + QLatin1Char('_') + QString::number(it.key()));
        }
    }
}

void AttachPropertyDialog::formatPropertySet(KTNEFPropertySet *pSet, QTreeWidget *lv)
{
    formatProperties(pSet->properties(), lv, 0, QStringLiteral("prop"));
    QTreeWidgetItem *item
        = new QTreeWidgetItem(lv,
                              QStringList(i18nc("@label", "TNEF Attributes")));
    item->setExpanded(true);
    formatProperties(pSet->attributes(), 0, item, QStringLiteral("attr"));
}

bool AttachPropertyDialog::saveProperty(QTreeWidget *lv, KTNEFPropertySet *pSet, QWidget *parent)
{
    QList<QTreeWidgetItem *> list = lv->selectedItems();
    if (list.isEmpty() || !list.first()) {
        KMessageBox::error(
            parent,
            i18nc("@info",
                  "Must select an item first."));
        return false;
    }

    QTreeWidgetItem *item = list.first();
    if (item->text(2).isEmpty()) {
        KMessageBox::error(
            parent,
            i18nc("@info",
                  "The selected item cannot be saved because it has an empty tag."));
    } else {
        QString tag = item->text(2);
        int key = tag.midRef(5).toInt();
        QVariant prop = (tag.startsWith(QStringLiteral("attr_"))
                         ? pSet->attribute(key)
                         : pSet->property(key));
        QString filename = QFileDialog::getSaveFileName(parent, QString(), tag, QString());
        if (!filename.isEmpty()) {
            QFile f(filename);
            if (f.open(QIODevice::WriteOnly)) {
                switch (prop.type()) {
                case QVariant::ByteArray:
                    f.write(prop.toByteArray().data(), prop.toByteArray().size());
                    break;
                default:
                {
                    QTextStream t(&f);
                    t << prop.toString();
                    break;
                }
                }
                f.close();
            } else {
                KMessageBox::error(
                    parent,
                    i18nc("@info",
                          "Unable to open file for writing, check file permissions."));
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
        quint16 type, w, h;
        rendStream >> type >> w >> w; // read type and skip 4 bytes
        rendStream >> w >> h;
        rendBuffer.close();

        if (type == 1 && w > 0 && h > 0) {
            // Load WMF data
            QWinMetaFile wmfLoader;
            QByteArray qb = wmf.toByteArray();
            QBuffer wmfBuffer(&qb);
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
