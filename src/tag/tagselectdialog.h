/*
   SPDX-FileCopyrightText: 2011-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "MailCommon/Tag"
#include "kmail_private_export.h"
#include <Akonadi/Item>
#include <Akonadi/Tag>
#include <QDialog>
#include <QList>

class QListWidget;
class KActionCollection;
class KMAILTESTS_TESTS_EXPORT TagSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagSelectDialog(QWidget *parent, int numberOfSelectedMessages, const Akonadi::Item &selectedItem);
    ~TagSelectDialog() override;
    [[nodiscard]] Akonadi::Tag::List selectedTag() const;

    void setActionCollection(const QList<KActionCollection *> &actionCollectionList);

private:
    void slotAddNewTag();
    void slotTagsFetched(KJob *);
    void writeConfig();
    void readConfig();
    void createTagList(bool updateList);
    enum ItemType { UrlTag = Qt::UserRole + 1 };
    const int mNumberOfSelectedMessages = -1;
    const Akonadi::Item mSelectedItem;

    Akonadi::Tag::List mCurrentSelectedTags;
    QList<MailCommon::Tag::Ptr> mTagList;
    QList<KActionCollection *> mActionCollectionList;
    QListWidget *const mListTag;
};
