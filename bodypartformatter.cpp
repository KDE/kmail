/*  -*- c++ -*-
    bodypartformatter.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "bodypartformatter.h"

#include "objecttreeparser.h"

#include <mimelib/enum.h>
#include <mimelib/string.h>
#include <mimelib/utility.h>

namespace {
  class AnyTypeBodyPartFormatter : public KMail::BodyPartFormatter {
    static const AnyTypeBodyPartFormatter * self;
  public:
    bool process( KMail::ObjectTreeParser *, partNode *, KMail::ProcessResult & ) const {
      return false;
    }
    static const KMail::BodyPartFormatter * create() {
      if ( !self )
	self = new AnyTypeBodyPartFormatter();
      return self;
    }
  };

  const AnyTypeBodyPartFormatter * AnyTypeBodyPartFormatter::self = 0;

#define CREATE_BODY_PART_FORMATTER(subtype) \
  class subtype##BodyPartFormatter : public KMail::BodyPartFormatter { \
    static const subtype##BodyPartFormatter * self; \
  public: \
    bool process( KMail::ObjectTreeParser *, partNode *, KMail::ProcessResult & ) const; \
    static const KMail::BodyPartFormatter * create() { \
      if ( !self ) \
	self = new subtype##BodyPartFormatter(); \
      return self; \
    } \
  }; \
  \
  const subtype##BodyPartFormatter * subtype##BodyPartFormatter::self = 0; \
  \
  bool subtype##BodyPartFormatter::process( KMail::ObjectTreeParser * otp, partNode * node, KMail::ProcessResult & result ) const { \
    return otp->process##subtype##Subtype( node, result ); \
  }

  CREATE_BODY_PART_FORMATTER(TextPlain);
  CREATE_BODY_PART_FORMATTER(TextHtml);
  CREATE_BODY_PART_FORMATTER(TextRtf);
  CREATE_BODY_PART_FORMATTER(TextVCal);
  CREATE_BODY_PART_FORMATTER(TextVCard);
  //CREATE_BODY_PART_FORMATTER(TextEnriched)

  CREATE_BODY_PART_FORMATTER(ApplicationPostscript)
  CREATE_BODY_PART_FORMATTER(ApplicationOctetStream)
  CREATE_BODY_PART_FORMATTER(ApplicationPkcs7Mime)
  CREATE_BODY_PART_FORMATTER(ApplicationMsTnef)
  //CREATE_BODY_PART_FORMATTER(ApplicationPgp)

  typedef TextPlainBodyPartFormatter TextEnrichedBodyPartFormatter;
  typedef TextPlainBodyPartFormatter ApplicationPgpBodyPartFormatter;


#undef CREATE_BODY_PART_FORMATTER
} // anon namespace

typedef const KMail::BodyPartFormatter * (*BodyPartFormatterCreator)();

static const struct {
  const char * type;
  const char * subtype;
  BodyPartFormatterCreator create;
} builtins[] = {
  //{ "application", "octet-stream", &AppOctetStreamFormatter::create },
  //{ "application", "pkcs7-mime", &AppPkcs7MimeFormatter::create },
  //{ "application", "ms-tnef", &AppMsTnefFormatter::create },
  //{ "application", "postscript", &AppPostscriptFormatter::create },
  //{ "application", "pgp", &AppPgpFormatter::create },
  //{ "text", "html", &TextHtmlFormatter::create },
  { "text", "plain", &TextPlainBodyPartFormatter::create },
  //{ "text", "enriched", &TextEnrichedFormatter::create },
  //{ "text", "vcal", &TextVCalFormatter::create },
  //{ "text", "vcard", &TextVCardFormatter::create },
  //{ "text", "rtf", &TextRtfFormatter::create },
  //{ "multipart", "mixed", &MultiPartMixedFormatter::create },
  //{ "multipart", "alternative", &MultiPartAlternativeFormatter::create },
  //{ "multipart", "digest", &MultiPartDigestFormatter::create },
  //{ "multipart", "parallel", &MultiPartParallelFormatter::create },
  //{ "multipart", "related", &MultiPartRelatedFormatter::create },
  //{ "multipart", "signed", &MultiPartSignedFormatter::create },
  //{ "multipart", "encrypted", &MultiPartEncryptedFormatter::create },
  //{ "multipart", "report", &MultiPartReportFormatter::create },
  //{ "message", "rfc822", &MessageRfc822Formatter::create },
  //{ "image", "*", &AnyImageFormatter::create },
  //{ "audio", "*", &AnyAudioFormatter::create },
  //{ "model", "*", &AnyModelFormatter::create },
  //{ "video", "*", &AnyVideoFormatter::create },
  //{ "*", "*", &AnyTypeFormatter::create }
};

const KMail::BodyPartFormatter * KMail::BodyPartFormatter::createFor( int type, int subtype ) {
  DwString t, st;
  DwTypeEnumToStr( type, t );
  DwSubtypeEnumToStr( subtype, st );
  return createFor( t.c_str(), st.c_str() );
}

namespace {
  const KMail::BodyPartFormatter * createForText( const char * subtype ) {
    if ( !subtype || !*subtype )
      return 0;
    switch ( subtype[0] ) {
    case 'h':
    case 'H':
      if ( qstricmp( subtype, "html" ) == 0 )
	return TextHtmlBodyPartFormatter::create();
      break;
    case 'c':
    case 'C':
      if ( qstricmp( subtype, "calendar" ) == 0 )
	return TextVCalBodyPartFormatter::create();
      break;
    case 'e':
    case 'E':
    case 'r':
    case 'R':
      if ( qstricmp( subtype, "enriched" ) == 0 ||
	   qstricmp( subtype, "richtext" ) == 0 )
	return TextEnrichedBodyPartFormatter::create();
      else if ( qstricmp( subtype, "rtf" ) == 0 )
	return TextRtfBodyPartFormatter::create();
      break;
    case 'x':
    case 'X':
    case 'v':
    case 'V':
      if ( qstricmp( subtype, "x-vcard" ) == 0 ||
	   qstricmp( subtype, "vcard" ) == 0 )
	return TextVCardBodyPartFormatter::create();
      break;
    }

    return TextPlainBodyPartFormatter::create();
  }

  const KMail::BodyPartFormatter * createForImage( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForVideo( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForAudio( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForModel( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForMessage( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForMultiPart( const char * ) {
    return 0;
  }

  const KMail::BodyPartFormatter * createForApplication( const char * subtype ) {
    if ( !subtype || !*subtype )
      return 0;
    switch ( subtype[0] ) {
    case 'p':
    case 'P':
      if ( qstricmp( subtype, "pgp" ) == 0 )
	return ApplicationPgpBodyPartFormatter::create();
      else if ( qstricmp( subtype, "postscript" ) == 0 )
	return ApplicationPostscriptBodyPartFormatter::create();
      // fall through
    case 'x':
    case 'X':
      if ( qstricmp( subtype, "pkcs7-mime" ) == 0 ||
	   qstricmp( subtype, "x-pkcs7-mime" ) == 0 )
	return ApplicationPkcs7MimeBodyPartFormatter::create();
      break;
    case 'm':
    case 'M':
      if ( qstricmp( subtype, "ms-tnef" ) == 0 )
	return ApplicationMsTnefBodyPartFormatter::create();
      break;
    }

    return AnyTypeBodyPartFormatter::create();
  }
}

// OK, replace this with a factory with plugin support later on...
const KMail::BodyPartFormatter * KMail::BodyPartFormatter::createFor( const char * type, const char * subtype ) {
  if ( !type || !*type || !subtype || !*subtype )
    return 0;
  switch ( type[0] ) {
  case 'a': // application / audio
  case 'A':
    if ( qstricmp( type, "application" ) == 0 )
      return createForApplication( subtype );
    else if ( qstricmp( type, "audio" ) == 0 )
      return createForAudio( subtype );
    break;
  case 'i': // image
  case 'I':
    if ( qstricmp( type, "image" ) == 0 )
      return createForImage( subtype );
    break;
  case 'm': // multipart / message / model
  case 'M':
    if ( qstricmp( type, "multipart" ) == 0 )
      return createForMultiPart( subtype );
    else if ( qstricmp( type, "message" ) == 0 )
      return createForMessage( subtype );
    else if ( qstricmp( type, "model" ) == 0 )
      return createForModel( subtype );
    break;
  case 't': // text
  case 'T':
    if ( qstricmp( type, "text" ) == 0 )
      return createForText( subtype );
    break;
  case 'v': // video
  case 'V':
    if ( qstricmp( type, "video" ) == 0 )
      return createForVideo( subtype );
    break;
  }
  return 0;//createForApplication( "octet-stream" );
}

