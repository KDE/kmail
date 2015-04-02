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


#include "configureagentlistdelegate.h"

#include <KIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <qcheckbox.h>

ConfigureAgentListDelegate::ConfigureAgentListDelegate(QAbstractItemView* itemView, QObject* parent) :
    KWidgetItemDelegate(itemView, parent)
{
}

ConfigureAgentListDelegate::~ConfigureAgentListDelegate()
{
}

QSize ConfigureAgentListDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(index);

    const QStyle *style = itemView()->style();
    const int buttonHeight = style->pixelMetric(QStyle::PM_ButtonMargin) * 2 +
                             style->pixelMetric(QStyle::PM_ButtonIconSize);
    const int fontHeight = option.fontMetrics.height();
    return QSize(100, qMax(buttonHeight, fontHeight));
}

void ConfigureAgentListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    Q_UNUSED(index);
    painter->save();

    itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.highlightedText().color());
    }

    painter->restore();
}

QList<QWidget*> ConfigureAgentListDelegate::createItemWidgets() const
{
    QCheckBox* checkBox = new QCheckBox();
    QPalette palette = checkBox->palette();
    palette.setColor(QPalette::WindowText, palette.color(QPalette::Text));
    checkBox->setPalette(palette);
    connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(slotCheckboxClicked(bool)));

    QPushButton* configureButton = new QPushButton();
    connect(configureButton, SIGNAL(clicked()), this, SLOT(slotConfigure()));

    return QList<QWidget*>() << checkBox << configureButton;
}

void ConfigureAgentListDelegate::updateItemWidgets(const QList<QWidget*> widgets,
                                              const QStyleOptionViewItem& option,
                                              const QPersistentModelIndex& index) const
{
    QCheckBox* checkBox = static_cast<QCheckBox*>(widgets[0]);
    QPushButton *configureButton = static_cast<QPushButton*>(widgets[1]);

    const int itemHeight = sizeHint(option, index).height();

    const QAbstractItemModel* model = index.model();
    checkBox->setText(model->data(index).toString());
    checkBox->setChecked(model->data(index, Qt::CheckStateRole).toBool());

    int checkBoxWidth = option.rect.width();
    checkBoxWidth -= configureButton->sizeHint().width();
    checkBox->resize(checkBoxWidth, checkBox->sizeHint().height());
    checkBox->move(0, (itemHeight - checkBox->height()) / 2);

    configureButton->setEnabled(checkBox->isChecked());
    configureButton->setIcon(KIcon(QLatin1String("configure")));
    configureButton->resize(configureButton->sizeHint());
    configureButton->move(option.rect.right() - configureButton->width(),
                          (itemHeight - configureButton->height()) / 2);
}

void ConfigureAgentListDelegate::slotCheckboxClicked(bool checked)
{
    QAbstractItemModel* model = const_cast<QAbstractItemModel*>(focusedIndex().model());
    model->setData(focusedIndex(), checked, Qt::CheckStateRole);
    Q_EMIT requestChangeAgentState(focusedIndex(), checked);
}

void ConfigureAgentListDelegate::slotConfigure()
{
    Q_EMIT requestConfiguration(focusedIndex());
}
