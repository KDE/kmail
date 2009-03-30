/**
 * Copyright (C)  2005 Till Adam <adam@kde.org>
 * This file is subject to the GPL version 2.
 */

#include "messagedicttests.h"

#include "kmdict.h"

#include <kdebug.h>
#include "qtest_kde.h"

QTEST_KDEMAIN_CORE( MessageDictTester )

void MessageDictTester::initTestCase()
{
    m_dict = new KMDict( 4 ); // will be thrown away in init
}

void MessageDictTester::cleanupTestCase()
{
    delete m_dict;
}

void MessageDictTester::testKMDictCreation()
{
    kDebug(5006) << "Check creation with size of next prime: ";
    QCOMPARE( m_dict->size(), 31 );
    m_dict->init( 13 ); // will be created with a 13, no nextPrime()
    QCOMPARE( m_dict->size(), 13 );
}

void MessageDictTester::testKMDictInsert()
{
    kDebug(5006) << "Insert: ";
    KMDictItem *item = new KMDictItem();
    m_dict->insert( 12345, item );
    KMDictItem *found = m_dict->find( 12345 );
    QCOMPARE( item, found);
}
 
void MessageDictTester::testKMDictRemove()
{
  kDebug(5006) << "Remove: ";
  m_dict->remove( 12345 );
  KMDictItem *item = m_dict->find( 12345 );
  QCOMPARE( item, (KMDictItem*)0 );
}

void MessageDictTester::testKMDictClear()
{
  kDebug(5006) << "Check clear: ";
  for ( unsigned int i=0; i<11; ++i )
    m_dict->insert( i, new KMDictItem() );
  m_dict->clear();
  QCOMPARE( m_dict->mVecs, (KMDictItem**)0 );
}

void MessageDictTester::testKMDictReplace()
{
  kDebug(5006) << "Check replace: ";
  m_dict->init( 31 );
  KMDictItem *oldItem = new KMDictItem();
  KMDictItem *newItem = new KMDictItem();
  m_dict->insert( 12345, oldItem );
  m_dict->replace( 12345, newItem );
  KMDictItem *found = m_dict->find( 12345 );
  QCOMPARE( found, newItem );
}

#include "messagedicttests.moc"

