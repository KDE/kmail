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
  int addLine(QString& name, QString& value);
  int addQualifiedLine(QString& name, QString& qualifier, QString& value);
  // these add a new entry with multiple values (ie first;last;initial)
  int addLine(QString& name, QValueList<QString>& value);
  int addQualifiedLine(QString& name, QString& qualifier, QValueList<QString>& value);
  // these remove an entry from the vCard
  bool removeLine(QString& name);
  bool removeQualifiedLine(QString& name, QString& qualifier);

  // these query the card values
  QString getValue(QString& name, QString& qualifier);
  QString getValue(QString& name);
  QValueList<QString> getValues(QString& name, QString& qualifier);
  QValueList<QString> getValues(QString& name);

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
