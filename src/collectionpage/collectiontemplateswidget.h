/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLLECTIONTEMPLATESWIDGET_H
#define COLLECTIONTEMPLATESWIDGET_H

#include <QWidget>
#include <AkonadiCore/Collection>
class QCheckBox;
namespace TemplateParser {
class TemplatesConfiguration;
}


class CollectionTemplatesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionTemplatesWidget(QWidget *parent = nullptr);
    ~CollectionTemplatesWidget();
    void save(const Akonadi::Collection &);
    void load(const Akonadi::Collection &col);
private:
    void slotCopyGlobal();
    void slotChanged();
    QCheckBox *mCustom = nullptr;
    TemplateParser::TemplatesConfiguration *mWidget = nullptr;
    QString mCollectionId;
    uint mIdentity = 0;
    bool mChanged = false;
};

#endif // COLLECTIONTEMPLATESWIDGET_H
