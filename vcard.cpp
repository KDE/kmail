/**
 * vcard.cpp
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

//
// VCard class to handle creation and parsing of Netscape
// vcards as per RFC 2426 (and possibly more)
//

// FIXME: do proper CRLF/CR handling

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "vcard.h"
#include <klocale.h>
#include <kdebug.h>
#include <qregexp.h>

/*

  Parser structure:

vcard := <begin><body><end> | <begin><end>
body  := <line><body> | <line>
begin := begin:vcard
end   := end:vcard
line  := <name>;<qualifiers>:<value> | <name>:<value>
qualifiers := <qualifier> | <qualifier>;<qualifiers>
name  :=
qualifier :=
value :=

*/


// This code is screaming "come do me in PERL!"
//static QValueList<QString> tokenizeBy(const QString& str, char tok) {
static QValueList<QString> tokenizeBy(const QString& str, const QRegExp& tok, bool keepEmpties = false) {
QValueList<QString> tokens;
unsigned int head, tail;
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

    if (tail-head > 0 || keepEmpties) {    // it has to be at least 1 long!
      thisline = str.mid(head, tail-head);
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
      return i18n("An unknown internal error occurred. Please report to kmail@kde.org");
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

    //kdDebug(5006) << "parseVCard: parsing line '" << QString(*j) << "'" << endl;

    // take spaces off the end - ugly but necessary hack
    for (int g = (*j).length()-1; g > 0 && (*j)[g].isSpace(); g++)
      (*j)[g] = 0;

    //        first token:
    //              verify state, update if necessary
    if (_state & VC_STATE_BEGIN) {
      if (!qstricmp((*j).latin1(), VCARD_BEGIN)) {
        _state = VC_STATE_BODY;
        continue;
      } else {
        _err = VC_ERR_NO_BEGIN;
        break;
      }
    } else if (_state & VC_STATE_BODY) {
      if (!qstricmp((*j).latin1(), VCARD_END)) {
        _state |= VC_STATE_END;
        break;
      }

      // split into two tokens
      // QValueList<QString> linetokens = tokenizeBy(*j, QRegExp(":"));
      unsigned int tail = (*j).find(':', 0);
      if (tail > (*j).length()) {  // invalid line - no ':'
        _err = VC_ERR_INVALID_LINE;
        kdDebug(5006) << "parseVCard: " << getError(_err)
                      << "(no ':')\nLine: '" << QString(*j) << "'" << endl;
        break;
      }

      QValueList<QString> linetokens;
      QString tmplinetoken;
      tmplinetoken = (*j);
      tmplinetoken.truncate(tail);
      linetokens.append(tmplinetoken);
      tmplinetoken = (*j).mid(tail+1);
      linetokens.append(tmplinetoken);

      // check for qualifiers and
      // set name, qualified, qualifier(s)
      //QValueList<QString> nametokens = tokenizeBy(linetokens[0], ';');
      QValueList<QString> nametokens = tokenizeBy(linetokens[0], QRegExp(";"));
      bool qp = false, first_pass = true;
      bool b64 = false;

      if (nametokens.count() > 0) {
        _vcl.qualified = false;
        _vcl.name = nametokens[0];
        _vcl.name = _vcl.name.lower();
        for (QValueListIterator<QString> z = nametokens.begin();
                                         z != nametokens.end();
                                         ++z) {
          QString zz = (*z).lower();
          //kdDebug(5006) << "parseVCard: parsing name token '" << zz << "'\n";
          if (zz == VCARD_QUOTED_PRINTABLE) {
            qp = true;
          } else if (zz == VCARD_BASE64) {
            b64 = true;
	  } else if (!first_pass) {
            if (zz.startsWith(VCARD_ENCODING_BEGIN)) {
              // strip off the leading 'encoding='
              zz = zz.mid(QString(VCARD_ENCODING_BEGIN).length());
              //kdDebug(5006) << "parseVCard: parsing encoding token '" << zz << "'\n";
              if (zz == VCARD_QUOTED_PRINTABLE) {
                qp = true;
              } else if (zz == VCARD_B) {
                b64 = true;
              }
            }
            else if (zz.startsWith(VCARD_QUALIFIER_BEGIN)) {
              // strip off the leading 'type='
              zz = zz.mid(QString(VCARD_QUALIFIER_BEGIN).length());
              QValueList<QString> qualifiertokens = tokenizeBy(zz, QRegExp(","));
              if (qualifiertokens.count() > 0) {
                _vcl.qualified = true;
                for (QValueListIterator<QString> qit = qualifiertokens.begin();
                                                 qit != qualifiertokens.end();
                                               ++qit) {
                  //kdDebug(5006) << "parseVCard: parsing qualifier token '" << (*qit) << "'\n";
                  _vcl.qualifiers.append((*qit).lower());
                }
              }
            }
            else {
              _vcl.qualified = true;
              _vcl.qualifiers.append(zz);
            }
          }
          first_pass = false;
	}
      } else {
        _err = VC_ERR_INVALID_LINE;
        kdDebug(5006) << "parseVCard: " << getError(_err)
                      << "(nametokens.count() == 0)\nLine: '" << QString(*j) << "'" << endl;
      }

      if (_err != 0) break;

      if (_vcl.name == VCARD_VERSION)
        _state |= VC_STATE_HAVE_VERSION;

      if (_vcl.name == VCARD_N || _vcl.name == VCARD_FN)
        _state |= VC_STATE_HAVE_N;

      // second token:
      // unfold the token (i.e. join any split lines)
      if (!qp) {
        QValueListIterator<QString> nextLine = j;
        ++nextLine;
        for (; ( nextLine != lines.end() ) && ( (*nextLine)[0] == ' ' );
            ++nextLine, ++j) {
          linetokens[1] += (*nextLine).stripWhiteSpace();
        }
      } else {
        while (linetokens[1].endsWith("=")) {
          linetokens[1].truncate(linetokens[1].length()-1);
          linetokens[1].append(*(++j));
        }
      }
      if (b64) {
        //linetokens[1] = linetokens[1];
      }
      else {
        //    split into tokens by ;
        //    add to parameters vector
        //_vcl.parameters = tokenizeBy(linetokens[1], ';');
        _vcl.parameters = tokenizeBy(linetokens[1], QRegExp(";"), true);
        if (qp) {        // decode the quoted printable
          for (QValueListIterator<QString> z = _vcl.parameters.begin();
                                           z != _vcl.parameters.end();
	                                   ++z) {
            _vcl.qpDecode(*z);
          }
        }
      }
    } else {
      _err = VC_ERR_INTERNAL;
      break;
    }

    // validate VCardLine
    if (!_vcl.isValid()) {
      _err = VC_ERR_INVALID_LINE;
      kdDebug(5006) << "parseVCard: " << getError(_err)
                    << "(_vcl.isValid() == false)\nLine: '" << QString(*j) << "'" << endl;
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
    _vcdata = 0;
    return 0;
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
const QString lowqualifier = qualifier.lower();
const QString lowname = name.lower();
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && (*i).qualifiers.contains(lowqualifier)) {
      (*i).qualifiers.remove(lowqualifier);
      if ((*i).qualifiers.isEmpty()) {
        _vcdata->remove(i);
      }
      return true;
    }
  }
return false;
}


