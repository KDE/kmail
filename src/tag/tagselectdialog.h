/*
   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "MailCommon/Tag"
#include "kmail_private_export.h"
#include <AkonadiCore/Item>
#include <AkonadiCore/Tag>
#include <QDialog>
#include <QVector>

class QListWidget;
class KActionCollection;
class KMAILTESTS_TESTS_EXPORT TagSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagSelectDialog(QWidget *parent, int numberOfSelectedMessages, const Akonadi::Item &selectedItem);
    ~TagSelectDialog() override;
    Q_REQUIRED_RESULT Akonadi::Tag::List selectedTag() const;

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
    QVector<MailCommon::Tag::Ptr> mTagList;
    QList<KActionCollection *> mActionCollectionList;
    QListWidget *const mListTag;
};

