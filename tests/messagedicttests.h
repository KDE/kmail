/*
 * Copyright (C)  2005  Till Adam <adam@kde.org>
 *
 * This file is subject to the GPL version 2.
 */

#ifndef MESSAGEDICTTESTS_H
#define MESSAGEDICTTESTS_H

#include <kunittest/tester.h>

class KMDict;

class MessageDictTester : public KUnitTest::SlotTester
{
    Q_OBJECT

public slots:
    void setUp();
    void tearDown();
    void testKMDictCreation();
    void testKMDictInsert();
    void testKMDictRemove();
    void testKMDictClear();
    void testKMDictReplace();
private:
    KMDict *m_dict;
};

#endif
