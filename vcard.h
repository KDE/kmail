/**
 * vcard.h
 *
 * Copyright (c) 2000 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _KDE_VCARD_H
#define _KDE_VCARD_H

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

#define VCARD_QUALIFIER_BEGIN "type="
#define VCARD_ENCODING_BEGIN "encoding="
#define VCARD_QUOTED_PRINTABLE "quoted-printable"
#define VCARD_BASE64           "base64"
#define VCARD_B                "b"

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

//=========================================================
//
// class VCard
//
//=========================================================

/** This class implements support for parsing vCards. It does not
    make use of the versit VCard/VCalender classes, which are used
    elsewhere (by KOrganizer, for example)

    Every @p VCard object represents a visiting card with a lot of
    attributes to a person.

    @sect Creating vCard objects

    The usual way to construct a VCard object is by parsing
    it from the contents of a file (or from a message part):
    <pre>
      #include "vcard.h"
      ...
      VCard *vc = VCard::parseVCard(string);
    </pre>
    where @p string contains the complete vcard.
    On success, @p vc is different from @p 0 and refers to a valid VCard object.

    It should be possible to construct a VCard from scratch
    (using the primitives @p addLine() and
    @p addQualifiedLine(). However, vCards are not
    created this way in KMail.

    @sect Querying Data from a VCard

    Generally spoken, VCard data is organized by keywords and,
    optionally, qualifiers. For example, a telephone number can
    be just `the' telephone number of the person, or it can
    be a special telephone number, let's say, that of
    the cellular.

    The methods getValue(...) and getValues(...) can be used
    to query data from a VCard object, either by querying by
    keyword, e.g.
    <pre>
      QString s = vc->getValue(VCARD_TEL);
    </pre>
    or by keyword and qualifier, e.g.
    <pre>
      QString s = vc->getValue(VCARD_TEL, VCARD_TEL_CELL);
    </pre>
    There are also entrys (like name, address of the person)
    which do not consist of one single text string, but rather
    consist of a number of strings. These entries are then
    queried by the corresponding @p getValues() methods.

    Again, querying by keyword
    <pre>
      QStringList l = vc->getValues(VCARD_ADR);
    </pre>
    or querying by keyword and qualifier, e.g.
    <pre>
      QStringList l = vc->getValues(VCARD_ADR, VCARD_ADR_HOME);
    </pre>
    is possible.

    Of course, one has to know which entry has to be queried
    by @p getValue() and which one by @p getValues(). Have a look at
    the reference section below or at @ref kmdisplayvcard.cpp for
    concrete examples.

    @sect Reference: Codes for querying @p VCard attributes

    The calls to @p getValue and @p getValues, resp., look like
    <pre>
      getValue([key]);
      getValue([key], [qualifier]);
    </pre>
    A list of keys and their qualifiers follows below.

    @p VCARD_VERSION
      vCard Version.
      Use @p getValue().

    @p VCARD_NAME
      Name of the person.
      Use @p getValues().
      Values come in the following order:
      last name, first name, middle name, prefix, jr..

    @p VCARD_NICKNAME        -
      Nickname of the person.
      Use @p getValue().

    @p VCARD_PHOTO           -
      Photo of the person.
      Use @p getValue().

    @p VCARD_BDAY            -
      Birthday of the person.
      Use @p getValue().

    @p VCARD_ADR             -
      Address of the person.
      Use @p getValues().
      Values come in the following order:
      P.O. box, extended address, street, city, province, zip code, country
      Types:
      @li @p VCARD_ADR_DOM       -
        type qualifier to @p VCARD_ADR: Domestic address
      @li @p VCARD_ADR_INTL      -
        type qualifier to @p VCARD_ADR: International address
      @li @p VCARD_ADR_POSTAL    -
        type qualifier to @p VCARD_ADR: Postal address
      @li @p VCARD_ADR_HOME      -
        type qualifier to @p VCARD_ADR: Home/private address
      @li @p VCARD_ADR_WORK      -
        type qualifier to @p VCARD_ADR: Address at work
      @li @p VCARD_ADR_PREF      -
        type qualifier to @p VCARD_ADR: Preferred address

      Single values can be queried by @p getValue(VCARD_ADR,[qualifier]),
      using the following value names. However, in that case,
      the default address is used (type cannot be specified)
      @li @p VCARD_ADR_POBOX     - P.O. Box field of the address entry
      @li @p VCARD_ADR_EXTADR    - extended address
      @li @p VCARD_ADR_STREET    - street of the address entry
      @li @p VCARD_ADR_LOCALITY  - refers to the city or village
      @li @p VCARD_ADR_REGION    - refers to the region or province, resp.
      @li @p VCARD_ADR_POSTCODE  - field for the zip or post code, resp.
      @li @p VCARD_ADR_COUNTRY   - refers to the country

    @p VCARD_LABEL           -  Label

    @p VCARD_PROFILE         -  Profile

    @p VCARD_SOURCE          -  Source

    @p VCARD_TEL             -
      Telephone number of the person.
      Use @p getValue().

      The following type qualifiers may be used for @p VCARD_TEL
      @li @p VCARD_TEL_HOME      - telephone at home
      @li @p VCARD_TEL_WORK      - dto. at work
      @li @p VCARD_TEL_PREF      - preferred telephone number
      @li @p VCARD_TEL_VOICE     -
      @li @p VCARD_TEL_FAX       - fax number
      @li @p VCARD_TEL_MSG       -
      @li @p VCARD_TEL_CELL      - number of cellular phone
      @li @p VCARD_TEL_PAGER     -
      @li @p VCARD_TEL_BBS       -
      @li @p VCARD_TEL_MODEM     -
      @li @p VCARD_TEL_CAR       -
      @li @p VCARD_TEL_ISDN      -
      @li @p VCARD_TEL_VIDEO     -
      @li @p VCARD_TEL_PCS       -

    @p VCARD_EMAIL           -
      email address of the person.
      Use @p getValue().
      The following type qualifiers may be used
      @li @p VCARD_EMAIL_PREF     -
        preferred email address
      @li @p VCARD_EMAIL_INTERNET  -
        internet email address
      @li @p VCARD_EMAIL_X400    -
        x400 email address

    @p VCARD_TZ              - Time zone

    @p VCARD_GEO             -

    @p VCARD_MAILER          -

    @p VCARD_TITLE           - Title of the person

    @p VCARD_ROLE            - Role of the person in the organization

    @p VCARD_LOGO            -

    @p VCARD_AGENT           -

    @p VCARD_ORG             - Organization the person belongs to. Use @p getValues().
    Values come in the following order: organization, department

    @p VCARD_CATEGORIES      -

    @p VCARD_NOTE            -

    @p VCARD_PRODID          -

    @p VCARD_REV             -

    @p VCARD_SOUND           -

    @p VCARD_UID             -

    @p VCARD_URL             -
      Homepage URL of the person.
      Use @p getValue().

    @p VCARD_CLASS           -
      @li @p VCARD_CLASS_PUBLIC  -
      @li @p VCARD_CLASS_PRIVATE  -
      @li @p VCARD_CLASS_CONFIDENTIAL  -

    @p VCARD_KEY             -
      Signature key of the person.
      Use @p getValue().
      The following type qualifiers may be used
      @li @p VCARD_KEY_X509      -
      @li @p VCARD_KEY_PGP       -

    @short This class implements support for parsing vCards
    @author George Staikos <staikos@kde.org>.
    @see KMDisplayVCard
*/
class VCard {
 friend class VCardLine;

