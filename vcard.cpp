#include "vcard.h"
#include <stdlib.h>
#include <ctype.h>
#include <klocale.h>

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
// Requires QT.
//

// FIXME: do proper CRLF/CR handling

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

/* X-xxxxx also usable */

/*

  Parser structure:

vcard := <begin><body><end> | <begin><end>
body  := <line><body> | <line>
begin := begin:vcard
end   := end:vcard
line  := <name>;<qualifier>:<value> | <name>:<value>
name  := 
qualifier :=
value := 

*/


// This code is screaming "come do me in PERL!"
// This STRIPS OUT EMPTY TOKENS
//static QValueList<QString> tokenizeBy(const QString& str, char tok) {
static QValueList<QString> tokenizeBy(const QString& str, const QRegExp& tok) {
QValueList<QString> tokens;
unsigned int head, tail;
const char *chstr = str.ascii();
unsigned int length = str.length();

  if (length < 1) return tokens;

  if (length == 1) {
    tokens.append(str);
    return tokens;
  }

  for(head = 0, tail = 0; tail < length-1; head = tail+1) {
    QString thisline;

    tail = str.find(tok, head);

    if (tail > length)           // last token - none at end
      tail = length;

    if (tail-head > 0) {    // it has to be at least 1 long!
      thisline = &(chstr[head]);
      thisline.truncate(tail-head);
      tokens.append(thisline);
    }
  }
return tokens;
}


VCard::VCard() {
  _vcdata = new QValueList<VCardLine>;
}


VCard::~VCard() {
  delete _vcdata;
}


VCard::VCard(QValueList<VCardLine> *_vcd) : _vcdata(_vcd) {

}


#define VC_ERR_NO_BEGIN            1
#define VC_ERR_NO_END              2
#define VC_ERR_INVALID_LINE        3
#define VC_ERR_INTERNAL            4
#define VC_ERR_INVALID_NAME        5
#define VC_ERR_MISSING_MANDATORY   6

// static
QString VCard::getError(int err) {
    switch(err) {
    case VC_ERR_INVALID_LINE:
      return i18n("The vCard line was invalid.");
    case VC_ERR_INTERNAL:
      return i18n("An unknown internal error occurred.  Please report to kmail@kde.org");
    case VC_ERR_INVALID_NAME:
      return i18n("The vCard contained an invalid field.");
    case VC_ERR_MISSING_MANDATORY:
      return i18n("The vCard is missing a mandatory field.");
    case VC_ERR_NO_BEGIN:
      return i18n("No beginning was found.");
    case VC_ERR_NO_END:
      return i18n("No end was found.");
    case 0:
      return i18n("No error.");
    default:
      return i18n("Invalid error number.");
    }
}


// thses are BITS  (1,2,4,8,16,...) !
#define VC_STATE_BEGIN          1
#define VC_STATE_BODY           2
#define VC_STATE_END            4
#define VC_STATE_HAVE_N         8
#define VC_STATE_HAVE_VERSION  16

//
// This class is not particularily optimized.  This is by design.
// The code is rather complicated (much token finding/parsing) and
// it's much easier to understand in this form.  It is still a
// good candidate for optimzation in the future though. (once the code
// has been proven and stabilized.)
//

