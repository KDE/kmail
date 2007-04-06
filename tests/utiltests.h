/*
 * Copyright (C)  2005  David Faure <faure@kde.org>
 *
 * This file is subject to the GPL version 2.
 */

#ifndef UTILTESTS_H
#define UTILTESTS_H

#include <qobject.h>

class UtilTester : public QObject
{
  Q_OBJECT

private slots:
  void test_lf2crlf();
  void test_crlf2lf();
  void test_escapeFrom();
  void test_DwStringConversions();
private:
  void test_DwStringConversions( const QByteArray& cstr );
};

#endif
