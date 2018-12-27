/*
   Copyright (C) 2011-2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef TAGSELECTDIALOG_H
#define TAGSELECTDIALOG_H

#include <QDialog>
#include <AkonadiCore/Item>
#include <AkonadiCore/Tag>
#include "MailCommon/Tag"

class QListWidget;
class KActionCollection;
class TagSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagSelectDialog(QWidget *parent, int numberOfSelectedMessages, const Akonadi::Item &selectedItem);
    ~TagSelectDialog();
    Akonadi::Tag::List selectedTag() const;

    void setActionCollection(const QList<KActionCollection *> &actionCollectionList);

private:
    void slotAddNewTag();
    void slotTagsFetched(KJob *);
    void writeConfig();
    void readConfig();
    void createTagList(bool updateList);
    enum ItemType {
        UrlTag = Qt::UserRole + 1
    };
    int mNumberOfSelectedMessages = -1;
    Akonadi::Item mSelectedItem;

    Akonadi::Tag::List mCurrentSelectedTags;
    QList<MailCommon::Tag::Ptr> mTagList;
    QList<KActionCollection *> mActionCollectionList;
    QListWidget *mListTag = nullptr;
};

#endif /* TAGSELECTDIALOG_H */