// static
VCard *VCard::parseVCard(const QString& vc, int *err) {
int _err = 0;
int _state = VC_STATE_BEGIN;
QValueList<VCardLine> *_vcdata;
QValueList<QString> lines;

  _vcdata = new QValueList<VCardLine>;

  // lines = tokenizeBy(vc, '\n');
  lines = tokenizeBy(vc, QRegExp("[\x0d\x0a]"));

  // for each line in the vCard
  for (QValueListIterator<QString> j = lines.begin();
                                   j != lines.end(); ++j) {
    VCardLine _vcl;

    // take spaces off the end - ugly but necessary hack
    for (int g = (*j).length()-1; g > 0 && (*j)[g].isSpace(); g++)
      (*j)[g] = 0;

    //        first token:
    //              verify state, update if necessary
    if (_state & VC_STATE_BEGIN) {
      if (!strcasecmp(*j, VCARD_BEGIN)) {
        _state = VC_STATE_BODY;
        continue;
      } else {
        _err = VC_ERR_NO_BEGIN;
        break;
      }
    } else if (_state & VC_STATE_BODY) {
      if (!strcasecmp(*j, VCARD_END)) {
        _state |= VC_STATE_END;
        break;
      }

      // split into two tokens
      // QValueList<QString> linetokens = tokenizeBy(*j, QRegExp(":"));
      unsigned int tail = (*j).find(':', 0);
      if (tail > (*j).length()) {  // invalid line - no ':'
        _err = VC_ERR_INVALID_LINE;
        break;
      }

      QValueList<QString> linetokens;
      QString tmplinetoken;
      tmplinetoken = (*j);
      tmplinetoken.truncate(tail);
      linetokens.append(tmplinetoken);
      tmplinetoken = &((*j).ascii()[tail+1]); 
      linetokens.append(tmplinetoken);

      // check for qualifier and
      // set name, qualified, qualifier
      //QValueList<QString> nametokens = tokenizeBy(linetokens[0], ';');
      QValueList<QString> nametokens = tokenizeBy(linetokens[0], QRegExp(";"));
      bool qp = false, first_pass = true;

      if (nametokens.count() > 0) {
        _vcl.qualified = false;
        _vcl.name = nametokens[0];
        for (QValueListIterator<QString> z = nametokens.begin();
                                         z != nametokens.end();
                                         ++z) {
          if (*z == VCARD_QUOTED_PRINTABLE) {
            qp = true;
	  } else if (!first_pass && !_vcl.qualified) {
            _vcl.qualified = true;
            _vcl.qualifier = *z;
          }
          first_pass = false;
	}
      } else {
        _err = VC_ERR_INVALID_LINE;
      }
 
      if (_err != 0) break;

      if (_vcl.name == VCARD_VERSION)
        _state |= VC_STATE_HAVE_VERSION;

      if (_vcl.name == VCARD_N || _vcl.name == VCARD_FN)
        _state |= VC_STATE_HAVE_N;

      // second token:
      //    split into tokens by ;
      //    add to parameters vector
      //_vcl.parameters = tokenizeBy(linetokens[1], ';');
      _vcl.parameters = tokenizeBy(linetokens[1], QRegExp(";"));
      if (qp) {
        for (QValueListIterator<QString> z = _vcl.parameters.begin();
                                         z != _vcl.parameters.end();
	                                 ++z) {
          _vcl.qpDecode(*z);
	}
      }
    } else {
      _err = VC_ERR_INTERNAL;
      break;
    }

    // validate VCardLine
    if (!_vcl.isValid()) {
      _err = VC_ERR_INVALID_LINE;
      break;
    }

    // add to vector
    _vcdata->append(_vcl);
  }

  // errors to check at the last minute (exit state related)
  if (_err == 0) {
    if (!(_state & VC_STATE_END))         // we have to have an end!!
      _err = VC_ERR_NO_END;

    if (!(_state & VC_STATE_HAVE_N) ||    // we have to have the mandatories!
        !(_state & VC_STATE_HAVE_VERSION))
      _err = VC_ERR_MISSING_MANDATORY;
  }

  // set the error message if we can, and only return an object
  // if the vCard was valid.

  if (err)
    *err = _err;

  if (_err != 0) {
    delete _vcdata;
    return NULL;
  }

  return new VCard(_vcdata);
}



void VCard::clean() {
  _vcdata->clear();
}


bool VCard::removeLine(const QString& name) {
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name  &&  !(*i).qualified) {
      _vcdata->remove(i);
      return true;
    }
  }
return false;
}


bool VCard::removeQualifiedLine(const QString& name, const QString& qualifier) {
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && (*i).qualifier == qualifier) {
      _vcdata->remove(i);
      return true;
    }
  }
return false;
}


int VCard::addLine(const QString& name, const QString& value) {
QValueList<QString> values;

  values.append(value);

return addLine(name, values);
}


int VCard::addQualifiedLine(const QString& name, const QString& qualifier, const QString& value) {
QValueList<QString> values;

  values.append(value);

return addQualifiedLine(name, qualifier, values);
}


int VCard::addLine(const QString& name, const QValueList<QString>& value) {
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && !(*i).qualified) {
      (*i).parameters = value;
      if ((*i).isValid()) {
        return 0;
      }
      _vcdata->remove(i);
      return VC_ERR_INVALID_LINE;
    }
  }
