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


// required
#define VCARD_BEGIN          "begin:vcard"
#define VCARD_END            "end:vcard"
#define VCARD_BEGIN_N        "begin"
#define VCARD_END_N          "end"
#define VCARD_VERSION        "version"
// one of the following two??
#define VCARD_FN             "fn"
#define VCARD_N              "n"

// optional
#define VCARD_NAME           "name"
#define VCARD_NICKNAME       "nickname"
#define VCARD_PHOTO          "photo"
#define VCARD_BDAY           "bday"
#define VCARD_ADR            "adr"
  // types
  #define VCARD_ADR_DOM      "dom"
  #define VCARD_ADR_INTL     "intl"
  #define VCARD_ADR_POSTAL   "postal"
  #define VCARD_ADR_HOME     "home"
  #define VCARD_ADR_WORK     "work"
  #define VCARD_ADR_PREF     "pref"
  // values
  #define VCARD_ADR_POBOX    "PO Box"
  #define VCARD_ADR_EXTADR   "Extended Address"
  #define VCARD_ADR_STREET   "Street"
  #define VCARD_ADR_LOCALITY "Locality"
  #define VCARD_ADR_REGION   "Region"
  #define VCARD_ADR_POSTCODE "Postal Code"
  #define VCARD_ADR_COUNTRY  "Country Name"
#define VCARD_LABEL          "label"
#define VCARD_PROFILE        "profile"
#define VCARD_SOURCE         "source"
#define VCARD_TEL            "tel"
  // types
  #define VCARD_TEL_HOME     "home"
  #define VCARD_TEL_WORK     "work"
  #define VCARD_TEL_PREF     "pref"
  #define VCARD_TEL_VOICE    "voice"
  #define VCARD_TEL_FAX      "fax"
  #define VCARD_TEL_MSG      "msg"
  #define VCARD_TEL_CELL     "cell"
  #define VCARD_TEL_PAGER    "pager"
  #define VCARD_TEL_BBS      "bbs"
  #define VCARD_TEL_MODEM    "modem"
  #define VCARD_TEL_CAR      "car"
  #define VCARD_TEL_ISDN     "isdn"
  #define VCARD_TEL_VIDEO    "video"
  #define VCARD_TEL_PCS      "pcs"
#define VCARD_EMAIL          "email"
  // types
  #define VCARD_EMAIL_PREF    "pref"
  #define VCARD_EMAIL_INTERNET "internet"
  #define VCARD_EMAIL_X400   "x400"
#define VCARD_TZ             "tz"
#define VCARD_GEO            "geo"
#define VCARD_MAILER         "mailer"
#define VCARD_TITLE          "title"
#define VCARD_ROLE           "role"
#define VCARD_LOGO           "logo"
#define VCARD_AGENT          "agent"
#define VCARD_ORG            "org"
#define VCARD_CATEGORIES     "categories"
#define VCARD_NOTE           "note"
#define VCARD_PRODID         "prodid"
#define VCARD_REV            "rev"
#define VCARD_SOUND          "sound"
#define VCARD_UID            "uid"
#define VCARD_URL            "url"
#define VCARD_CLASS          "class"
  #define VCARD_CLASS_PUBLIC "public"
  #define VCARD_CLASS_PRIVATE "private"
  #define VCARD_CLASS_CONFIDENTIAL "confidential"
#define VCARD_KEY            "key"
  // types
  #define VCARD_KEY_X509     "x509"
  #define VCARD_KEY_PGP      "pgp"

#define VCARD_QUOTED_PRINTABLE "quoted-printable"
// this one is a temporary hack until we support TYPE=VALUE
#define VCARD_ENCODING_QUOTED_PRINTABLE "encoding=quoted-printable"
#define VCARD_BASE64           "base64"

/* X-xxxxx also usable */

#include <qstring.h>
#include <qvaluelist.h>

class VCardLine {
 friend class VCard;
 protected:
  QString         name;
  bool            qualified;
  QValueList<QString> qualifiers;
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
