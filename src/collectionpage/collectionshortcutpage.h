/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2004 Till Adam <adam@kde.org>
  Copyright (c) 2013 Jonathan Marten <jjm@keelhaul.me.uk>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COLLECTIONSHORTCUTPAGE_H
#define COLLECTIONSHORTCUTPAGE_H

#include <MailCommon/FolderSettings>

#include <AkonadiWidgets/collectionpropertiespage.h>

template<typename T> class QSharedPointer;

class KKeySequenceWidget;

class CollectionShortcutPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionShortcutPage(QWidget *parent = nullptr);
    ~CollectionShortcutPage();

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private:
    void slotShortcutChanged();
    void init(const Akonadi::Collection &);
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    Akonadi::Collection mCurrentCollection;
    KKeySequenceWidget *mKeySeqWidget = nullptr;
    bool mShortcutChanged = false;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionShortcutPageFactory, CollectionShortcutPage)

#endif /* COLLECTIONSHORTCUTPAGE_H */
