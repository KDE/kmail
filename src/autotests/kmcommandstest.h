/*
  Copyright (c) 2016 Sandro Knauß <sknauss@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef KMCOMMANDSTEST_H
#define KMCOMMANDSTEST_H

#include <QObject>

class KMKernel;

class KMCommandsTest : public QObject
{
    Q_OBJECT
public:
    explicit KMCommandsTest(QObject *parent = nullptr);
    ~KMCommandsTest();
private Q_SLOTS:
    void testMailtoReply();
    void testReply();
    void testReplyWithoutDefaultGPGSign();
    void initTestCase();
private:
    void resetIdentities();
    void verifySignature(bool sign);
    void verifyEncryption(bool encrypt);
    void waitForMainWindowToClose();
    KMKernel *mKernel;
};

#endif // KMCOMMANDSTEST_H