 public:
  /** create a new, blank vCard */
  VCard();
  ~VCard();

  /** this parses a vcard in a string and if it's valid, returns
      a new VCard object, else returns 0 and sets err if it is not 0 */
  static VCard *parseVCard(const QString& vc, int *err = 0);
  static QString getError(int err);
  
  /** these add a new entry with a single value */
  int addLine(const QString& name, const QString& value);
  int addQualifiedLine(const QString& name, const QString& qualifier, const QString& value);
  /** these add a new entry with multiple values (ie first;last;initial) */
  int addLine(const QString& name, const QValueList<QString>& value);
  int addQualifiedLine(const QString& name, const QString& qualifier, const QValueList<QString>& value);
  /** these remove an entry from the vCard */
  bool removeLine(const QString& name);
  bool removeQualifiedLine(const QString& name, const QString& qualifier);

  /** these query the card values */
  QString getValue(const QString& name, const QString& qualifier);
  QString getValue(const QString& name);
  QValueList<QString> getValues(const QString& name, const QString& qualifier);
  QValueList<QString> getValues(const QString& name);

  /** this clears all entries */
  void clean();

  /** this returns the vCard as a string */
  QString getVCard() const;
  inline QString operator()() const { return getVCard(); }

 private:
  VCard(QValueList<VCardLine> *_vcd);

 protected:
  QValueList<VCardLine> *_vcdata;
};


#endif