int VCard::addLine(const QString& name, const QString& value) {
QValueList<QString> values;

  values.append(value);

return addLine(name.lower(), values);
}


// FIXME: We should see if we can just add a new qualifier first
//         - involves testing name and value.
int VCard::addQualifiedLine(const QString& name, const QString& qualifier, const QString& value) {
QValueList<QString> values;

  values.append(value);

return addQualifiedLine(name, qualifier, values);
}


int VCard::addLine(const QString& name, const QValueList<QString>& value) {
const QString lowname = name.lower();
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && !(*i).qualified) {
      (*i).parameters = value;
      if ((*i).isValid()) {
        return 0;
      }
      _vcdata->remove(i);
      return VC_ERR_INVALID_LINE;
    }
  }
VCardLine vcl;
  vcl.name = lowname;
  vcl.qualified = false;
  vcl.parameters = value;
  _vcdata->append(vcl);
return false;
}


int VCard::addQualifiedLine(const QString& name, const QString& qualifier, const QValueList<QString>& value) {
const QString lowname = name.lower();
const QString lowqualifier = qualifier.lower();
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && (*i).qualified && (*i).qualifiers.contains(lowqualifier)) {
      if ((*i).parameters == value) return 0;     // it's already there
      if ((*i).qualifiers.count() > 1) {   // there are others, remove and break
        (*i).qualifiers.remove(lowqualifier);
        break;
      }

      (*i).parameters = value;
      if ((*i).isValid()) {
        return 0;
      }
      _vcdata->remove(i);
      return VC_ERR_INVALID_LINE;

    }
  }

