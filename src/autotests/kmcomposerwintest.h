/*
  SPDX-FileCopyrightText: 2021 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMCOMPOSERWINTEST_H
#define KMCOMPOSERWINTEST_H

#include <QObject>
#include <QDir>

class KMKernel;

class KMComposerWinTest : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerWinTest(QObject *parent = nullptr);
    ~KMComposerWinTest();
    
private Q_SLOTS:
    void init();
    void cleanup();
    void initTestCase();

    void testEncryption_data();
    void testEncryption();

    void testSignature_data();
    void testSignature();

    void testChangeIdentity();
private:
    void resetIdentities();
    KMKernel *mKernel = nullptr;
    QDir autocryptDir;
};

#endif // KMCOMPOSERWINTEST_H
