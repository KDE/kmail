#ifndef _KDE_VCARD_H
#define _KDE_VCARD_H

//
// VCard class to handle creation and parsing of Netscape
// vcards as per RFC 2426 (and possibly more)
//
// Copyright 2000, George Staikos <staikos@kde.org>
//
// Released under GPL.
//
// Also freely useable under the QPL.
//
// Requires QT 2.1+
//

#include <qstring.h>
#include <qvaluelist.h>

class VCardLine {
 friend class VCard;
 protected:
  QString         name;
  bool            qualified;
  QString         qualifier;
  QValueList<QString> parameters;
  bool isValid() const;
  void qpEncode(QString& x);
  void qpDecode(QString& x);
};



class VCard {
 friend class VCardLine;

 public:
  VCard();       // create a new, blank vCard
  ~VCard();

  // this parses a vcard in a string and if it's valid, returns
  // a new VCard object, else returns NULL and sets err if it is not NULL
  static VCard *parseVCard(const QString& vc, int *err = NULL);
  static QString getError(int err);
  
  // these add a new entry with a single value
  int addLine(const QString& name, const QString& value);
  int addQualifiedLine(const QString& name, const QString& qualifier, const QString& value);
  // these add a new entry with multiple values (ie first;last;initial)
  int addLine(const QString& name, const QValueList<QString>& value);
  int addQualifiedLine(const QString& name, const QString& qualifier, const QValueList<QString>& value);
  // these remove an entry from the vCard
  bool removeLine(const QString& name);
  bool removeQualifiedLine(const QString& name, const QString& qualifier);

  // these query the card values
  QString getValue(const QString& name, const QString& qualifier);
  QString getValue(const QString& name);
  QValueList<QString> getValues(const QString& name, const QString& qualifier);
  QValueList<QString> getValues(const QString& name);

  // this clears all entries
  void clean();

  // this returns the vCard as a string
  QString getVCard() const;
  inline QString operator()() const { return getVCard(); }

 private:
  VCard(QValueList<VCardLine> *_vcd);

 protected:
  QValueList<VCardLine> *_vcdata;
};


#endif