VCardLine vcl;
  vcl.name = name;
  vcl.qualified = false;
  vcl.parameters = value;
  _vcdata->append(vcl);
return false;
}


int VCard::addQualifiedLine(const QString& name, const QString& qualifier, const QValueList<QString>& value) {
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && (*i).qualified && (*i).qualifier == qualifier) {
      (*i).parameters = value;
      if ((*i).isValid()) {
        return 0;
      }
      _vcdata->remove(i);
      return VC_ERR_INVALID_LINE;
    }
  }

VCardLine vcl;
  vcl.name = name;
  vcl.qualifier = qualifier;
  vcl.qualified = true;
  vcl.parameters = value;
  _vcdata->append(vcl);
return 0;
}


QString VCard::getValue(const QString& name, const QString& qualifier) {
QString failed = "";

  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && (*i).qualified && (*i).qualifier == qualifier) {
      if ((*i).parameters.count() > 0)
        return (*i).parameters[0];
      else return failed;
    }
  }
return failed;
}


QString VCard::getValue(const QString& name) {
QString failed = "";

  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && !(*i).qualified) {
      if ((*i).parameters.count() > 0)
        return (*i).parameters[0];
      else return failed;
    }
  }
return failed;
}


QValueList<QString> VCard::getValues(const QString& name, const QString& qualifier) {
QString failedstr = "";
QValueList<QString> failed;
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && (*i).qualified && (*i).qualifier == qualifier) {
      return (*i).parameters;
    }
  }
failed.append(failedstr);
return failed;
}


QValueList<QString> VCard::getValues(const QString& name) {
QString failedstr = "";
QValueList<QString> failed;
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == name && !(*i).qualified) {
      return (*i).parameters;
    }
  }
failed.append(failedstr);
return failed;
}



QString VCard::getVCard() const {
QString vct = VCARD_BEGIN;
int cnt, cur;

  vct += "\n";

  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    QString parms = "";
    vct += (*i).name;
    if ((*i).qualified) {
      vct += ";";
      vct += (*i).qualifier;
    }
    cnt = (*i).parameters.count();
    cur = 0;
    for (QValueListIterator<QString> j = (*i).parameters.begin();
                                     j != (*i).parameters.end();
	                             ++j) {
      QString qpj = *j;
      (*i).qpEncode(qpj);
      if (qpj != *j) {
        vct += ";";
        vct += VCARD_QUOTED_PRINTABLE;  // Netscape prints it multiple times.
      }                                 // I don't know if this is necessary.
      parms += qpj;
      if (++cur < cnt)
        parms += ";";
    }
    vct += ":";
    vct += parms;
    vct += "\n";
  }

  vct += VCARD_END;
  vct += "\n";
return vct;
}


//
// Characters 0x20->0x7E are considered "printable".  The rest must be
// encoded.   0x3B, 0x3D are also special (not printable).  Any others?
//

void VCardLine::qpEncode(QString& x) {
QString y = x;
int c;

  x = "";
  c = y.length();

  for (int i = 0; i < c; i++) {
    if (y[i].latin1() == 0x13 && y[i+1].latin1() != 0x10) {
      x += "=0D=0A";
    } else if (y[i].latin1() >  0x7e ||
               y[i].latin1() <  0x20 ||
               y[i].latin1() == 0x3B ||
               y[i].latin1() == 0x3D   ) {
      QString z;
      z.sprintf("=%.2X", y[i].latin1());
      x += z;
    } else {
      x += y[i];
    }
  }  
}


void VCardLine::qpDecode(QString& x) {
QString y = x;
int c;

  x = "";
  c = y.length();

  for (int i = 0; i < c; i++) {
    if (y[i] == '=') {
      char p = y[++i].latin1();
      char q = y[++i].latin1();
      x += (char) ((p <= '9' ? p - '0': p - 'A' + 10)*16 + 
                   (q <= '9' ? q - '0': q - 'A' + 10));
    } else {
      x += y[i];
    }
  }  
}



