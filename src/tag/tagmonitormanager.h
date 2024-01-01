/*
   SPDX-FileCopyrightText: 2020-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Akonadi/Tag>
#include <QObject>
#include <mailcommon/tag.h>
namespace Akonadi
{
class Monitor;
}
class KJob;

class TagMonitorManager : public QObject
{
    Q_OBJECT
public:
    explicit TagMonitorManager(QObject *parent = nullptr);
    ~TagMonitorManager() override;

    static TagMonitorManager *self();

    [[nodiscard]] QList<MailCommon::Tag::Ptr> tags() const;

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
    QList<MailCommon::Tag::Ptr> mTags;

    Akonadi::Monitor *const mMonitor;
};
