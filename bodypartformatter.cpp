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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bodypartformatter.h"
#include "bodypartformatterfactory_p.h"
#include "interfaces/bodypartformatter.h"

#include "objecttreeparser.h"
#include "partNode.h"

#include <mimelib/enum.h>
#include <mimelib/string.h>
#include <mimelib/utility.h>

#include <kdebug.h>

namespace {
  class AnyTypeBodyPartFormatter
    : public KMail::BodyPartFormatter,
      public KMail::Interface::BodyPartFormatter {
    static const AnyTypeBodyPartFormatter * self;
  public:
    Result format( KMail::Interface::BodyPart *, KMail::HtmlWriter * ) const {
      kdDebug(5006) << "AnyTypeBodyPartFormatter::format() acting as a KMail::Interface::BodyPartFormatter!" << endl;
      return AsIcon;
    }

    bool process( KMail::ObjectTreeParser *, partNode *, KMail::ProcessResult & result ) const {
      result.setNeverDisplayInline( true );
      return false;
    }
    static const KMail::BodyPartFormatter * create() {
      if ( !self )
	self = new AnyTypeBodyPartFormatter();
      return self;
    }
  };

  const AnyTypeBodyPartFormatter * AnyTypeBodyPartFormatter::self = 0;


  class ImageTypeBodyPartFormatter : public KMail::BodyPartFormatter {
    static const ImageTypeBodyPartFormatter * self;
  public:
    bool process( KMail::ObjectTreeParser *, partNode *, KMail::ProcessResult & result ) const {
      result.setIsImage( true );
      return false;
    }
    static const KMail::BodyPartFormatter * create() {
      if ( !self )
	self = new ImageTypeBodyPartFormatter();
      return self;
    }
  };

  const ImageTypeBodyPartFormatter * ImageTypeBodyPartFormatter::self = 0;

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
  const subtype##BodyPartFormatter * subtype##BodyPartFormatter::self; \
  \
  bool subtype##BodyPartFormatter::process( KMail::ObjectTreeParser * otp, partNode * node, KMail::ProcessResult & result ) const { \
    return otp->process##subtype##Subtype( node, result ); \
  }

  CREATE_BODY_PART_FORMATTER(TextPlain)
  CREATE_BODY_PART_FORMATTER(TextHtml)
  //CREATE_BODY_PART_FORMATTER(TextEnriched)

  CREATE_BODY_PART_FORMATTER(ApplicationOctetStream)
  CREATE_BODY_PART_FORMATTER(ApplicationPkcs7Mime)
  //CREATE_BODY_PART_FORMATTER(ApplicationPgp)

  CREATE_BODY_PART_FORMATTER(MessageRfc822)

  CREATE_BODY_PART_FORMATTER(MultiPartMixed)
  CREATE_BODY_PART_FORMATTER(MultiPartAlternative)
  CREATE_BODY_PART_FORMATTER(MultiPartSigned)
  CREATE_BODY_PART_FORMATTER(MultiPartEncrypted)

  typedef TextPlainBodyPartFormatter ApplicationPgpBodyPartFormatter;


#undef CREATE_BODY_PART_FORMATTER
} // anon namespace

// FIXME: port some more KMail::BodyPartFormatters to KMail::Interface::BodyPartFormatters
void KMail::BodyPartFormatterFactoryPrivate::kmail_create_builtin_bodypart_formatters( KMail::BodyPartFormatterFactoryPrivate::TypeRegistry * reg ) {
  if ( !reg ) return;
  (*reg)["application"]["octet-stream"] = new AnyTypeBodyPartFormatter();
}

typedef const KMail::BodyPartFormatter * (*BodyPartFormatterCreator)();

struct SubtypeBuiltin {
  const char * subtype;
  BodyPartFormatterCreator create;
};

static const SubtypeBuiltin applicationSubtypeBuiltins[] = {
  { "octet-stream", &ApplicationOctetStreamBodyPartFormatter::create },
  { "pkcs7-mime", &ApplicationPkcs7MimeBodyPartFormatter::create },
  { "x-pkcs7-mime", &ApplicationPkcs7MimeBodyPartFormatter::create },
  { "pgp", &ApplicationPgpBodyPartFormatter::create },
};

static const SubtypeBuiltin textSubtypeBuiltins[] = {
  { "html", &TextHtmlBodyPartFormatter::create },
  //{ "enriched", &TextEnrichedBodyPartFormatter::create },
  { "x-vcard", &AnyTypeBodyPartFormatter::create },
  { "vcard", &AnyTypeBodyPartFormatter::create },
  { "rtf", &AnyTypeBodyPartFormatter::create },
  { "*", &TextPlainBodyPartFormatter::create },
};

static const SubtypeBuiltin multipartSubtypeBuiltins[] = {
  { "mixed", &MultiPartMixedBodyPartFormatter::create },
  { "alternative", &MultiPartAlternativeBodyPartFormatter::create },
  //{ "digest", &MultiPartDigestFormatter::create },
  //{ "parallel", &MultiPartParallelFormatter::create },
  //{ "related", &MultiPartRelatedFormatter::create },
  { "signed", &MultiPartSignedBodyPartFormatter::create },
  { "encrypted", &MultiPartEncryptedBodyPartFormatter::create },
  //{ "report", &MultiPartReportFormatter::create },
};

static const SubtypeBuiltin messageSubtypeBuiltins[] = {
  { "rfc822", &MessageRfc822BodyPartFormatter::create },
};

