/*
 * Copyright (C)  2005  Till Adam <adam@kde.org>
 *
 * This file is subject to the GPL version 2.
 */

#ifndef MESSAGEDICTTESTS_H
#define MESSAGEDICTTESTS_H

#include <qobject.h>

class KMDict;

class MessageDictTester : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_KMDictCreation();
    void test_KMDictInsert();
    void test_KMDictRemove();
    void test_KMDictClear();
    void test_KMDictReplace();
private:
    KMDict *m_dict;
};

#endif