// true if the line is a valid vcard line
bool VCardLine::isValid() const {

  // invalid: if it is "begin:vcard" or "end:vcard"
  if (name == VCARD_BEGIN_N || name == VCARD_END_N)
    return false;

  if (name[0] == 'x' && name[1] == '-')      // a custom x- line
    return true;

  // this is long but it makes it a bit faster (and saves me from using
  // a trie which is probably the ideal situation, but a bit memory heavy)
  switch(name[0]) {
  case 'a':
    // GS - this seems to not be necessary? - netscape doesn't do it
//     if (name == VCARD_ADR && qualified &&
//                             (qualifier == VCARD_ADR_DOM    ||
//                              qualifier == VCARD_ADR_INTL   ||
//                              qualifier == VCARD_ADR_POSTAL ||
//                              qualifier == VCARD_ADR_HOME   ||
//                              qualifier == VCARD_ADR_WORK   ||
//                              qualifier == VCARD_ADR_PREF
//                             ))
    if (name == VCARD_ADR)
      return true;
    if (name == VCARD_AGENT)
      return true;
   break;
  case 'b':
    if (name == VCARD_BDAY)
      return true;
   break;
  case 'c': 
    if (name == VCARD_CATEGORIES)
      return true;
    if (name == VCARD_CLASS && qualified &&
                              (qualifier == VCARD_CLASS_PUBLIC       ||
                               qualifier == VCARD_CLASS_PRIVATE      ||
                               qualifier == VCARD_CLASS_CONFIDENTIAL
                              ))
      return true;
   break;
  case 'd':
   break;
  case 'e':
    if (name == VCARD_EMAIL && qualified &&
                              (qualifier == VCARD_EMAIL_INTERNET ||
                               qualifier == VCARD_EMAIL_X400
                              ))
      return true;
   break;
  case 'f':
    if (name == VCARD_FN)
      return true;
   break;
  case 'g':
    if (name == VCARD_GEO)
      return true;
   break;
  case 'h':
   break;
  case 'i':
   break;
  case 'j':
   break;
  case 'k':
    if (name == VCARD_KEY && qualified &&
                            (qualifier == VCARD_KEY_X509 ||
                             qualifier == VCARD_KEY_PGP
                            ))
      return true;
   break;
  case 'l':
    if (name == VCARD_LABEL)
      return true;
    if (name == VCARD_LOGO)
      return true;
   break;
  case 'm':
    if (name == VCARD_MAILER)
      return true;
   break;
  case 'n':
    if (name == VCARD_N)
      return true;
    if (name == VCARD_NAME)
      return true;
    if (name == VCARD_NICKNAME)
      return true;
    if (name == VCARD_NOTE)
      return true;
   break;
  case 'o':
    if (name == VCARD_ORG)
      return true;
   break;
  case 'p':
    if (name == VCARD_PHOTO)
      return true;
    if (name == VCARD_PROFILE)
      return true;
    if (name == VCARD_PRODID)
      return true;
   break;
  case 'q':
   break;
  case 'r': 
    if (name == VCARD_ROLE)
      return true;
    if (name == VCARD_REV)
      return true;
   break;
  case 's':
    if (name == VCARD_SOURCE)
      return true;
    if (name == VCARD_SOUND)
      return true;
   break;
  case 't':
    if (name == VCARD_TEL && qualified && 
                            (qualifier == VCARD_TEL_HOME  ||
                             qualifier == VCARD_TEL_WORK  ||
                             qualifier == VCARD_TEL_PREF  ||
                             qualifier == VCARD_TEL_VOICE ||
                             qualifier == VCARD_TEL_FAX   ||
                             qualifier == VCARD_TEL_MSG   ||
                             qualifier == VCARD_TEL_CELL  ||
                             qualifier == VCARD_TEL_PAGER ||
                             qualifier == VCARD_TEL_BBS   ||
                             qualifier == VCARD_TEL_MODEM ||
                             qualifier == VCARD_TEL_CAR   ||
                             qualifier == VCARD_TEL_ISDN  ||
                             qualifier == VCARD_TEL_VIDEO ||
                             qualifier == VCARD_TEL_PCS
                            ))
      return true;
    if (name == VCARD_TZ)
      return true;
    if (name == VCARD_TITLE)
      return true;
   break;
  case 'u':
    if (name == VCARD_URL)
      return true;
    if (name == VCARD_UID)
      return true;
   break;
  case 'v':
    if (name == VCARD_VERSION)
      return true;
   break;
  case 'w':
   break;
  case 'x':
   break;
  case 'y':
   break;
  case 'z':
   break;
  default:
   break;
  }

return false;
}