VCardLine vcl;
  vcl.name = lowname;
  vcl.qualifiers.append(lowqualifier);
  vcl.qualified = true;
  vcl.parameters = value;
  _vcdata->append(vcl);
return 0;
}


QString VCard::getValue(const QString& name, const QString& qualifier) {
QString failed = "";
const QString lowname = name.lower();
const QString lowqualifier = qualifier.lower();

  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname) {
      if (    ( !lowqualifier.isEmpty() && (*i).qualified
                && (*i).qualifiers.contains(lowqualifier) ) 
           || ( lowqualifier.isEmpty() && !(*i).qualified ) ) {
        if ((*i).parameters.count() > 0)
          return (*i).parameters[0];
        else return failed;
      }
    }
  }
return failed;
}


QString VCard::getValue(const QString& name) {
QString failed = "";
const QString lowname = name.lower();

  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && !(*i).qualified) {
      if ((*i).parameters.count() > 0)
        return (*i).parameters[0];
      else return failed;
    }
  }
return failed;
}


QValueList<QString> VCard::getValues(const QString& name, const QString& qualifier) {
//QString failedstr = "";
QValueList<QString> failed;
const QString lowname = name.lower();
const QString lowqualifier = qualifier.lower();
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && (*i).qualified && (*i).qualifiers.contains(lowqualifier)) {
      return (*i).parameters;
    }
  }
//failed.append(failedstr);
return failed;
}


QValueList<QString> VCard::getValues(const QString& name) {
//QString failedstr = "";
QValueList<QString> failed;
const QString lowname = name.lower();
  for (QValueListIterator<VCardLine> i = _vcdata->begin();
                                     i != _vcdata->end();
                                     ++i) {
    if ((*i).name == lowname && !(*i).qualified) {
      return (*i).parameters;
    }
  }
//failed.append(failedstr);
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
      for (QValueListIterator<QString> k = (*i).qualifiers.begin();
                                       k != (*i).qualifiers.end();
                                       ++k) {
        vct += ";";
        vct += *k;
      }
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
//                             (qualifiers.contains(VCARD_ADR_DOM)    ||
//                              qualifiers.contains(VCARD_ADR_INTL)   ||
//                              qualifiers.contains(VCARD_ADR_POSTAL) ||
//                              qualifiers.contains(VCARD_ADR_HOME)   ||
//                              qualifiers.contains(VCARD_ADR_WORK)   ||
//                              qualifiers.contains(VCARD_ADR_PREF)
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
    // This causes false alerts with v3.0 vCards
    //if (name == VCARD_CLASS && qualified &&
    //                          (qualifiers.contains(VCARD_CLASS_PUBLIC)      ||
    //                           qualifiers.contains(VCARD_CLASS_PRIVATE)     ||
    //                           qualifiers.contains(VCARD_CLASS_CONFIDENTIAL)
    //                          ))
    if (name == VCARD_CLASS)
      return true;
   break;
  case 'd':
   break;
  case 'e':
    // Email doesn't need to be qualified.
    if (name == VCARD_EMAIL)
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
    // This causes false alerts with v3.0 vCards
    //if (name == VCARD_KEY && qualified &&
    //                        (qualifiers.contains(VCARD_KEY_X509) ||
    //                         qualifiers.contains(VCARD_KEY_PGP)
    //                        ))
    if (name == VCARD_KEY)
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
    if (name == VCARD_TEL)
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

