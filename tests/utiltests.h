/*
 * Copyright (C)  2007  David Faure <faure@kde.org>
 *
 * This file is subject to the GPL version 2.
 */

#ifndef UTILTESTS_H
#define UTILTESTS_H

#include <kunittest/tester.h>

class UtilTester : public KUnitTest::SlotTester
{
  Q_OBJECT

public slots:
  void setUp();
  void tearDown();
  void test_lf2crlf();
  void test_crlf2lf();
  void test_escapeFrom();
  void test_append();
  void test_insert();
  void test_DwStringConversions();
  void test_QByteArrayQCString();
private:
  void test_DwStringConversions( const QCString& cstr );
};

#endif
