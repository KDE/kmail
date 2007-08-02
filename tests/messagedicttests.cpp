/**
 * Copyright (C)  2005 Till Adam <adam@kde.org>
 * This file is subject to the GPL version 2.
 */

#include <kdebug.h>
#include <kunittest/runner.h>
#include <kunittest/module.h>

#include "kmdict.h"

#include "messagedicttests.h"

static void p( const QString & str )
{
  kDebug(5006) << str;
}

void MessageDictTester::setUp()
{
    kDebug(5006) <<"setUp";
    m_dict = new KMDict( 4 ); // will be thrown away in init
}

void MessageDictTester::tearDown()
{
    kDebug(5006) <<"tearDown";  
    delete m_dict;
}

void MessageDictTester::testKMDictCreation()
{
    p("MessageDictTester::testKMDict()");
    p("Check creation with size of next prime: ");
    CHECK( m_dict->size(), 31 );
    m_dict->init( 13 ); // will be created with a 13, no nextPrime()
    CHECK( m_dict->size(), 13 );
}

void MessageDictTester::testKMDictInsert()
{
    p("Insert: ");
    KMDictItem *item = new KMDictItem();
    m_dict->insert( 12345, item );
    KMDictItem *found = m_dict->find( 12345 );
    CHECK( item, found);
}
 
void MessageDictTester::testKMDictRemove()
{
  p("Remove: ");
  m_dict->remove( 12345 );
  KMDictItem *item = m_dict->find( 12345 );
  CHECK( item, (KMDictItem*)0 );
}

void MessageDictTester::testKMDictClear()
{
    p("Check clear: ");
    for ( unsigned int i=0; i<11; ++i )
      m_dict->insert( i, new KMDictItem() );
    m_dict->clear();
    CHECK( m_dict->mVecs, (KMDictItem**)0 );
}

void MessageDictTester::testKMDictReplace()
{
  p("Check replace: ");
  m_dict->init( 31 );
  KMDictItem *oldItem = new KMDictItem();
  KMDictItem *newItem = new KMDictItem();
  m_dict->insert( 12345, oldItem );
  m_dict->replace( 12345, newItem );
  KMDictItem *found = m_dict->find( 12345 );
  CHECK( found, newItem );
}

#include "messagedicttests.moc"

