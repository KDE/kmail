/*
   SPDX-FileCopyrightText: 2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TAGMONITORMANAGER_H
#define TAGMONITORMANAGER_H

#include <QObject>
#include <AkonadiCore/Tag>
#include <mailcommon/tag.h>
namespace Akonadi {
class Monitor;
}
class KJob;

class TagMonitorManager : public QObject
{
    Q_OBJECT
public:
    explicit TagMonitorManager(QObject *parent = nullptr);
    ~TagMonitorManager();

    static TagMonitorManager *self();


    Q_REQUIRED_RESULT QVector<MailCommon::Tag::Ptr> tags() const;

Q_SIGNALS:
    void tagAdded();
    void tagChanged();
    void tagRemoved();
    void fetchTagDone();

private:
    void createActions();
    void finishedTagListing(KJob *job);
    void onTagAdded(const Akonadi::Tag &akonadiTag);
    void onTagRemoved(const Akonadi::Tag &akonadiTag);
    void onTagChanged(const Akonadi::Tag &akonadiTag);

    // A sorted list of all tags
    QVector<MailCommon::Tag::Ptr> mTags;

    Akonadi::Monitor *const mMonitor;
};

#endif // TAGMONITORMANAGER_H