static const SubtypeBuiltin imageSubtypeBuiltins[] = {
  { "*", &ImageTypeBodyPartFormatter::create },
};

static const SubtypeBuiltin anySubtypeBuiltins[] = {
  { "*", &AnyTypeBodyPartFormatter::create },
};

#ifdef DIM
#undef DIM
#endif
#define DIM(x) sizeof(x) / sizeof(*x)

static const struct {
  const char * type;
  const SubtypeBuiltin * subtypes;
  unsigned int num_subtypes;
} builtins[] = {
  { "application", applicationSubtypeBuiltins, DIM(applicationSubtypeBuiltins) },
  { "text", textSubtypeBuiltins, DIM(textSubtypeBuiltins) },
  { "multipart", multipartSubtypeBuiltins, DIM(multipartSubtypeBuiltins) },
  { "message", messageSubtypeBuiltins, DIM(messageSubtypeBuiltins) },
  { "image", imageSubtypeBuiltins, DIM(imageSubtypeBuiltins) },
  //{ "audio", audioSubtypeBuiltins, DIM(audioSubtypeBuiltins) },
  //{ "model", modelSubtypeBuiltins, DIM(modelSubtypeBuiltins) },
  //{ "video", videoSubtypeBuiltins, DIM(videoSubtypeBuiltins) },
  { "*", anySubtypeBuiltins, DIM(anySubtypeBuiltins) },
};

#undef DIM

const KMail::BodyPartFormatter * KMail::BodyPartFormatter::createFor( int type, int subtype ) {
  DwString t, st;
  DwTypeEnumToStr( type, t );
  DwSubtypeEnumToStr( subtype, st );
  return createFor( t.c_str(), st.c_str() );
}

static const KMail::BodyPartFormatter * createForText( const char * subtype ) {
  if ( subtype && *subtype )
    switch ( subtype[0] ) {
    case 'h':
    case 'H':
      if ( qstricmp( subtype, "html" ) == 0 )
	return TextHtmlBodyPartFormatter::create();
      break;
    case 'r':
    case 'R':
      if ( qstricmp( subtype, "rtf" ) == 0 )
	return AnyTypeBodyPartFormatter::create();
      break;
    case 'x':
    case 'X':
    case 'v':
    case 'V':
      if ( qstricmp( subtype, "x-vcard" ) == 0 ||
	   qstricmp( subtype, "vcard" ) == 0 )
	return AnyTypeBodyPartFormatter::create();
      break;
    }

  return TextPlainBodyPartFormatter::create();
}

static const KMail::BodyPartFormatter * createForImage( const char * ) {
  return ImageTypeBodyPartFormatter::create();
}

static const KMail::BodyPartFormatter * createForMessage( const char * subtype ) {
  if ( qstricmp( subtype, "rfc822" ) == 0 )
    return MessageRfc822BodyPartFormatter::create();
  return AnyTypeBodyPartFormatter::create();
}

static const KMail::BodyPartFormatter * createForMultiPart( const char * subtype ) {
  if ( subtype && *subtype )
    switch ( subtype[0] ) {
    case 'a':
    case 'A':
      if ( qstricmp( subtype, "alternative" ) == 0 )
	return MultiPartAlternativeBodyPartFormatter::create();
      break;
    case 'e':
    case 'E':
      if ( qstricmp( subtype, "encrypted" ) == 0 )
	return MultiPartEncryptedBodyPartFormatter::create();
      break;
    case 's':
    case 'S':
      if ( qstricmp( subtype, "signed" ) == 0 )
	return MultiPartSignedBodyPartFormatter::create();
      break;
    }

  return MultiPartMixedBodyPartFormatter::create();
}

static const KMail::BodyPartFormatter * createForApplication( const char * subtype ) {
  if ( subtype && *subtype )
    switch ( subtype[0] ) {
    case 'p':
    case 'P':
      if ( qstricmp( subtype, "pgp" ) == 0 )
	return ApplicationPgpBodyPartFormatter::create();
      // fall through
    case 'x':
    case 'X':
      if ( qstricmp( subtype, "pkcs7-mime" ) == 0 ||
	   qstricmp( subtype, "x-pkcs7-mime" ) == 0 )
	return ApplicationPkcs7MimeBodyPartFormatter::create();
      break;
    case 'm':
    case 'M':
      //if ( qstricmp( subtype, "ms-tnef" ) == 0 )
      //  return ApplicationMsTnefBodyPartFormatter::create();
      break;
    }

  return AnyTypeBodyPartFormatter::create();
}

// OK, replace this with a factory with plugin support later on...
const KMail::BodyPartFormatter * KMail::BodyPartFormatter::createFor( const char * type, const char * subtype ) {
  if ( type && *type )
    switch ( type[0] ) {
    case 'a': // application
    case 'A':
      if ( qstricmp( type, "application" ) == 0 )
	return createForApplication( subtype );
      break;
    case 'i': // image
    case 'I':
      if ( qstricmp( type, "image" ) == 0 )
	return createForImage( subtype );
      break;
    case 'm': // multipart / message
    case 'M':
      if ( qstricmp( type, "multipart" ) == 0 )
	return createForMultiPart( subtype );
      else if ( qstricmp( type, "message" ) == 0 )
	return createForMessage( subtype );
      break;
    case 't': // text
    case 'T':
      if ( qstricmp( type, "text" ) == 0 )
	return createForText( subtype );
      break;
    }

  return AnyTypeBodyPartFormatter::create();
}

