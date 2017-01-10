/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifndef SAVEDRAFTJOB_H
#define SAVEDRAFTJOB_H

#include <KJob>
#include <AkonadiCore/Collection>
#include <KMime/KMimeMessage>

class SaveDraftJob : public KJob
{
    Q_OBJECT
public:
    explicit SaveDraftJob(const KMime::Message::Ptr &msg, const Akonadi::Collection &col, QObject *parent = nullptr);
    ~SaveDraftJob();

    void start() Q_DECL_OVERRIDE;
private Q_SLOTS:
    void slotStoreDone(KJob *job);
private:
    KMime::Message::Ptr mMsg;
    Akonadi::Collection mCollection;
};

#endif // SAVEDRAFTJOB_H
