/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class KMKernel;

class KMCommandsTest : public QObject
{
    Q_OBJECT
public:
    explicit KMCommandsTest(QObject *parent = nullptr);
    ~KMCommandsTest() override;
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
    KMKernel *mKernel = nullptr;
};

